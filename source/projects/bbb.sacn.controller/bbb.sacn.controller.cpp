#include "c74_min.h"
#include <bbb/sacn/sacn_packet.h>
#include <bbb/version.h>

#include <bbb/net_compat.hpp>

#include <cstring>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>

class sacn_controller : public c74::min::object<sacn_controller> {
public:
    MIN_DESCRIPTION{"Send DMX via sACN (E1.31) protocol."};
    MIN_TAGS{"dmx, sacn, e1.31, lighting"};
    MIN_AUTHOR{"bbb"};
    MIN_RELATED{"bbb.sacn.node"};

    c74::min::inlet<> input{this, "(list/bang/message) DMX data input"};
    c74::min::outlet<> output{this, "(bang) bang on packet transmission"};

    c74::min::argument<int> universe_arg{this, "universe", "sACN universe (1-63999).",
        MIN_ARGUMENT_FUNCTION {
            universe = arg;
        }
    };

    c74::min::attribute<int> universe{this, "universe", 1,
        c74::min::description{"sACN universe (1-63999)."},
        c74::min::range{1, 63999}
    };

    c74::min::attribute<int> num_universes{this, "num_universes", 1,
        c74::min::description{"Number of universes to manage."},
        c74::min::range{1, 32}
    };

    c74::min::attribute<int> num_channels{this, "num_channels", 512,
        c74::min::description{"Number of DMX channels per universe."},
        c74::min::range{1, 512}
    };

    c74::min::attribute<bool> sync_universes{this, "sync_universes", true,
        c74::min::description{"Send all universes when any data changes."}
    };

    c74::min::attribute<bool> blackout{this, "blackout", false,
        c74::min::description{"Send all zeros (blackout)."}
    };

    c74::min::attribute<c74::min::symbol> mode{this, "mode", "automatic",
        c74::min::description{"Output mode: automatic, bang, update, change, forced."},
        c74::min::range{"automatic", "bang", "update", "change", "forced"}
    };

    c74::min::attribute<double> framerate{this, "framerate", 40.0,
        c74::min::description{"Framerate for forced mode (0.01 - 44)."},
        c74::min::range{0.01, 44.0}
    };

    c74::min::attribute<int> priority_attr{this, "priority", 100,
        c74::min::description{"sACN priority (0-200)."},
        c74::min::range{0, 200}
    };

    c74::min::attribute<c74::min::symbol> source_name{this, "source_name", "bbb.sacn.controller",
        c74::min::description{"sACN source name."}
    };

    c74::min::attribute<bool> unicast{this, "unicast", false,
        c74::min::description{"Use unicast mode instead of multicast."}
    };

    c74::min::attribute<c74::min::symbol> unicast_ip{this, "unicast_ip", "127.0.0.1",
        c74::min::description{"Destination IP for unicast mode."}
    };

    sacn_controller(const c74::min::atoms& args = {})
#ifdef _WIN32
        : m_fd{INVALID_SOCKET}
#else
        : m_fd{-1}
#endif
        , m_running{false}
        , m_dirty{false}
        , m_sequence{0}
    {
        bbb::net::ensure_init();
        m_buffer.resize(512 * num_universes, 0);
        m_prev_buffer.resize(512 * num_universes, 0);
        m_cid = sacn::generate_cid();
        init_socket();
    }

    ~sacn_controller() {
        m_running = false;
        if(m_thread.joinable()) {
            m_thread.join();
        }
        if(bbb::net::socket_valid(m_fd)) {
            bbb::net::close_socket(m_fd);
        }
    }

    c74::min::message<> list_msg{this, "list", "Set DMX values from a list.",
        MIN_FUNCTION {
            std::lock_guard<std::mutex> lock(m_mutex);
            size_t count = std::min(args.size(), m_buffer.size());
            for(size_t i = 0; i < count; ++i) {
                int val = static_cast<int>(args[i]);
                m_buffer[i] = static_cast<uint8_t>(std::max(0, std::min(255, val)));
            }
            m_dirty = true;
            handle_mode_send();
            return {};
        }
    };

    c74::min::message<> bang_msg{this, "bang", "Trigger send in bang mode.",
        MIN_FUNCTION {
            std::lock_guard<std::mutex> lock(m_mutex);
            send_all();
            return {};
        }
    };

    c74::min::message<> channel_msg{this, "channel", "Set a single DMX channel (1-based index).",
        MIN_FUNCTION {
            if(args.size() < 2) return {};
            std::lock_guard<std::mutex> lock(m_mutex);
            int index = static_cast<int>(args[0]) - 1;
            int value = static_cast<int>(args[1]);
            if(0 <= index && static_cast<size_t>(index) < m_buffer.size()) {
                m_buffer[index] = static_cast<uint8_t>(std::max(0, std::min(255, value)));
                m_dirty = true;
                handle_mode_send();
            }
            return {};
        }
    };

    c74::min::message<> setchannel_msg{this, "setchannel", "Set a single DMX channel without sending.",
        MIN_FUNCTION {
            if(args.size() < 2) return {};
            std::lock_guard<std::mutex> lock(m_mutex);
            int index = static_cast<int>(args[0]) - 1;
            int value = static_cast<int>(args[1]);
            if(0 <= index && static_cast<size_t>(index) < m_buffer.size()) {
                m_buffer[index] = static_cast<uint8_t>(std::max(0, std::min(255, value)));
                m_dirty = true;
            }
            return {};
        }
    };

    c74::min::message<> set_msg{this, "set", "Store DMX values without sending.",
        MIN_FUNCTION {
            std::lock_guard<std::mutex> lock(m_mutex);
            size_t count = std::min(args.size(), m_buffer.size());
            for(size_t i = 0; i < count; ++i) {
                int val = static_cast<int>(args[i]);
                m_buffer[i] = static_cast<uint8_t>(std::max(0, std::min(255, val)));
            }
            m_dirty = true;
            return {};
        }
    };

    c74::min::message<> set_offset_msg{this, "set_offset", "Store DMX values at offset (0-based) without sending.",
        MIN_FUNCTION {
            if(args.empty()) return {};
            std::lock_guard<std::mutex> lock(m_mutex);
            int offset = static_cast<int>(args[0]);
            for(size_t i = 1; i < args.size(); ++i) {
                size_t idx = static_cast<size_t>(offset + static_cast<int>(i) - 1);
                if(idx < m_buffer.size()) {
                    int val = static_cast<int>(args[i]);
                    m_buffer[idx] = static_cast<uint8_t>(std::max(0, std::min(255, val)));
                }
            }
            m_dirty = true;
            return {};
        }
    };

    c74::min::message<> maxclass_setup{this, "maxclass_setup",
        MIN_FUNCTION {
            cout << "bbb.sacn.controller v" BBB_ARTNET_VERSION << c74::min::endl;
            return {};
        }
    };

private:
    void init_socket() {
        m_fd = socket(AF_INET, SOCK_DGRAM, 0);
        if(!bbb::net::socket_valid(m_fd)) {
            cerr << "bbb.sacn.controller: failed to create socket" << c74::min::endl;
            return;
        }

        int ttl = 4;
        setsockopt(m_fd, IPPROTO_IP, IP_MULTICAST_TTL,
            reinterpret_cast<const char*>(&ttl), sizeof(ttl));

        char loop = 1;
        setsockopt(m_fd, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));

        start_thread();
    }

    void start_thread() {
        if(mode == c74::min::symbol("forced")) {
            if(!m_running) {
                m_running = true;
                if(m_thread.joinable()) {
                    m_thread.join();
                }
                m_thread = std::thread([this]() {
                    forced_framerate_loop();
                });
            }
        } else {
            m_running = false;
            if(m_thread.joinable()) {
                m_thread.join();
            }
        }
    }

    void forced_framerate_loop() {
        while(m_running) {
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                send_all();
            }
            double interval = 1000.0 / framerate;
            std::this_thread::sleep_for(
                std::chrono::milliseconds(static_cast<int>(interval))
            );
        }
    }

    void handle_mode_send() {
        if(mode == c74::min::symbol("automatic")) {
            send_all();
        } else if(mode == c74::min::symbol("update")) {
            send_all();
        } else if(mode == c74::min::symbol("change")) {
            if(m_buffer != m_prev_buffer) {
                send_all();
            }
        }
    }

    void send_all() {
        if(!bbb::net::socket_valid(m_fd)) return;

        for(int i = 0; i < num_universes; ++i) {
            int offset = i * 512;
            int length = std::min(512, static_cast<int>(m_buffer.size()) - offset);

            sacn::packet pkt;
            const uint8_t* data_ptr;
            std::vector<uint8_t> zeros;

            if(blackout) {
                zeros.resize(512, 0);
                data_ptr = zeros.data();
                length = 512;
            } else {
                data_ptr = m_buffer.data() + offset;
            }

            if(length <= 0) continue;

            uint16_t univ = static_cast<uint16_t>(universe + i);
            c74::min::symbol name_sym = source_name;
            std::string name_str(static_cast<const char*>(name_sym));

            sacn::init_packet(pkt, m_cid.data(), name_str.c_str(),
                static_cast<uint8_t>(static_cast<int>(priority_attr)),
                m_sequence++, univ, data_ptr, length);

            struct sockaddr_in addr;
            std::memset(&addr, 0, sizeof(addr));
            addr.sin_family = AF_INET;
            addr.sin_port = htons(5568);

            if(unicast) {
                c74::min::symbol ip_sym = unicast_ip;
                std::string ip_str(static_cast<const char*>(ip_sym));
                inet_pton(AF_INET, ip_str.c_str(), &addr.sin_addr);
            } else {
                auto mc = sacn::universe_to_multicast(univ);
                char mc_str[16];
                std::snprintf(mc_str, sizeof(mc_str), "%d.%d.%d.%d", mc.a, mc.b, mc.c, mc.d);
                inet_pton(AF_INET, mc_str, &addr.sin_addr);
            }

            int pkt_size = 126 + 1 + length;
            sendto(m_fd, reinterpret_cast<const char*>(&pkt), pkt_size, 0,
                reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr));
        }

        m_prev_buffer = m_buffer;
        m_dirty = false;
        output.send(c74::min::k_sym_bang);
    }

#ifdef _WIN32
    SOCKET m_fd;
#else
    int m_fd;
#endif
    std::vector<uint8_t> m_buffer;
    std::vector<uint8_t> m_prev_buffer;
    std::mutex m_mutex;
    std::thread m_thread;
    std::atomic<bool> m_running;
    std::atomic<bool> m_dirty;
    uint8_t m_sequence;
    std::array<uint8_t, 16> m_cid;
};

MIN_EXTERNAL(sacn_controller);
