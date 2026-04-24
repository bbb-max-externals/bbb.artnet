# bbb.artnet

## 概要

* Max/MSP のエクスターナルオブジェクト
* artnetの送受信を行う
* 一旦macOSのみでOK
	* 安定してきたらWin版も考える
* libartnetを使用しても良い
	* RDM対応は独自で開発する必要あり？
* max-sdk もしくは min-sdk どちらを使っても良い
* buildはcmakeで行えるように

## 設計

* imp.artnet.controller
* imp.artnet.node

を参考に

* bbb.artnet.controller
* bbb.artnet.ndoe

作りたい.

追加機能として

* set_offset ADDRESS_OFFSET V0 V1 ... VN メッセージ
	* imp.artnet.controller の機能
		* ADDRESS_OFFSET から V0, V1, ..., VNを埋める
* OSCでオブジェクト操作と同様のメッセージを送って操作
	* portはattributeで指定
	* /set 
* bbb.artnet.rdm オブジェクト
	* RDM over artnetの機能に準拠した機能

## 進捗

- [x] プロジェクト構造・CMakeビルドセットアップ
- [x] bbb.artnet.controller（送信）— list, channel, setchannel, set, set_offset, blackout, 各種mode, unicast/broadcast, OSC
- [x] bbb.artnet.node（受信）— DMX受信→list出力, sync_universes, 各種mode, OSC
- [x] OSC受信機能（controller / node）— bbb-oscのasio_receiver使用、NILマクロ衝突回避済み

## 今後の予定

### 動作確認・安定化
- [ ] Max/MSP上でcontroller/nodeの基本動作確認
- [ ] 実際のArt-Net機器との送受信テスト
- [ ] OSC経由での操作テスト
- [ ] メモリリーク・スレッド安全性の確認
- [ ] エッジケースの動作確認（num_universes変更時、mode切替時など）

### 機能追加
- [ ] attribute変更時の動的再初期化（net/subnet/universe/unicast_ip等の変更を即時反映）
- [ ] osc_port属性変更時の動的再設定
- [ ] node側のOSC出力機能（受信したDMXデータをOSCとして送信）
- [ ] controller側でOSC経由のattribute変更対応（/net, /subnet等）
- [ ] helpファイル（.maxhelp）の作成

### bbb.artnet.rdm
- [x] RDM over Art-Netの仕様調査
- [x] Discovery (DUB) 実装の詳細調査
- [ ] bbb.artnet.rdmオブジェクトの実装

### RDM over Art-Net 仕様調査結果

#### Art-Net RDMパケット

| Packet | Opcode | 用途 |
|---|---|---|
| ArtTodRequest | 0x8000 | TOD（UID一覧）要求 |
| ArtTodData | 0x8100 | TOD応答 |
| ArtTodControl | 0x8200 | TOD操作（flush等） |
| ArtRDM | 0x8300 | RDMフレーム送受信（E1.20生データを内包） |

#### 通信フロー

```
Controller → broadcast ArtTodRequest → Node
Node → broadcast ArtTodData (UID一覧) → Controller
Controller → broadcast ArtRDM (E1.20 GET/SET) → Node
Node → broadcast ArtRDM (E1.20 response) → Controller
Controller → ArtTodControl(TodFlush) → Node (再Discovery要求)
```

#### RDMフレーム構造 (E1.20)

```
Byte  Field
0     Start Code (0xCC)
1     Sub-Start Code (0x01)
2     Message Length
3-8   Destination UID (6 bytes)
9-14  Source UID (6 bytes)
15    Transaction Number
16    Port ID
17    Message Count
18-19 Sub-Device
20    Command Class (CC_GET / CC_SET / CC_GET_RESPONSE等)
21-22 Parameter ID (PID)
23    Parameter Data Length (PDL)
24+   Parameter Data
最後2byte  Checksum
```

#### libartnetのRDM対応状況

**対応している。** 以下のAPIが利用可能:

- 送信: `artnet_send_tod_request()`, `artnet_send_tod_control()`, `artnet_send_rdm()`
- 受信ハンドラ: `artnet_set_rdm_handler()`, `artnet_set_rdm_tod_handler()`, `artnet_set_rdm_initiate_handler()`
- TOD管理: `artnet_add_rdm_device()`, `artnet_remove_rdm_device()`

#### libartnetの制限

- **ArtRdmSub (0x8400) 未対応** — Art-Net 4のみ
- **RDM Discovery (DUB) 未対応** — binary searchパターンなし
- **rdmVerフィールドの誤用** — RDM versionではなくArt-Net versionを入れている
- **RDM responseの紐付けなし** — Transaction Number管理はアプリ側で必要

#### 実装難所

1. **DUB Discovery** — binary search + 衝突検出が最も複雑。libartnetは未実装
2. **Transaction Number管理** — request/responseの紐付け
3. **タイムアウト管理** — response待ち（推奨2-3秒）
4. **PID実装** — DEVICE_INFO, DMX_START_ADDRESS, IDENTIFY_DEVICE等の標準PID

#### 結論

libartnetのRDM APIで基本送受信は可能。Discovery（DUB）は自前実装が必要。
**OLAはDUB over Art-Netをサポートしていない**（DISCOVER_COMMANDをリジェクト）。
DUB Discoveryは完全に自前実装。OLAのDiscoveryAgentロジックは参考になる。
参考実装: OLA (Open Lighting Architecture) の `plugins/artnet/`, `libs/rdm/`

### RDM DUB Discovery 詳細

#### DISC_UNIQUE_BRANCH (PID 0x0001) Request

```
Byte  Field
0     Start Code 0xCC
1     Sub-Start 0x01
2     Length 0x24 (36)
3-8   Dest UID: ffff:ffffffff (broadcast)
9-14  Source UID: controller
15    Transaction Number
16    Port ID
17    Message Count 0x00
18-19 Sub-Device 0x0000
20    CC: 0x10 (DISCOVER_COMMAND)
21-22 PID: 0x0001
23    PDL: 0x0C (12)
24-29 Lower UID bound (6 bytes)
30-35 Upper UID bound (6 bytes)
36-37 Checksum
```

#### DUB Response（特殊エンコーディング）

通常のRDMフレームではない。XOR符号化で衝突検出を実現:

```
0-6   Preamble: 0xFE × 7
7     Separator: 0xAA
8-19  EUID: 12 bytes
        euid[2i]   = UID_byte[5-i] ^ 0xAA
        euid[2i+1] = UID_byte[5-i]
20-23 Encoded Checksum: 4 bytes
        ecs[0] = checksum_high ^ 0xAA
        ecs[1] = checksum_high
        ecs[2] = checksum_low ^ 0xAA
        ecs[3] = checksum_low
```

#### Mute / Un-Mute

- DISC_MUTE (PID 0x0002): CC=0x10, dest=特定UID, PDL=0
- DISC_UN_MUTE (PID 0x0003): CC=0x10, dest=broadcast, PDL=0
- Un-Muteは3回連続broadcast推奨

#### Binary Search アルゴリズム

```
1. UNMUTE broadcast × 3
2. Stack.push(0x0000:00000000, ffff:ffffffff)
3. DUB(range.lower, range.upper) をArtRDM broadcastで送信
4. timeout → Stack.pop、空なら終了
5. 有効なUID → MUTE送信 → 同じ範囲を再クエリ
6. collision → 中点で2分割し Stack.push(upper_half), Stack.push(lower_half)
7. Stack空になるまで 3-6 を繰り返し
```

#### 衝突検出（3段階）

1. Preamble validation: 0xFE列 + 0xAA separator
2. Checksum検証: EUID 12byteの合計 vs encoded checksum
3. 最小長チェック: 17byte以上必要

#### タイミング要件

| 項目 | 値 |
|---|---|
| DUB response timeout | 2-3秒 |
| Mute retry | 最大5回 |
| Unmute broadcast | 3回連続 |
| 同範囲再試行上限 | 5回 |
| 分岐失敗上限 | 5回 |

#### Art-Net上のラッピング

ArtRDM (opcode 0x8300) の data[] にRDMフレーム（0xCC start code抜き）を格納。
rdmVerフィールドは 0x01（RDM version）を設定すべき（libartnetはArt-Net version 14を誤設定）。

#### 実装方針

1. ArtRDMパケットの自前serialize/deserialize（libartnetのrdmVerバグ回避）
2. DUB Binary Searchエンジンの実装（OLAのDiscoveryAgentが参考）
3. Transaction Number管理
4. TOD (ArtTodRequest/Data) ベースの簡易Discoveryも検討
   - 全NodeがTODを返す → UID一覧が得られる（DUB不要）
   - ただしNode側がTODを正しく管理している前提

### 将来検討
- [ ] DUB Discovery (E1.20 binary search) — 自前実装が必要、OLAも未対応
- [ ] Windows対応

### Art-Net 4 対応状況

#### libartnet: Art-Net 3まで（実質放置）

- 最終リリース v1.1.2 (2013年12月)
- Art-Net 4の新opcode未定義、ポート数4固定
- ArtDMXのwire formatはArt-Net 3/4で互換 → **基本送受信はArt-Net 3/4で動作する**

#### Art-Net 4の主な変更点

| 項目 | Art-Net 3 | Art-Net 4 |
|---|---|---|
| ポート数/IP | 4in/4out | 1000+in/1000+out |
| sACN連携 | なし | ArtDataRequest/Reply |
| RDM Sub-device | なし | ArtRdmSub (0x8400) |
| 高速Poll | なし | ArtPollFpReply (0x2200) |
| TimeSync | なし | ArtTimeSync (0x9800) |
| Trigger | なし | ArtTrigger (0x9900) |

#### 成熟したArt-Net 4ライブラリは存在しない

- OLA: Art-Net 3まで（ArtRdmSubの定義のみ）
- lib-artnet-4-cpp: 未完成 (10 stars)
- libartnet: Art-Net 3、放置

#### 結論

基本DMX送受信はArt-Net 3/4互換なのでlibartnetで問題なし。
Art-Net 4固有機能（多ポート、sACN連携、RDM Sub-device）が必要になった場合は自前拡張 or ライブラリ乗り換え。

### sACN (E1.31) 調査結果

#### Art-Net vs sACN 比較

| 項目 | Art-Net | sACN |
|---|---|---|
| Port | 6454 | 5568 |
| 配信方式 | Broadcast | Multicast (universe単位) |
| Universe | net+subnet+universe (3階層) | flat 1-63999 |
| Priority | なし | あり (0-200、マージ対応) |
| 複数source | Last wins | Priority-based merge (HTP/LTP) |
| パケットヘッダ | ~18 bytes | ~126 bytes |
| 業界採用 | 広範（中小規模） | プロ用途主流（ETC, MA等） |
| 仕様 | Artistic Licence (独自) | ANSI E1.31 (オープン標準) |

#### Multicastアドレス

`239.255.(universe >> 8).(universe & 0xFF)` — Port 5568

#### ライブラリ

- **ETCLabs/sACN** (Apache 2.0) — ETC製の公式実装だが、EtcPalという大きな依存がありMax/MSP externalには重い
- OLA: sACN対応済みだがdaemon-based、external用途には不適
- **推奨: 自前の最小実装**

#### 自前実装の構成（推奨）

```
source/sacn/
├── sacn_packet.h       # パケット構造定義
├── sacn_sender.h/cpp   # 送信 (~200行)
└── sacn_receiver.h/cpp  # 受信 (~300行)
```

- BSD sockets直叩き、multicast join/leave
- 既存のbbb.artnet.controller/nodeを流用可能
- universe addressingがflatになるだけ

#### bbb.sacn.* のインターフェース案

```
bbb.sacn.controller
  attrs: universe(1-63999), priority(0-200), num_universes,
         mode, framerate, unicast, unicast_ip, osc_port,
         cid(auto), source_name
  messages: list, channel, setchannel, set, set_offset, bang

bbb.sacn.node
  attrs: universe(1-63999), num_universes, num_channels,
         mode, sync_universes, osc_port
  messages: bang
```

#### 工数見積もり

| タスク | 日数 |
|---|---|
| sACN packet送信 | 1-2日 |
| sACN packet受信 | 1-2日 |
| Multicast管理 | 0.5日 |
| bbb.sacn.controller | 1日 |
| bbb.sacn.node | 1日 |
| 実機テスト | 1-2日 |
| **合計** | **5-8日** |

#### 優先度

中。Art-Netでユースケースの90%をカバー。sACNは以下のニーズが出たら追加:
- ETC/MA機器との直接通信
- 複数sourceからのマージ（priority）
- Multicastベースの大規模ネットワーク

