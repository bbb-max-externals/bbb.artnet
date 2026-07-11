#include "c74_min.h"
#include <bbb/artnet/artnet_node_manager.hpp>
#include <bbb/dmx_universe_messages.hpp>
#include <bbb/universe_remap.hpp>
#include <bbb/version.h>
#pragma push_macro("NIL")
#undef NIL
#include <bbb/osc/asio_receiver.hpp>
#include <bbb/osc/message.hpp>
#include <cstring>
#include <vector>
#include <mutex>
#include <map>
#include <atomic>
#include <chrono>
#include <exception>

class artnet_controller : public c74::min::object<artnet_controller> {
private:
    bool m_constructed{false};

public:
    MIN_DESCRIPTION{"Send DMX via Art-Net protocol."};
    MIN_TAGS{"dmx, artnet, lighting"};
    MIN_AUTHOR{"bbb"};
    MIN_RELATED{"bbb.artnet.node"};

    c74::min::inlet<> input{this, "(list/bang/message) DMX data input"};
    c74::min::outlet<> output{this, "(bang/list) bang on packet transmission; list on dump"};

    c74::min::argument<int> net_arg{this, "net", "Art-Net net address (0-127).",
        MIN_ARGUMENT_FUNCTION {
            net = arg;
        }
    };

    c74::min::argument<int> subnet_arg{this, "subnet", "Art-Net subnet address (0-15).",
        MIN_ARGUMENT_FUNCTION {
            subnet = arg;
        }
    };

    c74::min::argument<int> universe_arg{this, "universe", "Art-Net universe address (0-15).",
        MIN_ARGUMENT_FUNCTION {
            universe = arg;
        }
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

    c74::min::attribute<int> input_universe_start{this, "input_universe_start", 1,
        c74::min::description{"First logical universe accepted by universe remapping."},
        c74::min::range{1, 32768}
    };

    c74::min::attribute<int> output_universe_start{this, "output_universe_start", 0,
        c74::min::description{"First output universe for 1-based remapping; 0 disables remapping and preserves net/subnet/universe behavior."},
        c74::min::range{0, 32768}
    };

    c74::min::attribute<int> num_universes{this, "num_universes", 1,
        c74::min::description{"Number of universes to manage."},
        c74::min::range{1, 32},
        c74::min::setter{[this](const c74::min::atoms& args, int) -> c74::min::atoms {
            if(m_constructed && !args.empty()) {
                try {
                    resize_universe_buffers(static_cast<int>(args[0]));
                } catch(const std::exception& e) {
                    cerr << "bbb.artnet.controller: failed to resize universes: " << e.what() << c74::min::endl;
                } catch(...) {
                    cerr << "bbb.artnet.controller: failed to resize universes" << c74::min::endl;
                }
            }
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

    c74::min::attribute<int> mode{this, "mode", 0,
        c74::min::description{"Output mode: 0=automatic, 1=bang, 2=update, 3=change, 4=forced."},
        c74::min::enum_map{"automatic", "bang", "update", "change", "forced"},
        c74::min::setter{[this](const c74::min::atoms& args, int) -> c74::min::atoms {
            c74::min::atoms normalized_args = args;
            try {
                if(m_constructed && !args.empty()) {
                    int mode_value = mode_atom_to_int(args[0]);
                    normalized_args = c74::min::atoms{mode_value};
                    start_forced_timer_for_mode(mode_value);
                }
            } catch(const std::exception& e) {
                m_running = false;
                cerr << "bbb.artnet.controller: failed to set mode: " << e.what() << c74::min::endl;
            } catch(...) {
                m_running = false;
                cerr << "bbb.artnet.controller: failed to set mode" << c74::min::endl;
            }
            return normalized_args;
        }}
    };

    c74::min::attribute<double> framerate{this, "framerate", 40.0,
        c74::min::description{"Framerate for forced mode (0.01 - 44)."},
        c74::min::range{0.01, 44.0}
    };

    c74::min::attribute<c74::min::symbol> target_ip{this, "target_ip", "",
        c74::min::description{"Destination IP (empty = broadcast, broadcast addr = broadcast, unicast addr = unicast)."}
    };

    c74::min::attribute<int> origin{this, "origin", 1,
        c74::min::description{"Channel index origin: 1 = 1-based (default), 0 = 0-based."},
        c74::min::range{0, 1},
        c74::min::style::enum_index,
        c74::min::category{"DMX"}
    };

    c74::min::attribute<c74::min::symbol> bind_ip{this, "bind_ip", "",
        c74::min::description{"Local IP to bind (empty = auto-detect from target_ip subnet)."}
    };

    c74::min::attribute<bool> send_only{this, "send_only", false,
        c74::min::description{"Send ArtDmx from an ephemeral UDP socket without binding/listening on Art-Net port 6454."}
    };

    c74::min::attribute<c74::min::symbol> osc_bind_ip{this, "osc_bind_ip", "0.0.0.0",
        c74::min::description{"OSC listen address (0.0.0.0 = all interfaces)."}
    };

    c74::min::attribute<int> osc_port{this, "osc_port", 0,
        c74::min::description{"OSC receive port (0 = disabled)."},
        c74::min::range{0, 65535}
    };

    c74::min::attribute<bool> verbose{this, "verbose", false,
        c74::min::description{"Enable verbose logging."}
    };

    c74::min::timer<c74::min::timer_options::defer_delivery> m_init_timer{this,
        MIN_FUNCTION {
            try {
                init_artnet();
                setup_osc();
            } catch(const std::exception& e) {
                cerr << "bbb.artnet.controller: initialization failed: " << e.what() << c74::min::endl;
            } catch(...) {
                cerr << "bbb.artnet.controller: initialization failed" << c74::min::endl;
            }
            return {};
        }
    };

    c74::min::timer<c74::min::timer_options::defer_delivery> m_osc_timer{this,
        MIN_FUNCTION {
            try {
                if(m_osc_receiver) {
                    m_osc_receiver->update();
                }
            } catch(const std::exception& e) {
                cerr << "bbb.artnet.controller: OSC update failed: " << e.what() << c74::min::endl;
            } catch(...) {
                cerr << "bbb.artnet.controller: OSC update failed" << c74::min::endl;
            }
            m_osc_timer.delay(10);
            return {};
        }
    };

    artnet_controller(const c74::min::atoms& args = {})
        : m_running{false}
        , m_dirty{false}
        , m_send_only_socket(m_send_only_io)
    {
        m_constructed = true;
        resize_universe_buffers(static_cast<int>(num_universes));
        m_init_timer.delay(0);
    }

    ~artnet_controller() {
        m_running = false;
        if(m_managed_node) {
            m_managed_node->remove_callbacks(this);
        }
        close_send_only_socket();
    }

    c74::min::message<> list_msg{this, "list", "Set DMX values from a list.",
        MIN_FUNCTION {
            guard_message("list", [&]() {
                std::lock_guard<std::mutex> lock(m_mutex);
                size_t count = std::min(args.size(), m_buffer.size());
                for(size_t i = 0; i < count; ++i) {
                    int val = static_cast<int>(args[i]);
                    m_buffer[i] = static_cast<uint8_t>(std::max(0, std::min(255, val)));
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

    c74::min::message<> channel_msg{this, "channel", "Set a single DMX channel.",
        MIN_FUNCTION {
            guard_message("channel", [&]() {
                if(args.size() < 2) {
                    return;
                }
                std::lock_guard<std::mutex> lock(m_mutex);
                int index = static_cast<int>(args[0]) - static_cast<int>(origin);
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
                int index = static_cast<int>(args[0]) - static_cast<int>(origin);
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
                    int val = static_cast<int>(args[i]);
                    m_buffer[i] = static_cast<uint8_t>(std::max(0, std::min(255, val)));
                }
                m_dirty = true;
            });
            return {};
        }
    };

    c74::min::message<> set_offset_msg{this, "set_offset", "Store DMX values at offset without sending.",
        MIN_FUNCTION {
            guard_message("set_offset", [&]() {
                if(args.empty()) {
                    return;
                }
                std::lock_guard<std::mutex> lock(m_mutex);
                int offset = static_cast<int>(args[0]) - static_cast<int>(origin);
                for(size_t i = 1; i < args.size(); ++i) {
                    size_t idx = static_cast<size_t>(offset + static_cast<int>(i) - 1);
                    if(idx < m_buffer.size()) {
                        int val = static_cast<int>(args[i]);
                        m_buffer[idx] = static_cast<uint8_t>(std::max(0, std::min(255, val)));
                    }
                }
                m_dirty = true;
            });
            return {};
        }
    };

    c74::min::message<> maxclass_setup{this, "maxclass_setup",
        MIN_FUNCTION {
            cout << "bbb.artnet.controller v" BBB_ARTNET_VERSION << c74::min::endl;
            return {};
        }
    };

    c74::min::message<> dump_msg{this, "dump", "Output current DMX buffer as list.",
        MIN_FUNCTION {
            guard_message("dump", [&]() {
                std::lock_guard<std::mutex> lock(m_mutex);
                send_universe_remap_dump();
                c74::min::atoms result;
                int total = std::min(num_channels * num_universes, static_cast<int>(m_buffer.size()));
                result.reserve(total);
                for(int i = 0; i < total; ++i) {
                    result.push_back(static_cast<int>(m_buffer[i]));
                }
                output.send(result);
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
                    cerr << "bbb.artnet.controller: unknown universe " << universe_identifier << c74::min::endl;
                    return;
                }
                c74::min::atoms result;
                bbb::dmx::append_universe_dump(result, universe_identifier, m_buffer, universe_index, static_cast<int>(num_channels));
                output.send(result);
            });
            return {};
        }
    };

    c74::min::message<> set_universe_msg{this, "set_universe", "Store one universe without sending: set_universe N values...",
        MIN_FUNCTION {
            guard_message("set_universe", [&]() {
                if(args.empty()) {
                    return;
                }
                std::lock_guard<std::mutex> lock(m_mutex);
                int universe_identifier = static_cast<int>(args[0]);
                int universe_index = universe_index_for_identifier(universe_identifier);
                if(universe_index < 0) {
                    cerr << "bbb.artnet.controller: unknown universe " << universe_identifier << c74::min::endl;
                    return;
                }
                if(bbb::dmx::set_universe_data(m_buffer, universe_index, args, 1)) {
                    m_dirty = true;
                }
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
            cerr << "bbb.artnet.controller: " << name << " failed: " << e.what() << c74::min::endl;
        } catch(...) {
            m_running = false;
            cerr << "bbb.artnet.controller: " << name << " failed" << c74::min::endl;
        }
    }

    void resize_universe_buffers(int universe_count) {
        std::lock_guard<std::mutex> lock(m_mutex);
        size_t buffer_size = 512 * static_cast<size_t>(std::max(1, universe_count));
        m_buffer.resize(buffer_size, 0);
        m_prev_buffer.resize(buffer_size, 0);
    }

    bool universe_remap_is_enabled() const {
        return bbb::dmx::universe_remap_enabled(static_cast<int>(output_universe_start));
    }

    int universe_index_for_identifier(int universe_identifier) const {
        if(universe_remap_is_enabled()) {
            int index = universe_identifier - static_cast<int>(input_universe_start);
            if(0 <= index && index < static_cast<int>(num_universes)) {
                return index;
            }
            return -1;
        }

        uint16_t requested = static_cast<uint16_t>(universe_identifier & 0x7FFF);
        for(int i = 0; i < num_universes; ++i) {
            uint16_t port_address = bbb::artnet::protocol::make_sequential_port_address(net, subnet, universe, i);
            if(port_address == requested) {
                return i;
            }
        }
        return -1;
    }

    bool port_address_for_buffer_index(int index, uint16_t& port_address) {
        if(universe_remap_is_enabled()) {
            int logical_universe = static_cast<int>(input_universe_start) + index;
            bbb::dmx::universe_remap_result result = bbb::dmx::remap_artnet_port_address(
                logical_universe,
                static_cast<int>(input_universe_start),
                static_cast<int>(output_universe_start));
            if(!result.valid) {
                cerr << "bbb.artnet.controller: remapped universe out of Art-Net range for logical universe "
                     << logical_universe << c74::min::endl;
                return false;
            }
            port_address = static_cast<uint16_t>(result.universe);
            return true;
        }

        port_address = bbb::artnet::protocol::make_sequential_port_address(net, subnet, universe, index);
        return true;
    }

    void send_universe_remap_dump() {
        if(!universe_remap_is_enabled()) {
            return;
        }

        c74::min::atoms result;
        result.push_back(c74::min::symbol("universe_remap"));
        result.push_back(c74::min::symbol("input_start"));
        result.push_back(static_cast<int>(input_universe_start));
        result.push_back(c74::min::symbol("output_start"));
        result.push_back(static_cast<int>(output_universe_start));
        result.push_back(c74::min::symbol("enabled"));
        result.push_back(1);
        output.send(result);
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

    bool is_unicast_mode() const {
        c74::min::symbol tip = target_ip;
        std::string tip_str(static_cast<const char*>(tip));
        return !tip_str.empty() && !bbb::artnet::is_broadcast_ip(tip_str);
    }

    std::string get_target_ip_str() const {
        c74::min::symbol tip = target_ip;
        return static_cast<const char*>(tip);
    }

    void init_artnet() {
        if(static_cast<bool>(send_only)) {
            if(m_managed_node) {
                m_managed_node->remove_callbacks(this);
                m_managed_node.reset();
            }
            open_send_only_socket();
            start_forced_timer();
            return;
        }

        close_send_only_socket();
        const char* ip = resolve_bind_ip();
        m_managed_node = bbb::artnet::managed_node::get_or_create(ip);
        if(!m_managed_node || !m_managed_node->valid()) {
            cerr << "bbb.artnet.controller: failed to create artnet node" << c74::min::endl;
            return;
        }
        cout << "bbb.artnet.controller: bound to " << (ip ? ip : "0.0.0.0 (all interfaces)")
             << ", mode: " << (is_unicast_mode() ? "unicast" : "broadcast")
             << c74::min::endl;

        start_forced_timer();
    }

    c74::min::timer<c74::min::timer_options::defer_delivery> m_forced_timer{this,
        MIN_FUNCTION {
            try {
                if(m_running) {
                    send_all();
                    double interval = 1000.0 / static_cast<double>(framerate);
                    m_forced_timer.delay(interval);
                }
            } catch(const std::exception& e) {
                m_running = false;
                cerr << "bbb.artnet.controller: forced send failed: " << e.what() << c74::min::endl;
            } catch(...) {
                m_running = false;
                cerr << "bbb.artnet.controller: forced send failed" << c74::min::endl;
            }
            return {};
        }
    };

    void start_forced_timer() {
        start_forced_timer_for_mode(static_cast<int>(mode));
    }

    int mode_atom_to_int(const c74::min::atom& atom) const {
        if(atom.a_type == c74::max::A_SYM) {
            c74::min::symbol value{atom};
            if(value == "automatic") return 0;
            if(value == "bang") return 1;
            if(value == "update") return 2;
            if(value == "change") return 3;
            if(value == "forced") return 4;
        }

        int mode_value = static_cast<int>(atom);
        return std::max(0, std::min(4, mode_value));
    }

    void start_forced_timer_for_mode(int mode_value) {
        try {
            if(mode_value == 4) {
                m_running = true;
                double interval = 1000.0 / static_cast<double>(framerate);
                m_forced_timer.delay(interval);
            } else {
                m_running = false;
            }
        } catch(const std::exception& e) {
            m_running = false;
            cerr << "bbb.artnet.controller: failed to start forced timer: " << e.what() << c74::min::endl;
        } catch(...) {
            m_running = false;
            cerr << "bbb.artnet.controller: failed to start forced timer" << c74::min::endl;
        }
    }

    void handle_mode_send() {
        int m = mode;
        if(m == 0) {
            send_all();
        } else if(m == 2) {
            send_all();
        } else if(m == 3) {
            if(m_buffer != m_prev_buffer) {
                send_all();
            }
        }
    }

    void send_all() {
        try {
            if(static_cast<bool>(send_only)) {
                send_all_send_only();
                return;
            }

            if(!m_managed_node || !m_managed_node->valid()) return;

            bool unicast = is_unicast_mode();
            std::string tip = get_target_ip_str();

            if(blackout) {
                std::vector<uint8_t> zeros(512, 0);
                for(int i = 0; i < num_universes; ++i) {
                    uint16_t port_addr{0};
                    if(!port_address_for_buffer_index(i, port_addr)) continue;
                    if(unicast) {
                        m_managed_node->send_dmx_unicast(tip.c_str(),
                            port_addr, 512, zeros.data());
                    } else {
                        m_managed_node->send_dmx_broadcast(
                            port_addr, 512, zeros.data());
                    }
                }
            } else {
                for(int i = 0; i < num_universes; ++i) {
                    int offset = i * 512;
                    int length = std::min(512, static_cast<int>(m_buffer.size()) - offset);
                    if(length > 0) {
                        uint16_t port_addr{0};
                        if(!port_address_for_buffer_index(i, port_addr)) continue;
                        if(unicast) {
                            m_managed_node->send_dmx_unicast(tip.c_str(),
                                port_addr,
                                static_cast<int16_t>(length),
                                m_buffer.data() + offset);
                        } else {
                            m_managed_node->send_dmx_broadcast(
                                port_addr,
                                static_cast<int16_t>(length),
                                m_buffer.data() + offset);
                        }
                    }
                }
            }

            m_prev_buffer = m_buffer;
            m_dirty = false;
            output.send(c74::min::k_sym_bang);
        } catch(const std::exception& e) {
            m_running = false;
            cerr << "bbb.artnet.controller: send failed: " << e.what() << c74::min::endl;
        } catch(...) {
            m_running = false;
            cerr << "bbb.artnet.controller: send failed" << c74::min::endl;
        }
    }

    void open_send_only_socket() {
        close_send_only_socket();

        asio::error_code error_code;
        m_send_only_socket.open(asio::ip::udp::v4(), error_code);
        if(error_code) {
            cerr << "bbb.artnet.controller: send_only socket open failed: "
                 << error_code.message().c_str() << c74::min::endl;
            return;
        }

        m_send_only_socket.set_option(asio::socket_base::broadcast(true), error_code);
        if(error_code) {
            cerr << "bbb.artnet.controller: send_only broadcast option failed: "
                 << error_code.message().c_str() << c74::min::endl;
            close_send_only_socket();
            return;
        }

        std::string bind_ip_string = get_bind_ip_str();
        if(!bind_ip_string.empty() && bind_ip_string != "0.0.0.0") {
            asio::ip::address_v4 bind_address = asio::ip::make_address_v4(bind_ip_string, error_code);
            if(error_code) {
                cerr << "bbb.artnet.controller: invalid send_only bind_ip: "
                     << bind_ip_string.c_str() << c74::min::endl;
                close_send_only_socket();
                return;
            }

            m_send_only_socket.bind(asio::ip::udp::endpoint(bind_address, 0), error_code);
            if(error_code) {
                cerr << "bbb.artnet.controller: send_only bind failed for "
                     << bind_ip_string.c_str() << ":0: "
                     << error_code.message().c_str() << c74::min::endl;
                close_send_only_socket();
                return;
            }

            asio::ip::udp::endpoint local_endpoint = m_send_only_socket.local_endpoint(error_code);
            if(error_code) {
                cerr << "bbb.artnet.controller: send_only local endpoint query failed: "
                     << error_code.message().c_str() << c74::min::endl;
                close_send_only_socket();
                return;
            }

            cout << "bbb.artnet.controller: send_only bound to "
                 << local_endpoint.address().to_string().c_str()
                 << ":" << local_endpoint.port()
                 << ", mode: " << (is_unicast_mode() ? "unicast" : "broadcast")
                 << c74::min::endl;
            return;
        }

        cout << "bbb.artnet.controller: send_only open with OS-selected interface, mode: "
             << (is_unicast_mode() ? "unicast" : "broadcast")
             << c74::min::endl;
    }

    void close_send_only_socket() {
        if(m_send_only_socket.is_open()) {
            asio::error_code error_code;
            m_send_only_socket.close(error_code);
        }
    }

    bool send_dmx_send_only(uint16_t port_address, int16_t length, const uint8_t* data) {
        if(!m_send_only_socket.is_open()) {
            open_send_only_socket();
        }
        if(!m_send_only_socket.is_open()) {
            return false;
        }
        if(length < 1 || 512 < length || data == nullptr) {
            return false;
        }

        std::vector<uint8_t> packet = bbb::artnet::protocol::build_dmx_packet(
            next_dmx_sequence(port_address),
            0,
            port_address,
            data,
            static_cast<uint16_t>(length)
        );

        asio::error_code error_code;
        asio::ip::udp::endpoint destination_endpoint;
        if(is_unicast_mode()) {
            destination_endpoint = asio::ip::udp::endpoint(
                asio::ip::make_address(get_target_ip_str(), error_code),
                bbb::artnet::protocol::ARTNET_PORT
            );
            if(error_code) {
                cerr << "bbb.artnet.controller: invalid target_ip: "
                     << get_target_ip_str().c_str() << c74::min::endl;
                return false;
            }
        } else {
            destination_endpoint = asio::ip::udp::endpoint(
                asio::ip::address_v4::broadcast(),
                bbb::artnet::protocol::ARTNET_PORT
            );
        }

        m_send_only_socket.send_to(asio::buffer(packet), destination_endpoint, 0, error_code);
        if(!error_code) {
            return true;
        }

        asio::error_code first_error = error_code;
        if(is_retryable_network_error(first_error)) {
            close_send_only_socket();
            open_send_only_socket();
            if(m_send_only_socket.is_open()) {
                error_code.clear();
                m_send_only_socket.send_to(asio::buffer(packet), destination_endpoint, 0, error_code);
                if(!error_code) {
                    if(verbose) {
                        cout << "bbb.artnet.controller: send_only recovered after reopening socket"
                             << c74::min::endl;
                    }
                    return true;
                }
            }
        }

        asio::error_code local_endpoint_error;
        asio::ip::udp::endpoint local_endpoint = m_send_only_socket.is_open()
            ? m_send_only_socket.local_endpoint(local_endpoint_error)
            : asio::ip::udp::endpoint{};

        cerr << "bbb.artnet.controller: send_only send failed: "
             << error_code.message().c_str()
             << ", destination=" << destination_endpoint.address().to_string().c_str()
             << ":" << destination_endpoint.port();
        if(!local_endpoint_error && 0 < local_endpoint.port()) {
            cerr << ", local=" << local_endpoint.address().to_string().c_str()
                 << ":" << local_endpoint.port();
        }
        if(error_code != first_error) {
            cerr << ", first_error=" << first_error.message().c_str();
        }
        cerr << c74::min::endl;
        return false;
    }

    void send_all_send_only() {
        bool success = true;
        if(blackout) {
            std::vector<uint8_t> zeros(512, 0);
            for(int i = 0; i < num_universes; ++i) {
                uint16_t port_address{0};
                if(!port_address_for_buffer_index(i, port_address)) continue;
                success = send_dmx_send_only(port_address, 512, zeros.data()) && success;
            }
        } else {
            for(int i = 0; i < num_universes; ++i) {
                int offset = i * 512;
                int length = std::min(512, static_cast<int>(m_buffer.size()) - offset);
                if(0 < length) {
                    uint16_t port_address{0};
                    if(!port_address_for_buffer_index(i, port_address)) continue;
                    success = send_dmx_send_only(
                        port_address,
                        static_cast<int16_t>(length),
                        m_buffer.data() + offset
                    ) && success;
                }
            }
        }

        if(!success) {
            m_running = false;
            return;
        }

        m_prev_buffer = m_buffer;
        m_dirty = false;
        output.send(c74::min::k_sym_bang);
    }

    bool is_retryable_network_error(const asio::error_code& error_code) const {
        if(error_code == asio::error::host_unreachable
            || error_code == asio::error::network_down
            || error_code == asio::error::network_unreachable) {
            return true;
        }
#if defined(EHOSTDOWN)
        if(error_code.value() == EHOSTDOWN) {
            return true;
        }
#endif
        return false;
    }

    uint8_t next_dmx_sequence(uint16_t port_address) {
        uint8_t& sequence = m_dmx_sequences[port_address];
        sequence = bbb::artnet::protocol::next_enabled_sequence(sequence);
        return sequence;
    }

    std::string get_bind_ip_str() const {
        c74::min::symbol bind = bind_ip;
        return static_cast<const char*>(bind);
    }

    std::shared_ptr<bbb::artnet::managed_node> m_managed_node;
    std::vector<uint8_t> m_buffer;
    std::vector<uint8_t> m_prev_buffer;
    std::mutex m_mutex;
    std::atomic<bool> m_running;
    std::atomic<bool> m_dirty;
    asio::io_context m_send_only_io;
    asio::ip::udp::socket m_send_only_socket;
    std::map<uint16_t, uint8_t> m_dmx_sequences;

    std::shared_ptr<bbb::osc::asio_receiver> m_osc_receiver;
    std::string m_bip_str;

    void setup_osc() {
        int port = osc_port;
        if(port <= 0) return;

        c74::min::symbol bind = osc_bind_ip;
        std::string host(static_cast<const char*>(bind));
        m_osc_receiver = bbb::osc::asio_receiver::get<bbb::osc::asio_receiver>(port, host);
        if(!m_osc_receiver) {
            cerr << "bbb.artnet.controller: failed to setup OSC on port " << port << c74::min::endl;
            return;
        }

        auto dispatch = [this](const std::string& address, const c74::min::atoms& max_args) {
            if(address == "/list" || address == "/set") {
                try_call("list", max_args);
            } else if(address == "/bang") {
                try_call("bang", c74::min::atoms{});
            } else if(address == "/channel") {
                try_call("channel", max_args);
            } else if(address == "/setchannel") {
                try_call("setchannel", max_args);
            } else if(address == "/set_offset") {
                try_call("set_offset", max_args);
            } else if(address == "/dump") {
                try_call("dump", c74::min::atoms{});
            } else if(address == "/dump_universe") {
                try_call("dump_universe", max_args);
            } else if(address == "/set_universe") {
                try_call("set_universe", max_args);
            } else if(address == "/blackout") {
                if(!max_args.empty()) {
                    blackout = static_cast<bool>(max_args[0]);
                }
            }
        };

        m_osc_receiver->bind("/list", [this, dispatch](bbb::osc::message& m) {
            c74::min::atoms max_args;
            for(auto& arg : m) {
                max_args.push_back(static_cast<int>(arg));
            }
            dispatch("/list", max_args);
        });

        m_osc_receiver->bind("/set", [this, dispatch](bbb::osc::message& m) {
            c74::min::atoms max_args;
            for(auto& arg : m) {
                max_args.push_back(static_cast<int>(arg));
            }
            dispatch("/set", max_args);
        });

        m_osc_receiver->bind("/bang", [this, dispatch](bbb::osc::message&) {
            dispatch("/bang", c74::min::atoms{});
        });

        m_osc_receiver->bind("/channel", [this, dispatch](bbb::osc::message& m) {
            c74::min::atoms max_args;
            for(auto& arg : m) {
                max_args.push_back(static_cast<int>(arg));
            }
            dispatch("/channel", max_args);
        });

        m_osc_receiver->bind("/setchannel", [this, dispatch](bbb::osc::message& m) {
            c74::min::atoms max_args;
            for(auto& arg : m) {
                max_args.push_back(static_cast<int>(arg));
            }
            dispatch("/setchannel", max_args);
        });

        m_osc_receiver->bind("/set_offset", [this, dispatch](bbb::osc::message& m) {
            c74::min::atoms max_args;
            for(auto& arg : m) {
                max_args.push_back(static_cast<int>(arg));
            }
            dispatch("/set_offset", max_args);
        });

        m_osc_receiver->bind("/set_universe", [this, dispatch](bbb::osc::message& m) {
            c74::min::atoms max_args;
            for(auto& arg : m) {
                max_args.push_back(static_cast<int>(arg));
            }
            dispatch("/set_universe", max_args);
        });

        m_osc_receiver->bind("/dump", [this, dispatch](bbb::osc::message&) {
            dispatch("/dump", c74::min::atoms{});
        });

        m_osc_receiver->bind("/dump_universe", [this, dispatch](bbb::osc::message& m) {
            c74::min::atoms max_args;
            for(auto& arg : m) {
                max_args.push_back(static_cast<int>(arg));
            }
            dispatch("/dump_universe", max_args);
        });

        m_osc_receiver->bind("/blackout", [this](bbb::osc::message& m) {
            if(m.size() > 0) {
                blackout = static_cast<bool>(m[0]);
            }
        });

        m_osc_timer.delay(10);
    }
};

MIN_EXTERNAL(artnet_controller);
