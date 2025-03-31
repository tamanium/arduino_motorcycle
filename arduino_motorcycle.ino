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

// --------------------定数--------------------
const int CLOCK_INTERVAL   = 200;	//ms
const int MONITOR_INTERVAL = 5;		//ms
const int DISPLAY_INTERVAL = 30;	//ms
const int TEMP_INTERVAL    = 1000;	//ms
const int BUZZER_DURATION  = 100;	//ms
const int WINKER_DURATION  = 380;	//ms

// フォントの寸法
const int DATE_SIZE   = 2;
const int TIME_SIZE   = 3;
const int TEMP_SIZE   = 3;
const int GEAR_SIZE   = 24;

// --------------------変数--------------------
unsigned long displayTime = 0;	// 表示処理
unsigned long monitorTime = 0;	// 各種読み取り
unsigned long clockTime = 0;	// 時計表示
unsigned long tempTime = 0;		// 温度測定にて使用
unsigned long bzzTime = 0;

unsigned long debugWinkerTime  = 0;	//疑似ウインカー

// 保持用char配列
uint16_t timeItems[4] = {0, 0, 0, 0};
// シフトポジション配列
int gears[] = {PIN.IOEXP.POS.nwt,
				PIN.IOEXP.POS.low,
				PIN.IOEXP.POS.sec,
				PIN.IOEXP.POS.thi,
				PIN.IOEXP.POS.top};
// 明るさレベル
byte brightLevel[] = {50, 100, 250};
char nowTime[] = " 0:00";

// 三角形描画用座標
struct TriangleLocation {
	int x1, y1, x2, y2, x3, y3;
};
// 座標
struct Location {
	int x = 0;
	int y = 0;
};

// 表示情報
struct DispInfo{
	int x = 0;
	int y = 0;
	int size = 0;
};

uint16_t OKNGColor(bool b){
	if(b){
		return ST77XX_GREEN;
	}
	else{
		return ST77XX_RED;
	}
}

/**
 * i2cモジュールのアドレスから名前を取得 
 *
 * @param adrs i2cモジュールのアドレス
 * @param arr モジュール配列
 * @param size i2cモジュール数
 * @return i2cモジュールの名前 hitしなければアドレス
 */
String getModuleName(byte adrs, Module* arr, int size){
	for(int i=0; i<size; i++){
		if(arr[i].address == adrs){
			return arr[i].name;
		}
	}
	return String(adrs, HEX)+"   ";
}

/**
 * i2cモジュールのアドレスから接続中モジュールの有無を取得
 *
 * @param adrs i2cモジュールのアドレス
 * @param arr モジュール配列
 * @param size i2cモジュール数
 * @return モジュールが接続されていればtrue
 */
bool existsModule(byte adrs, Module* arr, int size){
	for(int i=0; i<size; i++){
		if(arr[i].address == adrs){
			return true;
		}
	}
	return false;
}

// --------------------インスタンス--------------------
// 表示座標
TriangleLocation triCoords[2] = {
	{30, 34, 30, 160+14, 0, 80+24},
	{fromRight(30), 34, fromRight(30), 160+14, fromRight(0), 80+24}
};

struct PrintProperty {
	int x = 0;
	int y = 0;
	int size = 1;
	uint16_t color = ST77XX_WHITE;
	boolean disable = false;
};
// 表示設定
struct PrintProperties{
	PrintProperty Month;	// 月
	PrintProperty Day;		// 日
	PrintProperty Hour;		// 時
	PrintProperty Min;		// 分
	PrintProperty Sec;		// 秒
	PrintProperty Temp;		// 温度
	PrintProperty Gear;		// ギア
	PrintProperty Speed;	// 速度
	PrintProperty SpUnit;	// 速度単位
	PrintProperty InitMsg;	// 初期表示：「hello」
	PrintProperty InitInfo;	// 初期表示：モジュール検索
};
// 表示設定宣言
PrintProperties PRINT_PROP;
// シフトポジション表示座標
Location gearCoord = {98,24};
// シフトポジション：座標と文字倍率
DispInfo gearDispInfo = {200, 0, 1};
// 温度表示：座標と文字倍率
DispInfo tempDispInfo = {fromRight(FONT.WIDTH * TEMP_SIZE * 5), fromBottom(FONT.HEIGHT * TEMP_SIZE), TEMP_SIZE};
// 電圧表示：座標と文字倍率
DispInfo voltDispInfo = {0, 0, 3};

RTC_DS1307 rtc;			// RTC
Adafruit_PCF8574 pcf;	// IOエキスパンダ
Adafruit_ADS1X15 ads;	// ADコンバータ
// 温度計
Generic_LM75 lm75(&Wire1, MODULES.therm.address);
// ディスプレイ
Adafruit_ST7789 tft(&SPI, PIN.SPI.cs, PIN.SPI.dc, PIN.SPI.rst);
// ギアポジション
GearPositions gearPositions(gears, sizeof(gears)/sizeof(int), &pcf);
// ウインカー
Winkers winkers(PIN.IOEXP.WNK.left, PIN.IOEXP.WNK.right, &pcf);
// スイッチ
Switch pushSw(PIN.IOEXP.sw, &pcf);

/**
 * ディスプレイ表示設定
 * 
 * @param p 表示情報
 * @param isTrans 文字出力時に背景を透過させるか
 */
void setProp(PrintProperty* p, bool isTrans=false){
	tft.setCursor(p->x, p->y);
	tft.setTextSize(p->size);
	if(isTrans){
		tft.setTextColor(p->color);
	}
	else{
		tft.setTextColor(p->color, ST77XX_BLACK);
	}
}



// ------------------------------初期設定------------------------------
void setup(void) {
	// デバッグ用シリアル設定
	Serial.begin(9600);

	// I2C設定
	Wire1.setSDA(PIN.I2C.sda);
	Wire1.setSCL(PIN.I2C.scl);
	Wire1.begin();// いらないけど明示しておく

	//SPI1設定
	SPI.setTX(PIN.SPI.mosi);
	SPI.setSCK(PIN.SPI.sclk);

	// ディスプレイ明るさ設定(0-255)
	analogWrite(PIN.SPI.bl, brightLevel[0]);

	// ディスプレイ初期化・画面向き・画面リセット
	tft.init(OLED.HEIGHT, OLED.HEIGHT);
	tft.setRotation(3);
	tft.fillScreen(ST77XX_BLACK);

	// 月
	PRINT_PROP.Month = {
		int(FONT.WIDTH * DATE_SIZE * 2.5),
		fromBottom(FONT.HEIGHT * TIME_SIZE + FONT.HEIGHT * DATE_SIZE + FONT.HEIGHT * DATE_SIZE / 2),
		DATE_SIZE
	};
	// 日
	PRINT_PROP.Day = {
		int(FONT.WIDTH * DATE_SIZE * 5.5),
		PRINT_PROP.Month.y,
		DATE_SIZE
	};
	// 時間
	PRINT_PROP.Hour = {
		0,
		fromBottom(FONT.HEIGHT * TIME_SIZE),
		TIME_SIZE
	};
	// 分
	PRINT_PROP.Min = {
		FONT.WIDTH * TIME_SIZE * 3,
		PRINT_PROP.Hour.y,
		TIME_SIZE
	};
	// 秒
	PRINT_PROP.Sec = {
		FONT.WIDTH * TIME_SIZE * 6,
		PRINT_PROP.Hour.y,
		TIME_SIZE
	};
	// 温度
	PRINT_PROP.Temp = {
		fromRight(FONT.WIDTH * TEMP_SIZE * 5), 
		fromBottom(FONT.HEIGHT * TEMP_SIZE), 
		TEMP_SIZE
	};
	// ギア
	PRINT_PROP.Gear = {
		200, 24, 8
	};
	// 速度
	PRINT_PROP.Speed = {
		50, 80, 12
	};
	// 速度単位
	PRINT_PROP.SpUnit = {
		200, 145, 3
	};
	// 初期表示メッセージ
	PRINT_PROP.InitMsg = {
		0, 0, 3, ST77XX_GREEN
	};
	// 初期情報表示
	PRINT_PROP.InitInfo = {
		0,
		FONT.HEIGHT * PRINT_PROP.InitMsg.size,
		2
	};


	// 初期表示
	setProp(&PRINT_PROP.InitMsg);
	tft.println("hello");
	delay(2000);

	// モジュールの配列を作成
	Module moduleArr[] = {
		MODULES.ioExp,
		MODULES.therm,
		MODULES.adCnv,
		MODULES.rtcMm,
		MODULES.rtcIC
	};
	// i2cモジュールの検索
	setProp(&PRINT_PROP.InitInfo);
	for(byte adrs=1;adrs<127;adrs++){
		tft.setTextColor(PRINT_PROP.InitInfo.color);
		Wire1.beginTransmission(adrs);
		byte error = Wire1.endTransmission();
		// 登録済みモジュールの場合
		if(existsModule(adrs, moduleArr, MODULES.size)){
			//module->disable = true;
			String name = getModuleName(adrs, moduleArr, MODULES.size);
			tft.print(name + " : ");
			tft.setTextColor(OKNGColor(error == 0));
			tft.println(OKNGMsg(error == 0));
		}
	}
	tft.setTextColor(ST77XX_GREEN);
	tft.print("done");
	delay(5000);

	// IOエキスパンダ
	pcf.begin(MODULES.ioExp.address, &Wire1);
	// RTC
	rtc.begin(&Wire1);
	// 時計合わせ
	//rtc.adjust(DateTime(F(__DATE__),F(__TIME__)));

	// ADコンバータ
	ads.begin(MODULES.adCnv.address, &Wire1);
	// 疑似ウインカーリレー
	pinMode(PIN.relay, OUTPUT);
	// ウインカー音
	pinMode(PIN.buzzer, OUTPUT);
	// 画面リセット
	tft.fillScreen(ST77XX_BLACK);

	// ギアポジション表示開始その1
	//tft.setCursor(184, 8*8);
	//tft.setTextSize(3);
	//tft.print("gear");
	
	// ギアポジション表示開始
	setProp(&PRINT_PROP.Gear);
	tft.print('-');
	// 速度
	setProp(&PRINT_PROP.Speed);
	tft.print("00");
	// 速度単位
	setProp(&PRINT_PROP.SpUnit);
	tft.print("km/h");
	// 時間
	setProp(&PRINT_PROP.Hour);
	tft.print("  :  :");
	// 日付
	setProp(&PRINT_PROP.Month);
	tft.print("  /");
	// 温度の値
	setProp(&PRINT_PROP.Temp);
	tft.print("  .");
	// 温度の単位
	tft.setTextColor(ST77XX_WHITE);
	tft.setTextSize(2);
	tft.setCursor(fromRight(FONT.WIDTH * 2), fromBottom(FONT.HEIGHT * 2));
	tft.print('C');
	tft.setTextSize(1);
	tft.setCursor(fromRight(FONT.WIDTH * 2) - 3, fromBottom(FONT.HEIGHT * 2) - 8);
	tft.print('o');

}

// ------------------------------ループ------------------------------
void loop() {
	// 経過時間(ms)取得
	unsigned long time = millis();
	
	// 疑似ウインカーリレー
	if(debugWinkerTime <= time){
		//if(digitalRead(DMY_RELAY) == HIGH){
		// 	digitalWrite(DMY_RELAY, LOW);
		//}
		//else{
		//	digitalWrite(DMY_RELAY, HIGH);
		//}
		// 出力反転
		//digitalWrite(DMY_RELAY, !digitalRead(DMY_RELAY));
		digitalWrite(PIN.relay, !digitalRead(PIN.relay));
		
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
	// 温度電圧モニタリング・表示
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
			digitalWrite(PIN.buzzer, HIGH);
			// 時間設定
			bzzTime = time + BUZZER_DURATION;
		}
		displayTime += DISPLAY_INTERVAL;
	}
	//ブザーOFF処理
	if(bzzTime != 0 && bzzTime <= time){
		digitalWrite(PIN.buzzer, LOW);
		bzzTime = 0;
	}
}

// -------------------------------------------------------------------
// ------------------------------メソッド------------------------------
// -------------------------------------------------------------------

/**
 * ギアポジションの表示処理
 * @param dispChar char型 表示文字列
 * @param tft Adafruit_ST7735クラス ディスプレイ設定
 */
void gearDisplay(char newGear, Adafruit_ST77xx *tft){
	// バッファ文字列
	static char beforeGear = '-';
	// バッファと引数が同じ場合スキップ
	if(beforeGear == newGear){
		return;
	}
	// ディスプレイ設定
	setProp(&PRINT_PROP.Gear);
	// ギア表示更新
	tft->print(newGear);
	// バッファ文字列を上書き
	beforeGear = newGear;
}

/**
 * ウインカー表示処理
 * @param winkers Winkers型 ウインカークラス
 * @param tft Adafruit_ST7735クラス ディスプレイ設定
 * @return isSwitchStatus bool型 左右いずれかが点灯状態が切り替わった場合true
 */
bool winkersDisplay(Winkers &winkers, Adafruit_ST77xx *tft){
	// バッファ状態
	static bool buffer[2] = {OFF, OFF};
	// 返却用フラグ
	bool isSwitched = false;

	for(int side=LEFT; side<=RIGHT; side++){
		// 左ウインカー状態を判定
		if(buffer[side] == winkers.getStatus(side)){
			continue;
		}
		// バッファ上書き
		buffer[side] = winkers.getStatus(side);
		// ディスプレイ表示処理
		displayTriangle(triCoords[side], buffer[side], tft);
		// フラグ立てる
		isSwitched = true;
	}
	return isSwitched;
}
/**
 * 三角形表示処理
 * @param coord TriangleLocation型 
 * @param status bool型 true...点灯, false...消灯
 * @param tft Adafruit_ST7735クラス ディスプレイ設定
 */
void displayTriangle(TriangleLocation coord, bool status, Adafruit_ST77xx *tft){
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
	static bool beforeLong = false;
	static int brightLvMax = sizeof(brightLevel) / sizeof(byte) - 1;
	static int nowBrightLv = 0;
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
	// キーダウンの場合
	if(nowSw){
		if(beforeSw != nowSw){
			tft->setTextColor(ST77XX_RED, ST77XX_BLACK);
			tft->print("ON ");
			beforeSw = ON;
		}
		// 長押し
		else if(beforeLong != sw->isLongPress()){
			tft->setTextColor(ST77XX_RED, ST77XX_BLACK);
			tft->setCursor(200,0);
			tft->print("long");
			beforeLong = sw->isLongPress();
		}
	}
	// キーアップの場合
	else{
		tft->setTextColor(ST77XX_BLUE);
		tft->print("OFF ");
		// 長押し
		if(beforeLong){
			tft->setTextColor(ST77XX_WHITE, ST77XX_BLACK);
			tft->setCursor(200,0);
			tft->print("    ");
			beforeLong = false;
		}
		// プッシュ
		else if(sw->isPush()){
			tft->setTextColor(ST77XX_BLUE, ST77XX_BLACK);
			tft->setCursor(100,0);
			nowBrightLv++;
			if(brightLvMax < nowBrightLv){
				nowBrightLv = 0;
			}
			//analogWrite(TFT_BL,brightLevel[nowBrightLv]);
			analogWrite(PIN.SPI.bl, brightLevel[nowBrightLv]);
			tft->print(nowBrightLv);
			beforeSw = OFF;
		}
	}
}

/**
 * 温度表示
 * 
 * @param *tft IOエキスパンダ
 * @param *lm75 温度計モジュール
 */
void tempDisplay(Adafruit_ST77xx *tft, Generic_LM75 *lm75){
	static int beforeTempx10 = 0;
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
	if(beforeTempx10 == newTempx10){
		return;
	}
	setProp(&PRINT_PROP.Temp);
	// 温度が一桁以下の場合、十の位にスペース
	if(10 <= newTempx10 && newTempx10 < 100){
		tft->print(' ');
	}
	tft->print(int(newTempx10/10));
	tft->print('.');
	tft->print(int(newTempx10)%10);
	// 保持変数を更新
	beforeTempx10 = newTempx10;
}

/**
 * 現在時刻表示処理
 *
 * @param totalSec long型 経過時間(秒)
 * @param tft Adafruit_ST7735クラス ディスプレイ設定
 * @param dispInfo 表示文字情報構造体 文字の座標と大きさ
 */
void realTimeDisplay(Adafruit_ST77xx *tft, RTC_DS1307 *rtc_ds1307){
	// 前回日時
	static int beforeTimeItems[5] = {99,99,99,99,99};
	// 表示情報配列
	static PrintProperty* printPropArr[5] = {
		&PRINT_PROP.Month,
		&PRINT_PROP.Day,
		&PRINT_PROP.Hour,
		&PRINT_PROP.Min,
		&PRINT_PROP.Sec,
	};
	static int itemLen = 5;
	
	// 時刻用変数
	int newTimeItems[5] = {99,99,99,99,99};

	// 現在時刻取得
	DateTime now = rtc_ds1307->now();
	// 各配列に格納
	int j = 0;
	newTimeItems[j++] = now.month();
	newTimeItems[j++] = now.day();
	newTimeItems[j++] = now.hour();
	newTimeItems[j++] = now.minute();
	newTimeItems[j++] = now.second();

	// 出力処理
	for(int i=0;i<itemLen;i++){
		// 前回日時と値が同じ場合スキップ
		if(beforeTimeItems[i] == newTimeItems[i]){
			continue;
		}
		// 表示設定を反映
		setProp(printPropArr[i]);
		// 値が1桁の場合は0埋め
		if(newTimeItems[i] < 10){
			tft->print('0');
		}
		// 値を出力
		tft->print(newTimeItems[i]);
		// 前回日時を更新
		beforeTimeItems[i] = newTimeItems[i];
	}
}
