#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <array>
#include <vector>
#include <random>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

namespace sacn {

#pragma pack(push, 1)

struct packet {
    uint8_t preamble_size[2];
    uint8_t postamble_size[2];
    uint8_t acn_packet_id[12];
    uint16_t flags_length1;
    uint32_t root_vector;
    uint8_t cid[16];
    uint16_t flags_length2;
    uint32_t framing_vector;
    uint8_t source_name[64];
    uint8_t priority;
    uint16_t reserved;
    uint8_t sequence;
    uint8_t options;
    uint16_t universe;
    uint16_t flags_length3;
    uint8_t dmp_vector;
    uint8_t address_type;
    uint16_t first_property;
    uint16_t address_increment;
    uint8_t property_values[513];
};

#pragma pack(pop)

inline std::array<uint8_t, 16> generate_cid() {
    std::array<uint8_t, 16> cid;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, 255);
    for(auto& b : cid) b = static_cast<uint8_t>(dist(gen));
    cid[6] = (cid[6] & 0x0F) | 0x40;
    cid[8] = (cid[8] & 0x3F) | 0x80;
    return cid;
}

inline void init_packet(packet& p, const uint8_t cid[16],
                        const char* source_name, uint8_t priority,
                        uint8_t sequence, uint16_t universe,
                        const uint8_t* data, int length)
{
    std::memset(&p, 0, sizeof(p));
    p.preamble_size[0] = 0x00; p.preamble_size[1] = 0x10;
    p.postamble_size[0] = 0x00; p.postamble_size[1] = 0x00;
    const uint8_t acn_id[12] = {0x41,0x53,0x43,0x2D,0x45,0x31,0x2E,0x31,0x37,0x00,0x00,0x00};
    std::memcpy(p.acn_packet_id, acn_id, 12);
    uint16_t root_len = static_cast<uint16_t>(sizeof(packet) - 16);
    p.flags_length1 = htons(0x7000 | root_len);
    p.root_vector = htonl(0x00000004);
    std::memcpy(p.cid, cid, 16);
    uint16_t framing_len = static_cast<uint16_t>(sizeof(packet) - 38);
    p.flags_length2 = htons(0x7000 | framing_len);
    p.framing_vector = htonl(0x00000002);
    std::strncpy(reinterpret_cast<char*>(p.source_name), source_name, 63);
    p.source_name[63] = 0;
    p.priority = priority;
    p.sequence = sequence;
    p.universe = htons(universe);
    uint16_t dmp_len = static_cast<uint16_t>(length + 11);
    p.flags_length3 = htons(0x7000 | dmp_len);
    p.dmp_vector = 0x02;
    p.address_type = 0xA1;
    p.first_property = htons(0x0000);
    p.address_increment = htons(0x0001);
    p.property_values[0] = 0x00;
    std::memcpy(p.property_values + 1, data, std::min(length, 512));
}

struct multicast_addr {
    uint8_t a, b, c, d;
};

inline multicast_addr universe_to_multicast(uint16_t universe) {
    return {239, 255, static_cast<uint8_t>((universe >> 8) & 0xFF),
            static_cast<uint8_t>(universe & 0xFF)};
}

}
