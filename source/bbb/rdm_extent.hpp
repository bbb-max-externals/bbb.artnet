#pragma once

#include <cstdint>

namespace bbb::artnet {
// ArtRdm omits the DMX start code included in the declared RDM length,
// but carries the two checksum bytes excluded from that length.
constexpr bool complete_rdm_extent(int received, std::uint8_t declared, std::uint8_t parameters) {
    return declared == 24 + parameters && declared + 1 <= received;
}

inline bool complete_rdm_message(const std::uint8_t* data, int received) {
    if(!data || received < 25) return false;
    return data[0] == 0x01 && complete_rdm_extent(received, data[1], data[22]);
}
}
