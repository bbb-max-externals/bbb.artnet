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
- [ ] RDM over Art-Netの仕様調査
- [ ] RDM用独自スタックの検討（libartnetはRDM非対応の可能性）
- [ ] bbb.artnet.rdmオブジェクトの実装

### 将来検討
- [ ] Windows対応
- [ ] Art-Net 4対応（libartnetの対応状況確認）
- [ ] sACN (E1.31) 等の他プロトコル対応の要望確認

