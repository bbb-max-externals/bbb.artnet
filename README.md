# bbb.artnet

Max/MSP externals for Art-Net and sACN DMX send/receive.

## Objects

### bbb.artnet.controller

Sends DMX data via Art-Net protocol. Supports broadcast and unicast.

**Inlets/Outlets:**
- inlet 0: `(list/bang/message)` DMX data input
- outlet 0: `(bang)` bang on packet transmission

**Messages:**
- `list` — set DMX values from a list (0–255)
- `bang` — trigger send (in bang mode)
- `channel INDEX VALUE` — set a single channel (1-based by default)
- `setchannel INDEX VALUE` — set without sending
- `set V0 V1 ...` — store values without sending
- `set_offset OFFSET V0 V1 ...` — store values at offset (1-based by default)
- `dump` — output current DMX buffer as list from outlet

**Attributes:**
| Attribute | Default | Description |
|---|---|---|
| `net` | 0 | Art-Net net (0–127) |
| `subnet` | 0 | Art-Net subnet (0–15) |
| `universe` | 0 | Art-Net universe (0–15) |
| `num_universes` | 1 | Number of universes (1–32) |
| `num_channels` | 512 | Channels per universe |
| `sync_universes` | true | Send all universes on any change |
| `blackout` | false | Send all zeros |
| `mode` | 0 (automatic) | Output mode: 0=automatic, 1=bang, 2=update, 3=change, 4=forced |
| `framerate` | 40.0 | Framerate for forced mode |
| `target_ip` | "" | Destination IP (empty=broadcast, broadcast addr=broadcast, unicast addr=unicast) |
| `bind_ip` | "" | Local IP to bind (empty=auto-detect from target_ip subnet) |
| `origin` | 1 | Channel index origin: 1=1-based, 0=0-based |
| `osc_port` | 0 | OSC receive port (0 = disabled) |
| `osc_bind_ip` | 0.0.0.0 | OSC listen address |
| `verbose` | false | Enable verbose logging (not yet implemented) |

**OSC addresses** (when `osc_port` is set):
`/list`, `/set`, `/bang`, `/channel`, `/setchannel`, `/set_offset`, `/dump`, `/blackout`

### bbb.artnet.node

Receives DMX data via Art-Net protocol.

**Inlets/Outlets:**
- inlet 0: `(bang)` request output in bang mode
- outlet 0: `(list)` DMX values as list of integers

**Messages:**
- `bang` — output current data (in bang mode)

**Attributes:**
| Attribute | Default | Description |
|---|---|---|
| `net` | 0 | Art-Net net (0–127) |
| `subnet` | 0 | Art-Net subnet (0–15) |
| `universe` | 0 | Art-Net universe (0–15) |
| `num_universes` | 1 | Number of universes to receive (1–32) |
| `num_channels` | 512 | Channels to output |
| `sync_universes` | true | Wait for all universes before output |
| `mode` | 1 (update) | Output mode: 0=automatic, 1=update, 2=bang, 3=change, 4=forced |
| `framerate` | 40.0 | Framerate for forced mode |
| `target_ip` | "" | Destination IP for filter (empty = receive from all) |
| `bind_ip` | "" | Local IP to bind (empty = auto-detect) |
| `osc_port` | 0 | OSC receive port (0 = disabled) |
| `osc_bind_ip` | 0.0.0.0 | OSC listen address |
| `verbose` | false | Enable verbose logging (not yet implemented) |

**OSC addresses** (when `osc_port` is set):
`/bang`

### bbb.artnet.rdm

RDM controller over Art-Net. Discovers and controls RDM devices.

**Inlets/Outlets:**
- inlet 0: `(messages)` RDM commands
- outlet 0: `(response/uids/ack)` RDM data
- outlet 1: `(nack/timeout/error)` RDM status

**Messages:**
- `discover` / `tod` — discover RDM devices
- `identify UID [0/1]` — GET/SET IDENTIFY_DEVICE
- `start_address UID [ADDR]` — GET/SET DMX_START_ADDRESS
- `label UID [TEXT]` — GET/SET DEVICE_LABEL
- `device_info UID` — GET DEVICE_INFO
- `manufacturer_label UID` — GET MANUFACTURER_LABEL
- `software_version UID` — GET SOFTWARE_VERSION_LABEL
- `get UID PID` — generic RDM GET
- `set UID PID VAL ...` — generic RDM SET
- `mute UID` / `unmute` — DISC_MUTE / DISC_UN_MUTE

**Attributes:**
| Attribute | Default | Description |
|---|---|---|
| `net` | 0 | Art-Net net (0–127) |
| `subnet` | 0 | Art-Net subnet (0–15) |
| `universe` | 0 | Art-Net universe (0–15) |
| `target_ip` | "" | Destination IP (empty=broadcast) |
| `bind_ip` | "" | Local IP to bind (empty=auto-detect) |
| `source_uid` | bbbb:00000001 | RDM controller source UID |
| `timeout` | 2000 | RDM response timeout (ms) |
| `verbose` | false | Enable verbose logging (not yet implemented) |

**Output format:**
- data_out: `response UID PID d0 d1 ...` | `uids UID1 UID2 ...`
- status_out: `nack UID PID REASON` | `timeout UID PID` | `error MESSAGE`

### bbb.sacn.controller

Sends DMX data via sACN (E1.31) protocol.

**Messages:**
- `list` — set DMX values from a list
- `bang` — trigger send (in bang mode)
- `channel INDEX VALUE` — set a single channel
- `setchannel INDEX VALUE` — set without sending
- `set V0 V1 ...` — store values without sending
- `set_offset OFFSET V0 V1 ...` — store values at offset

**Attributes:**
| Attribute | Default | Description |
|---|---|---|
| `universe` | 1 | sACN universe (1–63999) |
| `num_universes` | 1 | Number of universes |
| `num_channels` | 512 | Channels per universe |
| `sync_universes` | true | Sync all universes on change |
| `blackout` | false | Send all zeros |
| `mode` | 0 | Output mode: 0=automatic, 1=bang, 2=update, 3=change, 4=forced |
| `framerate` | 40.0 | Framerate for forced mode |
| `priority` | 100 | sACN priority (0–200) |
| `source_name` | bbb.sacn.controller | sACN source name |
| `target_ip` | "" | Destination IP (empty = per-universe multicast, unicast addr = unicast) |
| `bind_ip` | "" | Local interface IP for outgoing sACN (sets multicast interface) |
| `origin` | 1 | Channel index origin: 1=1-based, 0=0-based |
| `unicast` | false | Legacy compatibility: use `unicast_ip` when `target_ip` is empty |
| `unicast_ip` | 127.0.0.1 | Legacy destination IP for unicast |

### bbb.sacn.node

Receives DMX data via sACN (E1.31) protocol.

**Attributes:**
| Attribute | Default | Description |
|---|---|---|
| `universe` | 1 | sACN universe (1–63999) |
| `num_universes` | 1 | Number of universes |
| `num_channels` | 512 | Channels to output |
| `sync_universes` | true | Wait for all universes before output |
| `mode` | update | Output mode: update, bang, automatic, change, forced |

## Build

### Prerequisites

- macOS or Windows
- CMake 3.19+
- macOS: Xcode CLI tools
- Windows: Visual Studio 2022

### Setup

```bash
git clone <repo-url> bbb.artnet
cd bbb.artnet
git submodule update --init --recursive
git clone https://github.com/2bbb/asio.git deps/osc/third-party/asio
```

### Build

```bash
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

Built externals are output to `externals/`.

## Project Structure

```
bbb.artnet/
├── CMakeLists.txt
├── deps/
│   ├── bbb-artnet/       # Art-Net protocol/node helper library (submodule)
│   ├── min-api/          # Max min-api (submodule)
│   └── osc/              # bbb-osc (submodule) + asio
├── source/
│   ├── bbb/              # shared headers for this package
│   └── projects/         # external source per object
├── help/                 # help files for Max
├── externals/            # build output (.mxo / .mxe64)
└── scripts/              # help file generator
```

## Acknowledgments

This project was inspired by imp.dmx by David Butler. His work on DMX over Art-Net for Max/MSP provided valuable reference during development.

## Third-Party Licenses

| Library | Author | License |
|---|---|---|
| [min-api](https://github.com/Cycling74/min-api) | Cycling '74 | MIT |
| [max-sdk-base](https://github.com/Cycling74/max-sdk-base) | Cycling '74 | MIT |
| [bbb-artnet](https://github.com/2bbb/bbb-artnet) | 2bit | MIT |
| [bbb-osc](https://github.com/2bbb/bbb-osc) | 2bit | MIT |
| [asio](https://github.com/chriskohlhoff/asio) | Christopher M. Kohlhoff | BSL-1.0 |
| [oscpp](https://github.com/kaoskorobase/oscpp) | kaoskorobase | MIT |
| [bit_by_bit](https://github.com/2bbb/bit_by_bit) | 2bit | MIT |

## License

MIT License. See [LICENSE](LICENSE) for details.
