#include "c74_min.h"
#include <artnet/artnet.h>
#pragma push_macro("NIL")
#undef NIL
#include <bbb/osc/asio_receiver.hpp>
#include <bbb/osc/message.hpp>
#include <cstring>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>

class artnet_node_obj : public c74::min::object<artnet_node_obj> {
public:
    MIN_DESCRIPTION{"Receive DMX via Art-Net protocol."};
    MIN_TAGS{"dmx, artnet, lighting"};
    MIN_AUTHOR{"bbb"};
    MIN_RELATED{"bbb.artnet.controller"};

    c74::min::inlet<> input{this, "(bang) request output in bang mode"};
    c74::min::outlet<> output{this, "(list) DMX values as list of integers"};

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

    c74::min::timer<c74::min::timer_options::defer_delivery> m_osc_timer{this,
        MIN_FUNCTION {
            if(m_osc_receiver) {
                m_osc_receiver->update();
            }
            m_osc_timer.delay(10);
            return {};
        }
    };

    artnet_node_obj(const c74::min::atoms& args = {})
        : m_node{nullptr}
        , m_running{false}
    {
        m_buffer.resize(512 * num_universes, 0);
        m_prev_buffer.resize(512 * num_universes, 0);
        m_received_universes.resize(num_universes, false);
        init_artnet();
        setup_osc();
    }

    ~artnet_node_obj() {
        m_running = false;
        if(m_thread.joinable()) {
            m_thread.join();
        }
        if(m_read_thread.joinable()) {
            m_read_thread.join();
        }
        if(m_node) {
            artnet_stop(m_node);
            artnet_destroy(m_node);
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
            cout << "bbb.artnet.node v0.1.0" << c74::min::endl;
            return {};
        }
    };

private:
    static int dmx_handler(artnet_node n, int port, void* d) {
        auto* self = static_cast<artnet_node_obj*>(d);
        int length = 0;
        uint8_t* data = artnet_read_dmx(n, port, &length);
        if(!data || length <= 0) return 0;

        std::lock_guard<std::mutex> lock(self->m_mutex);
        int offset = port * 512;
        int copy_len = std::min(length, 512);
        if(offset + copy_len <= static_cast<int>(self->m_buffer.size())) {
            std::memcpy(self->m_buffer.data() + offset, data, copy_len);
        }

        if(self->sync_universes && self->num_universes > 1) {
            if(port < static_cast<int>(self->m_received_universes.size())) {
                self->m_received_universes[port] = true;
            }
            bool all_received = true;
            for(auto received : self->m_received_universes) {
                if(!received) {
                    all_received = false;
                    break;
                }
            }
            if(!all_received) return 0;

            for(auto&& received : self->m_received_universes) {
                received = false;
            }
        }

        self->handle_mode_output();
        return 0;
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

    void init_artnet() {
        if(m_node) {
            artnet_stop(m_node);
            artnet_destroy(m_node);
        }

        m_node = artnet_new(nullptr, 0);
        if(!m_node) {
            cerr << "bbb.artnet.node: failed to create artnet node" << c74::min::endl;
            return;
        }

        artnet_set_node_type(m_node, ARTNET_NODE);
        artnet_set_short_name(m_node, "bbb.artnet.node");
        artnet_set_long_name(m_node, "bbb.artnet.node - Art-Net DMX Receiver");

        for(int i = 0; i < num_universes; ++i) {
            artnet_set_port_type(m_node, i, ARTNET_ENABLE_INPUT, ARTNET_PORT_DMX);
            artnet_set_port_addr(m_node, i, ARTNET_INPUT_PORT,
                (universe + i) & 0x0F);
        }
        artnet_set_subnet_addr(m_node, subnet);

        artnet_set_dmx_handler(m_node, dmx_handler, this);

        if(artnet_start(m_node) != ARTNET_EOK) {
            cerr << "bbb.artnet.node: failed to start artnet node" << c74::min::endl;
            return;
        }

        m_running = true;
        m_read_thread = std::thread([this]() {
            while(m_running) {
                artnet_read(m_node, 1);
            }
        });
    }

    ::artnet_node m_node;
    std::vector<uint8_t> m_buffer;
    std::vector<uint8_t> m_prev_buffer;
    std::vector<bool> m_received_universes;
    std::mutex m_mutex;
    std::thread m_read_thread;
    std::thread m_thread;
    std::atomic<bool> m_running;

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
