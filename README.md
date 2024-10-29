# バイク用自作マルチディスプレイ

## 対象バイク
クロスカブ JA60

## 機能
### ギアポジション認識・ウインカー認識
* マイコンのピン
    * 読取...OK
    * 表示...OK
        * ケーブルが多くなるため別手段検討
* IOエキスパンダー
    * 検討中
### ウインカー音
* ブザー(continurous)
    * 接続(IOピン-GND)...OK
    * 発音...OK
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
    * ドライバ：ST7789
    * 解像度：240*360 
* ケース
    * GoPro13対応防水ハウジング
   
