#include "c74_min.h"
#include <bbb/sacn/sacn_packet.h>
#include <bbb/version.h>

#include <bbb/net_compat.hpp>

#include <algorithm>
#include <atomic>
#include <cstring>
#include <exception>
#include <mutex>
#include <thread>
#include <vector>

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
        c74::min::range{1, 63999},
        c74::min::setter{[this](const c74::min::atoms& args, int) -> c74::min::atoms {
            guard_message("universe", [&]() {
                if(!args.empty()) {
                    reconfigure_universe_range(static_cast<int>(args[0]), static_cast<int>(num_universes));
                }
            });
            return args;
        }}
    };

    c74::min::attribute<int> num_universes{this, "num_universes", 1,
        c74::min::description{"Number of universes to receive."},
        c74::min::range{1, 32},
        c74::min::setter{[this](const c74::min::atoms& args, int) -> c74::min::atoms {
            guard_message("num_universes", [&]() {
                if(!args.empty()) {
                    reconfigure_universe_range(static_cast<int>(universe), static_cast<int>(args[0]));
                }
            });
            return args;
        }}
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

    c74::min::queue<> m_output_queue{this,
        MIN_FUNCTION {
            guard_message("queued output", [&]() {
                std::lock_guard<std::mutex> lock(m_mutex);
                output_data();
            });
            return {};
        }
    };

    sacn_node(const c74::min::atoms& args = {})
#ifdef _WIN32
        : m_fd{INVALID_SOCKET}
#else
        : m_fd{-1}
#endif
        , m_running{false}
    {
        bbb::net::ensure_init();
        resize_universe_buffers(static_cast<int>(num_universes));
        init_socket();
    }

    ~sacn_node() {
        m_running = false;
        if(bbb::net::socket_valid(m_fd)) {
            leave_multicast_groups(static_cast<int>(universe), static_cast<int>(num_universes));
#ifdef _WIN32
            SOCKET fd = m_fd;
            m_fd = INVALID_SOCKET;
#else
            int fd = m_fd;
            m_fd = -1;
#endif
            bbb::net::close_socket(fd);
        }
        if(m_read_thread.joinable()) {
            m_read_thread.join();
        }
    }

    c74::min::message<> bang_msg{this, "bang", "Output current data in bang mode.",
        MIN_FUNCTION {
            guard_message("bang", [&]() {
                std::lock_guard<std::mutex> lock(m_mutex);
                output_data();
            });
            return {};
        }
    };

    c74::min::message<> maxclass_setup{this, "maxclass_setup",
        MIN_FUNCTION {
            guard_message("maxclass_setup", [&]() {
                cout << "bbb.sacn.node v" BBB_ARTNET_VERSION << c74::min::endl;
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
            cerr << "bbb.sacn.node: " << name << " failed: " << e.what() << c74::min::endl;
        } catch(...) {
            cerr << "bbb.sacn.node: " << name << " failed" << c74::min::endl;
        }
    }

    void resize_universe_buffers(int universe_count) {
        std::lock_guard<std::mutex> lock(m_mutex);
        int clamped_universe_count = std::max(1, universe_count);
        size_t buffer_size = 512 * static_cast<size_t>(clamped_universe_count);
        m_buffer.resize(buffer_size, 0);
        m_prev_buffer.resize(buffer_size, 0);
        m_received_universes.assign(static_cast<size_t>(clamped_universe_count), false);
        m_last_sequence.assign(static_cast<size_t>(clamped_universe_count), 255);
    }

    void reconfigure_universe_range(int first_universe, int universe_count) {
        int clamped_first_universe = std::max(1, first_universe);
        int clamped_universe_count = std::max(1, universe_count);
        if(bbb::net::socket_valid(m_fd)) {
            leave_multicast_groups(static_cast<int>(universe), static_cast<int>(num_universes));
        }
        resize_universe_buffers(clamped_universe_count);
        if(bbb::net::socket_valid(m_fd)) {
            join_multicast_groups(clamped_first_universe, clamped_universe_count);
        }
    }

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

        struct sockaddr_in address;
        std::memset(&address, 0, sizeof(address));
        address.sin_family = AF_INET;
        address.sin_port = htons(5568);
        address.sin_addr.s_addr = htonl(INADDR_ANY);

        if(bind(m_fd, reinterpret_cast<struct sockaddr*>(&address), sizeof(address)) < 0) {
            cerr << "bbb.sacn.node: failed to bind socket" << c74::min::endl;
            return;
        }

        join_multicast_groups(static_cast<int>(universe), static_cast<int>(num_universes));

        m_running = true;
        m_read_thread = std::thread([this]() {
            read_loop();
        });
    }

    void join_multicast_groups(int first_universe, int universe_count) {
        for(int i = 0; i < universe_count; ++i) {
            uint16_t current_universe = static_cast<uint16_t>(first_universe + i);
            auto multicast = sacn::universe_to_multicast(current_universe);

            struct ip_mreq request;
            std::memset(&request, 0, sizeof(request));
            char multicast_string[16];
            std::snprintf(multicast_string, sizeof(multicast_string), "%d.%d.%d.%d",
                multicast.a, multicast.b, multicast.c, multicast.d);
            inet_pton(AF_INET, multicast_string, &request.imr_multiaddr);
            request.imr_interface.s_addr = htonl(INADDR_ANY);
            setsockopt(m_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                reinterpret_cast<const char*>(&request), sizeof(request));
        }
    }

    void leave_multicast_groups(int first_universe, int universe_count) {
        for(int i = 0; i < universe_count; ++i) {
            uint16_t current_universe = static_cast<uint16_t>(first_universe + i);
            auto multicast = sacn::universe_to_multicast(current_universe);

            struct ip_mreq request;
            std::memset(&request, 0, sizeof(request));
            char multicast_string[16];
            std::snprintf(multicast_string, sizeof(multicast_string), "%d.%d.%d.%d",
                multicast.a, multicast.b, multicast.c, multicast.d);
            inet_pton(AF_INET, multicast_string, &request.imr_multiaddr);
            request.imr_interface.s_addr = htonl(INADDR_ANY);
            setsockopt(m_fd, IPPROTO_IP, IP_DROP_MEMBERSHIP,
                reinterpret_cast<const char*>(&request), sizeof(request));
        }
    }

    void read_loop() {
        try {
            uint8_t buffer[65536];
            while(m_running) {
                bbb::net::recv_len_t length = recvfrom(m_fd, reinterpret_cast<char*>(buffer), sizeof(buffer), 0, nullptr, nullptr);
                if(length <= 0) continue;
                if(static_cast<size_t>(length) < 126) continue;

                const uint8_t acn_id[12] = {0x41,0x53,0x43,0x2D,0x45,0x31,0x2E,0x31,0x37,0x00,0x00,0x00};
                if(std::memcmp(buffer + 4, acn_id, 12) != 0) continue;

                uint32_t root_vector = ntohl(*reinterpret_cast<const uint32_t*>(buffer + 18));
                if(root_vector != 0x00000004) continue;

                uint32_t framing_vector = ntohl(*reinterpret_cast<const uint32_t*>(buffer + 40));
                if(framing_vector != 0x00000002) continue;

                uint8_t sequence = buffer[75];
                uint16_t packet_universe = ntohs(*reinterpret_cast<const uint16_t*>(buffer + 78));

                uint8_t dmp_vector = buffer[117];
                if(dmp_vector != 0x02) continue;

                int dmp_data_length = static_cast<int>(length) - 126 - 1;
                if(dmp_data_length <= 0) continue;
                int copy_length = std::min(dmp_data_length, 512);

                std::lock_guard<std::mutex> lock(m_mutex);
                int universe_index = -1;
                int universe_count = static_cast<int>(m_received_universes.size());
                for(int i = 0; i < universe_count; ++i) {
                    if(static_cast<uint16_t>(universe + i) == packet_universe) {
                        universe_index = i;
                        break;
                    }
                }
                if(universe_index < 0) continue;

                if(sequence == m_last_sequence[universe_index]) continue;
                m_last_sequence[universe_index] = sequence;

                int offset = universe_index * 512;
                if(offset + copy_length <= static_cast<int>(m_buffer.size())) {
                    std::memcpy(m_buffer.data() + offset, buffer + 126, copy_length);
                }

                if(sync_universes && 1 < universe_count) {
                    m_received_universes[universe_index] = true;
                    bool all_received = true;
                    for(auto received : m_received_universes) {
                        if(!received) {
                            all_received = false;
                            break;
                        }
                    }
                    if(!all_received) continue;

                    for(auto&& received : m_received_universes) {
                        received = false;
                    }
                }

                handle_mode_output();
            }
        } catch(...) {
            m_running = false;
        }
    }

    void handle_mode_output() {
        if(mode == c74::min::symbol("update")) {
            m_output_queue.set();
        } else if(mode == c74::min::symbol("automatic")) {
            m_output_queue.set();
        } else if(mode == c74::min::symbol("change")) {
            if(m_buffer != m_prev_buffer) {
                m_output_queue.set();
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
