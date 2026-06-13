#include "c74_min.h"
#include <bbb/sacn/sacn_packet.h>
#include <bbb/version.h>

#include <bbb/net_compat.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstring>
#include <exception>
#include <mutex>
#include <string>
#include <vector>

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
        c74::min::range{1, 32},
        c74::min::setter{[this](const c74::min::atoms& args, int) -> c74::min::atoms {
            guard_message("num_universes", [&]() {
                if(!args.empty()) {
                    resize_universe_buffers(static_cast<int>(args[0]));
                }
            });
            return args;
        }}
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
        c74::min::range{"automatic", "bang", "update", "change", "forced"},
        c74::min::setter{[this](const c74::min::atoms& args, c74::min::symbol) -> c74::min::atoms {
            guard_message("mode", [&]() {
                if(m_constructed && !args.empty()) {
                    c74::min::symbol mode_value{args[0]};
                    start_forced_timer_for_mode(mode_value);
                }
            });
            return args;
        }}
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

    c74::min::timer<c74::min::timer_options::defer_delivery> m_init_timer{this,
        MIN_FUNCTION {
            guard_message("initialization", [&]() {
                init_socket();
            });
            return {};
        }
    };

    c74::min::timer<c74::min::timer_options::defer_delivery> m_forced_timer{this,
        MIN_FUNCTION {
            guard_message("forced send", [&]() {
                if(m_running) {
                    std::lock_guard<std::mutex> lock(m_mutex);
                    send_all();
                    double interval = 1000.0 / static_cast<double>(framerate);
                    m_forced_timer.delay(interval);
                }
            });
            return {};
        }
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
        resize_universe_buffers(static_cast<int>(num_universes));
        m_cid = sacn::generate_cid();
        m_constructed = true;
        m_init_timer.delay(0);
    }

    ~sacn_controller() {
        m_running = false;
        if(bbb::net::socket_valid(m_fd)) {
            bbb::net::close_socket(m_fd);
        }
    }

    c74::min::message<> list_msg{this, "list", "Set DMX values from a list.",
        MIN_FUNCTION {
            guard_message("list", [&]() {
                std::lock_guard<std::mutex> lock(m_mutex);
                size_t count = std::min(args.size(), m_buffer.size());
                for(size_t i = 0; i < count; ++i) {
                    int value = static_cast<int>(args[i]);
                    m_buffer[i] = static_cast<uint8_t>(std::max(0, std::min(255, value)));
                }
                m_dirty = true;
                handle_mode_send();
            });
            return {};
        }
    };

    c74::min::message<> bang_msg{this, "bang", "Trigger send in bang mode.",
        MIN_FUNCTION {
            guard_message("bang", [&]() {
                std::lock_guard<std::mutex> lock(m_mutex);
                send_all();
            });
            return {};
        }
    };

    c74::min::message<> channel_msg{this, "channel", "Set a single DMX channel (1-based index).",
        MIN_FUNCTION {
            guard_message("channel", [&]() {
                if(args.size() < 2) {
                    return;
                }
                std::lock_guard<std::mutex> lock(m_mutex);
                int index = static_cast<int>(args[0]) - 1;
                int value = static_cast<int>(args[1]);
                if(0 <= index && static_cast<size_t>(index) < m_buffer.size()) {
                    m_buffer[index] = static_cast<uint8_t>(std::max(0, std::min(255, value)));
                    m_dirty = true;
                    handle_mode_send();
                }
            });
            return {};
        }
    };

    c74::min::message<> setchannel_msg{this, "setchannel", "Set a single DMX channel without sending.",
        MIN_FUNCTION {
            guard_message("setchannel", [&]() {
                if(args.size() < 2) {
                    return;
                }
                std::lock_guard<std::mutex> lock(m_mutex);
                int index = static_cast<int>(args[0]) - 1;
                int value = static_cast<int>(args[1]);
                if(0 <= index && static_cast<size_t>(index) < m_buffer.size()) {
                    m_buffer[index] = static_cast<uint8_t>(std::max(0, std::min(255, value)));
                    m_dirty = true;
                }
            });
            return {};
        }
    };

    c74::min::message<> set_msg{this, "set", "Store DMX values without sending.",
        MIN_FUNCTION {
            guard_message("set", [&]() {
                std::lock_guard<std::mutex> lock(m_mutex);
                size_t count = std::min(args.size(), m_buffer.size());
                for(size_t i = 0; i < count; ++i) {
                    int value = static_cast<int>(args[i]);
                    m_buffer[i] = static_cast<uint8_t>(std::max(0, std::min(255, value)));
                }
                m_dirty = true;
            });
            return {};
        }
    };

    c74::min::message<> set_offset_msg{this, "set_offset", "Store DMX values at offset (0-based) without sending.",
        MIN_FUNCTION {
            guard_message("set_offset", [&]() {
                if(args.empty()) {
                    return;
                }
                std::lock_guard<std::mutex> lock(m_mutex);
                int offset = static_cast<int>(args[0]);
                for(size_t i = 1; i < args.size(); ++i) {
                    size_t index = static_cast<size_t>(offset + static_cast<int>(i) - 1);
                    if(index < m_buffer.size()) {
                        int value = static_cast<int>(args[i]);
                        m_buffer[index] = static_cast<uint8_t>(std::max(0, std::min(255, value)));
                    }
                }
                m_dirty = true;
            });
            return {};
        }
    };

    c74::min::message<> maxclass_setup{this, "maxclass_setup",
        MIN_FUNCTION {
            guard_message("maxclass_setup", [&]() {
                cout << "bbb.sacn.controller v" BBB_ARTNET_VERSION << c74::min::endl;
            });
            return {};
        }
    };

private:
    template <typename function_type>
    void guard_message(const char* name, function_type&& function) {
        try {
            function();
        } catch(const std::exception& e) {
            m_running = false;
            cerr << "bbb.sacn.controller: " << name << " failed: " << e.what() << c74::min::endl;
        } catch(...) {
            m_running = false;
            cerr << "bbb.sacn.controller: " << name << " failed" << c74::min::endl;
        }
    }

    void resize_universe_buffers(int universe_count) {
        std::lock_guard<std::mutex> lock(m_mutex);
        size_t buffer_size = 512 * static_cast<size_t>(std::max(1, universe_count));
        m_buffer.resize(buffer_size, 0);
        m_prev_buffer.resize(buffer_size, 0);
    }

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

        c74::min::symbol mode_value = mode;
        start_forced_timer_for_mode(mode_value);
    }

    void start_forced_timer_for_mode(c74::min::symbol mode_value) {
        if(mode_value == "forced") {
            m_running = true;
            double interval = 1000.0 / static_cast<double>(framerate);
            m_forced_timer.delay(interval);
        } else {
            m_running = false;
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

            sacn::packet packet;
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

            uint16_t current_universe = static_cast<uint16_t>(universe + i);
            c74::min::symbol name_sym = source_name;
            std::string name_string(static_cast<const char*>(name_sym));

            sacn::init_packet(packet, m_cid.data(), name_string.c_str(),
                static_cast<uint8_t>(static_cast<int>(priority_attr)),
                m_sequence++, current_universe, data_ptr, length);

            struct sockaddr_in address;
            std::memset(&address, 0, sizeof(address));
            address.sin_family = AF_INET;
            address.sin_port = htons(5568);

            if(unicast) {
                c74::min::symbol ip_symbol = unicast_ip;
                std::string ip_string(static_cast<const char*>(ip_symbol));
                inet_pton(AF_INET, ip_string.c_str(), &address.sin_addr);
            } else {
                auto multicast = sacn::universe_to_multicast(current_universe);
                char multicast_string[16];
                std::snprintf(multicast_string, sizeof(multicast_string), "%d.%d.%d.%d",
                    multicast.a, multicast.b, multicast.c, multicast.d);
                inet_pton(AF_INET, multicast_string, &address.sin_addr);
            }

            int packet_size = 126 + 1 + length;
            sendto(m_fd, reinterpret_cast<const char*>(&packet), packet_size, 0,
                reinterpret_cast<struct sockaddr*>(&address), sizeof(address));
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
    std::atomic<bool> m_running;
    std::atomic<bool> m_dirty;
    uint8_t m_sequence;
    std::array<uint8_t, 16> m_cid;
    bool m_constructed{false};
};

MIN_EXTERNAL(sacn_controller);
