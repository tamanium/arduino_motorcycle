// 課題解決1. TFTへデータ送信前後でcsピンのHIGH-LOWを変更
// 課題解決2. SPI1を利用して接続

// --------------------ライブラリ--------------------
// 画面出力
#include <Adafruit_GFX.h>
// ディスプレイ
#include <Adafruit_ST7789.h>
//SPI通信
#include <SPI.h>
//時計機能
#include <RTClib.h>
//IOエキスパンダ
#include <Adafruit_PCF8574.h>
//温度計
#include <Temperature_LM75_Derived.h>
//温度計
//#include <M2M_LM75A.h>          

// --------------------自作クラス・ピン定義--------------------
#include "Define.h"			// 値定義
#include "GearPositions.h"	// ギアポジションクラス
#include "Winker.h"			// ウインカークラス

// --------------------ピン定義--------------------
// I2C
#define I2C_SCL   15
#define I2C_SDA   14
// ディスプレイ・SPI
#define TFT_MOSI  3
#define TFT_SCLK  2
#define TFT_BL    5
#define TFT_CS    6
#define TFT_DC    7
#define TFT_RST   8
// ギアポジション(IOエキスパンダ)
#define POSN  2
#define POS1  7
#define POS2  6
#define POS3  5
#define POS4  4
// ウインカー(IOエキスパンダ)
#define WNK_LEFT  0
#define WNK_RIGHT 1
// ウインカー音
#define BZZ_PIN 27
// 疑似ウインカーリレー
#define DMY_RELAY 0

// IOエキスパンダのアドレス
#define PCF_ADDRESS 0x27
// 温度計のアドレス
#define LM75_ADDRESS 0x48
#define I2C_ERROR 4
#define I2C_SUCCESS 0

// --------------------定数--------------------
const int CLOCK_INTERVAL   = 250;//ms
const int MONITOR_INTERVAL = 5;//ms
const int DISPLAY_INTERVAL = 30;//ms
const int BUZZER_DURATION  = 100;//ms
const int WINKER_DURATION  = 380;//ms
// 時刻表示の時・分表示位置
//const uint8_t TIME_INDEXES[2] = {0,3};
// フォントの寸法
const int FONT_HEIGHT = 8;
const int FONT_WIDTH  = 6;
const int TIME_SIZE   = 3;
const int GEAR_SIZE   = 24;
// ディスプレイの解像度
const int DISP_WIDTH = 320;
const int DISP_HEIGHT = 240;

enum LR{
    LEFT,
    RIGHT
};

enum TimeItem{
    MONTH,
    DAY,
    HOUR,
    MINUTE
};

// --------------------変数--------------------
unsigned long displayTime = 0;	// 表示処理
unsigned long monitorTime = 0;	// 各種読み取り
unsigned long clockTime = 0;	// 時計表示
unsigned long tempTime = 0;		// 温度測定にて使用
unsigned long bzzTime = 0;

unsigned long debugWinkerTime  = 0; //疑似ウインカー

// 保持用char配列
//uint16_t dateItems[3] = {0,0,0};
uint16_t timeItems[4] = {0,0,0,0};
// シフトポジション配列
int gears[] = {POSN, POS1, POS2, POS3, POS4};

//String defaultRealTime ="2024/10/25 00:00:00";
char nowTime[] = " 0:00";
// 三角形描画用座標
struct Triangle_coordinate {
	int x1;
	int y1;
	int x2;
	int y2;
	int x3;
	int y3;
};
// 座標
struct Coordinate {
	int x = 0;
	int y = 0;
};
/*
struct TriangleCoords{
	Coordinate c1;
	Coordinate c2;
	Coordinate c3;
	TriangleCoords(int x1, int y1, int x2, int y2, int x3, int y3){
		this->c1 = new Coordinate(x1, y1);
		this->c2 = new Coordinate(x2, y2);
		this->c3 = new Coordinate(x3, y3);
	}
};
*/
// 座標と倍率
struct DispInfo{
    int x;
    int y;
    int size;
};

// --------------------インスタンス--------------------
// 表示座標
Triangle_coordinate triCoords[2] = {
    {30, 0+24, 30, 160+24, 0, 80+24},
    {DISP_WIDTH-30-1, 0+24, DISP_WIDTH-30-1, 160+24, DISP_WIDTH-1, 80+24}
	/*new TriangleCoords(30, 0, 30, 160, 0, 80);
	new TriangleCoords(DISP_WIDTH-30-1, 0, DISP_WIDTH-30-1, 160, DISP_WIDTH-1, 8)*/
};

Coordinate timeCoords[4] = {
    // 月
    {DISP_WIDTH - FONT_WIDTH * TIME_SIZE * 5 - 1, 0},
    // 日
    {DISP_WIDTH - FONT_WIDTH * TIME_SIZE * 2 - 1, 0},
    // 時間
    {0, DISP_HEIGHT - FONT_HEIGHT * TIME_SIZE - 1},
    // 分
    {FONT_WIDTH * TIME_SIZE * 3, DISP_HEIGHT - FONT_HEIGHT * TIME_SIZE - 1}
};
// シフトポジション表示座標
Coordinate gearCoord = {98,24};
// シフトポジション：座標と文字倍率
DispInfo gearDispInfo = {200, 0, 1};
// 温度表示：座標と文字倍率
DispInfo tempDispInfo = {DISP_HEIGHT-24-1, DISP_WIDTH-90-1, 3};
// 電圧表示：座標と文字倍率
DispInfo voltDispInfo = {0, 0, 3};
// RTC
RTC_DS1307 rtc;
// IOエキスパンダ
Adafruit_PCF8574 pcf;
// 温度計
//M2M_LM75A lm75a;
Generic_LM75 lm75(&Wire1, LM75_ADDRESS);
//Generic_LM75 lm75(&Wire1);
// ディスプレイ
Adafruit_ST7789 tft(&SPI, TFT_CS, TFT_DC, TFT_RST);
// ギアポジション
GearPositions gearPositions(gears, sizeof(gears)/sizeof(int), &pcf);
// ウインカー
Winkers winkers(WNK_LEFT, WNK_RIGHT, &pcf);

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

	// I2C設定
    Wire1.setSDA(I2C_SDA);
    Wire1.setSCL(I2C_SCL);
    Wire1.begin();// いらないけど明示しておく

    // IOエキスパンダ
    pcf.begin(PCF_ADDRESS, &Wire1);
    //tft.print("IO Exp = ");
	//delay(500);
    //if(checkI2CDevice(PCF_ADDRESS) != I2C_SUCCESS){
    //    tft.print("NG! abort starting this device");
    //    while(true){};
    //}
    //tft.println("OK!");

	// RTC
    rtc.begin(&Wire1);
    //tft.print("clock = ");
	//delay(500);
    //if(checkI2CDevice(0x68) != I2C_SUCCESS){
    //    tft.print("NG! abort starting this device");
    //    while(true){};
    //}
    //tft.println("OK!");
	// 時計合わせ
    //rtc.adjust(DateTime(F(__DATE__),F(__TIME__)));


	// 温度計
    //rtc.begin(&Wire1);
    //tft.print("thermo = ");
	//delay(500);
    //if(checkI2CDevice(LM75_ADDRESS) != I2C_SUCCESS){
    //    tft.print("NG! abort starting this device");
    //    while(true){};
    //}
    //tft.println("OK!");


    // 疑似ウインカーリレー
    pinMode(DMY_RELAY, OUTPUT);
    // ウインカー音
    pinMode(BZZ_PIN, OUTPUT);
  
    // 画面リセット
	tft.fillScreen(ST77XX_BLACK);

	// ギアポジション表示開始その1
	//tft.setCursor(184, 8*8);
	//tft.setTextSize(3);
	//tft.print("gear");
	
	// ギアポジション表示開始その2
	//tft.setTextColor(ST77XX_WHITE);
	//tft.setTextSize(8);
	//tft.setCursor(200, 0);
	//tft.print('-');
    
    tft.setFont();
	tft.setTextSize(TIME_SIZE);
    tft.setCursor(timeCoords[HOUR].x, timeCoords[HOUR].y);
    tft.print("  :  ");
    tft.setCursor(timeCoords[MONTH].x, timeCoords[MONTH].y);
    tft.print("  /  ");

}

// ------------------------------ループ------------------------------
void loop() {
	// 経過時間(ms)取得
	unsigned long time = millis();
   
    // 疑似ウインカーリレー
    if(debugWinkerTime <= time){
        if(digitalRead(DMY_RELAY) == HIGH){
            digitalWrite(DMY_RELAY, LOW);
        }
        else{
            digitalWrite(DMY_RELAY, HIGH);
        }
        debugWinkerTime += WINKER_DURATION;
    }

    // 各種モニタリング・更新
	if(monitorTime <= time){
        // 現在のギアポジを取得
		gearPositions.monitor();
        // 現在のウインカー状態を取得
		winkers.monitor();
		monitorTime += MONITOR_INTERVAL;
	}
    /*
    // 温度モニタリング・表示
    if(tempTime <= time){
        tempTime += TEMP_INTERVAL;
    }
    */
	// 時計表示処理
	if(clockTime <= time){
        // 時刻表示
        realTimeDisplay(&tft, &rtc);
        Serial.println(lm75.readTemperatureC());
		clockTime += CLOCK_INTERVAL;
	}
	// 各種表示処理
	if(displayTime <= time){
		timeDisplay(time/1000, &tft);
		gearDisplay(gearPositions.getGear(), &tft);
		bool isSwitchStatus = winkersDisplay(winkers, &tft);
		// ウインカー点灯状態が切り替わった場合
		if(isSwitchStatus == true && bzzTime == 0 ){
			// ブザーON
			digitalWrite(BZZ_PIN, HIGH);
            // 時間設定
			bzzTime = time + BUZZER_DURATION;
		}
		displayTime += DISPLAY_INTERVAL;
	}
	//ブザーOFF処理
	if(bzzTime != 0 && bzzTime <= time){
		digitalWrite(BZZ_PIN, LOW);
        bzzTime = 0;
	}
}

// ------------------------------メソッド------------------------------
/**
 * ギアポジションの表示処理
 * @param dispChar char型 表示文字列
 * @param tft Adafruit_ST7735クラス ディスプレイ設定
 */
void gearDisplay(char newGear, Adafruit_ST77xx *tft){
	// バッファ文字列
	static char nowGear = '-';
	// 文字列比較
	if(nowGear != newGear){
		tft->setTextSize(8);
		// ----------表示文字削除----------
		tft->setCursor(gearCoord.x, gearCoord.y);
        tft->setTextColor(ST77XX_BLACK);
        tft->print(nowGear);
		// ----------新規文字表示----------
		tft->setCursor(gearCoord.x, gearCoord.y);
        tft->setTextColor(ST77XX_WHITE);
		tft->print(newGear);
		// バッファ文字列を上書き
		nowGear = newGear;
	}
}

/**
 * ウインカー表示処理
 * @param winkers Winkers型 ウインカークラス
 * @param tft Adafruit_ST7735クラス ディスプレイ設定
 * @return isSwitchStatus bool型 左右いずれかが点灯状態が切り替わった場合true
 */
bool winkersDisplay(Winkers &winkers, Adafruit_ST77xx *tft){
	// バッファ状態
    static bool buffer[2] = {false, false}; 
    // 返却用フラグ
	bool isSwitched = false;

	for(int side=LEFT; side<=RIGHT; side++){
        // 左ウインカー状態を判定
        if(buffer[side] != winkers.getStatus(side)){
            // バッファ上書き
            buffer[side] = winkers.getStatus(side);
            // ディスプレイ表示処理
            displayTriangle(triCoords[side], buffer[side], tft);
            // フラグ立てる
            isSwitched = true;
        }
    }
	return isSwitched;
}

/**
 * 左ウインカーのディスプレイ表示処理
 * @param status bool型 true...点灯, false...消灯
 * @param tft Adafruit_ST7735クラス ディスプレイ設定
 
void displayLeft(bool status, Adafruit_ST77xx &tft){
	// 文字色宣言（初期値は黒）
	uint16_t color = ST77XX_BLACK;
	// 条件trueの場合は文字色変更
	if(status == true){
		color = ST77XX_YELLOW;
	}
	// 図形表示（BLACKの場合は削除）
	tft.fillTriangle(triCoords[LEFT].x1,
					 triCoords[LEFT].y1,
					 triCoords[LEFT].x2,
					 triCoords[LEFT].y2,
					 triCoords[LEFT].x3,
					 triCoords[LEFT].y3,
					 color);
}
*/
/**
 * 右ウインカーのディスプレイ表示処理
 * @param status bool型 true...点灯, false...消灯
 * @param tft Adafruit_ST7735クラス ディスプレイ設定
 
void displayRight(bool status, Adafruit_ST77xx &tft){
	// 文字色宣言（初期値は黒）
	uint16_t color = ST77XX_BLACK;

	// 条件trueの場合は文字色変更
	if(status == true){
		color = ST77XX_YELLOW;
	}
	// 図形表示（BLACKの場合は削除）
	tft.fillTriangle(triCoords[RIGHT].x1,
					 triCoords[RIGHT].y1,
					 triCoords[RIGHT].x2,
					 triCoords[RIGHT].y2,
					 triCoords[RIGHT].x3,
					 triCoords[RIGHT].y3,
					 color);
}
*/
/**
 * 三角形表示処理
 * @param coord Triangle_coordinate型 
 * @param status bool型 true...点灯, false...消灯
 * @param tft Adafruit_ST7735クラス ディスプレイ設定
 */
void displayTriangle(Triangle_coordinate coord, bool status, Adafruit_ST77xx *tft){
	// 文字色宣言（初期値は黒）
	uint16_t color = ST77XX_BLACK;
	// 条件trueの場合は文字色変更
	if(status == true){
		color = ST77XX_YELLOW;
	}
	// 図形表示（BLACKの場合は削除）
	tft->fillTriangle(coord.x1, coord.y1,
					 coord.x2, coord.y2,
					 coord.x3, coord.y3,
					 color);
}
/**
 * 経過時間表示処理
 * @param totalSec long型 経過時間(秒)
 * @param tft Adafruit_ST7735クラス ディスプレイ設定
 * @param dispInfo 表示文字情報構造体 文字の座標と大きさ
 */
void realTimeDisplay(Adafruit_ST77xx *tft, RTC_DS1307 *rtc_ds1307){
    // 時刻用変数
    int newTimeItems[4] = {0,0,0,0};
    // 現在時刻取得
    DateTime now = rtc_ds1307->now();
    // 月・日・時間・分取得
    newTimeItems[MONTH]  = now.month();
    newTimeItems[DAY]    = now.day();
    newTimeItems[HOUR]   = now.hour();
    newTimeItems[MINUTE] = now.minute();

    tft->setTextSize(TIME_SIZE);
    // 時刻データでループ
    for(int i=0; i<sizeof(newTimeItems)/sizeof(int); i++){
        if(timeItems[i] != newTimeItems[i]){
            // ----------削除処理----------
            // 背景色・カーソル設定・値表示
            tft->setTextColor(ST77XX_BLACK);
            tft->setCursor(timeCoords[i].x, timeCoords[i].y);
            if(timeItems[i] < 10){
                tft->print('0');
            }
            tft->print(timeItems[i]);
            // ----------表示処理----------
            // 背景色・カーソル設定・値表示
            tft->setTextColor(ST77XX_WHITE);
            tft->setCursor(timeCoords[i].x, timeCoords[i].y);
            if(newTimeItems[i] < 10){
                tft->print('0');
            }
            tft->print(newTimeItems[i]);

            // 保持値更新
            timeItems[i] = newTimeItems[i];
        }
    }
    /*
    for(int i=HOUR; i<=MINUTE; i++){
        if(timeItems[i] != newTimeItems[i]){
            // ----------削除処理----------
            // 背景色・カーソル設定・値表示
            tft->setTextColor(ST77XX_BLACK);
            tft->setCursor(timeDispInfo->x + FONT_WIDTH * timeDispInfo->size * TIME_INDEXES[i], timeDispInfo->y);
            if(timeItems[i] < 10){
                tft->print('0');
            }
            tft->print(timeItems[i]);
            // ----------表示処理----------
            // 背景色・カーソル設定・値表示
            tft->setTextColor(ST77XX_WHITE);
            tft->setCursor(timeDispInfo->x + FONT_WIDTH * timeDispInfo->size * TIME_INDEXES[i], timeDispInfo->y);
            if(newTimeItems[i] < 10){
                tft->print('0');
            }
            tft->print(newTimeItems[i]);

            // 保持値更新
            timeItems[i] = newTimeItems[i];
        }
    }
    */
}

/**
 * 経過時間表示処理
 * @param totalSec long型 経過時間(秒)
 * @param tft Adafruit_ST7735クラス ディスプレイ設定
 */
void timeDisplay(long totalSec, Adafruit_ST77xx *tft){
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
    tft->setFont();
	tft->setTextSize(TIME_SIZE);

	for(int i=len-2; i>=0; i--){
		// 値が同じ場合処理スキップ
		if(nowTime[i] == newTime[i]){
			continue;
		}
        // ----------文字削除処理----------
        tft->setTextColor(ST77XX_BLACK);
        tft->setCursor(6*TIME_SIZE*i, 200-8*TIME_SIZE);
		tft->print(nowTime[i]);
        // ----------文字表示処理----------
        tft->setTextColor(ST77XX_WHITE);
        tft->setCursor(6*TIME_SIZE*i, 200-8*TIME_SIZE);
		// 数値を表示
		tft->print(newTime[i]);
		// 表示データを格納
		nowTime[i] = newTime[i];
	}
}

int checkI2CDevice(int address){
    Wire.beginTransmission(address);
    return Wire.endTransmission();
}
