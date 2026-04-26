#pragma once
//
// bbb/artnet/artnet_node_manager.hpp
// Clean-room Art-Net node manager using asio (no libartnet dependency)
//
// Based on the Art-Net 4 public specification.
// This is NOT derived from libartnet or any LGPL-licensed code.
//

#include <bbb/artnet/protocol.hpp>
#include <bbb/net_compat.hpp>

#include <asio.hpp>
#include <cstring>
#include <cstdio>
#include <string>
#include <map>
#include <vector>
#include <functional>
#include <algorithm>
#include <mutex>
#include <thread>
#include <atomic>
#include <memory>
#include <array>

namespace bbb { namespace artnet {

enum class callback_type {
    dmx,
    rdm_raw,
    tod_data,
    poll_reply
};

struct callback_entry {
    callback_type type;
    void* owner;
    std::function<void(const uint8_t* data, int length, int universe_addr)> dmx_fn;
    std::function<void(int address, uint8_t* rdm, int length)> rdm_fn;
    std::function<void(const protocol::tod_data& tod)> tod_fn;
    std::function<void(const protocol::poll_reply_data& reply)> poll_reply_fn;
};

class managed_node {
public:
    static std::shared_ptr<managed_node> get_or_create(const char* ip) {
        static std::mutex s_mutex;
        static std::map<std::string, std::weak_ptr<managed_node>> s_instances;

        std::lock_guard<std::mutex> lock(s_mutex);
        std::string key = ip ? ip : "";

        auto it = s_instances.find(key);
        if(it != s_instances.end() && !it->second.expired()) {
            return it->second.lock();
        }

        auto node = std::make_shared<managed_node>(ip);
        s_instances[key] = node;
        return node;
    }

    std::string effective_ip() const { return m_effective_ip; }

    int send_dmx_broadcast(uint16_t universe_16, int16_t length, const uint8_t* data) {
        if(!m_socket.is_open()) return -1;
        auto pkt = protocol::build_dmx_packet(m_sequence++, 0, universe_16, data, static_cast<uint16_t>(length));
        asio::ip::udp::endpoint broadcast_ep(
            asio::ip::address_v4::broadcast(),
            protocol::ARTNET_PORT
        );
        asio::error_code ec;
        m_socket.send_to(asio::buffer(pkt), broadcast_ep, 0, ec);
        return ec ? -1 : 0;
    }

    int send_dmx_unicast(const char* target_ip, uint16_t universe_16, int16_t length, const uint8_t* data) {
        if(!m_socket.is_open()) return -1;
        if(length < 1 || length > 512) return -1;
        auto pkt = protocol::build_dmx_packet(m_sequence++, 0, universe_16, data, static_cast<uint16_t>(length));
        try {
            asio::ip::udp::endpoint target_ep(
                asio::ip::make_address(target_ip),
                protocol::ARTNET_PORT
            );
            asio::error_code ec;
            m_socket.send_to(asio::buffer(pkt), target_ep, 0, ec);
            return ec ? -1 : 0;
        } catch(...) {
            return -1;
        }
    }

    int send_tod_request(uint8_t net) {
        if(!m_socket.is_open()) return -1;
        auto pkt = protocol::build_tod_request(net);
        asio::ip::udp::endpoint broadcast_ep(
            asio::ip::address_v4::broadcast(),
            protocol::ARTNET_PORT
        );
        asio::error_code ec;
        m_socket.send_to(asio::buffer(pkt), broadcast_ep, 0, ec);
        return ec ? -1 : 0;
    }

    int send_rdm(uint8_t net, uint8_t address, const uint8_t* data, int length) {
        if(!m_socket.is_open()) return -1;
        auto pkt = protocol::build_rdm_packet(net, address, 0x00, data, static_cast<uint16_t>(length));
        asio::ip::udp::endpoint broadcast_ep(
            asio::ip::address_v4::broadcast(),
            protocol::ARTNET_PORT
        );
        asio::error_code ec;
        m_socket.send_to(asio::buffer(pkt), broadcast_ep, 0, ec);
        return ec ? -1 : 0;
    }

    managed_node(const char* ip)
        : m_io()
        , m_socket(m_io)
        , m_running(false)
        , m_ref_count(0)
        , m_sequence(0)
    {
        bbb::net::ensure_init();
        m_effective_ip = detect_local_ip(ip);

        try {
            std::string bind_addr = "0.0.0.0";
            if(!m_effective_ip.empty()) {
                bind_addr = m_effective_ip;
            }

            asio::ip::udp::endpoint local_ep(
                asio::ip::make_address(bind_addr),
                protocol::ARTNET_PORT
            );
            m_socket.open(asio::ip::udp::v4());
            m_socket.set_option(asio::ip::udp::socket::reuse_address(true));
            m_socket.bind(local_ep);
            m_socket.set_option(asio::socket_base::broadcast(true));
            m_socket.non_blocking(true);
        } catch(std::exception& e) {
            std::fprintf(stderr, "bbb.artnet: bind failed: %s\n", e.what());
        }
    }

    ~managed_node() {
        m_running = false;
        if(m_recv_thread.joinable()) {
            m_recv_thread.join();
        }
        if(m_socket.is_open()) {
            asio::error_code ec;
            m_socket.close(ec);
        }
    }

    bool valid() const { return m_socket.is_open(); }

    void retain() {
        std::lock_guard<std::mutex> lock(m_mutex);
        ++m_ref_count;
        if(m_ref_count == 1 && valid()) {
            m_running = true;
            m_recv_thread = std::thread([this]() { recv_loop(); });
        }
    }

    void release() {
        std::thread local_thread;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            --m_ref_count;
            if(m_ref_count <= 0) {
                m_running = false;
                if(m_recv_thread.joinable()) {
                    local_thread = std::move(m_recv_thread);
                }
            }
        }
        if(local_thread.joinable()) {
            local_thread.join();
        }
    }

    void add_callback(const callback_entry& entry) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_callbacks.push_back(entry);
    }

    void remove_callbacks(void* owner) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_callbacks.erase(
            std::remove_if(m_callbacks.begin(), m_callbacks.end(),
                [owner](const callback_entry& e) { return e.owner == owner; }),
            m_callbacks.end());
    }

private:
    void recv_loop() {
        std::array<uint8_t, 65536> buf;
        while(m_running) {
            asio::error_code ec;
            asio::ip::udp::endpoint remote;
            std::size_t len = m_socket.receive_from(asio::buffer(buf), remote, 0, ec);
            if(!ec && len > 10) {
                dispatch_packet(buf.data(), len);
            } else if(ec) {
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        }
    }

    void dispatch_packet(const uint8_t* data, size_t len) {
        if(!protocol::is_artnet_packet(data, len)) return;
        uint16_t opcode = protocol::read_opcode(data);

        switch(opcode) {
            case protocol::OP_OUTPUT: {
                protocol::dmx_data dmx;
                if(protocol::parse_dmx(data, len, dmx)) {
                    dispatch_dmx(dmx.universe, dmx.data, dmx.length);
                }
                break;
            }
            case protocol::OP_RDM: {
                protocol::rdm_data rdm;
                if(protocol::parse_rdm(data, len, rdm)) {
                    dispatch_rdm(rdm.address,
                        const_cast<uint8_t*>(rdm.data),
                        static_cast<int>(rdm.length));
                }
                break;
            }
            case protocol::OP_TOD_DATA: {
                protocol::tod_data tod;
                if(protocol::parse_tod_data(data, len, tod)) {
                    dispatch_tod(tod);
                }
                break;
            }
            case protocol::OP_POLL_REPLY: {
                protocol::poll_reply_data reply;
                if(protocol::parse_poll_reply(data, len, reply)) {
                    dispatch_poll_reply(reply);
                }
                break;
            }
            default:
                break;
        }
    }

    void dispatch_dmx(int universe, const uint8_t* data, int length) {
        std::lock_guard<std::mutex> lock(m_mutex);
        for(auto& cb : m_callbacks) {
            if(cb.type == callback_type::dmx && cb.dmx_fn) {
                cb.dmx_fn(data, length, universe);
            }
        }
    }

    void dispatch_rdm(int address, uint8_t* rdm, int length) {
        std::lock_guard<std::mutex> lock(m_mutex);
        for(auto& cb : m_callbacks) {
            if(cb.type == callback_type::rdm_raw && cb.rdm_fn) {
                cb.rdm_fn(address, rdm, length);
            }
        }
    }

    void dispatch_tod(const protocol::tod_data& tod) {
        std::lock_guard<std::mutex> lock(m_mutex);
        for(auto& cb : m_callbacks) {
            if(cb.type == callback_type::tod_data && cb.tod_fn) {
                cb.tod_fn(tod);
            }
        }
    }

    void dispatch_poll_reply(const protocol::poll_reply_data& reply) {
        std::lock_guard<std::mutex> lock(m_mutex);
        for(auto& cb : m_callbacks) {
            if(cb.type == callback_type::poll_reply && cb.poll_reply_fn) {
                cb.poll_reply_fn(reply);
            }
        }
    }

    static std::string detect_local_ip(const char* preferred) {
        auto adapters = bbb::net::get_adapters();

        struct in_addr wanted;
        bool has_wanted = preferred && inet_pton(AF_INET, preferred, &wanted) == 1;

        char buf[INET_ADDRSTRLEN];

        if(has_wanted) {
            for(const auto& a : adapters) {
                if(!a.is_up) continue;
                if(a.addr == wanted.s_addr) {
                    struct in_addr tmp; tmp.s_addr = a.addr;
                    inet_ntop(AF_INET, &tmp, buf, sizeof(buf));
                    return std::string(buf);
                }
            }
        }

        for(const auto& a : adapters) {
            if(!a.is_up) continue;
            if(a.is_loopback) continue;
            struct in_addr tmp; tmp.s_addr = a.addr;
            inet_ntop(AF_INET, &tmp, buf, sizeof(buf));
            return std::string(buf);
        }

        return preferred ? preferred : "";
    }

    asio::io_context m_io;
    asio::ip::udp::socket m_socket;
    std::thread m_recv_thread;
    std::atomic<bool> m_running;
    int m_ref_count;
    uint8_t m_sequence;
    std::mutex m_mutex;
    std::vector<callback_entry> m_callbacks;
    std::string m_effective_ip;
};

inline const char* resolve_bind_ip(const std::string& target_ip) {
    if(target_ip.empty() || target_ip == "0.0.0.0") return nullptr;

    struct in_addr target;
    if(inet_pton(AF_INET, target_ip.c_str(), &target) != 1) return nullptr;

    auto adapters = bbb::net::get_adapters();

    static char buf[INET_ADDRSTRLEN];
    for(const auto& a : adapters) {
        if(!a.is_up) continue;
        if((a.addr & a.netmask) == (target.s_addr & a.netmask)) {
            struct in_addr tmp; tmp.s_addr = a.addr;
            inet_ntop(AF_INET, &tmp, buf, sizeof(buf));
            return buf;
        }
    }

    return nullptr;
}

inline bool is_broadcast_ip(const std::string& target_ip) {
    if(target_ip.empty()) return true;

    struct in_addr target;
    if(inet_pton(AF_INET, target_ip.c_str(), &target) != 1) return false;

    uint32_t t = ntohl(target.s_addr);
    if(t == 0xFFFFFFFF || t == 0) return true;

    auto adapters = bbb::net::get_adapters();
    for(const auto& a : adapters) {
        if(a.is_loopback) continue;
        if(a.broadcast == target.s_addr) return true;
    }

    return false;
}

}} // namespace bbb::artnet
