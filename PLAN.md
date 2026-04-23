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

