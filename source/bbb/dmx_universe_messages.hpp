#pragma once

#include "c74_min.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace bbb { namespace dmx {

inline std::uint8_t clamp_value(int value) {
    return static_cast<std::uint8_t>(std::max(0, std::min(255, value)));
}

inline int per_universe_channel_count(int channel_count) {
    return std::max(1, std::min(512, channel_count));
}

inline bool valid_universe_index(
    int universe_index,
    const std::vector<std::uint8_t> &buffer)
{
    if(universe_index < 0) {
        return false;
    }

    const int offset = universe_index * 512;
    return 0 <= offset && offset < static_cast<int>(buffer.size());
}

inline void append_universe_dump(
    c74::min::atoms &result,
    int universe_identifier,
    const std::vector<std::uint8_t> &buffer,
    int universe_index,
    int channel_count)
{
    result.push_back(c74::min::symbol("universe"));
    result.push_back(universe_identifier);

    if(!valid_universe_index(universe_index, buffer)) {
        return;
    }

    const int offset = universe_index * 512;
    const int available = static_cast<int>(buffer.size()) - offset;
    const int count = std::min(per_universe_channel_count(channel_count), available);
    result.reserve(static_cast<std::size_t>(count + 2));
    for(int i = 0; i < count; ++i) {
        result.push_back(static_cast<int>(buffer[static_cast<std::size_t>(offset + i)]));
    }
}

inline bool set_universe_data(
    std::vector<std::uint8_t> &buffer,
    int universe_index,
    const c74::min::atoms &args,
    std::size_t first_value_index)
{
    if(!valid_universe_index(universe_index, buffer) || args.size() <= first_value_index) {
        return false;
    }

    const int offset = universe_index * 512;
    const std::size_t count = std::min<std::size_t>(args.size() - first_value_index, 512);
    for(std::size_t i = 0; i < count; ++i) {
        buffer[static_cast<std::size_t>(offset) + i] = clamp_value(
            static_cast<int>(args[first_value_index + i]));
    }
    return true;
}

}} // namespace bbb::dmx
