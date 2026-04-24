#include "c74_min.h"
#include <artnet/artnet.h>
#include <bbb/artnet/artnet_node_manager.hpp>
#include <bbb/version.h>
#pragma push_macro("NIL")
#undef NIL
#include <bbb/osc/asio_receiver.hpp>
#include <bbb/osc/message.hpp>
#include <cstring>
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

    c74::min::attribute<double> framerate{this, "framerate", 40.0,
        c74::min::description{"Framerate for forced mode (0.01 - 44)."},
        c74::min::range{0.01, 44.0}
    };

    c74::min::attribute<int> osc_port{this, "osc_port", 0,
        c74::min::description{"OSC receive port (0 = disabled)."},
        c74::min::range{0, 65535}
    };

    c74::min::timer<c74::min::timer_options::defer_delivery> m_init_timer{this,
        MIN_FUNCTION {
            init_artnet();
            setup_osc();
            return {};
        }
    };

    c74::min::timer<c74::min::timer_options::defer_delivery> m_osc_timer{this,
        MIN_FUNCTION {
            if(m_osc_receiver) {
                m_osc_receiver->update();
            }
            m_osc_timer.delay(10);
            return {};
        }
    };

    c74::min::timer<c74::min::timer_options::defer_delivery> m_output_timer{this,
        MIN_FUNCTION {
            std::lock_guard<std::mutex> lock(m_mutex);
            output_data();
            return {};
        }
    };

    artnet_node_obj(const c74::min::atoms& args = {}) {
        m_buffer.resize(512 * num_universes, 0);
        m_prev_buffer.resize(512 * num_universes, 0);
        m_received_universes.resize(num_universes, false);
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
            std::lock_guard<std::mutex> lock(m_mutex);
            output_data();
            return {};
        }
    };

    c74::min::message<> maxclass_setup{this, "maxclass_setup",
        MIN_FUNCTION {
            cout << "bbb.artnet.node v" BBB_ARTNET_VERSION << c74::min::endl;
            return {};
        }
    };

private:
    void init_artnet() {
        m_managed_node = bbb::artnet::managed_node::get_or_create(nullptr);
        if(!m_managed_node || !m_managed_node->valid()) {
            cerr << "bbb.artnet.node: failed to create artnet node" << c74::min::endl;
            return;
        }

        artnet_set_node_type(m_managed_node->node(), ARTNET_NODE);
        artnet_set_short_name(m_managed_node->node(), "bbb.artnet.node");
        artnet_set_long_name(m_managed_node->node(), "bbb.artnet.node - Art-Net DMX Receiver");

        for(int i = 0; i < num_universes; ++i) {
            artnet_set_port_type(m_managed_node->node(), i, ARTNET_ENABLE_INPUT, ARTNET_PORT_DMX);
            artnet_set_port_addr(m_managed_node->node(), i, ARTNET_INPUT_PORT,
                (universe + i) & 0x0F);
        }
        artnet_set_subnet_addr(m_managed_node->node(), subnet);

        bbb::artnet::callback_entry cb;
        cb.type = bbb::artnet::callback_type::dmx;
        cb.owner = this;
        cb.dmx_fn = [this](const uint8_t* data, int length, int universe_addr) {
            handle_dmx(data, length, universe_addr);
        };
        m_managed_node->add_callback(cb);
        m_managed_node->retain();
        cout << "bbb.artnet.node: bound to 0.0.0.0 (all interfaces)" << c74::min::endl;
    }

    void handle_dmx(const uint8_t* data, int length, int universe_addr) {
        std::lock_guard<std::mutex> lock(m_mutex);
        int port = -1;
        for(int i = 0; i < num_universes; ++i) {
            if(((universe + i) & 0x0F) == (universe_addr & 0x0F)) {
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
        if(mode == c74::min::symbol("update")) {
            m_output_timer.delay(0);
        } else if(mode == c74::min::symbol("automatic")) {
            m_output_timer.delay(0);
        } else if(mode == c74::min::symbol("change")) {
            if(m_buffer != m_prev_buffer) {
                m_output_timer.delay(0);
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

        m_osc_receiver = bbb::osc::asio_receiver::get<bbb::osc::asio_receiver>(port);
        if(!m_osc_receiver) {
            cerr << "bbb.artnet.node: failed to setup OSC on port " << port << c74::min::endl;
            return;
        }

        m_osc_receiver->bind("/bang", [this](bbb::osc::message&) {
            try_call("bang", c74::min::atoms{});
        });

        m_osc_timer.delay(10);
    }
};

MIN_EXTERNAL(artnet_node_obj);
