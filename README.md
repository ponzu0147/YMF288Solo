S98Player in ESP32 with YMF288.  
README.md is written by Japanese.  

ようこそ。

YMF288を使ったESP32(ESP-WROOM-32E)用 S98プレイヤーのプロジェクトです。  
機能の解説及び進捗状況はここに追記していきます。 

# 開発環境
Windows11 Pro  
WSL2 Ubuntu 18.04  
ESP-IDF

# 使用ハード
ESP32-DevKitC-32E(4MB) (https://akizukidenshi.com/catalog/g/gM-15673/)  
YMF288変換基板 GMC-MOD01 (https://gimic.jp/index.php?GMC-MOD01%E8%AA%AC%E6%98%8E)  
MicroSD用コネクタ変換基板 CK-40 (https://www.sunhayato.co.jp/material2/ucb08/item_490)  
ハイレゾリューションオーディオDACモジュール MM-5102 (https://www.sunhayato.co.jp/material2/ett09/item_765)

# 機能
### S98ファイルの再生
YM2608向けS98ファイルを再生します。  
現段階でADPCMには非対応（対応予定）  
その他の音源チップ向けデータの演奏は行いません。

### 物理ボタンによる選曲
1.次の曲に移動  
2.現在の曲を無限ループ再生（デフォルト：3ループ）  
3.前の曲に移動  

上記を3ボタンでの操作ができるようにしています。  
ディレクトリ内の最後・先頭のファイルの場合は、  
ディレクトリを跨いでの移動を可能としました。

### microSDカードからの読み取り
microSDカードのルートディレクトリおよび  
サブディレクトリ1段階までファイルを検索します。  
サブディレクトリの中にあるディレクトリには  
アクセスしません。

### リストファイルでのファイル管理
microSDカードのルートディレクトリに、  
s98.lst というファイルを作成します。  
このリストに記載されているS98ファイルの  
再生を行います。

現在は、電源投入後にSDカードへの検索を行い、  
リストファイルを作成してから演奏していますが、  
将来的にはユーザーによって任意のタイミングで  
リストの初期化、保存ができるようにします。  
（電源投入後すぐに演奏を始めるように待機時間の短縮をはかります）

### SSGボリューム簡易調整
YMF288はFM音源とSSG音源の音量バランスが、  
YM2608と異なっています。  
YM2608用のS98ファイルを再生すると、全体的に  
SSG音源の音量は小さめになってしまいます。

そこで現在はソースファイル内にて定義している、  
SSGボリュームを簡易的に調整する機能を付与しています。  
将来的にはFM音源部のキャリアの音量を減らす事で  
より自然なボリューム調整ができるようにします。

# 参考資料・謝辞(敬称略)
YM2608 OPNA アプリケーションマニュアル (c)YAMAHA  
YAMAHA LSI Data Book より YMF288 (c)YAMAHA  
GMC-MOD01 接続資料 (c)G.I.M.I.C Project([@gimicproject](https://twitter.com/gimicproject))  
s98dmp.c ソースファイル より s98フォーマット解析(Thanks for [@ume3fmp](https://twitter.com/ume3fmp/))  
ADPCM->PCM 変換ルーチン等 プログラム指南(Thanks for [@unn4m3cl](https://twitter.com/unn4m3cl/))  
[DAC出力フォーマット](https://github.com/osafune/fm_test_siggen/blob/master/doc/fmdac_output_format.pdf)(Thanks for [@s_osafune](https://twitter.com/s_osafune/))  

# 現在の課題(着手日)
2022/01/04 ソースのリファクタリング  
2022/01/09 ADPCM演奏  
2022/01/11 頭出し時のデータ化け対策  
~~2022/01/18 ver.0.1.1 試作基板作成(完了)~~  
~~2022/01/28 PCM5102AからFM/SSG演奏可能に(完了)~~  
2022/02/24 操作ボタンの仕様変更  
2022/xx/xx チャタリング対策・最適化  
2022/xx/xx リストの初期化・保存を任意に  
2022/xx/xx FM/SSG/RHYTHMの同期演奏ルーチン改良  

# リリース
2022/01/04 ver.0.1.0 初出  
2022/01/31 ver.0.1.1 基板作成にあたってGPIO番号の配置を変更  
2022/02/24 ver.0.1.2 ソフトウェア及び基板図とドキュメントの修正  

# ライセンス
ここにあるソース、ドキュメント、回路図、基板データについて  
Apache License 2.0 に基づきます。著作権は放棄しておりません。
