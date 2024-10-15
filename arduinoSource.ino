// --------------------ライブラリ--------------------
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include "MAX6675.h"
#include <SPI.h>

// --------------------自作クラス・ピン定義--------------------
#include "Define.h"			//値定義
#include "Pins.h"			//ピン設定
#include "GearPositions.h"	//ギアポジションクラス
#include "Winker.h"			//ウインカークラス


// --------------------定数--------------------
const int monitorInterval = 10;//ms
const int displayInterval = 50;//ms
//const int gearDispInterval = 100;//ms

// --------------------変数--------------------

unsigned long startTime = 0;
unsigned long posTime = 0;
unsigned long wnkTime = 0;
unsigned long bzzTime = 1;
unsigned long displayTime = 0;
unsigned long monitorTime = 0;
unsigned long tempTime = 0;

char  nowTime[6] = " 0:00";
unsigned long realTimeLong = 0;
int timeFontSize = 2;

// --------------------インスタンス--------------------
// ディスプレイ設定
Adafruit_ST7735 tft = Adafruit_ST7735(&SPI, TFT_CS, TFT_DC, TFT_RST);
// ギアポジション設定
int gears[] = {POSN, POS1, POS2, POS3, POS4};
GearPositions gearPositions = GearPositions(gears, sizeof(gears)/sizeof(int));
// ウインカー設定
Winkers winkers = Winkers(WNK_LEFT, WNK_RIGHT);
// 温度計設定
MAX6675 thermoCouple(THM_CS, THM_MOSI, THM_SCLK);

// ------------------------------初期設定------------------------------
void setup(void) {
  
	Serial.begin(9600);

	//SPI接続設定
	SPI.setTX(TFT_MOSI);
	SPI.setSCK(TFT_SCLK);
	
	thermoCouple.begin();
	thermoCouple.setSPIspeed(4000000);

	// ディスプレイ初期化・画面向き・画面リセット
	tft.initR(INITR_BLACKTAB);
	tft.setSPISpeed(4000000);
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
	tft.setTextColor(ST77XX_WHITE);
	tft.setTextSize(2);
	tft.setCursor(0,128-8*2);
	tft.print("00:00");

	// 起動時の時間を取得
	startTime = millis();
}

// ------------------------------ループ------------------------------
void loop() {
	// 経過時間(ms)取得
	unsigned long time = millis() - startTime;
	
    if(tempTime <= time){
        //Serial.print(thermoCouple.read());
        //Serial.print("\ttemp: ");
        //Serial.println(thermoCouple.getTemperature());
        tempTime += 2000;
    }
	
    // 各種モニタリング・更新
	if(monitorTime <= time){
		gearPositions.monitor();
		winkers.monitor();
		monitorTime += monitorInterval;
	}
	
	// 各種表示処理
	if(displayTime <= time){
		timeDisplay(time/1000, tft);
		gearDisplay(gearPositions.getGear(), tft);
		winkersDisplay(winkers, tft);
		displayTime += displayInterval;
	}
	
	/*
	// ウインカー表示処理
	if(winkers[0].getStatus() == true){
		tft.fillTriangle(31, 0, 31, 62, 0, 31, ST7735_YELLOW);
	}
	else{
		tft.fillRect(0, 0, 32, 63, ST77XX_BLACK);
	}
	*/
}

// ------------------------------メソッド------------------------------
/**
 * ギアポジションの表示処理
 * @param dispChar char型 表示文字列
 * @param tft Adafruit_ST7735クラス ディスプレイ設定
 */
void gearDisplay(char dispChar, Adafruit_ST7735 &tft){
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
void winkersDisplay(Winkers &winkers, Adafruit_ST7735 &tft){
	// バッファ状態
	static bool bufferStatusLeft = false;
	static bool bufferStatusRight = false;
	
	// 左ウインカー状態を判定
	if(bufferStatusLeft != winkers.getStatusLeft()){
		// バッファ状態を上書き
		bufferStatusLeft = winkers.getStatusLeft();
		if(bufferStatusLeft == true){
			// 図形表示
			tft.fillTriangle(31, 0, 31, 62, 0, 31, ST7735_YELLOW);
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
			tft.fillTriangle(160-31-1, 0, 160-31-1, 62, 160-0-1, 31, ST7735_YELLOW);
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
void timeDisplay(long totalSec, Adafruit_ST7735 &tft){
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
		tft.fillRect(6*timeFontSize*i, 128-8*timeFontSize, 6*timeFontSize, 8*timeFontSize, ST7735_BLACK);
		// カーソル設定
		tft.setCursor(6*timeFontSize*i, 128-8*timeFontSize);
		// 数値を表示
		tft.print(nowTime[i]);
	}
}
