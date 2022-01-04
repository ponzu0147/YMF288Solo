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
YMF288変換基板 (https://gimic.jp/index.php?GMC-MOD01%E8%AA%AC%E6%98%8E)  
MicroSD用コネクタ変換基板 CK-40 (https://www.sunhayato.co.jp/material2/ucb08/item_490)  

# 機能
## S98ファイルの再生
YM2608向けS98ファイルを再生します。  
現段階でADPCMには非対応（対応予定）  
その他の音源チップ向けデータの演奏は行いません。

## SDカードからの読み取り
SDカードのルートディレクトリおよび  
サブディレクトリ1段階までファイルを検索します。  
サブディレクトリの中にあるディレクトリには  
アクセスしません。

## SSGボリューム簡易調整
現在はソースファイル内にて定義している、  
SSGボリュームを簡易的に調整する機能を付与します。  
将来的にはFM音源部のキャリアの音量を減らす事で  
より自然なボリューム調整を可能にします。

# 現在の課題
2022/01/04 ソースのリファクタリング
