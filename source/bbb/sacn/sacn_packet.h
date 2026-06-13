#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <array>
#include <vector>
#include <random>
#include <algorithm>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

namespace sacn {

constexpr int acn_packet_id_offset = 4;
constexpr int root_pdu_offset = 16;
constexpr int root_vector_offset = 18;
constexpr int framing_pdu_offset = 38;
constexpr int framing_vector_offset = 40;
constexpr int dmp_pdu_offset = 115;
constexpr int sequence_offset = 111;
constexpr int universe_offset = 113;
constexpr int dmp_vector_offset = 117;
constexpr int property_value_count_offset = 123;
constexpr int property_values_offset = 125;
constexpr int dmx_data_offset = 126;
constexpr int max_dmx_data_length = 512;

inline int clamp_data_length(int length) {
    if(length < 0) {
        return 0;
    }
    return (std::min)(length, max_dmx_data_length);
}

inline int packet_size_for_data_length(int length) {
    return dmx_data_offset + clamp_data_length(length);
}

inline uint16_t read_be16(const uint8_t* buffer, int offset) {
    uint16_t value{0};
    std::memcpy(&value, buffer + offset, sizeof(value));
    return ntohs(value);
}

inline uint32_t read_be32(const uint8_t* buffer, int offset) {
    uint32_t value{0};
    std::memcpy(&value, buffer + offset, sizeof(value));
    return ntohl(value);
}

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
    uint16_t property_value_count;
    uint8_t property_values[513];
};

#pragma pack(pop)

static_assert(offsetof(packet, sequence) == sequence_offset, "sACN sequence offset mismatch");
static_assert(offsetof(packet, acn_packet_id) == acn_packet_id_offset, "sACN ACN packet identifier offset mismatch");
static_assert(offsetof(packet, root_vector) == root_vector_offset, "sACN root vector offset mismatch");
static_assert(offsetof(packet, framing_vector) == framing_vector_offset, "sACN framing vector offset mismatch");
static_assert(offsetof(packet, universe) == universe_offset, "sACN universe offset mismatch");
static_assert(offsetof(packet, dmp_vector) == dmp_vector_offset, "sACN DMP vector offset mismatch");
static_assert(offsetof(packet, property_value_count) == property_value_count_offset, "sACN property count offset mismatch");
static_assert(offsetof(packet, property_values) == property_values_offset, "sACN property values offset mismatch");
static_assert(sizeof(packet) == dmx_data_offset + max_dmx_data_length, "sACN packet size mismatch");

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
    int data_length = clamp_data_length(length);
    int packet_size = packet_size_for_data_length(data_length);
    int property_value_count = data_length + 1;

    std::memset(&p, 0, sizeof(p));
    p.preamble_size[0] = 0x00; p.preamble_size[1] = 0x10;
    p.postamble_size[0] = 0x00; p.postamble_size[1] = 0x00;
    const uint8_t acn_id[12] = {0x41,0x53,0x43,0x2D,0x45,0x31,0x2E,0x31,0x37,0x00,0x00,0x00};
    std::memcpy(p.acn_packet_id, acn_id, 12);
    uint16_t root_len = static_cast<uint16_t>(packet_size - root_pdu_offset);
    p.flags_length1 = htons(0x7000 | root_len);
    p.root_vector = htonl(0x00000004);
    std::memcpy(p.cid, cid, 16);
    uint16_t framing_len = static_cast<uint16_t>(packet_size - framing_pdu_offset);
    p.flags_length2 = htons(0x7000 | framing_len);
    p.framing_vector = htonl(0x00000002);
    std::strncpy(reinterpret_cast<char*>(p.source_name), source_name, 63);
    p.source_name[63] = 0;
    p.priority = priority;
    p.sequence = sequence;
    p.universe = htons(universe);
    uint16_t dmp_len = static_cast<uint16_t>(packet_size - dmp_pdu_offset);
    p.flags_length3 = htons(0x7000 | dmp_len);
    p.dmp_vector = 0x02;
    p.address_type = 0xA1;
    p.first_property = htons(0x0000);
    p.address_increment = htons(0x0001);
    p.property_value_count = htons(static_cast<uint16_t>(property_value_count));
    p.property_values[0] = 0x00;
    std::memcpy(p.property_values + 1, data, static_cast<size_t>(data_length));
}

struct multicast_addr {
    uint8_t a, b, c, d;
};

inline multicast_addr universe_to_multicast(uint16_t universe) {
    return {239, 255, static_cast<uint8_t>((universe >> 8) & 0xFF),
            static_cast<uint8_t>(universe & 0xFF)};
}

}
