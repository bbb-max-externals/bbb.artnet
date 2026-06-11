#pragma once
//
// bbb/artnet/protocol.hpp
// Art-Net IV protocol definitions (header-only)
//
// Based on the Art-Net 4 specification by Artistic Licence (Holdings) Ltd.
// Reference: https://art-net.org.uk/downloads/art-net.pdf
//
// This is a clean-room implementation from the public specification.
// It does not derive from, reference, or incorporate any code from
// libartnet or any other LGPL-licensed Art-Net implementation.
//

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <vector>

namespace bbb { namespace artnet { namespace protocol {

// Opcodes (little-endian in packets)
constexpr uint16_t OP_POLL        = 0x2000;
constexpr uint16_t OP_POLL_REPLY  = 0x2100;
constexpr uint16_t OP_DIAG_DATA   = 0x2200;
constexpr uint16_t OP_COMMAND     = 0x2300;
constexpr uint16_t OP_OUTPUT      = 0x5000; // OpDmx
constexpr uint16_t OP_NZS         = 0x5100;
constexpr uint16_t OP_SYNC        = 0x5200;
constexpr uint16_t OP_ADDRESS     = 0x6000;
constexpr uint16_t OP_INPUT       = 0x7000;
constexpr uint16_t OP_TOD_REQUEST = 0x8000;
constexpr uint16_t OP_TOD_DATA    = 0x8100;
constexpr uint16_t OP_TOD_CONTROL = 0x8200;
constexpr uint16_t OP_RDM         = 0x8300;
constexpr uint16_t OP_RDM_SUB     = 0x8400;

constexpr uint16_t PROTOCOL_VERSION = 14;
constexpr uint16_t ARTNET_PORT      = 6454;
constexpr size_t   DMX_MAX_CHANNELS = 512;
constexpr size_t   MAX_UID_COUNT    = 200;
constexpr size_t   UID_SIZE         = 6;
constexpr size_t   MAX_RDM_ADCOUNT  = 32;

// -- Header validation --

inline bool is_artnet_packet(const uint8_t* data, size_t len) {
    if(len < 12) return false;
    static const uint8_t magic[8] = {'A','r','t','-','N','e','t',0};
    return std::memcmp(data, magic, 8) == 0;
}

inline uint16_t read_opcode(const uint8_t* data) {
    return static_cast<uint16_t>(data[8]) | (static_cast<uint16_t>(data[9]) << 8);
}

inline uint16_t read_version(const uint8_t* data) {
    return (static_cast<uint16_t>(data[10]) << 8) | static_cast<uint16_t>(data[11]);
}

// -- Port-address helpers --

// Build a 16-bit port-address from net (7-bit), subnet (4-bit), universe (4-bit).
// Stored in DMX packets as two bytes: [low=SubNet<<4|Universe, high=Net]
inline uint16_t make_port_address(int net, int subnet, int universe) {
    return (static_cast<uint16_t>(net & 0x7F) << 8)
         | (static_cast<uint16_t>(subnet & 0x0F) << 4)
         | (static_cast<uint16_t>(universe & 0x0F));
}

// Extract the sub-universe address byte (SubNet<<4 | Universe) from a port-address
inline uint8_t port_address_sub(uint16_t port_addr) {
    return static_cast<uint8_t>(port_addr & 0xFF);
}

// Extract the net byte from a port-address
inline uint8_t port_address_net(uint16_t port_addr) {
    return static_cast<uint8_t>((port_addr >> 8) & 0x7F);
}

// -- Build ArtDmx (OpOutput) packet --
// universe_16 is the full 16-bit port-address (see make_port_address).
inline std::vector<uint8_t> build_dmx_packet(
    uint8_t sequence,
    uint8_t physical,
    uint16_t universe_16,
    const uint8_t* data,
    uint16_t length)
{
    std::vector<uint8_t> pkt(18 + length);
    static const uint8_t header[8] = {'A','r','t','-','N','e','t',0};
    std::memcpy(pkt.data(), header, 8);
    // OpCode LE: 0x5000
    pkt[8] = 0x00; pkt[9] = 0x50;
    // Protocol version BE: 14
    pkt[10] = 0x00; pkt[11] = 0x0E;
    pkt[12] = sequence;
    pkt[13] = physical;
    // Universe LE
    pkt[14] = static_cast<uint8_t>(universe_16 & 0xFF);
    pkt[15] = static_cast<uint8_t>((universe_16 >> 8) & 0xFF);
    // Length BE
    pkt[16] = static_cast<uint8_t>((length >> 8) & 0xFF);
    pkt[17] = static_cast<uint8_t>(length & 0xFF);
    if(length > 0) {
        std::memcpy(pkt.data() + 18, data, length);
    }
    return pkt;
}

// -- Build ArtTodRequest (0x8000) packet --
inline std::vector<uint8_t> build_tod_request(
    uint8_t net,
    uint8_t command = 0x01,
    const std::vector<uint8_t>& addresses = {})
{
    size_t ad_count = std::min(addresses.size(), MAX_RDM_ADCOUNT);
    std::vector<uint8_t> pkt(24 + ad_count, 0);
    static const uint8_t header[8] = {'A','r','t','-','N','e','t',0};
    std::memcpy(pkt.data(), header, 8);
    // OpCode LE: 0x8000
    pkt[8] = 0x00; pkt[9] = 0x80;
    // Protocol version BE: 14
    pkt[10] = 0x00; pkt[11] = 0x0E;
    pkt[21] = net;
    pkt[22] = command;
    pkt[23] = static_cast<uint8_t>(ad_count);
    for(size_t i = 0; i < ad_count; ++i) {
        pkt[24 + i] = addresses[i];
    }
    return pkt;
}

// -- Build ArtRDM (0x8300) packet --
inline std::vector<uint8_t> build_rdm_packet(
    uint8_t net,
    uint8_t address,
    uint8_t cmd,
    const uint8_t* data,
    uint16_t length)
{
    std::vector<uint8_t> pkt(24 + length, 0);
    static const uint8_t header[8] = {'A','r','t','-','N','e','t',0};
    std::memcpy(pkt.data(), header, 8);
    // OpCode LE: 0x8300
    pkt[8] = 0x00; pkt[9] = 0x83;
    // Protocol version BE: 14
    pkt[10] = 0x00; pkt[11] = 0x0E;
    // RDM version
    pkt[12] = 0x01;
    pkt[21] = net;
    pkt[22] = cmd;
    pkt[23] = address;
    if(length > 0 && data) {
        std::memcpy(pkt.data() + 24, data, length);
    }
    return pkt;
}

// -- Parse ArtDmx (OpOutput) from raw packet --
struct dmx_data {
    uint8_t     sequence;
    uint8_t     physical;
    uint16_t    universe; // full 16-bit port-address
    const uint8_t* data;
    uint16_t    length;
};

inline bool parse_dmx(const uint8_t* pkt, size_t pkt_len, dmx_data& out) {
    if(pkt_len < 18) return false;
    if(read_opcode(pkt) != OP_OUTPUT) return false;
    if(read_version(pkt) != PROTOCOL_VERSION) return false;
    out.sequence = pkt[12];
    out.physical = pkt[13];
    out.universe = static_cast<uint16_t>(pkt[14]) | (static_cast<uint16_t>(pkt[15]) << 8);
    out.length   = (static_cast<uint16_t>(pkt[16]) << 8) | static_cast<uint16_t>(pkt[17]);
    if(out.length > DMX_MAX_CHANNELS) return false;
    if(pkt_len < static_cast<size_t>(18 + out.length)) return false;
    out.data = pkt + 18;
    return true;
}

// -- Parse ArtRDM (OpRdm) from raw packet --
struct rdm_data {
    uint8_t         net;
    uint8_t         cmd;
    uint8_t         address;
    const uint8_t*  data;
    size_t          length;
};

inline bool parse_rdm(const uint8_t* pkt, size_t pkt_len, rdm_data& out) {
    if(pkt_len < 24) return false;
    if(read_opcode(pkt) != OP_RDM) return false;
    if(read_version(pkt) != PROTOCOL_VERSION) return false;
    out.net     = pkt[21];
    out.cmd     = pkt[22];
    out.address = pkt[23];
    out.data    = pkt + 24;
    out.length  = pkt_len - 24;
    return true;
}

// -- Parse ArtTodData (0x8100) from raw packet --
struct tod_data {
    uint8_t         net;
    uint8_t         address;
    uint16_t        uid_total;
    uint8_t         block_count;
    uint8_t         uid_count;
    const uint8_t*  uids; // points to uid_count * 6 bytes within the packet

    // Access individual UID (6 bytes)
    const uint8_t* uid_at(int index) const {
        return uids + index * UID_SIZE;
    }
};

inline bool parse_tod_data(const uint8_t* pkt, size_t pkt_len, tod_data& out) {
    if(pkt_len < 28) return false;
    if(read_opcode(pkt) != OP_TOD_DATA) return false;
    if(read_version(pkt) != PROTOCOL_VERSION) return false;
    out.net         = pkt[21];
    out.address     = pkt[23];
    out.uid_total   = static_cast<uint16_t>(pkt[24]) | (static_cast<uint16_t>(pkt[25]) << 8);
    out.block_count = pkt[26];
    out.uid_count   = pkt[27];
    if(out.uid_count > MAX_UID_COUNT) return false;
    if(pkt_len < static_cast<size_t>(28 + out.uid_count * UID_SIZE)) return false;
    out.uids = pkt + 28;
    return true;
}

// -- Parse ArtPollReply (0x2100) from raw packet --
struct poll_reply_data {
    uint8_t  ip[4];
    uint16_t port;
    uint16_t firmware_version;
    uint8_t  net_switch;
    uint8_t  sub_switch;
    uint16_t oem;
    uint8_t  ubea_version;
    uint8_t  status1;
    uint16_t esta_man;
    char     short_name[18];
    char     long_name[64];
    char     node_report[64];
    uint8_t  num_ports;
    uint8_t  port_types[4];
};

inline bool parse_poll_reply(const uint8_t* pkt, size_t pkt_len, poll_reply_data& out) {
    if(pkt_len < 207) return false; // minimum ArtPollReply size
    if(read_opcode(pkt) != OP_POLL_REPLY) return false;
    std::memcpy(out.ip, pkt + 12, 4);
    out.port              = (static_cast<uint16_t>(pkt[16]) << 8) | static_cast<uint16_t>(pkt[17]);
    out.firmware_version  = (static_cast<uint16_t>(pkt[18]) << 8) | static_cast<uint16_t>(pkt[19]);
    out.net_switch        = pkt[20];
    out.sub_switch        = pkt[21];
    out.oem               = (static_cast<uint16_t>(pkt[22]) << 8) | static_cast<uint16_t>(pkt[23]);
    out.ubea_version      = pkt[24];
    out.status1           = pkt[25];
    out.esta_man          = (static_cast<uint16_t>(pkt[26]) << 8) | static_cast<uint16_t>(pkt[27]);
    std::memcpy(out.short_name,  pkt + 28,  18);
    std::memcpy(out.long_name,   pkt + 46,  64);
    std::memcpy(out.node_report, pkt + 110, 64);
    out.num_ports = pkt[174];
    std::memcpy(out.port_types, pkt + 175, 4);
    return true;
}

}}} // namespace bbb::artnet::protocol
