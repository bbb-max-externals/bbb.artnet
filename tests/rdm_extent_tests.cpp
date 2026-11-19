#include <bbb/rdm_extent.hpp>
#include <array>

using bbb::artnet::complete_rdm_extent;
static_assert(complete_rdm_extent(25, 24, 0));
static_assert(complete_rdm_extent(27, 26, 2));
static_assert(!complete_rdm_extent(24, 24, 0)); // missing checksum byte
static_assert(!complete_rdm_extent(27, 26, 3)); // inconsistent declared sizes
static_assert(!complete_rdm_extent(26, 26, 2)); // extent ends early

int main() {
    std::array<std::uint8_t, 25> empty_response{};
    empty_response[0] = 1;
    empty_response[1] = 24;
    return bbb::artnet::complete_rdm_message(empty_response.data(), empty_response.size()) &&
           !bbb::artnet::complete_rdm_message(nullptr, 0) ? 0 : 1;
}
