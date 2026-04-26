#include "c74_min.h"
#include <bbb/sacn/sacn_packet.h>
#include <bbb/version.h>

#include <bbb/net_compat.hpp>

#include <cstring>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>

class sacn_node : public c74::min::object<sacn_node> {
public:
    MIN_DESCRIPTION{"Receive DMX via sACN (E1.31) protocol."};
    MIN_TAGS{"dmx, sacn, e1.31, lighting"};
    MIN_AUTHOR{"bbb"};
    MIN_RELATED{"bbb.sacn.controller"};

    c74::min::inlet<> input{this, "(bang) request output in bang mode"};
    c74::min::outlet<> output{this, "(list) DMX values as list of integers"};

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
        c74::min::description{"Number of universes to receive."},
        c74::min::range{1, 32}
    };

    c74::min::attribute<int> num_channels{this, "num_channels", 512,
        c74::min::description{"Number of DMX channels to output."},
        c74::min::range{1, 512}
    };

    c74::min::attribute<bool> sync_universes{this, "sync_universes", true,
        c74::min::description{"Wait for all universes before outputting."}
    };

    c74::min::attribute<c74::min::symbol> mode{this, "mode", "update",
        c74::min::description{"Output mode: update, bang, automatic, change, forced."},
        c74::min::range{"update", "bang", "automatic", "change", "forced"}
    };

    sacn_node(const c74::min::atoms& args = {})
        : m_fd{-1}
        , m_running{false}
    {
        bbb::net::ensure_init();
        m_buffer.resize(512 * num_universes, 0);
        m_prev_buffer.resize(512 * num_universes, 0);
        m_received_universes.resize(num_universes, false);
        m_last_sequence.resize(num_universes, 255);
        init_socket();
    }

    ~sacn_node() {
        m_running = false;
        if(m_read_thread.joinable()) {
            m_read_thread.join();
        }
        if(bbb::net::socket_valid(m_fd)) {
            leave_multicast_groups();
            bbb::net::close_socket(m_fd);
        }
    }

    c74::min::message<> bang_msg{this, "bang", "Output current data in bang mode.",
        MIN_FUNCTION {
            std::lock_guard<std::mutex> lock(m_mutex);
            output_data();
            return {};
        }
    };

    c74::min::message<> maxclass_setup{this, "maxclass_setup",
        MIN_FUNCTION {
            cout << "bbb.sacn.node v" BBB_ARTNET_VERSION << c74::min::endl;
            return {};
        }
    };

private:
    void init_socket() {
        m_fd = socket(AF_INET, SOCK_DGRAM, 0);
        if(!bbb::net::socket_valid(m_fd)) {
            cerr << "bbb.sacn.node: failed to create socket" << c74::min::endl;
            return;
        }

        int reuse = 1;
        setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR,
            reinterpret_cast<const char*>(&reuse), sizeof(reuse));
#ifdef SO_REUSEPORT
        setsockopt(m_fd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse));
#endif

        struct sockaddr_in addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(5568);
        addr.sin_addr.s_addr = htonl(INADDR_ANY);

        if(bind(m_fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
            cerr << "bbb.sacn.node: failed to bind socket" << c74::min::endl;
            return;
        }

        join_multicast_groups();

        m_running = true;
        m_read_thread = std::thread([this]() {
            read_loop();
        });
    }

    void join_multicast_groups() {
        for(int i = 0; i < num_universes; ++i) {
            uint16_t univ = static_cast<uint16_t>(universe + i);
            auto mc = sacn::universe_to_multicast(univ);

            struct ip_mreq mreq;
            std::memset(&mreq, 0, sizeof(mreq));
            char mc_str[16];
            std::snprintf(mc_str, sizeof(mc_str), "%d.%d.%d.%d", mc.a, mc.b, mc.c, mc.d);
            inet_pton(AF_INET, mc_str, &mreq.imr_multiaddr);
            mreq.imr_interface.s_addr = htonl(INADDR_ANY);
            setsockopt(m_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                reinterpret_cast<const char*>(&mreq), sizeof(mreq));
        }
    }

    void leave_multicast_groups() {
        for(int i = 0; i < num_universes; ++i) {
            uint16_t univ = static_cast<uint16_t>(universe + i);
            auto mc = sacn::universe_to_multicast(univ);

            struct ip_mreq mreq;
            std::memset(&mreq, 0, sizeof(mreq));
            char mc_str[16];
            std::snprintf(mc_str, sizeof(mc_str), "%d.%d.%d.%d", mc.a, mc.b, mc.c, mc.d);
            inet_pton(AF_INET, mc_str, &mreq.imr_multiaddr);
            mreq.imr_interface.s_addr = htonl(INADDR_ANY);
            setsockopt(m_fd, IPPROTO_IP, IP_DROP_MEMBERSHIP,
                reinterpret_cast<const char*>(&mreq), sizeof(mreq));
        }
    }

    void read_loop() {
        uint8_t buf[65536];
        while(m_running) {
            bbb::net::recv_len_t len = recvfrom(m_fd, reinterpret_cast<char*>(buf), sizeof(buf), 0, nullptr, nullptr);
            if(len <= 0) continue;
            if(static_cast<size_t>(len) < 126) continue;

            const uint8_t acn_id[12] = {0x41,0x53,0x43,0x2D,0x45,0x31,0x2E,0x31,0x37,0x00,0x00,0x00};
            if(std::memcmp(buf + 4, acn_id, 12) != 0) continue;

            uint32_t root_vector = ntohl(*reinterpret_cast<const uint32_t*>(buf + 18));
            if(root_vector != 0x00000004) continue;

            uint32_t framing_vector = ntohl(*reinterpret_cast<const uint32_t*>(buf + 40));
            if(framing_vector != 0x00000002) continue;

            uint8_t sequence = buf[75];
            uint16_t pkt_universe = ntohs(*reinterpret_cast<const uint16_t*>(buf + 78));

            int univ_index = -1;
            for(int i = 0; i < num_universes; ++i) {
                if(static_cast<uint16_t>(universe + i) == pkt_universe) {
                    univ_index = i;
                    break;
                }
            }
            if(univ_index < 0) continue;

            if(sequence == m_last_sequence[univ_index]) continue;
            m_last_sequence[univ_index] = sequence;

            uint8_t dmp_vector = buf[117];
            if(dmp_vector != 0x02) continue;

            int dmp_data_len = static_cast<int>(len) - 126 - 1;
            if(dmp_data_len <= 0) continue;
            int copy_len = std::min(dmp_data_len, 512);

            {
                std::lock_guard<std::mutex> lock(m_mutex);
                int offset = univ_index * 512;
                if(offset + copy_len <= static_cast<int>(m_buffer.size())) {
                    std::memcpy(m_buffer.data() + offset, buf + 126, copy_len);
                }

                if(sync_universes && num_universes > 1) {
                    m_received_universes[univ_index] = true;
                    bool all_received = true;
                    for(auto&& received : m_received_universes) {
                        if(!received) { all_received = false; break; }
                    }
                    if(!all_received) return;

                    for(auto&& received : m_received_universes) {
                        received = false;
                    }
                }

                handle_mode_output();
            }
        }
    }

    void handle_mode_output() {
        if(mode == c74::min::symbol("update")) {
            output_data();
        } else if(mode == c74::min::symbol("automatic")) {
            output_data();
        } else if(mode == c74::min::symbol("change")) {
            if(m_buffer != m_prev_buffer) {
                output_data();
            }
        }
    }

    void output_data() {
        int total = std::min(num_channels * num_universes, static_cast<int>(m_buffer.size()));
        c74::min::atoms result;
        result.reserve(total);
        for(int i = 0; i < total; ++i) {
            result.push_back(static_cast<int>(m_buffer[i]));
        }
        output.send(result);
        m_prev_buffer = m_buffer;
    }

#ifdef _WIN32
    SOCKET m_fd;
#else
    int m_fd;
#endif
    std::vector<uint8_t> m_buffer;
    std::vector<uint8_t> m_prev_buffer;
    std::vector<bool> m_received_universes;
    std::vector<uint8_t> m_last_sequence;
    std::mutex m_mutex;
    std::thread m_read_thread;
    std::atomic<bool> m_running;
};

MIN_EXTERNAL(sacn_node);
