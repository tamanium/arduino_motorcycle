# バイク用自作マルチディスプレイ

## 対象バイク
クロスカブ JA60

## 機能
### シフトインジケーター
* マイコン
    * 読取...OK
    * 表示...OK
        * ケーブルが多くなるため別手段検討
### ウインカー表示
* マイコン
    * 読取...OK
    * 表示...OK
        * ケーブルが多くなるため別手段検討
### ウインカー音
* ブザー
### 時計
* Tiny RTC(-)
    * 接続...OK(I2C)
    * 読取...OK
    * 表示...OK
    * 外部電源無しでの稼働... OK
### 温度計
使用部品
* MAX6675
    * 接続...OK(SPI1)
    * 読取...OK
    * 表示...OK
        * データが怪しいので別手段検討
## その他使用部品
* マイコン
    * WaveShare RP2040-Zero
* ディスプレイ
* ケース