#pragma once

#include <artnet/artnet.h>
#include <artnet/packets.h>
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
#include <arpa/inet.h>

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
    std::function<void(artnet_packet_t* pkt)> tod_fn;
    std::function<void(artnet_packet_t* pkt)> poll_reply_fn;
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

    managed_node(const char* ip)
        : m_artnet_node(nullptr)
        , m_running(false)
        , m_ref_count(0)
    {
        m_artnet_node = artnet_new(ip, 0);
        if(!m_artnet_node && ip) {
            m_artnet_node = artnet_new(nullptr, 0);
        }
        if(!m_artnet_node) return;

        artnet_set_handler(m_artnet_node, ARTNET_DMX_HANDLER,
            raw_dmx_callback, this);
        artnet_set_handler(m_artnet_node, ARTNET_RDM_HANDLER,
            raw_rdm_callback, this);
        artnet_set_handler(m_artnet_node, ARTNET_TOD_DATA_HANDLER,
            raw_tod_callback, this);
        artnet_set_handler(m_artnet_node, ARTNET_REPLY_HANDLER,
            raw_poll_reply_callback, this);
    }

    ~managed_node() {
        m_running = false;
        if(m_read_thread.joinable()) {
            m_read_thread.join();
        }
        if(m_artnet_node) {
            artnet_stop(m_artnet_node);
            artnet_destroy(m_artnet_node);
        }
    }

    bool valid() const { return m_artnet_node != nullptr; }

    ::artnet_node node() { return m_artnet_node; }

    void retain() {
        std::lock_guard<std::mutex> lock(m_mutex);
        ++m_ref_count;
        if(m_ref_count == 1 && m_artnet_node) {
            if(artnet_start(m_artnet_node) == ARTNET_EOK) {
                m_running = true;
                m_read_thread = std::thread([this]() { read_loop(); });
            }
        }
    }

    void release() {
        std::lock_guard<std::mutex> lock(m_mutex);
        --m_ref_count;
        if(m_ref_count <= 0) {
            m_running = false;
            if(m_read_thread.joinable()) {
                m_read_thread.join();
            }
            if(m_artnet_node) {
                artnet_stop(m_artnet_node);
            }
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
    void read_loop() {
        while(m_running) {
            artnet_read(m_artnet_node, 100);
        }
    }

    void dispatch_dmx(int port, const uint8_t* data, int length) {
        std::lock_guard<std::mutex> lock(m_mutex);
        for(auto& cb : m_callbacks) {
            if(cb.type == callback_type::dmx && cb.dmx_fn) {
                cb.dmx_fn(data, length, port);
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

    void dispatch_tod(artnet_packet_t* pkt) {
        std::lock_guard<std::mutex> lock(m_mutex);
        for(auto& cb : m_callbacks) {
            if(cb.type == callback_type::tod_data && cb.tod_fn) {
                cb.tod_fn(pkt);
            }
        }
    }

    void dispatch_poll_reply(artnet_packet_t* pkt) {
        std::lock_guard<std::mutex> lock(m_mutex);
        for(auto& cb : m_callbacks) {
            if(cb.type == callback_type::poll_reply && cb.poll_reply_fn) {
                cb.poll_reply_fn(pkt);
            }
        }
    }

    static int raw_dmx_callback(artnet_node /*n*/, void* pp, void* data) {
        auto* self = static_cast<managed_node*>(data);
        auto* packet = static_cast<artnet_packet_t*>(pp);
        int port = packet->data.admx.universe;
        int length = (packet->data.admx.lengthHi << 8) | packet->data.admx.length;
        self->dispatch_dmx(port, packet->data.admx.data, length);
        return 0;
    }

    static int raw_rdm_callback(artnet_node /*n*/, void* pp, void* data) {
        auto* self = static_cast<managed_node*>(data);
        auto* packet = static_cast<artnet_packet_t*>(pp);
        self->dispatch_rdm(packet->data.rdm.address,
            packet->data.rdm.data, ARTNET_MAX_RDM_DATA);
        return 0;
    }

    static int raw_tod_callback(artnet_node /*n*/, void* pp, void* data) {
        auto* self = static_cast<managed_node*>(data);
        auto* packet = static_cast<artnet_packet_t*>(pp);
        self->dispatch_tod(packet);
        return 0;
    }

    static int raw_poll_reply_callback(artnet_node /*n*/, void* pp, void* data) {
        auto* self = static_cast<managed_node*>(data);
        auto* packet = static_cast<artnet_packet_t*>(pp);
        self->dispatch_poll_reply(packet);
        return 0;
    }

    ::artnet_node m_artnet_node;
    std::thread m_read_thread;
    std::atomic<bool> m_running;
    int m_ref_count;
    std::mutex m_mutex;
    std::vector<callback_entry> m_callbacks;
};

inline const char* infer_bind_ip(const std::string& target_ip) {
    if(target_ip.empty() || target_ip == "0.0.0.0") return nullptr;

    struct in_addr addr;
    if(inet_pton(AF_INET, target_ip.c_str(), &addr) != 1) return nullptr;

    uint32_t a = ntohl(addr.s_addr);
    uint8_t o[4] = {
        static_cast<uint8_t>((a >> 24) & 0xFF),
        static_cast<uint8_t>((a >> 16) & 0xFF),
        static_cast<uint8_t>((a >> 8) & 0xFF),
        static_cast<uint8_t>(a & 0xFF)
    };

    static char buf[16];
    if(o[0] == 10) {
        std::snprintf(buf, sizeof(buf), "10.%d.%d.%d", o[1], o[2], 1);
    } else if(o[0] == 172 && o[1] >= 16 && o[1] <= 31) {
        std::snprintf(buf, sizeof(buf), "172.%d.%d.%d", o[1], o[2], 1);
    } else if(o[0] == 192 && o[1] == 168) {
        std::snprintf(buf, sizeof(buf), "192.168.%d.%d", o[2], 1);
    } else {
        return nullptr;
    }
    return buf;
}

}} // namespace bbb::artnet
