// --------------------ライブラリ--------------------
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

// --------------------自作クラス・ピン定義--------------------

#include "Define.h"		//値定義
#include "Pins.h"		//ピン設定
#include "Classes.h"	//クラス


// --------------------変数--------------------

unsigned long startTime = 0;
unsigned long posTime = 0;
unsigned long wnkTime = 0;
unsigned long bzzTime = 1;

bool wnkRightStatus = false;
bool wnkLeftStatus = false;

bool isWnkRight = false;
bool isWnkLeft = false;

char  nowTime[6] = " 0:00";
unsigned long realTimeLong = 0;

int timeFontSize = 2;

// --------------------インスタンス--------------------
// ディスプレイ設定
Adafruit_ST7735 tft = Adafruit_ST7735(&SPI, TFT_CS, TFT_DC, TFT_RST);
// ギアポジション設定
GearPosition gearArray[] = {
	GearPosition(POSN, 'N'),
	GearPosition(POS1, '1'),
	GearPosition(POS2, '2'),
	GearPosition(POS3, '3'),
	GearPosition(POS4, '4')
};
int gearArrayLen = sizeof(gearArray)/sizeof(GearPosition);

// ウインカー設定
Winker winkers[] = {
	Winker(WNK_LEFT),
	Winker(WNK_RIGHT)
};

void setup(void) {
  
	Serial.begin(9600);

	//SPI接続設定
	SPI.setTX(TFT_MOSI);
	SPI.setSCK(TFT_SCLK);

	// ディスプレイ初期化・画面向き・画面リセット
	tft.initR(INITR_BLACKTAB);
	tft.setRotation(3);
	tft.fillScreen(ST77XX_BLACK);
  
	// 初期表示
	tft.setCursor(0, 0);
	tft.setTextColor(ST77XX_GREEN);
	tft.setTextSize(3);
	tft.setTextWrap(true);
	tft.print("hello");
	delay(2000);
  
	// ギアポジション表示開始
	tft.fillScreen(ST77XX_BLACK);
	tft.setCursor(8*5+4, 8*8);
	tft.setTextSize(3);
	tft.print("gear");

	tft.setTextColor(ST77XX_WHITE);
	tft.setTextSize(8);
	tft.setCursor(8*7+4, 0);
	tft.print('-');

	// 時計表示開始
	tft.setTextColor(ST77XX_WHITE);
	tft.setTextSize(2);
	tft.setCursor(0,128-8*2);
	tft.print("00:00");

/*
	tone(tonePin, 220);
	delay(100);
	noTone(tonePin);
	delay(150);
	tone(tonePin, 220);
	delay(100);
	noTone(tonePin);
	delay(150);
*/

	// 起動時の時間を取得
	startTime = millis();
	posTime = startTime + posInterval;
	wnkTime = startTime + wnkInterval;
}

void loop() {
	unsigned long time = millis() - startTime;
	// --------------------時計機能--------------------
	// システム時間(秒)取得
	unsigned long newTimeSec = time/1000;

	// --------------------経過時間表示処理--------------------
	timeDisplay(newTimeSec);
  
	// --------------------ギアポジ表示処理--------------------
	gearDisplay(gearArray, gearArrayLen);
	
	// --------------------ウインカー表示処理--------------------
  if(winkers[0].getStatus() == true){
    tft.fillTriangle(31, 0, 31, 62, 0, 31, ST7735_YELLOW);
  }
  else{
    tft.fillRect(0, 0, 32, 63, ST77XX_BLACK);
  }
}

void timeDisplay(long totalSec){
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

/**
 * ギアポジションの表示処理
 * @param gearArray GearPositionクラス(ポインタ) ギアポジションクラス配列
 * @param len int型 配列の長さ
 */
void gearDisplay(GearPosition *gearArray, int len){
	// 表示値保持用変数
	static char nowGear = '-';
	// エッジカウント変数
	static int countEdge = 0;
	// 現在のギアポジション表示文字列取得
	char newGear = getGearPos(gearArray, len);

	// エッジカウント変数加算
	if(nowGear != newGear){
		countEdge++;
	}
	else{
		countEdge = 0;
		return;
	}

	// カウント変数が一定数以上となった場合
	if(10 <= countEdge){
		// 格納用変数に代入
		nowGear = newGear;
		// 表示値削除・カーソル設定・フォント設定・表示処理
		tft.fillRect(8*7+4,0,6*8,8*8,ST77XX_BLACK);
		tft.setCursor(8*7+4, 0);
		tft.setTextSize(8);
		tft.print(nowGear);
		// カウント変数リセット
		countEdge = 0;
	}
}

/**
 * ギアポジション表示値の取得
 * @param gearArray GearPositionクラス(ポインタ) ギアポジションクラス配列
 * @param len int型 配列の長さ
 * @return char
 */
char getGearPos(GearPosition *gearArray, int len ){
	for(int i=0; i<len; i++ ){
		if(gearArray[i].isActive() == true){
			return gearArray[i].getChar();
		}
	}
	return '-';
}