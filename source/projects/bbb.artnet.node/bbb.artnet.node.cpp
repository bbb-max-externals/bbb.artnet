#include "c74_min.h"
#include <bbb/artnet/artnet_node_manager.hpp>
#include <bbb/dmx_universe_messages.hpp>
#include <bbb/version.h>
#pragma push_macro("NIL")
#undef NIL
#include <bbb/osc/asio_receiver.hpp>
#include <bbb/osc/message.hpp>
#include <cstring>
#include <exception>
#include <vector>
#include <mutex>
#include <atomic>

class artnet_node_obj : public c74::min::object<artnet_node_obj> {
public:
    MIN_DESCRIPTION{"Receive DMX via Art-Net protocol."};
    MIN_TAGS{"dmx, artnet, lighting"};
    MIN_AUTHOR{"bbb"};
    MIN_RELATED{"bbb.artnet.controller"};

    c74::min::inlet<> input{this, "(bang) request output in bang mode"};
    c74::min::outlet<> output{this, "(list) DMX values as list of integers"};

    c74::min::argument<int> net_arg{this, "net", "Art-Net net address (0-127).",
        MIN_ARGUMENT_FUNCTION { net = arg; }
    };

    c74::min::argument<int> subnet_arg{this, "subnet", "Art-Net subnet address (0-15).",
        MIN_ARGUMENT_FUNCTION { subnet = arg; }
    };

    c74::min::argument<int> universe_arg{this, "universe", "Art-Net universe address (0-15).",
        MIN_ARGUMENT_FUNCTION { universe = arg; }
    };

    c74::min::attribute<int> net{this, "net", 0,
        c74::min::description{"Art-Net net address (0-127)."},
        c74::min::range{0, 127}
    };

    c74::min::attribute<int> subnet{this, "subnet", 0,
        c74::min::description{"Art-Net subnet address (0-15)."},
        c74::min::range{0, 15}
    };

    c74::min::attribute<int> universe{this, "universe", 0,
        c74::min::description{"Art-Net universe address (0-15)."},
        c74::min::range{0, 15}
    };

    c74::min::attribute<int> num_universes{this, "num_universes", 1,
        c74::min::description{"Number of universes to receive."},
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
        c74::min::description{"Number of DMX channels to output."},
        c74::min::range{1, 512}
    };

    c74::min::attribute<bool> sync_universes{this, "sync_universes", true,
        c74::min::description{"Wait for all universes before outputting."}
    };

    c74::min::attribute<int> mode{this, "mode", 1,
        c74::min::description{"Output mode: 0=automatic, 1=update, 2=bang, 3=change, 4=forced."},
        c74::min::enum_map{"automatic", "update", "bang", "change", "forced"}
    };

    c74::min::attribute<double> framerate{this, "framerate", 40.0,
        c74::min::description{"Framerate for forced mode (0.01 - 44)."},
        c74::min::range{0.01, 44.0}
    };

    c74::min::attribute<int> osc_port{this, "osc_port", 0,
        c74::min::description{"OSC receive port (0 = disabled)."},
        c74::min::range{0, 65535}
    };

    c74::min::attribute<c74::min::symbol> osc_bind_ip{this, "osc_bind_ip", "0.0.0.0",
        c74::min::description{"OSC listen address (0.0.0.0 = all interfaces)."}
    };

    c74::min::attribute<c74::min::symbol> target_ip{this, "target_ip", "",
        c74::min::description{"Destination IP for filter (empty = receive from all)."}
    };

    c74::min::attribute<c74::min::symbol> bind_ip{this, "bind_ip", "",
        c74::min::description{"Local IP to bind (empty = auto-detect)."}
    };

    c74::min::attribute<bool> verbose{this, "verbose", false,
        c74::min::description{"Enable verbose logging."}
    };

    c74::min::timer<c74::min::timer_options::defer_delivery> m_init_timer{this,
        MIN_FUNCTION {
            guard_message("initialization", [&]() {
                init_artnet();
                setup_osc();
            });
            return {};
        }
    };

    c74::min::timer<c74::min::timer_options::defer_delivery> m_osc_timer{this,
        MIN_FUNCTION {
            guard_message("OSC update", [&]() {
                if(m_osc_receiver) {
                    m_osc_receiver->update();
                }
            });
            m_osc_timer.delay(10);
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

    artnet_node_obj(const c74::min::atoms& args = {}) {
        resize_universe_buffers(static_cast<int>(num_universes));
        m_init_timer.delay(0);
    }

    ~artnet_node_obj() {
        if(m_managed_node) {
            m_managed_node->remove_callbacks(this);
            m_managed_node->release();
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

    c74::min::message<> dump_universe_msg{this, "dump_universe", "Output one universe as: universe N values...",
        MIN_FUNCTION {
            guard_message("dump_universe", [&]() {
                if(args.empty()) {
                    return;
                }
                std::lock_guard<std::mutex> lock(m_mutex);
                int universe_identifier = static_cast<int>(args[0]);
                int universe_index = universe_index_for_identifier(universe_identifier);
                if(universe_index < 0) {
                    cerr << "bbb.artnet.node: unknown universe " << universe_identifier << c74::min::endl;
                    return;
                }
                c74::min::atoms result;
                bbb::dmx::append_universe_dump(result, universe_identifier, m_buffer, universe_index, static_cast<int>(num_channels));
                output.send(result);
            });
            return {};
        }
    };

    c74::min::message<> set_universe_msg{this, "set_universe", "Store one universe without output: set_universe N values...",
        MIN_FUNCTION {
            guard_message("set_universe", [&]() {
                if(args.empty()) {
                    return;
                }
                std::lock_guard<std::mutex> lock(m_mutex);
                int universe_identifier = static_cast<int>(args[0]);
                int universe_index = universe_index_for_identifier(universe_identifier);
                if(universe_index < 0) {
                    cerr << "bbb.artnet.node: unknown universe " << universe_identifier << c74::min::endl;
                    return;
                }
                bbb::dmx::set_universe_data(m_buffer, universe_index, args, 1);
            });
            return {};
        }
    };

    c74::min::message<> maxclass_setup{this, "maxclass_setup",
        MIN_FUNCTION {
            guard_message("maxclass_setup", [&]() {
                cout << "bbb.artnet.node v" BBB_ARTNET_VERSION << c74::min::endl;
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
            cerr << "bbb.artnet.node: " << name << " failed: " << e.what() << c74::min::endl;
        } catch(...) {
            cerr << "bbb.artnet.node: " << name << " failed" << c74::min::endl;
        }
    }

    void resize_universe_buffers(int universe_count) {
        std::lock_guard<std::mutex> lock(m_mutex);
        int clamped_universe_count = std::max(1, universe_count);
        size_t buffer_size = 512 * static_cast<size_t>(clamped_universe_count);
        m_buffer.resize(buffer_size, 0);
        m_prev_buffer.resize(buffer_size, 0);
        m_received_universes.assign(static_cast<size_t>(clamped_universe_count), false);
    }

    std::string m_bip_str;

    int universe_index_for_identifier(int universe_identifier) const {
        uint16_t requested = static_cast<uint16_t>(universe_identifier & 0x7FFF);
        for(int i = 0; i < num_universes; ++i) {
            uint16_t port_address = bbb::artnet::protocol::make_sequential_port_address(net, subnet, universe, i);
            if(port_address == requested) {
                return i;
            }
        }
        return -1;
    }

    const char* resolve_bind_ip() {
        c74::min::symbol bip = bind_ip;
        m_bip_str = static_cast<const char*>(bip);
        if(!m_bip_str.empty() && m_bip_str != "0.0.0.0") {
            return m_bip_str.c_str();
        }

        c74::min::symbol tip = target_ip;
        std::string tip_str(static_cast<const char*>(tip));
        return bbb::artnet::resolve_bind_ip(tip_str);
    }

    void init_artnet() {
        const char* ip = resolve_bind_ip();
        m_managed_node = bbb::artnet::managed_node::get_or_create(ip);
        if(!m_managed_node || !m_managed_node->valid()) {
            cerr << "bbb.artnet.node: failed to create artnet node" << c74::min::endl;
            return;
        }

        bbb::artnet::callback_entry cb;
        cb.type = bbb::artnet::callback_type::dmx;
        cb.owner = this;
        cb.dmx_fn = [this](const uint8_t* data, int length, int universe_addr) {
            try {
                handle_dmx(data, length, universe_addr);
            } catch(...) {
            }
        };
        m_managed_node->add_callback(cb);
        m_managed_node->retain();
        cout << "bbb.artnet.node: bound to " << (ip ? ip : "0.0.0.0 (all interfaces)")
             << c74::min::endl;
    }

    void handle_dmx(const uint8_t* data, int length, int universe_addr) {
        std::lock_guard<std::mutex> lock(m_mutex);
        int port = -1;
        for(int i = 0; i < num_universes; ++i) {
            const uint16_t port_address = bbb::artnet::protocol::make_sequential_port_address(net, subnet, universe, i);
            if(port_address == static_cast<uint16_t>(universe_addr & 0x7FFF)) {
                port = i;
                break;
            }
        }
        if(port < 0) return;

        int offset = port * 512;
        int copy_len = std::min(length, 512);
        if(offset + copy_len <= static_cast<int>(m_buffer.size())) {
            std::memcpy(m_buffer.data() + offset, data, copy_len);
        }

        if(sync_universes && num_universes > 1) {
            if(port < static_cast<int>(m_received_universes.size())) {
                m_received_universes[port] = true;
            }
            bool all_received = true;
            for(auto received : m_received_universes) {
                if(!received) { all_received = false; break; }
            }
            if(!all_received) return;
            for(auto&& received : m_received_universes) {
                received = false;
            }
        }

        handle_mode_output();
    }

    void handle_mode_output() {
        int m = mode;
        if(m == 0 || m == 1) {
            m_output_queue.set();
        } else if(m == 3) {
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

    std::shared_ptr<bbb::artnet::managed_node> m_managed_node;
    std::vector<uint8_t> m_buffer;
    std::vector<uint8_t> m_prev_buffer;
    std::vector<bool> m_received_universes;
    std::mutex m_mutex;

    std::shared_ptr<bbb::osc::asio_receiver> m_osc_receiver;

    void setup_osc() {
        int port = osc_port;
        if(port <= 0) return;

        c74::min::symbol bind = osc_bind_ip;
        std::string host(static_cast<const char*>(bind));
        m_osc_receiver = bbb::osc::asio_receiver::get<bbb::osc::asio_receiver>(port, host);
        if(!m_osc_receiver) {
            cerr << "bbb.artnet.node: failed to setup OSC on port " << port << c74::min::endl;
            return;
        }

        m_osc_receiver->bind("/bang", [this](bbb::osc::message&) {
            try_call("bang", c74::min::atoms{});
        });

        m_osc_receiver->bind("/dump_universe", [this](bbb::osc::message& m) {
            c74::min::atoms max_args;
            for(auto& arg : m) {
                max_args.push_back(static_cast<int>(arg));
            }
            try_call("dump_universe", max_args);
        });

        m_osc_receiver->bind("/set_universe", [this](bbb::osc::message& m) {
            c74::min::atoms max_args;
            for(auto& arg : m) {
                max_args.push_back(static_cast<int>(arg));
            }
            try_call("set_universe", max_args);
        });

        m_osc_timer.delay(10);
    }
};

MIN_EXTERNAL(artnet_node_obj);
