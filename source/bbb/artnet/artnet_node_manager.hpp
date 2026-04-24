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
#include <ifaddrs.h>
#include <net/if.h>
#include <sys/socket.h>
#include <netinet/in.h>

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

    std::string effective_ip() const { return m_effective_ip; }

    int send_dmx_broadcast(uint8_t universe, int16_t length, const uint8_t* data) {
        if(!m_artnet_node) return -1;
        return artnet_raw_send_dmx(m_artnet_node, universe, length, data);
    }

    int send_dmx_unicast(const char* target_ip, uint8_t universe, int16_t length, const uint8_t* data) {
        if(length < 1 || length > 512) return -1;

        uint8_t buf[18 + 512];
        std::memcpy(buf, "Art-Net\0", 8);
        buf[8] = 0x00; buf[9] = 0x50;
        buf[10] = 0x00; buf[11] = 0x0E;
        buf[12] = 0x00;
        buf[13] = 0x00;
        buf[14] = static_cast<uint8_t>(universe & 0xFF);
        buf[15] = static_cast<uint8_t>((universe >> 8) & 0xFF);
        buf[16] = static_cast<uint8_t>((length >> 8) & 0xFF);
        buf[17] = static_cast<uint8_t>(length & 0xFF);
        std::memcpy(buf + 18, data, length);

        struct sockaddr_in addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(6454);
        if(inet_pton(AF_INET, target_ip, &addr.sin_addr) != 1) return -1;

        int fd = socket(AF_INET, SOCK_DGRAM, 0);
        if(fd < 0) return -1;

        int enable = 1;
        setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &enable, sizeof(enable));

        ssize_t result = sendto(fd, buf, 18 + length, 0,
            reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr));
        int err = errno;
        close(fd);

        if(result < 0) {
            fprintf(stderr, "bbb.artnet: sendto(%s) failed: %s\n", target_ip, strerror(err));
        }
        return result > 0 ? 0 : -1;
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

        m_effective_ip = detect_local_ip(ip);

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
        std::thread local_thread;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            --m_ref_count;
            if(m_ref_count <= 0) {
                m_running = false;
                if(m_artnet_node) {
                    artnet_stop(m_artnet_node);
                }
                if(m_read_thread.joinable()) {
                    local_thread = std::move(m_read_thread);
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
    std::string m_effective_ip;

    static std::string detect_local_ip(const char* preferred) {
        struct ifaddrs* ifa_list = nullptr;
        if(getifaddrs(&ifa_list) != 0) return preferred ? preferred : "";

        struct in_addr wanted;
        bool has_wanted = preferred && inet_pton(AF_INET, preferred, &wanted) == 1;

        char buf[INET_ADDRSTRLEN];

        if(has_wanted) {
            for(auto* ifa = ifa_list; ifa; ifa = ifa->ifa_next) {
                if(!ifa->ifa_addr || ifa->ifa_addr->sa_family != AF_INET) continue;
                if(!(ifa->ifa_flags & IFF_UP)) continue;
                auto* sin = reinterpret_cast<struct sockaddr_in*>(ifa->ifa_addr);
                if(sin->sin_addr.s_addr == wanted.s_addr) {
                    inet_ntop(AF_INET, &sin->sin_addr, buf, sizeof(buf));
                    freeifaddrs(ifa_list);
                    return std::string(buf);
                }
            }
        }

        for(auto* ifa = ifa_list; ifa; ifa = ifa->ifa_next) {
            if(!ifa->ifa_addr || ifa->ifa_addr->sa_family != AF_INET) continue;
            if(!(ifa->ifa_flags & IFF_UP)) continue;
            if(ifa->ifa_flags & IFF_LOOPBACK) continue;
            auto* sin = reinterpret_cast<struct sockaddr_in*>(ifa->ifa_addr);
            inet_ntop(AF_INET, &sin->sin_addr, buf, sizeof(buf));
            freeifaddrs(ifa_list);
            return std::string(buf);
        }

        freeifaddrs(ifa_list);
        return preferred ? preferred : "";
    }
};

inline const char* resolve_bind_ip(const std::string& target_ip) {
    if(target_ip.empty() || target_ip == "0.0.0.0") return nullptr;

    struct in_addr target;
    if(inet_pton(AF_INET, target_ip.c_str(), &target) != 1) return nullptr;

    struct ifaddrs* ifa_list = nullptr;
    if(getifaddrs(&ifa_list) != 0) return nullptr;

    static char buf[INET_ADDRSTRLEN];
    for(auto* ifa = ifa_list; ifa; ifa = ifa->ifa_next) {
        if(!ifa->ifa_addr || ifa->ifa_addr->sa_family != AF_INET) continue;
        if(!(ifa->ifa_flags & IFF_UP)) continue;
        if(!ifa->ifa_netmask) continue;

        auto* sin = reinterpret_cast<struct sockaddr_in*>(ifa->ifa_addr);
        auto* mask = reinterpret_cast<struct sockaddr_in*>(ifa->ifa_netmask);

        if((sin->sin_addr.s_addr & mask->sin_addr.s_addr) ==
           (target.s_addr & mask->sin_addr.s_addr)) {
            inet_ntop(AF_INET, &sin->sin_addr, buf, sizeof(buf));
            freeifaddrs(ifa_list);
            return buf;
        }
    }

    freeifaddrs(ifa_list);
    return nullptr;
}

inline bool is_broadcast_ip(const std::string& target_ip) {
    if(target_ip.empty()) return true;

    struct in_addr target;
    if(inet_pton(AF_INET, target_ip.c_str(), &target) != 1) return false;

    uint32_t t = target.s_addr;
    if((t & 0xFF) == 0xFF || t == 0) return true;

    struct ifaddrs* ifa_list = nullptr;
    if(getifaddrs(&ifa_list) != 0) return false;

    for(auto* ifa = ifa_list; ifa; ifa = ifa->ifa_next) {
        if(!ifa->ifa_addr || ifa->ifa_addr->sa_family != AF_INET) continue;
        if(ifa->ifa_flags & IFF_LOOPBACK) continue;
        if(!ifa->ifa_broadaddr) continue;
        auto* bcast = reinterpret_cast<struct sockaddr_in*>(ifa->ifa_broadaddr);
        if(bcast->sin_addr.s_addr == target.s_addr) {
            freeifaddrs(ifa_list);
            return true;
        }
    }

    freeifaddrs(ifa_list);
    return false;
}

}} // namespace bbb::artnet
