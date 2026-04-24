# bbb.artnet

Max/MSP externals for Art-Net and sACN DMX send/receive.

## Objects

### bbb.artnet.controller

Sends DMX data via Art-Net protocol. Supports broadcast and unicast.

**Messages:**
- `list` — set DMX values from a list (0–255)
- `bang` — trigger send (in bang mode)
- `channel INDEX VALUE` — set a single channel (1-based)
- `setchannel INDEX VALUE` — set without sending
- `set V0 V1 ...` — store values without sending
- `set_offset OFFSET V0 V1 ...` — store values at offset (0-based)

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
| `mode` | automatic | Output mode: automatic, bang, update, change, forced |
| `framerate` | 40.0 | Framerate for forced mode |
| `target_ip` | "" | Destination IP (empty=broadcast, broadcast addr=broadcast, unicast addr=unicast) |
| `bind_ip` | "" | Local IP to bind (empty=auto-detect from target_ip subnet) |
| `osc_port` | 0 | OSC receive port (0 = disabled) |

**OSC addresses** (when `osc_port` is set):
`/list`, `/set`, `/bang`, `/channel`, `/setchannel`, `/set_offset`, `/blackout`

### bbb.artnet.node

Receives DMX data via Art-Net protocol.

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
| `mode` | update | Output mode: update, bang, automatic, change |
| `osc_port` | 0 | OSC receive port (0 = disabled) |

### bbb.artnet.rdm

RDM controller over Art-Net. Discovers and controls RDM devices.

**Attributes:**
| Attribute | Default | Description |
|---|---|---|
| `net` | 0 | Art-Net net (0–127) |
| `subnet` | 0 | Art-Net subnet (0–15) |
| `universe` | 0 | Art-Net universe (0–15) |
| `source_uid` | auto | RDM controller source UID |
| `timeout` | 2000 | RDM response timeout (ms) |

### bbb.sacn.controller

Sends DMX data via sACN (E1.31) protocol.

**Attributes:**
| Attribute | Default | Description |
|---|---|---|
| `universe` | 1 | sACN universe (1–63999) |
| `num_universes` | 1 | Number of universes |
| `num_channels` | 512 | Channels per universe |
| `mode` | automatic | Output mode |
| `framerate` | 40.0 | Framerate for forced mode |
| `priority` | 100 | sACN priority (0–200) |
| `source_name` | auto | sACN source name |
| `unicast_ip` | "" | Destination IP for unicast |

### bbb.sacn.node

Receives DMX data via sACN (E1.31) protocol.

**Attributes:**
| Attribute | Default | Description |
|---|---|---|
| `universe` | 1 | sACN universe (1–63999) |
| `num_universes` | 1 | Number of universes |
| `num_channels` | 512 | Channels to output |
| `mode` | update | Output mode |

## Build

### Prerequisites

- macOS
- CMake 3.19+
- Xcode CLI tools

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
cmake --build .
```

Built externals are output to `externals/`.

## Project Structure

```
bbb.artnet/
├── CMakeLists.txt
├── deps/
│   ├── min-api/          # Max min-api (submodule)
│   ├── libartnet/        # Art-Net C library
│   └── osc/              # bbb-osc (submodule) + asio
├── source/
│   ├── bbb/              # shared headers (artnet_node_manager, sacn_packet)
│   └── projects/         # external source + maxhelp per object
├── help/                 # help files for Max
├── externals/            # build output (.mxo)
└── scripts/              # help file generator
```

## License

TODO
