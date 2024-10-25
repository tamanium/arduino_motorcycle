// 課題解決1. TFTへデータ送信前後でcsピンのHIGH-LOWを変更
// 課題解決2. SPI1を利用して接続




// --------------------ライブラリ--------------------
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <RTClib.h>


// --------------------自作クラス・ピン定義--------------------
#include "Define.h"			// 値定義
#include "GearPositions.h"	// ギアポジションクラス
#include "Winker.h"			// ウインカークラス

// --------------------ピン定義--------------------

#define I2C_SCL   5
#define I2C_TX    4
// ディスプレイ
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
#define WNK_RIGHT 26
#define WNK_LEFT 27
// ビープ音
#define BZZ_PIN 29

// --------------------定数--------------------
const int CLOCK_INTERVAL = 50;//ms
const int MONITOR_INTERVAL = 5;//ms
const int DISPLAY_INTERVAL = 30;//ms

const int BUFFER_LENGTH = 128;

// --------------------変数--------------------
unsigned long displayTime = 0;	// 表示処理
unsigned long monitorTime = 0;	// 各種読み取り
unsigned long ClockTime = 0;	// GPSデータ取得・表示
unsigned long tempTime = 0;		// 温度測定にて使用

//String defaultRealTime ="2024/10/25 00:00:00";
char nowTime[] = " 0:00";
int timeFontSize = 2;

// --------------------インスタンス--------------------
RTC_DS1307 rtc;
Adafruit_ST7789 tft(&SPI, TFT_CS, TFT_DC, TFT_RST);// ディスプレイ設定
int gears[] = {POSN, POS1, POS2, POS3, POS4};
GearPositions gearPositions(gears, sizeof(gears)/sizeof(int));// ギアポジション設定
Winkers winkers(WNK_LEFT, WNK_RIGHT);// ウインカー設定

// ------------------------------初期設定------------------------------
void setup(void) {
    
    // デバッグ用シリアル設定
	Serial.begin(9600);

    Wire.setSDA(I2C_TX);
    Wire.setSCL(I2C_SCL);
    rtc.begin();

    rtc.adjust(DateTime(2024,10,26,12,41,0));
    //rtc.adjust(DateTime(F(__DATE__),F(__TIME__)));
    // ピン設定
    analogWrite(TFT_BL,30);

	//SPI1接続設定
	SPI.setTX(TFT_MOSI);
	SPI.setSCK(TFT_SCLK);
    
	// ディスプレイ初期化・画面向き・画面リセット
	//tft.initR(INITR_BLACKTAB);
	tft.init(240,320);
    tft.setRotation(3);
	tft.fillScreen(ST77XX_BLACK);
  
	// 初期表示
	tft.setCursor(0, 0);
	tft.setTextColor(ST77XX_GREEN);
	tft.setTextSize(3);
	tft.setTextWrap(true);
	tft.print("hello");
	
	delay(2000);
  
	// ギアポジション表示開始その1
	tft.fillScreen(ST77XX_BLACK);
	tft.setCursor(8*5+4, 8*8);
	tft.setTextSize(3);
	tft.print("gear");
	
	// ギアポジション表示開始その2
	tft.setTextColor(ST77XX_WHITE);
	tft.setTextSize(8);
	tft.setCursor(8*7+4, 0);
	tft.print('-');

	// 時計表示開始
    /*
	tft.setTextColor(ST77XX_WHITE);
	tft.setTextSize(2);
	tft.setCursor(0,128-8*2);
	tft.print(defaultRealTime);
    */
}

// ------------------------------ループ------------------------------
void loop() {
    
    //char static buf[128];
	// 経過時間(ms)取得
	unsigned long time = millis();

    // 各種モニタリング・更新
	if(monitorTime <= time){
		gearPositions.monitor();
		winkers.monitor();
		monitorTime += MONITOR_INTERVAL;
	}
	// 各種表示処理
	if(displayTime <= time){
		timeDisplay(time/1000, tft);
        realTimeDisplay(tft);
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
void gearDisplay(char dispChar, Adafruit_ST77xx &tft){
	// バッファ文字列
	static char bufferChar = '-';
	// 文字列比較
	if(bufferChar != dispChar){
		// バッファ文字列を上書き
		bufferChar = dispChar;
		// 表示処理
		tft.fillRect(8*7+4,0,6*8,8*8,ST77XX_BLACK);
		tft.setCursor(8*7+4, 0);
		tft.setTextSize(8);
		tft.print(bufferChar);
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
		// バッファ状態を上書き
		bufferStatusLeft = winkers.getStatusLeft();
		if(bufferStatusLeft == true){
			// 図形表示
			tft.fillTriangle(31, 0, 31, 62, 0, 31, ST77XX_YELLOW);
		}
		else{
			// 図形削除
			tft.fillRect(0, 0, 32, 63, ST77XX_BLACK);
		}
	}
	// 右ウインカー状態を判定
	if(bufferStatusRight != winkers.getStatusRight()){
		// バッファ状態を上書き
		bufferStatusRight = winkers.getStatusRight();
		if(bufferStatusRight == true){
			// 図形表示
			tft.fillTriangle(160-31-1, 0, 160-31-1, 62, 160-0-1, 31, ST77XX_YELLOW);
		}
		else{
			// 図形削除
			tft.fillRect(160-31-1, 0, 32, 63, ST77XX_BLACK);
		}
	}
}

/**
 * 経過時間表示処理
 * @param totalSec long型 経過時間(秒)
 * @param tft Adafruit_ST7735クラス ディスプレイ設定
 */
void realTimeDisplay(Adafruit_ST77xx &tft){
	// 保持用char配列
    static String realTime = "";
    String _realTime = "";

    DateTime now = rtc.now();
	_realTime += now.year();
    _realTime += '/';
    uint16_t tmp = now.month();
    if(tmp < 10){
        _realTime +='0';
    }
    _realTime += tmp;
    _realTime += '/';
    tmp = now.day();
    if(tmp < 10){
        _realTime +='0';
    }
    _realTime += tmp;
    _realTime += ' ';
    tmp = now.hour();
    if(tmp < 10){
        _realTime +='0';
    }
    _realTime += tmp;
    _realTime += ':';
    tmp = now.minute();
    if(tmp < 10){
        _realTime +='0';
    }
    _realTime += tmp;
    _realTime += ':';
    tmp = now.second();
    if(tmp < 10){
        _realTime +='0';
    }
    _realTime += tmp;
    if(realTime.equals(_realTime) == 0){
        //return;
    }
	// フォント設定
	tft.setTextColor(ST77XX_BLACK);
	tft.setTextSize(timeFontSize);
    // カーソル設定
	tft.setCursor(6*timeFontSize*0, 128-8*timeFontSize);
    // 前回表示を削除
    tft.print(realTime);

	tft.setTextColor(ST77XX_WHITE);
    // カーソル設定
	tft.setCursor(6*timeFontSize*0, 128-8*timeFontSize);
    // 表示
    tft.print(_realTime);

    realTime = String(_realTime);
}

/**
 * 経過時間表示処理
 * @param totalSec long型 経過時間(秒)
 * @param tft Adafruit_ST7735クラス ディスプレイ設定
 */
void timeDisplay(long totalSec, Adafruit_ST77xx &tft){
	// 保持用char配列
	static char  nowTime[6] = " 0:00";
	static int len = sizeof(nowTime)/sizeof(char);
	 // 表示char配列作成(arduinoでは array[n+1]で定義
	char newTime[6] = "  :  ";

	// 経過時間を各桁で文字列化
	newTime[4] = '0' + (totalSec%60)%10;
	newTime[3] = '0' + (totalSec%60)/10;
	newTime[1] = '0' + (totalSec/60)%10;
	newTime[0] = '0' + (totalSec/60)/10;
  
	// フォント設定
	tft.setTextColor(ST77XX_WHITE);
	tft.setTextSize(timeFontSize);

	for(int i=len-2; i>=0; i--){
		// 値が同じ場合処理スキップ
		if(nowTime[i] == newTime[i]){
			continue;
		}
		// 表示データを格納
		nowTime[i] = newTime[i];
		// 該当表示をクリア
		tft.fillRect(6*timeFontSize*i, 200-8*timeFontSize, 6*timeFontSize, 8*timeFontSize, ST77XX_BLACK);
		// カーソル設定
		tft.setCursor(6*timeFontSize*i, 200-8*timeFontSize);
		// 数値を表示
		tft.print(nowTime[i]);
	}
}
