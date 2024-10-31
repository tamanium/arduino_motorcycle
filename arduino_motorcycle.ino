// 課題解決1. TFTへデータ送信前後でcsピンのHIGH-LOWを変更
// 課題解決2. SPI1を利用して接続




// --------------------ライブラリ--------------------
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <RTClib.h>
#include <Fonts/FreeMono24pt7b.h>
#include <Fonts/FreeSerif24pt7b.h>


// --------------------自作クラス・ピン定義--------------------
#include "Define.h"			// 値定義
#include "GearPositions.h"	// ギアポジションクラス
#include "Winker.h"			// ウインカークラス

// --------------------ピン定義--------------------
// I2C
#define I2C_SCL   27
#define I2C_SDA   26
// ディスプレイ・SPI
#define TFT_MOSI  3
#define TFT_SCLK  2
#define TFT_BL    5
#define TFT_CS    6
#define TFT_DC    7
#define TFT_RST   8
// ギアポジション
#define POSN  13
#define POS1  9
#define POS2  10
#define POS3  11
#define POS4  12
// ウインカー
#define WNK_RIGHT 15
#define WNK_LEFT  14
// ウインカー音
#define BZZ_PIN 29

// --------------------定数--------------------
// 
const int CLOCK_INTERVAL = 250;//ms
const int MONITOR_INTERVAL = 5;//ms
const int DISPLAY_INTERVAL = 30;//ms

const uint8_t DATE_INDEXES[3] ={0,5,8};
const uint8_t TIME_INDEXES[2] = {0,3};
const int FONT_HEIGHT = 8;
const int FONT_WIDTH = 6;
const int FONT_SIZE_TIME = 3;

const int DISP_WIDTH = 320;
const int DISP_HEIGHT = 240;

enum DateItem{
    YEAR,
    MONTH,
    DAY
};

enum TimeItem{
    HOUR,
    MINUTE,
    SECOND
};

// --------------------変数--------------------
unsigned long displayTime = 0;	// 表示処理
unsigned long monitorTime = 0;	// 各種読み取り
unsigned long clockTime = 0;	// 時計表示
unsigned long tempTime = 0;		// 温度測定にて使用

// 保持用char配列
//uint16_t dateItems[3] = {0,0,0};
uint16_t timeItems[3] = {0,0,0};

//String defaultRealTime ="2024/10/25 00:00:00";
char nowTime[] = " 0:00";


struct Triangle_coordinates {
	int x1;
	int y1;
	int x2;
	int y2;
	int x3;
	int y3;
};

// --------------------インスタンス--------------------

Triangle_coordinates left = (30, 0, 30, 160, 0, 80};
Triangle_coordinates right = {DISP_WIDTH-30-1, 0, DISP_WIDTH-30-1, 160, DISP_WIDTH-1, 80};
RTC_DS1307 rtc;
Adafruit_ST7789 tft(&SPI, TFT_CS, TFT_DC, TFT_RST);// ディスプレイ設定
int gears[] = {POSN, POS1, POS2, POS3, POS4};
GearPositions gearPositions(gears, sizeof(gears)/sizeof(int));// ギアポジション設定
Winkers winkers(WNK_LEFT, WNK_RIGHT);// ウインカー設定

// ------------------------------初期設定------------------------------
void setup(void) {
    
    // デバッグ用シリアル設定
	Serial.begin(9600);
	// I2C設定
    Wire1.setSDA(I2C_SDA);
    Wire1.setSCL(I2C_SCL);

	//SPI1設定
	SPI.setTX(TFT_MOSI);
	SPI.setSCK(TFT_SCLK);
    // ディスプレイ明るさ設定(0-255)
    analogWrite(TFT_BL,30);
    
	//RTC起動
    rtc.begin(&Wire1);
	// 時計合わせ
    //rtc.adjust(DateTime(F(__DATE__),F(__TIME__)));
	
	// ディスプレイ初期化・画面向き・画面リセット
	tft.init(DISP_HEIGHT,DISP_WIDTH);
	tft.setRotation(3);
	tft.fillScreen(ST77XX_BLACK);
  
	// 初期表示
    tft.setTextSize(3);
	tft.setTextColor(ST77XX_GREEN);
	tft.setCursor(0, 0);
	tft.setTextWrap(true);
	tft.print("hello");
	
	delay(2000);
  
	// ギアポジション表示開始その1
	tft.fillScreen(ST77XX_BLACK);
	tft.setCursor(184, 8*8);
	tft.setTextSize(3);
	tft.print("gear");
	
	// ギアポジション表示開始その2
	tft.setTextColor(ST77XX_WHITE);
	tft.setTextSize(8);
	tft.setCursor(200, 0);
	tft.print('-');
    
    tft.setFont();
	tft.setTextSize(FONT_SIZE_TIME);
    tft.setCursor(0, DISP_HEIGHT-FONT_HEIGHT*FONT_SIZE_TIME);
    tft.print("  :  ");

}

// ------------------------------ループ------------------------------
void loop() {
	// 経過時間(ms)取得
	unsigned long time = millis();

    // 各種モニタリング・更新
	if(monitorTime <= time){
		gearPositions.monitor();
		winkers.monitor();
		monitorTime += MONITOR_INTERVAL;
	}
	// 時計表示処理
	if(clockTime <= time){
        realTimeDisplay(tft);
		clockTime += CLOCK_INTERVAL;
	}
	// 各種表示処理
	if(displayTime <= time){
		timeDisplay(time/1000, tft);
		gearDisplay(gearPositions.getGear(), tft);
		winkersDisplay(winkers, tft);
		displayTime += DISPLAY_INTERVAL;
	}
}

// ------------------------------メソッド------------------------------
/**
 * ギアポジションの表示処理
 * @param dispChar char型 表示文字列
 * @param tft Adafruit_ST7735クラス ディスプレイ設定
 */
void gearDisplay(char newGear, Adafruit_ST77xx &tft){
	// バッファ文字列
	static char nowGear = '-';
	// 文字列比較
	if(nowGear != newGear){
		tft.setTextSize(8);
		// ----------表示文字削除----------
		tft.setCursor(200, 0);
        tft.setTextColor(ST77XX_BLACK);
        tft.print(nowGear);
		// ----------新規文字表示----------
		tft.setCursor(200, 0);
        tft.setTextColor(ST77XX_WHITE);
		tft.print(newGear);
		// バッファ文字列を上書き
		nowGear = newGear;
	}
}

/**
 * ウインカー表示処理
 * @param winkers Winkers型 ウインカークラス
 * @param tft Adafruit_ST7735クラス ディスプレイ設定
 */
void winkersDisplay(Winkers &winkers, Adafruit_ST77xx &tft){
	// バッファ状態
	static bool bufferStatusLeft = false;
	static bool bufferStatusRight = false;
	
	// 左ウインカー状態を判定
	if(bufferStatusLeft != winkers.getStatusLeft()){
		// バッファ上書き
		bufferStatusLeft = winkers.getStatusLeft();
        // ディスプレイ表示処理
		displayLeft(bufferStatusLeft, tft);
	}
	// 右ウインカー状態を判定
	if(bufferStatusRight != winkers.getStatusRight()){
		// バッファ上書き
		bufferStatusRight = winkers.getStatusRight();
		// ディスプレイ表示処理
        displayRight(bufferStatusRight, tft);
	}
}

/**
 * 左ウインカーのディスプレイ表示処理
 * @param status bool型 true...点灯, false...消灯
 * @param tft Adafruit_ST7735クラス ディスプレイ設定
 */
void displayLeft(bool status, Adafruit_ST77xx &tft){
		if(status == true){
			// 図形表示
			//tft.fillTriangle(30, 0, 30, 160, 0, 80, ST77XX_YELLOW);
            tft.fillTriangle(left.x1, left.y1, left.x2, left.y2, left.x3, left.y3, ST77XX_YELLOW);
			return;
		}
        // 図形削除
        //tft.fillTriangle(30, 0, 30, 160, 0, 80, ST77XX_BLACK);
		tft.fillTriangle(left.x1, left.y1, left.x2, left.y2, left.x3, left.y3, ST77XX_BLACK);
}

/**
 * 右ウインカーのディスプレイ表示処理
 * @param status bool型 true...点灯, false...消灯
 * @param tft Adafruit_ST7735クラス ディスプレイ設定
 */
void displayRight(bool status, Adafruit_ST77xx &tft){
		if(status == true){
			// 図形表示
			//tft.fillTriangle(DISP_WIDTH-30-1, 0, DISP_WIDTH-30-1, 160, DISP_WIDTH-1, 80, ST77XX_YELLOW
            tft.fillTriangle(right.x1, right.y1, right.x2, right.y2, right.x3, right.y3, ST77XX_YELLOW);
            return;
		}
        // 図形削除
        //tft.fillTriangle(DISP_WIDTH-30-1, 0, DISP_WIDTH-30-1, 160, DISP_WIDTH-1, 80, ST77XX_BLACK);
        tft.fillTriangle(right.x1, right.y1, right.x2, right.y2, right.x3, right.y3, ST77XX_BLACK);
}
/**
 * 経過時間表示処理
 * @param totalSec long型 経過時間(秒)
 * @param tft Adafruit_ST7735クラス ディスプレイ設定
 */
void realTimeDisplay(Adafruit_ST77xx &tft){
    // 時刻用変数
    int newTimeItems[3] = {0,0,0};
    // 現在時刻取得
    DateTime now = rtc.now();
    // 時間・分取得
    newTimeItems[HOUR] = now.hour();
    newTimeItems[MINUTE] = now.minute();

    tft.setFont();
	tft.setTextSize(FONT_SIZE_TIME);
    // 時刻データでループ
    for(int i=0; i<sizeof(timeItems)/sizeof(uint16_t); i++){
        if(timeItems[i] != newTimeItems[i]){
            // ----------削除処理----------
            // 背景色・カーソル設定・値表示
            tft.setTextColor(ST77XX_BLACK);
            tft.setCursor(FONT_WIDTH*FONT_SIZE_TIME*TIME_INDEXES[i], DISP_HEIGHT-FONT_HEIGHT*FONT_SIZE_TIME);
            if(timeItems[i] < 10){
                tft.print('0');
            }
            tft.print(timeItems[i]);
            // ----------表示処理----------
            // 背景色・カーソル設定・値表示
            tft.setTextColor(ST77XX_WHITE);
            tft.setCursor(FONT_WIDTH*FONT_SIZE_TIME*TIME_INDEXES[i], DISP_HEIGHT-FONT_HEIGHT*FONT_SIZE_TIME);
            if(newTimeItems[i] < 10){
                tft.print('0');
            }
            tft.print(newTimeItems[i]);

            // 保持値更新
            timeItems[i] = newTimeItems[i];
        }
    }
}

/**
 * 経過時間表示処理
 * @param totalSec long型 経過時間(秒)
 * @param tft Adafruit_ST7735クラス ディスプレイ設定
 */
void timeDisplay(long totalSec, Adafruit_ST77xx &tft){
	// 保持用char配列
	static char nowTime[6] = " 0:00";
	static int len = sizeof(nowTime)/sizeof(char);
	 // 表示char配列作成(arduinoでは array[n+1]で定義
	char newTime[6] = "  :  ";

	// 経過時間を各桁で文字列化
	newTime[4] = '0' + (totalSec%60)%10;
	newTime[3] = '0' + (totalSec%60)/10;
	newTime[1] = '0' + (totalSec/60)%10;
	newTime[0] = '0' + (totalSec/60)/10;
  
	// フォント設定
    tft.setFont();
	tft.setTextSize(FONT_SIZE_TIME);

	for(int i=len-2; i>=0; i--){
		// 値が同じ場合処理スキップ
		if(nowTime[i] == newTime[i]){
			continue;
		}
        // ----------文字削除処理----------
        tft.setTextColor(ST77XX_BLACK);
        tft.setCursor(6*FONT_SIZE_TIME*i, 200-8*FONT_SIZE_TIME);
		tft.print(nowTime[i]);
        // ----------文字表示処理----------
        tft.setTextColor(ST77XX_WHITE);
        tft.setCursor(6*FONT_SIZE_TIME*i, 200-8*FONT_SIZE_TIME);
		// 数値を表示
		tft.print(newTime[i]);
		// 表示データを格納
		nowTime[i] = newTime[i];
	}
}
