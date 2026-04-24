#include "c74_min.h"
#include <artnet/artnet.h>
#include <artnet/packets.h>
#include <bbb/artnet/artnet_node_manager.hpp>
#include <bbb/version.h>
#include <cstring>
#include <string>
#include <vector>
#include <deque>
#include <mutex>
#include <chrono>
#include <memory>

namespace {
    constexpr uint8_t CC_GET = 0x20;
    constexpr uint8_t CC_SET = 0x30;
    constexpr uint8_t CC_GET_RESPONSE = 0x21;
    constexpr uint8_t CC_SET_RESPONSE = 0x31;

    constexpr uint16_t PID_DISC_MUTE = 0x0002;
    constexpr uint16_t PID_DISC_UN_MUTE = 0x0003;
    constexpr uint16_t PID_DEVICE_INFO = 0x0060;
    constexpr uint16_t PID_MANUFACTURER_LABEL = 0x0081;
    constexpr uint16_t PID_DEVICE_LABEL = 0x0082;
    constexpr uint16_t PID_SOFTWARE_VERSION_LABEL = 0x00C0;
    constexpr uint16_t PID_DMX_START_ADDRESS = 0x00F0;
    constexpr uint16_t PID_IDENTIFY_DEVICE = 0x1000;

    constexpr uint8_t RESP_ACK = 0x00;
    constexpr uint8_t RESP_NACK = 0x02;

    struct rdm_uid { uint8_t b[6]; };

    bool parse_uid(const std::string& s, rdm_uid& uid) {
        std::string cleaned;
        for(char c : s) {
            if(c != ':' && c != '-') cleaned += c;
        }
        if(cleaned.size() != 12) return false;
        for(int i = 0; i < 6; ++i) {
            unsigned int val;
            if(std::sscanf(cleaned.c_str() + i * 2, "%02x", &val) != 1) return false;
            uid.b[i] = static_cast<uint8_t>(val);
        }
        return true;
    }

    std::string format_uid(const rdm_uid& uid) {
        char buf[14];
        std::snprintf(buf, sizeof(buf), "%02x%02x:%02x%02x%02x%02x",
            uid.b[0], uid.b[1], uid.b[2], uid.b[3], uid.b[4], uid.b[5]);
        return std::string(buf);
    }

    std::vector<uint8_t> build_rdm_frame(
        const rdm_uid& dest, const rdm_uid& src,
        uint8_t tn, uint8_t cc, uint16_t pid,
        const uint8_t* param, uint8_t pdl)
    {
        std::vector<uint8_t> f;
        f.reserve(25 + pdl);
        f.push_back(0x01);
        f.push_back(static_cast<uint8_t>(24 + pdl));
        for(int i = 0; i < 6; ++i) f.push_back(dest.b[i]);
        for(int i = 0; i < 6; ++i) f.push_back(src.b[i]);
        f.push_back(tn);
        f.push_back(0x01);
        f.push_back(0x00);
        f.push_back(0x00);
        f.push_back(0x00);
        f.push_back(cc);
        f.push_back(static_cast<uint8_t>((pid >> 8) & 0xFF));
        f.push_back(static_cast<uint8_t>(pid & 0xFF));
        f.push_back(pdl);
        for(uint8_t i = 0; i < pdl; ++i) f.push_back(param[i]);
        uint16_t sum = 0xCC;
        for(uint8_t byte : f) sum += byte;
        f.push_back(static_cast<uint8_t>((sum >> 8) & 0xFF));
        f.push_back(static_cast<uint8_t>(sum & 0xFF));
        return f;
    }

    struct queued_response {
        enum type_t { ACK, NACK, TIMEOUT, UIDS } type;
        std::string uid_str;
        uint16_t pid;
        std::vector<int> data;
        std::vector<std::string> uids;
    };
}

class artnet_rdm : public c74::min::object<artnet_rdm> {
public:
    MIN_DESCRIPTION{"RDM controller over Art-Net."};
    MIN_TAGS{"dmx, artnet, rdm, lighting"};
    MIN_AUTHOR{"bbb"};
    MIN_RELATED{"bbb.artnet.controller"};

    c74::min::inlet<> input{this, "(messages) RDM commands"};
    c74::min::outlet<> data_out{this, "(response/uids/ack) RDM data"};
    c74::min::outlet<> status_out{this, "(nack/timeout/error) RDM status"};

    c74::min::attribute<int> net{this, "net", 0,
        c74::min::description{"Art-Net net (0-127)."},
        c74::min::range{0, 127}
    };

    c74::min::attribute<int> subnet{this, "subnet", 0,
        c74::min::description{"Art-Net subnet (0-15)."},
        c74::min::range{0, 15}
    };

    c74::min::attribute<int> universe{this, "universe", 0,
        c74::min::description{"Art-Net universe (0-15)."},
        c74::min::range{0, 15}
    };

    c74::min::attribute<bool> unicast{this, "unicast", true,
        c74::min::description{"Use unicast mode."}
    };

    c74::min::attribute<c74::min::symbol> unicast_ip{this, "unicast_ip", "127.0.0.1",
        c74::min::description{"Destination IP for unicast mode."}
    };

    c74::min::attribute<c74::min::symbol> source_uid{this, "source_uid", "bbbb:00000001",
        c74::min::description{"Source UID for RDM controller (MMMM:SSSSSSSS)."}
    };

    c74::min::attribute<int> timeout_attr{this, "timeout", 2000,
        c74::min::description{"Response timeout in ms."},
        c74::min::range{100, 10000}
    };

    c74::min::timer<c74::min::timer_options::defer_delivery> m_init_timer{this,
        MIN_FUNCTION {
            init_artnet();
            m_timer.delay(50);
            return {};
        }
    };

    c74::min::timer<c74::min::timer_options::defer_delivery> m_timer{this,
        MIN_FUNCTION {
            process_timeout();
            drain_queue();
            m_timer.delay(50);
            return {};
        }
    };

    artnet_rdm(const c74::min::atoms& args = {})
        : m_next_tn{0}
        , m_has_pending{false}
    {
        m_init_timer.delay(0);
    }

    ~artnet_rdm() {
        if(m_managed_node) {
            m_managed_node->remove_callbacks(this);
            m_managed_node->release();
        }
    }

    c74::min::message<> discover_msg{this, "discover", "Send ArtTodRequest to discover RDM devices.",
        MIN_FUNCTION {
            if(m_managed_node && m_managed_node->valid())
                artnet_send_tod_request(m_managed_node->node());
            return {};
        }
    };

    c74::min::message<> tod_msg{this, "tod", "Alias for discover.",
        MIN_FUNCTION {
            if(m_managed_node && m_managed_node->valid())
                artnet_send_tod_request(m_managed_node->node());
            return {};
        }
    };

    c74::min::message<> identify_msg{this, "identify", "GET/SET IDENTIFY_DEVICE (UID [0/1]).",
        MIN_FUNCTION {
            rdm_uid uid;
            if(!parse_uid_arg(args, uid)) return {};
            if(args.size() > 1) {
                uint8_t val = static_cast<uint8_t>(static_cast<int>(args[1]) ? 1 : 0);
                send_rdm(uid, CC_SET, PID_IDENTIFY_DEVICE, &val, 1);
            } else {
                send_rdm(uid, CC_GET, PID_IDENTIFY_DEVICE, nullptr, 0);
            }
            return {};
        }
    };

    c74::min::message<> start_address_msg{this, "start_address", "GET/SET DMX_START_ADDRESS (UID [addr]).",
        MIN_FUNCTION {
            rdm_uid uid;
            if(!parse_uid_arg(args, uid)) return {};
            if(args.size() > 1) {
                int addr = static_cast<int>(args[1]);
                uint8_t data[2] = {
                    static_cast<uint8_t>((addr >> 8) & 0xFF),
                    static_cast<uint8_t>(addr & 0xFF)
                };
                send_rdm(uid, CC_SET, PID_DMX_START_ADDRESS, data, 2);
            } else {
                send_rdm(uid, CC_GET, PID_DMX_START_ADDRESS, nullptr, 0);
            }
            return {};
        }
    };

    c74::min::message<> label_msg{this, "label", "GET/SET DEVICE_LABEL (UID [string]).",
        MIN_FUNCTION {
            rdm_uid uid;
            if(!parse_uid_arg(args, uid)) return {};
            if(args.size() > 1) {
                std::string lbl = static_cast<std::string>(c74::min::symbol(args[1]));
                uint8_t len = static_cast<uint8_t>(std::min(lbl.size(), static_cast<size_t>(32)));
                send_rdm(uid, CC_SET, PID_DEVICE_LABEL,
                    reinterpret_cast<const uint8_t*>(lbl.c_str()), len);
            } else {
                send_rdm(uid, CC_GET, PID_DEVICE_LABEL, nullptr, 0);
            }
            return {};
        }
    };

    c74::min::message<> device_info_msg{this, "device_info", "GET DEVICE_INFO (UID).",
        MIN_FUNCTION {
            rdm_uid uid;
            if(!parse_uid_arg(args, uid)) return {};
            send_rdm(uid, CC_GET, PID_DEVICE_INFO, nullptr, 0);
            return {};
        }
    };

    c74::min::message<> manufacturer_label_msg{this, "manufacturer_label", "GET MANUFACTURER_LABEL (UID).",
        MIN_FUNCTION {
            rdm_uid uid;
            if(!parse_uid_arg(args, uid)) return {};
            send_rdm(uid, CC_GET, PID_MANUFACTURER_LABEL, nullptr, 0);
            return {};
        }
    };

    c74::min::message<> software_version_msg{this, "software_version", "GET SOFTWARE_VERSION_LABEL (UID).",
        MIN_FUNCTION {
            rdm_uid uid;
            if(!parse_uid_arg(args, uid)) return {};
            send_rdm(uid, CC_GET, PID_SOFTWARE_VERSION_LABEL, nullptr, 0);
            return {};
        }
    };

    c74::min::message<> get_msg{this, "get", "Generic RDM GET (UID PID).",
        MIN_FUNCTION {
            if(args.size() < 2) return {};
            rdm_uid uid;
            if(!parse_uid_arg(args, uid)) return {};
            uint16_t pid = static_cast<uint16_t>(static_cast<int>(args[1]));
            send_rdm(uid, CC_GET, pid, nullptr, 0);
            return {};
        }
    };

    c74::min::message<> set_msg{this, "set", "Generic RDM SET (UID PID val1 val2 ...).",
        MIN_FUNCTION {
            if(args.size() < 3) return {};
            rdm_uid uid;
            if(!parse_uid_arg(args, uid)) return {};
            uint16_t pid = static_cast<uint16_t>(static_cast<int>(args[1]));
            std::vector<uint8_t> data;
            for(size_t i = 2; i < args.size(); ++i) {
                data.push_back(static_cast<uint8_t>(static_cast<int>(args[i]) & 0xFF));
            }
            send_rdm(uid, CC_SET, pid, data.data(), static_cast<uint8_t>(data.size()));
            return {};
        }
    };

    c74::min::message<> mute_msg{this, "mute", "Send DISC_MUTE (UID).",
        MIN_FUNCTION {
            rdm_uid uid;
            if(!parse_uid_arg(args, uid)) return {};
            send_rdm(uid, CC_GET, PID_DISC_MUTE, nullptr, 0);
            return {};
        }
    };

    c74::min::message<> unmute_msg{this, "unmute", "Send DISC_UN_MUTE broadcast.",
        MIN_FUNCTION {
            rdm_uid broadcast;
            std::memset(broadcast.b, 0xFF, 6);
            send_rdm(broadcast, CC_GET, PID_DISC_UN_MUTE, nullptr, 0);
            return {};
        }
    };

    c74::min::message<> maxclass_setup{this, "maxclass_setup",
        MIN_FUNCTION {
            cout << "bbb.artnet.rdm v" BBB_ARTNET_VERSION << c74::min::endl;
            return {};
        }
    };

private:
    bool parse_uid_arg(const c74::min::atoms& args, rdm_uid& uid) {
        if(args.empty()) {
            status_out.send({c74::min::atom("error"), c74::min::atom("missing UID argument")});
            return false;
        }
        c74::min::symbol sym = args[0];
        std::string s = static_cast<std::string>(sym);
        if(!parse_uid(s, uid)) {
            std::string msg = "invalid UID: " + s;
            status_out.send({c74::min::atom("error"), c74::min::atom(msg.c_str())});
            return false;
        }
        return true;
    }

    void init_artnet() {
        m_managed_node = bbb::artnet::managed_node::get_or_create(nullptr);
        if(!m_managed_node || !m_managed_node->valid()) {
            cerr << "bbb.artnet.rdm: failed to create artnet node" << c74::min::endl;
            return;
        }

        artnet_set_node_type(m_managed_node->node(), ARTNET_SRV);
        artnet_set_port_type(m_managed_node->node(), 0, ARTNET_ENABLE_OUTPUT, ARTNET_PORT_DMX);
        artnet_set_port_addr(m_managed_node->node(), 0, ARTNET_OUTPUT_PORT, universe);
        artnet_set_subnet_addr(m_managed_node->node(), subnet);

        bbb::artnet::callback_entry rdm_cb;
        rdm_cb.type = bbb::artnet::callback_type::rdm_raw;
        rdm_cb.owner = this;
        rdm_cb.rdm_fn = [this](int address, uint8_t* rdm, int length) {
            handle_rdm(address, rdm, length);
        };
        m_managed_node->add_callback(rdm_cb);

        bbb::artnet::callback_entry tod_cb;
        tod_cb.type = bbb::artnet::callback_type::tod_data;
        tod_cb.owner = this;
        tod_cb.tod_fn = [this](artnet_packet_t* pkt) {
            handle_tod(pkt);
        };
        m_managed_node->add_callback(tod_cb);

        m_managed_node->retain();
        cout << "bbb.artnet.rdm: bound to 0.0.0.0 (all interfaces)" << c74::min::endl;
    }

    void send_rdm(const rdm_uid& dest, uint8_t cc, uint16_t pid,
                  const uint8_t* param, uint8_t pdl)
    {
        rdm_uid src;
        {
            c74::min::symbol sym = source_uid;
            std::string s = static_cast<std::string>(sym);
            if(!parse_uid(s, src)) {
                std::memset(src.b, 0, 6);
            }
        }

        uint8_t tn = m_next_tn++;
        auto frame = build_rdm_frame(dest, src, tn, cc, pid, param, pdl);

        {
            std::lock_guard<std::mutex> lock(m_pending_mutex);
            if(m_has_pending) {
                queued_response resp;
                resp.type = queued_response::TIMEOUT;
                resp.uid_str = format_uid(m_pending.uid);
                resp.pid = m_pending.pid;
                std::lock_guard<std::mutex> qlock(m_queue_mutex);
                m_queue.push_back(std::move(resp));
            }
            m_pending.tn = tn;
            m_pending.uid = dest;
            m_pending.pid = pid;
            m_pending.sent_at = std::chrono::steady_clock::now();
            m_has_pending = true;
        }

        uint8_t address = static_cast<uint8_t>((subnet << 4) | (universe & 0x0F));
        if(m_managed_node && m_managed_node->valid())
            artnet_send_rdm(m_managed_node->node(), address, frame.data(), static_cast<int>(frame.size()));
    }

    void process_timeout() {
        std::lock_guard<std::mutex> lock(m_pending_mutex);
        if(!m_has_pending) return;
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - m_pending.sent_at
        ).count();
        if(elapsed >= timeout_attr) {
            queued_response resp;
            resp.type = queued_response::TIMEOUT;
            resp.uid_str = format_uid(m_pending.uid);
            resp.pid = m_pending.pid;
            m_has_pending = false;
            std::lock_guard<std::mutex> qlock(m_queue_mutex);
            m_queue.push_back(std::move(resp));
        }
    }

    void drain_queue() {
        std::deque<queued_response> local;
        {
            std::lock_guard<std::mutex> lock(m_queue_mutex);
            local = std::move(m_queue);
            m_queue.clear();
        }
        for(auto& resp : local) {
            output_response(resp);
        }
    }

    void output_response(const queued_response& resp) {
        c74::min::atoms args;
        switch(resp.type) {
            case queued_response::ACK:
                args.push_back(c74::min::atom("response"));
                args.push_back(c74::min::atom(resp.uid_str.c_str()));
                args.push_back(static_cast<int>(resp.pid));
                for(int v : resp.data) args.push_back(v);
                data_out.send(args);
                break;
            case queued_response::NACK:
                args.push_back(c74::min::atom("nack"));
                args.push_back(c74::min::atom(resp.uid_str.c_str()));
                args.push_back(static_cast<int>(resp.pid));
                for(int v : resp.data) args.push_back(v);
                status_out.send(args);
                break;
            case queued_response::TIMEOUT:
                args.push_back(c74::min::atom("timeout"));
                args.push_back(c74::min::atom(resp.uid_str.c_str()));
                args.push_back(static_cast<int>(resp.pid));
                status_out.send(args);
                break;
            case queued_response::UIDS:
                args.push_back(c74::min::atom("uids"));
                for(const auto& u : resp.uids) {
                    args.push_back(c74::min::atom(u.c_str()));
                }
                data_out.send(args);
                break;
        }
    }

    void handle_rdm(int /*address*/, uint8_t* rdm, int /*length*/) {
        if(rdm[0] != 0x01) return;

        uint8_t msg_len = rdm[1];
        if(msg_len < 24) return;

        uint8_t cc = rdm[19];
        if(cc != CC_GET_RESPONSE && cc != CC_SET_RESPONSE) return;

        rdm_uid src_uid;
        std::memcpy(src_uid.b, rdm + 8, 6);
        uint8_t tn = rdm[14];
        uint8_t resp_type = rdm[15];
        uint16_t pid = (static_cast<uint16_t>(rdm[20]) << 8) | rdm[21];
        uint8_t pdl = rdm[22];

        {
            std::lock_guard<std::mutex> lock(m_pending_mutex);
            if(!m_has_pending || m_pending.tn != tn) return;
            m_has_pending = false;
        }

        queued_response resp;
        resp.uid_str = format_uid(src_uid);
        resp.pid = pid;

        if(resp_type == RESP_ACK) {
            resp.type = queued_response::ACK;
            for(uint8_t i = 0; i < pdl; ++i) {
                resp.data.push_back(static_cast<int>(rdm[23 + i]));
            }
        } else if(resp_type == RESP_NACK) {
            resp.type = queued_response::NACK;
            if(pdl >= 2) {
                uint16_t reason = (static_cast<uint16_t>(rdm[23]) << 8) | rdm[24];
                resp.data.push_back(static_cast<int>(reason));
            }
        } else {
            return;
        }

        std::lock_guard<std::mutex> lock(m_queue_mutex);
        m_queue.push_back(std::move(resp));
    }

    void handle_tod(artnet_packet_t* pkt) {
        uint8_t uid_count = pkt->data.toddata.uidCount;
        if(uid_count == 0) return;

        queued_response resp;
        resp.type = queued_response::UIDS;
        for(int i = 0; i < uid_count; ++i) {
            rdm_uid uid;
            std::memcpy(uid.b, pkt->data.toddata.tod[i], 6);
            resp.uids.push_back(format_uid(uid));
        }

        std::lock_guard<std::mutex> lock(m_queue_mutex);
        m_queue.push_back(std::move(resp));
    }

    std::shared_ptr<bbb::artnet::managed_node> m_managed_node;
    uint8_t m_next_tn;

    struct pending_request {
        uint8_t tn;
        rdm_uid uid;
        uint16_t pid;
        std::chrono::steady_clock::time_point sent_at;
    };

    std::mutex m_pending_mutex;
    pending_request m_pending;
    bool m_has_pending;

    std::mutex m_queue_mutex;
    std::deque<queued_response> m_queue;
};

MIN_EXTERNAL(artnet_rdm);
