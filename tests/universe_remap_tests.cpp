#include <bbb/universe_remap.hpp>

#include <cstdlib>
#include <iostream>

namespace {

void expect(bool condition, const char* message) {
    if(!condition) {
        std::cerr << message << std::endl;
        std::exit(1);
    }
}

void test_identity_disabled() {
    auto result = bbb::dmx::remap_universe(13, 1, 0);
    expect(result.valid, "disabled remap should be valid");
    expect(result.universe == 13, "disabled remap should preserve input universe");
}

void test_basic_offset() {
    auto first = bbb::dmx::remap_universe(1, 1, 101);
    auto last = bbb::dmx::remap_universe(13, 1, 101);
    expect(first.valid && first.universe == 101, "1 -> 101 failed");
    expect(last.valid && last.universe == 113, "13 -> 113 failed");
}

void test_non_one_input_start() {
    auto first = bbb::dmx::remap_universe(10, 10, 201);
    auto second = bbb::dmx::remap_universe(11, 10, 201);
    expect(first.valid && first.universe == 201, "10 -> 201 failed");
    expect(second.valid && second.universe == 202, "11 -> 202 failed");
}

void test_below_input_start_drops() {
    auto result = bbb::dmx::remap_universe(9, 10, 201);
    expect(!result.valid, "below input start should drop when remap is enabled");
}

void test_sacn_range() {
    auto valid = bbb::dmx::remap_sacn_universe(1, 1, 63999);
    auto invalid = bbb::dmx::remap_sacn_universe(2, 1, 63999);
    expect(valid.valid && valid.universe == 63999, "sACN 63999 should be valid");
    expect(!invalid.valid, "sACN remap above 63999 should drop");
}

void test_artnet_one_based_to_zero_based() {
    auto result = bbb::dmx::remap_artnet_port_address(1, 1, 101);
    expect(result.valid && result.universe == 100,
        "Art-Net output_universe_start 101 should encode as port-address 100");
}

void test_artnet_range() {
    auto valid = bbb::dmx::remap_artnet_port_address(1, 1, 32768);
    auto invalid = bbb::dmx::remap_artnet_port_address(2, 1, 32768);
    expect(valid.valid && valid.universe == 32767, "Art-Net 32768 should map to port-address 32767");
    expect(!invalid.valid, "Art-Net remap above 32768 should drop");
}

} // namespace

int main() {
    test_identity_disabled();
    test_basic_offset();
    test_non_one_input_start();
    test_below_input_start_drops();
    test_sacn_range();
    test_artnet_one_based_to_zero_based();
    test_artnet_range();
    return 0;
}
