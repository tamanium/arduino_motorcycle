// --------------------ライブラリ--------------------
#include <Adafruit_GFX.h>               // 画面出力
#include <Adafruit_ST7789.h>            // ディスプレイ
#include <SPI.h>                        // SPI通信
#include <RTClib.h>                     // 時計機能
#include <Adafruit_PCF8574.h>           // IOエキスパンダ
#include <Temperature_LM75_Derived.h>   // 温度計
#include <Adafruit_ADS1X15.h>           // ADコンバータ

// --------------------自作クラス・ピン定義--------------------
#include "Define.h"			// 値定義
#include "GearPositions.h"	// ギアポジションクラス
#include "Winker.h"			// ウインカークラス
#include "Switch.h"         //スイッチクラス

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
// ウインカー音
#define BZZ_PIN 27
// 疑似ウインカーリレー
#define DMY_RELAY 0
// ウインカー(IOエキスパンダ)
#define WNK_LEFT  0
#define WNK_RIGHT 1
// スイッチ(IOエキスパンダ)
#define SW 3
// ギアポジション(IOエキスパンダ)
#define POSN  2
#define POS1  7
#define POS2  6
#define POS3  5
#define POS4  4

// --------------------定数--------------------
const int CLOCK_INTERVAL   = 200;   //ms
const int MONITOR_INTERVAL = 5;     //ms
const int DISPLAY_INTERVAL = 30;    //ms
const int TEMP_INTERVAL    = 1000;  //ms
const int BUZZER_DURATION  = 100;   //ms
const int WINKER_DURATION  = 380;   //ms
// 時刻表示の時・分表示位置
//const uint8_t TIME_INDEXES[2] = {0,3};
// フォントの寸法
const int FONT_HEIGHT = 8;
const int FONT_WIDTH  = 6;
const int DATE_SIZE   = 2;
const int TIME_SIZE   = 3;
const int TEMP_SIZE   = 3;
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
    MINUTE,
    SECOND
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

// 表示情報
struct DispInfo{
    int x = 0;
    int y = 0;
    int size = 0;
};

//
struct Module{
    String name;
    byte address;
};
Module moduleArr[] = {
    {"ioExp",0x27}, // PCF8574 エキスパンダ
    {"therm",0x48},
    {"adCnv",0x4A},  // ADS1115 ADコンバータ
    {"rtc_a",0x50},
    {"rtc_d",0x68}
};
enum ModuleNum{
    IOEXP,
    THERM,
    ADCNV,
    RTC_A,
    RTC_D
};

/**
 * i2cモジュールのアドレスから名前を取得 
 *
 * @param adrs i2cモジュールのアドレス
 * @return i2cモジュールの名前 hitしなければアドレス
 */
String getModuleName(byte adrs){
    String retStr = "";
    for(Module m : moduleArr){
        if(m.address == adrs){
            return m.name;
        }
    }
    return String(adrs, HEX)+"   ";
}

bool existsModule(byte adrs){
    for(Module m : moduleArr){
        if(m.address == adrs){
            return true;
        }
    }
    return false;
}

// --------------------インスタンス--------------------
// 表示座標
Triangle_coordinate triCoords[2] = {
    {30, 0+24, 30, 160+24, 0, 80+24},
    {DISP_WIDTH-30-1, 0+24, DISP_WIDTH-30-1, 160+24, DISP_WIDTH-1, 80+24}
	/*new TriangleCoords(30, 0, 30, 160, 0, 80);
	new TriangleCoords(DISP_WIDTH-30-1, 0, DISP_WIDTH-30-1, 160, DISP_WIDTH-1, 8)*/
};

DispInfo timeDispInfo[5] = {
    // 月
    {int(FONT_WIDTH * DATE_SIZE * 2.5), DISP_HEIGHT - FONT_HEIGHT * TIME_SIZE - FONT_HEIGHT * DATE_SIZE - FONT_HEIGHT * DATE_SIZE / 2 - 1, DATE_SIZE},
    // 日
    {int(FONT_WIDTH * DATE_SIZE * 5.5), DISP_HEIGHT - FONT_HEIGHT * TIME_SIZE - FONT_HEIGHT * DATE_SIZE - FONT_HEIGHT * DATE_SIZE / 2 - 1, DATE_SIZE},
    // 時間   
    {0, DISP_HEIGHT - FONT_HEIGHT * TIME_SIZE - 1, TIME_SIZE},
    // 分
    {FONT_WIDTH * TIME_SIZE * 3, DISP_HEIGHT - FONT_HEIGHT * TIME_SIZE - 1, TIME_SIZE},
    // 秒
    {FONT_WIDTH * TIME_SIZE * 6, DISP_HEIGHT - FONT_HEIGHT * TIME_SIZE - 1, TIME_SIZE}
};
// シフトポジション表示座標
Coordinate gearCoord = {98,24};
// シフトポジション：座標と文字倍率
DispInfo gearDispInfo = {200, 0, 1};
// 温度表示：座標と文字倍率
DispInfo tempDispInfo = {DISP_WIDTH - FONT_WIDTH * TEMP_SIZE * 5 - 1, DISP_HEIGHT - FONT_HEIGHT * TEMP_SIZE - 1, TEMP_SIZE};
// 電圧表示：座標と文字倍率
DispInfo voltDispInfo = {0, 0, 3};
// RTC
RTC_DS1307 rtc;
// IOエキスパンダ
Adafruit_PCF8574 pcf;
// ADコンバータ
Adafruit_ADS1X15 ads;
// 温度計
Generic_LM75 lm75(&Wire1, moduleArr[THERM].address);
// ディスプレイ
Adafruit_ST7789 tft(&SPI, TFT_CS, TFT_DC, TFT_RST);
// ギアポジション
GearPositions gearPositions(gears, sizeof(gears)/sizeof(int), &pcf);
// ウインカー
Winkers winkers(WNK_LEFT, WNK_RIGHT, &pcf);
// スイッチ
Switch pushSw(SW, &pcf);

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
	tft.println("hello");
	delay(2000);

	// I2C設定
    Wire1.setSDA(I2C_SDA);
    Wire1.setSCL(I2C_SCL);
    Wire1.begin();// いらないけど明示しておく

    // i2cモジュールの検索
    for(byte adrs=1;adrs<127;adrs++){
        tft.setTextColor(ST77XX_WHITE);
        Wire1.beginTransmission(adrs);
        byte error = Wire1.endTransmission();
        String name = getModuleName(adrs);
        if(error == 0){
            tft.print(name + " : ");
            tft.setTextColor(ST77XX_GREEN);
            tft.println("OK");
        }
        else if(existsModule(adrs)){
            tft.print(name + " : ");
            tft.println("NG");
        }
    }
    tft.setTextColor(ST77XX_GREEN);
    tft.print("done");
    delay(5000);

    // IOエキスパンダ
    pcf.begin(moduleArr[IOEXP].address, &Wire1);
	// RTC
    rtc.begin(&Wire1);
	// 時計合わせ
    //rtc.adjust(DateTime(F(__DATE__),F(__TIME__)));

    // ADコンバータ
    ads.begin(moduleArr[ADCNV].address, &Wire1);
    
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
    // 時間
	tft.setTextColor(ST77XX_WHITE);
    tft.setFont();
	tft.setTextSize(timeDispInfo[HOUR].size);
    tft.setCursor(timeDispInfo[HOUR].x, timeDispInfo[HOUR].y);
    tft.print("  :  :");
    // 日付
	tft.setTextSize(timeDispInfo[MONTH].size);
    tft.setCursor(timeDispInfo[MONTH].x, timeDispInfo[MONTH].y);
    tft.print("  /  ");
    // 温度の値
    tft.setTextSize(tempDispInfo.size);
    tft.setCursor(tempDispInfo.x, tempDispInfo.y);
    tft.print("  . ");
    // 温度の単位
    tft.setTextSize(2);
    tft.setCursor(DISP_WIDTH - FONT_WIDTH * 2- 1, DISP_HEIGHT - FONT_HEIGHT * 2 - 1);
    tft.print('C');
    tft.setTextSize(1);
    tft.setCursor(DISP_WIDTH - FONT_WIDTH * 2- 4, DISP_HEIGHT - FONT_HEIGHT * 2 - 1 - 8);
    tft.print('o');
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
        // スイッチ状態取得
        pushSw.monitor();
		monitorTime += MONITOR_INTERVAL;
	}
    // 温度モニタリング・表示
    if(tempTime <= time){
        tempDisplay(&tft, &lm75);
        tempTime += TEMP_INTERVAL;
        uint16_t raw = ads.readADC_SingleEnded(3);
        String voltage = String((raw * 0.0001875f), 2);
        Serial.print("Voltage:");
        Serial.println(voltage);
    }

	// 時計表示処理
	if(clockTime <= time){
        // 時刻表示
        realTimeDisplay(&tft, &rtc);
		clockTime += CLOCK_INTERVAL;
	}

	// 各種表示処理
	if(displayTime <= time){
        // デバッグ用スイッチ表示
        displaySwitch(&pushSw, &tft);
        // ギア表示
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

void displaySwitch(Switch *sw, Adafruit_ST77xx *tft){
    static bool beforeSw = false;
    bool nowSw = sw->getStatus();
    tft->setTextSize(tempDispInfo.size);
    tft->setCursor(0, 0);

    // 前回と状態が異なる場合
    if(beforeSw != nowSw){
        // 表示リセット
        tft->setTextColor(ST77XX_BLACK);
        if(beforeSw){
            tft->print("ON");
        }
        else{
            tft->print("OFF");
        }
        //　前回状態を更新
        beforeSw = nowSw;
        tft->setCursor(0, 0);
    }

    if(nowSw){
        tft->setTextColor(ST77XX_RED);
        tft->print("ON");
    }
    else{
        tft->setTextColor(ST77XX_BLUE);
        tft->print("OFF");
    }
}

/**
 * 温度表示
 * 
 * @param *tft IOエキスパンダ
 * @param *lm75 温度計モジュール
 */
void tempDisplay(Adafruit_ST77xx *tft, Generic_LM75 *lm75){
    static int nowTempx10 = 0;
    // 温度取得(10倍)
    int newTempx10 = lm75->readTemperatureC() * 10;
    // 100度以上の場合は99.9度(変数では999)に修正
    if(1000 <= newTempx10){
        newTempx10 = 999;
    }
    else if(newTempx10 <= 0){
        newTempx10 = 0;
    }
    // 前回温度と同じ場合、スキップ
    if(nowTempx10 == newTempx10){
        return;
    }
    //
    tft->setTextColor(ST77XX_BLACK);
    tft->setTextSize(tempDispInfo.size);
    tft->setCursor(tempDispInfo.x, tempDispInfo.y);
    // 温度が一桁の場合、10の位にスペース
    if(0 <= nowTempx10 && nowTempx10 < 100){
        tft->print(' ');
    }
    tft->print(int(nowTempx10/10));
    tft->print('.');
    tft->print(int(nowTempx10)%10);

    tft->setTextColor(ST77XX_WHITE);
    tft->setCursor(tempDispInfo.x, tempDispInfo.y);
    // 温度が一桁の場合、10の位にスペース
    if(0 <= newTempx10 && newTempx10 < 100){
        tft->print(' ');
    }
    tft->print(int(newTempx10/10));
    tft->print('.');
    tft->print(int(newTempx10)%10);
    nowTempx10 = newTempx10;
}

/**
 * 経過時間表示処理
 *
 * @param totalSec long型 経過時間(秒)
 * @param tft Adafruit_ST7735クラス ディスプレイ設定
 * @param dispInfo 表示文字情報構造体 文字の座標と大きさ
 */
void realTimeDisplay(Adafruit_ST77xx *tft, RTC_DS1307 *rtc_ds1307){
    // 時刻用変数
    int newTimeItems[5] = {99,99,99,99,99};
    static boolean firstFlag = true;
    // 現在時刻取得
    DateTime now = rtc_ds1307->now();
    // 月・日・時間・分取得
    newTimeItems[MONTH]  = now.month();
    newTimeItems[DAY]    = now.day();
    newTimeItems[HOUR]   = now.hour();
    newTimeItems[MINUTE] = now.minute();
    newTimeItems[SECOND] = now.second();
    // 時刻データでループ
    for(int i=0; i<5; i++){
        if(firstFlag == true || timeItems[i] != newTimeItems[i]){

            tft->setTextSize(timeDispInfo[i].size);
            // ----------削除処理----------
            // 背景色・カーソル設定・値表示
            tft->setTextColor(ST77XX_BLACK);
            tft->setCursor(timeDispInfo[i].x, timeDispInfo[i].y);
            if(timeItems[i] < 10){
                tft->print('0');
            }
            tft->print(timeItems[i]);
            // ----------表示処理----------
            // 背景色・カーソル設定・値表示
            tft->setTextColor(ST77XX_WHITE);
            tft->setCursor(timeDispInfo[i].x, timeDispInfo[i].y);
            if(newTimeItems[i] < 10){
                tft->print('0');
            }
            tft->print(newTimeItems[i]);

            // 保持値更新
            timeItems[i] = newTimeItems[i];
        }
    }
    firstFlag = false;
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
