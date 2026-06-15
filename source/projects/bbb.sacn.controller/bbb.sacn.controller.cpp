#include "c74_min.h"
#include <bbb/sacn/sacn_packet.h>
#include <bbb/sacn/transport.hpp>
#include <bbb/dmx_universe_messages.hpp>
#include <bbb/version.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstring>
#include <exception>
#include <mutex>
#include <string>
#include <vector>

class sacn_controller : public c74::min::object<sacn_controller> {
private:
    bool m_constructed{false};

public:
    MIN_DESCRIPTION{"Send DMX via sACN (E1.31) protocol."};
    MIN_TAGS{"dmx, sacn, e1.31, lighting"};
    MIN_AUTHOR{"bbb"};
    MIN_RELATED{"bbb.sacn.node"};

    c74::min::inlet<> input{this, "(list/bang/message) DMX data input"};
    c74::min::outlet<> output{this, "(bang/list) bang on packet transmission; list on dump"};

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
                if(m_constructed && !args.empty()) {
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

    c74::min::attribute<int> mode{this, "mode", 0,
        c74::min::description{"Output mode: 0=automatic, 1=bang, 2=update, 3=change, 4=forced."},
        c74::min::enum_map{"automatic", "bang", "update", "change", "forced"},
        c74::min::setter{[this](const c74::min::atoms& args, int) -> c74::min::atoms {
            c74::min::atoms normalized_args = args;
            guard_message("mode", [&]() {
                if(m_constructed && !args.empty()) {
                    int mode_value = mode_atom_to_int(args[0]);
                    normalized_args = c74::min::atoms{mode_value};
                    start_forced_timer_for_mode(mode_value);
                }
            });
            return normalized_args;
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

    c74::min::attribute<c74::min::symbol> target_ip{this, "target_ip", "",
        c74::min::description{"Destination IP (empty = per-universe multicast, unicast address = unicast)."}
    };

    c74::min::attribute<c74::min::symbol> bind_ip{this, "bind_ip", "",
        c74::min::description{"Local interface IP for outgoing sACN (empty = auto/default route)."}
    };

    c74::min::attribute<bool> send_only{this, "send_only", true,
        c74::min::description{"API parity with bbb.artnet.controller; sACN controller is always send-only."}
    };

    c74::min::attribute<int> origin{this, "origin", 1,
        c74::min::description{"Channel index origin: 1 = 1-based (default), 0 = 0-based."},
        c74::min::range{0, 1},
        c74::min::style::enum_index,
        c74::min::category{"DMX"}
    };

    c74::min::attribute<bool> unicast{this, "unicast", false,
        c74::min::description{"Legacy compatibility: use unicast_ip when target_ip is empty."}
    };

    c74::min::attribute<c74::min::symbol> unicast_ip{this, "unicast_ip", "127.0.0.1",
        c74::min::description{"Legacy compatibility: destination IP when unicast is enabled and target_ip is empty."}
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
        : m_running{false}
        , m_dirty{false}
        , m_sequence{0}
    {
        resize_universe_buffers(static_cast<int>(num_universes));
        m_cid = sacn::generate_cid();
        m_constructed = true;
        m_init_timer.delay(0);
    }

    ~sacn_controller() {
        m_running = false;
        m_sender.close();
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
                    int value = static_cast<int>(args[i]);
                    m_buffer[i] = static_cast<uint8_t>(std::max(0, std::min(255, value)));
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
                    cerr << "bbb.sacn.controller: unknown universe " << universe_identifier << c74::min::endl;
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
                    cerr << "bbb.sacn.controller: unknown universe " << universe_identifier << c74::min::endl;
                    return;
                }
                if(bbb::dmx::set_universe_data(m_buffer, universe_index, args, 1)) {
                    m_dirty = true;
                }
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

    int universe_index_for_identifier(int universe_identifier) const {
        int index = universe_identifier - static_cast<int>(universe);
        if(0 <= index && index < static_cast<int>(num_universes)) {
            return index;
        }
        return -1;
    }

    void init_socket() {
        std::string local_ip = resolve_bind_ip();
        bbb::sacn::sender_config config;
        config.bind_ip = local_ip;
        if(!m_sender.open(config)) {
            cerr << "bbb.sacn.controller: " << m_sender.last_error() << c74::min::endl;
            return;
        }

        log_bind_interface(local_ip);

        int mode_value = mode;
        start_forced_timer_for_mode(mode_value);
    }

    std::string symbol_to_string(c74::min::symbol value) const {
        return static_cast<const char*>(value);
    }

    std::string get_target_ip_str() const {
        c74::min::symbol value = target_ip;
        return symbol_to_string(value);
    }

    std::string get_legacy_unicast_ip_str() const {
        c74::min::symbol value = unicast_ip;
        return symbol_to_string(value);
    }

    std::string get_bind_ip_str() const {
        c74::min::symbol value = bind_ip;
        return symbol_to_string(value);
    }

    bool is_ipv4_multicast(const std::string& ip_string) const {
        return bbb::sacn::is_ipv4_multicast(ip_string);
    }

    std::string multicast_ip_for_universe(uint16_t current_universe) const {
        return bbb::sacn::multicast_ip_for_universe(current_universe);
    }

    std::string destination_ip_for_universe(uint16_t current_universe) const {
        std::string target = get_target_ip_str();
        if(!target.empty()) {
            return target;
        }

        if(static_cast<bool>(unicast)) {
            return get_legacy_unicast_ip_str();
        }

        return multicast_ip_for_universe(current_universe);
    }

    bool is_unicast_mode() const {
        std::string target = get_target_ip_str();
        if(!target.empty()) {
            return !is_ipv4_multicast(target);
        }

        return static_cast<bool>(unicast);
    }

    std::string resolve_bind_ip() {
        std::string bind_string = get_bind_ip_str();
        if(!bind_string.empty() && bind_string != "0.0.0.0") {
            return bind_string;
        }

        std::string target = get_target_ip_str();
        if(!target.empty()) {
            return bbb::sacn::resolve_bind_ip_for_target(target);
        }

        if(static_cast<bool>(unicast)) {
            return bbb::sacn::resolve_bind_ip_for_target(get_legacy_unicast_ip_str());
        }

        return {};
    }

    void log_bind_interface(const std::string& local_ip) {
        if(local_ip.empty()) {
            cout << "bbb.sacn.controller: send-only using default interface, mode: "
                 << (is_unicast_mode() ? "unicast" : "multicast") << c74::min::endl;
            return;
        }

        cout << "bbb.sacn.controller: send-only using " << local_ip
             << ", mode: " << (is_unicast_mode() ? "unicast" : "multicast")
             << c74::min::endl;
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
        if(mode_value == 4) {
            m_running = true;
            double interval = 1000.0 / static_cast<double>(framerate);
            m_forced_timer.delay(interval);
        } else {
            m_running = false;
        }
    }

    void handle_mode_send() {
        int mode_value = mode;
        if(mode_value == 0) {
            send_all();
        } else if(mode_value == 2) {
            send_all();
        } else if(mode_value == 3) {
            if(m_buffer != m_prev_buffer) {
                send_all();
            }
        }
    }

    void send_all() {
        if(!m_sender.valid()) return;

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

            std::string destination_ip = destination_ip_for_universe(current_universe);
            int packet_size = sacn::packet_size_for_data_length(length);
            if(!m_sender.send_packet(destination_ip, &packet, packet_size)) {
                cerr << "bbb.sacn.controller: " << m_sender.last_error() << c74::min::endl;
            }
        }

        m_prev_buffer = m_buffer;
        m_dirty = false;
        output.send(c74::min::k_sym_bang);
    }

    bbb::sacn::sender m_sender;
    std::vector<uint8_t> m_buffer;
    std::vector<uint8_t> m_prev_buffer;
    std::mutex m_mutex;
    std::atomic<bool> m_running;
    std::atomic<bool> m_dirty;
    uint8_t m_sequence;
    std::array<uint8_t, 16> m_cid;
};

MIN_EXTERNAL(sacn_controller);
