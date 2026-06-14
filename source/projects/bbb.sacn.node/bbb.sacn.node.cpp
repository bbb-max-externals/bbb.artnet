#include "c74_min.h"
#include <bbb/sacn/sacn_packet.h>
#include <bbb/sacn/transport.hpp>
#include <bbb/version.h>

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

    c74::min::attribute<c74::min::symbol> bind_ip{this, "bind_ip", "",
        c74::min::description{"Local interface IP for incoming sACN multicast (empty = all interfaces)."},
        c74::min::setter{[this](const c74::min::atoms& args, int) -> c74::min::atoms {
            if(m_constructed) {
                m_restart_timer.delay(0);
            }
            return args;
        }}
    };

    c74::min::timer<c74::min::timer_options::defer_delivery> m_init_timer{this,
        MIN_FUNCTION {
            guard_message("init", [&]() {
                init_socket();
            });
            return {};
        }
    };

    c74::min::timer<c74::min::timer_options::defer_delivery> m_restart_timer{this,
        MIN_FUNCTION {
            guard_message("restart", [&]() {
                restart_socket();
            });
            return {};
        }
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
        : m_running{false}
    {
        resize_universe_buffers(static_cast<int>(num_universes));
        m_constructed = true;
        m_init_timer.delay(0);
    }

    ~sacn_node() {
        stop_socket();
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
        if(m_receiver.valid()) {
            leave_multicast_groups(static_cast<int>(universe), static_cast<int>(num_universes), m_active_bind_ip);
        }
        resize_universe_buffers(clamped_universe_count);
        if(m_receiver.valid()) {
            join_multicast_groups(clamped_first_universe, clamped_universe_count, m_active_bind_ip);
        }
    }

    std::string get_bind_ip_str() {
        return std::string(bind_ip.get().c_str());
    }

    void init_socket() {
        bbb::sacn::receiver_config config;
        config.bind_ip = get_bind_ip_str();
        if(!m_receiver.open(config)) {
            cerr << "bbb.sacn.node: " << m_receiver.last_error() << c74::min::endl;
            return;
        }

        m_active_bind_ip = config.bind_ip;
        join_multicast_groups(static_cast<int>(universe), static_cast<int>(num_universes), m_active_bind_ip);

        cout << "bbb.sacn.node: bound to "
             << (m_active_bind_ip.empty() ? "all interfaces" : m_active_bind_ip)
             << c74::min::endl;

        m_running = true;
        m_read_thread = std::thread([this]() {
            read_loop();
        });
    }

    void stop_socket() {
        m_running = false;
        if(m_receiver.valid()) {
            leave_multicast_groups(static_cast<int>(universe), static_cast<int>(num_universes), m_active_bind_ip);
            m_receiver.close();
        }
        if(m_read_thread.joinable()) {
            m_read_thread.join();
        }
    }

    void restart_socket() {
        stop_socket();
        init_socket();
    }

    void join_multicast_groups(int first_universe, int universe_count, const std::string& interface_ip) {
        for(int i = 0; i < universe_count; ++i) {
            uint16_t current_universe = static_cast<uint16_t>(first_universe + i);
            if(!m_receiver.join_multicast_group(current_universe, interface_ip)) {
                cerr << "bbb.sacn.node: " << m_receiver.last_error() << c74::min::endl;
            }
        }
    }

    void leave_multicast_groups(int first_universe, int universe_count, const std::string& interface_ip) {
        for(int i = 0; i < universe_count; ++i) {
            uint16_t current_universe = static_cast<uint16_t>(first_universe + i);
            m_receiver.leave_multicast_group(current_universe, interface_ip);
        }
    }

    void read_loop() {
        try {
            uint8_t buffer[65536];
            while(m_running) {
                bbb::sacn::net::recv_len_t length = m_receiver.receive(buffer, sizeof(buffer));
                if(length <= 0) continue;

                sacn::dmx_data parsed{};
                if(!sacn::parse_dmx(buffer, static_cast<int>(length), parsed)) continue;

                std::lock_guard<std::mutex> lock(m_mutex);
                int universe_index = -1;
                int universe_count = static_cast<int>(m_received_universes.size());
                for(int i = 0; i < universe_count; ++i) {
                    if(static_cast<uint16_t>(universe + i) == parsed.universe) {
                        universe_index = i;
                        break;
                    }
                }
                if(universe_index < 0) continue;

                if(parsed.sequence == m_last_sequence[universe_index]) continue;
                m_last_sequence[universe_index] = parsed.sequence;

                int offset = universe_index * 512;
                if(offset + parsed.length <= static_cast<int>(m_buffer.size())) {
                    std::memcpy(m_buffer.data() + offset, parsed.data, static_cast<size_t>(parsed.length));
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

    bbb::sacn::receiver m_receiver;
    std::string m_active_bind_ip;
    std::vector<uint8_t> m_buffer;
    std::vector<uint8_t> m_prev_buffer;
    std::vector<bool> m_received_universes;
    std::vector<uint8_t> m_last_sequence;
    std::mutex m_mutex;
    std::thread m_read_thread;
    std::atomic<bool> m_running;
    bool m_constructed{false};
};

MIN_EXTERNAL(sacn_node);
