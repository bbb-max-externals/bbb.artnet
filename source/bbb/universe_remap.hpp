#pragma once

#include <cstdint>

namespace bbb::dmx {

struct universe_remap_result {
    bool valid{false};
    int universe{0};
};

inline bool universe_remap_enabled(int output_universe_start) {
    return 0 < output_universe_start;
}

inline universe_remap_result remap_universe(int input_universe,
                                            int input_universe_start,
                                            int output_universe_start) {
    if(!universe_remap_enabled(output_universe_start)) {
        return {true, input_universe};
    }

    const int sanitized_input_start = input_universe_start < 1 ? 1 : input_universe_start;
    if(input_universe < sanitized_input_start) {
        return {false, 0};
    }

    return {true, input_universe - sanitized_input_start + output_universe_start};
}

inline universe_remap_result remap_sacn_universe(int input_universe,
                                                 int input_universe_start,
                                                 int output_universe_start) {
    universe_remap_result result = remap_universe(input_universe, input_universe_start, output_universe_start);
    if(!result.valid) {
        return result;
    }
    if(universe_remap_enabled(output_universe_start) && (result.universe < 1 || 63999 < result.universe)) {
        return {false, 0};
    }
    return result;
}

inline universe_remap_result remap_artnet_port_address(int input_universe,
                                                       int input_universe_start,
                                                       int output_universe_start) {
    universe_remap_result result = remap_universe(input_universe, input_universe_start, output_universe_start);
    if(!result.valid) {
        return result;
    }
    if(!universe_remap_enabled(output_universe_start)) {
        return result;
    }
    if(result.universe < 1 || 32768 < result.universe) {
        return {false, 0};
    }

    // Remap attributes are user-facing 1-based universe numbers.
    // Art-Net Port-Address is protocol-facing 0-based.
    return {true, result.universe - 1};
}

} // namespace bbb::dmx
