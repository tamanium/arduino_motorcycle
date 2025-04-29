
// --------------------ライブラリ--------------------
#include <Adafruit_GFX.h>               // 画面出力
#include <SPI.h>                        // SPI通信
#include <RTClib.h>                     // 時計機能
#include <Adafruit_PCF8574.h>           // IOエキスパンダ
#include <Temperature_LM75_Derived.h>   // 温度計
#include <Adafruit_AHTX0.h>             // 温湿度計
#include <Adafruit_ADS1X15.h>           // ADコンバータ
#include <Adafruit_NeoPixel.h>          // オンボLED

// --------------------自作クラス・ピン定義--------------------
#include "Define.h"			// 値定義
#include "GearPositions.h"	// ギアポジションクラス
#include "Winker.h"			// ウインカークラス
#include "Switch.h"			// スイッチクラス
#include "MyLovyanGFX.h"	// ディスプレイ設定

// --------------------定数--------------------
const int MONITOR_INTERVAL = 5;		//ms
const int DISPLAY_INTERVAL = 30;	//ms
const int TEMP_INTERVAL    = 2000;	//ms
const int TIME_INTERVAL    = 30;	//ms
const int BUZZER_DURATION  = 50;	//ms
const int WINKER_DURATION  = 380;	//ms

// フォントの寸法
const int GEAR_SIZE   = 24;
// 中心座標
const int centerX = OLED.WIDTH>>1;
const int centerY = OLED.HEIGHT>>1;

// --------------------変数--------------------
unsigned long displayTime = 0; // 表示処理
unsigned long monitorTime = 0; // 各種読み取り
unsigned long tempTime = 0;    // 温度測定にて使用
unsigned long timeTime = 0;    // 時刻測定
unsigned long bzzTime = 0;     // ブザー
unsigned long debugWinkerTime  = 0;	//疑似ウインカー

// シフトポジション配列
int gears[] = {PIN.IOEXP.POS.nwt,
				PIN.IOEXP.POS.low,
				PIN.IOEXP.POS.sec,
				PIN.IOEXP.POS.thi,
				PIN.IOEXP.POS.top};
// 明るさレベル
byte brightLevel[] = {0x20,
                      0x40,
                      0x60,
                      0x80,
                      0xA0,
                      0xC0,
                      0xE0};

// ディスプレイ
LGFX display;

struct arcInfo{
	LGFX_Sprite sprite; // スプライト
	int x;              // 円弧中心x座標
	int y;              // 円弧中心y座標
	int r;              // 内径
	int d;              // 厚さ
	int angle0;         // 角度0
	int angle1;         // 角度1
	uint16_t colorON;   // 色
	uint16_t colorBG = TFT_BLUE; // 透過色
	/**
	 *  コンストラクタ
	 */
	arcInfo(LGFX *display) : sprite(display){}
	/**
	 * 表示
	 */
	void displayArc(int stdX, int stdY, bool onOff){
		sprite.fillScreen(TFT_BLUE);
		// on,offで色変更
		uint16_t color = onOff ? colorON : TFT_BLACK;
		// 弧の中心軸
		sprite.setPivot(x,y);
		// 弧描画
		sprite.fillArc(x,y,r+d,r,angle0,angle1,color);
		// 出力
		sprite.pushRotateZoom(stdX,stdY,0,1,1,colorBG);
	}
};
arcInfo arcM(&display);
arcInfo arcL(&display);
arcInfo arcR(&display);

/**
 * i2cモジュールのアドレスから接続中モジュールの有無を取得
 *
 * @param adrs i2cモジュールのアドレス
 * @param arr モジュール配列
 * @param size i2cモジュール数
 * @return モジュールが接続されていれば配列のインデックスを
 */
int existsModule(byte adrs, Module* arr, int size){
	for(int i=0; i<size; i++){
		if(arr[i].address == adrs){
			return i;
		}
	}
	return -1;
}

// --------------------インスタンス--------------------
// 表示設定まとめ
struct PrintProperties{
	PrintProperty Month;    // 月
	PrintProperty Day;      // 日
	PrintProperty Hour;     // 時
	PrintProperty Min;      // 分
	PrintProperty Sec;      // 秒
	PrintProperty Temp;     // 温度
	PrintProperty Gear;     // ギア
	PrintProperty Speed;    // 速度
	PrintProperty SpUnit;   // 速度単位
	PrintProperty InitMsg;  // 初期表示：「hello」
} PROP;

// オンボLED
Adafruit_NeoPixel pixels(1, PIN.LED);
// RTC
RTC_DS1307 rtc;
// IOエキスパンダ
Adafruit_PCF8574 pcf;
// ADコンバータ
Adafruit_ADS1X15 ads;
// 温度計
Generic_LM75 lm75(&Wire1, MODULES.thrm1.address);
Adafruit_AHTX0 aht;
// ギアポジション
GearPositions gearPositions(gears, sizeof(gears)/sizeof(int), &pcf);
// ウインカー
Winkers winkers(PIN.IOEXP.WNK.left, PIN.IOEXP.WNK.right, &pcf);
// スイッチ
Switch pushSw(PIN.IOEXP.sw, &pcf);

/**
 * ディスプレイ表示設定
 *
 * @param p 表示設定
 * @param isTrans 透過させるか
 */
void setDisplay(PrintProperty* p, bool isTrans=false){
	display.setCursor(p->x,p->y);   //描画位置
	display.setTextSize(p->size);   //テキスト倍率
	if(isTrans){
		display.setTextColor(TFT_WHITE);
	}
	else{
		display.setTextColor(TFT_WHITE, TFT_BLACK); //色
	}
	display.setFont(p->font);
}

/**
 * フォントの縦横を設定
 *
 * @param p 表示設定
 * @param size フォント倍率
 */
void setPropWH(PrintProperty* p, String str = "0"){
	display.setFont(p->font);
	display.setTextSize(p->size);
	p->width = display.textWidth(str);
	p->height = display.fontHeight();
	p->fontSize.WIDTH = display.textWidth(str);
	p->fontSize.HEIGHT = display.fontHeight();
}

// ------------------------------初期設定------------------------------
void setup(void) {
	// デバッグ用シリアル設定
	Serial.begin(9600);
	// I2C設定
	Wire1.setSDA(PIN.I2C.sda);
	Wire1.setSCL(PIN.I2C.scl);
	Wire1.begin();// いらないけど明示しておく

	int offsetY = 50;
	// 時間
	PROP.Hour = {
		0,
		0,
		1,
		&fonts::Font4
	};
	setPropWH(&PROP.Hour,"00:");

	// 分
	PROP.Min = {
		PROP.Hour.width,
		PROP.Hour.y,
		PROP.Hour.size,
		PROP.Hour.font
	};
	setPropWH(&PROP.Min,"00:");

	// 秒
	PROP.Sec = {
		PROP.Min.x + PROP.Min.width,
		PROP.Hour.y,
		PROP.Hour.size,
		PROP.Hour.font
	};
	setPropWH(&PROP.Sec,"00");

	// 月
	PROP.Month = {
		PROP.Hour.x,
		PROP.Hour.y + PROP.Hour.height + 4,
		PROP.Hour.size,
		PROP.Hour.font
	};
	setPropWH(&PROP.Month,"00/");

	// 日
	PROP.Day = {
		PROP.Month.x + PROP.Month.width,
		PROP.Month.y,
		PROP.Month.size,
		PROP.Month.font
	};
	setPropWH(&PROP.Day, "00");

	// 温度
	PROP.Temp = {
		0, 
		0,
		PROP.Hour.size,
		PROP.Hour.font
	};
	setPropWH(&PROP.Temp,"00.0 c");
 	PROP.Temp.x=fromRight(PROP.Temp.width);

	// ギア
	PROP.Gear = {
		0,
		140+offsetY,
		1,
		&fonts::lgfxJapanGothic_40
	};
	setPropWH(&PROP.Gear);
	PROP.Gear.x = centerHorizontal(PROP.Gear.width);

	// 速度
	PROP.Speed = {
		0,
		30+offsetY,
		1,
		&fonts::Font8
	};
	setPropWH(&PROP.Speed, "00");
	PROP.Speed.x = centerHorizontal(PROP.Speed.width);

	// 速度単位
	PROP.SpUnit = {
		0,
		110+offsetY,
		1,
		&fonts::Font4
	};
	setPropWH(&PROP.SpUnit,"km/h");
	PROP.SpUnit.x = centerHorizontal(PROP.SpUnit.width);

	// 初期表示メッセージ
	PROP.InitMsg = {
		0, 0, 2
	};
	// 初期情報表示
	// ディスプレイの初期化
	display.init();
	// 明るさ
	display.setBrightness(brightLevel[0]);

	// ブザー連動LED設定
	pixels.begin();
	pixels.setPixelColor(0, pixels.Color(1,1,0));
	pixels.show();

	// I2C通信スキャン
	scanModules();
	
	// 各モジュール動作開始
	pcf.begin(MODULES.ioExp.address, &Wire1); // IOエキスパンダ
	gearPositions.begin();                    // シフトインジケータ
	pushSw.begin();                           // スイッチ
	rtc.begin(&Wire1);                        // RTC
	winkers.begin();                          // ウインカー
	ads.begin(MODULES.adCnv.address, &Wire1); // ADコンバータ
	pinMode(PIN.buzzer, OUTPUT);              // ウインカー音
	aht.begin(&Wire1,0,MODULES.thrm0.address);// 温度計
	digitalWrite(PIN.buzzer, LOW);
	//rtc.adjust(DateTime(F(__DATE__),F(__TIME__))); // 時計合わせ

	// 画面リセット
	display.fillScreen(TFT_BLACK);
	// ギアポジション表示開始
	setDisplay(&PROP.Gear);
	display.print('-');
	// 速度
	setDisplay(&PROP.Speed);
	display.print("00");
	// 速度単位
	setDisplay(&PROP.SpUnit);
	display.print("km/h");
	// 時間
	setDisplay(&PROP.Hour);
	display.print("00:00:00");
	// 日付
	setDisplay(&PROP.Month);
	display.print("00/00");
	// 温度の値
	setDisplay(&PROP.Temp);
	display.print("00.0 c");
	display.fillCircle(306,6,3,TFT_WHITE);
	display.fillCircle(306,6,1,TFT_BLACK);

	// スプライト設定
	// 横縦
	int w = (PROP.Speed.y + 60 - offsetY) * 2;
	int h = w;
	// 弧の幅
	arcM.d = 10;
	arcL.d = 10;
	arcR.d = 10;
	// 弧の内外半径
	int rOUT = (w-1)>>1;
	arcM.r = rOUT - arcM.d;
	arcL.r = rOUT - arcL.d + 20;
	arcR.r = rOUT - arcR.d + 20;
	// 弧の中心座標
	arcM.x = w>>1;
	arcM.y = h>>1;
	arcL.x = arcL.r+arcL.d;
	arcL.y = arcM.y;
	arcR.x = 0;
	arcR.y = arcM.y;
	// 大きさ
	arcM.sprite.createSprite(w,h);
	arcL.sprite.createSprite(arcL.r+arcL.d+1,h);
	arcR.sprite.createSprite(arcR.r+arcR.d+1,h);
	// 角度
	arcM.angle0 = 120;
	arcM.angle1 = 60;
	arcL.angle0 = 90 +45;
	arcL.angle1 = 270-37;
	arcR.angle0 = 270+37;
	arcR.angle1 = 90 -45;
	// 色
	arcM.colorON=TFT_GREEN;
	arcL.colorON=TFT_YELLOW;
	arcR.colorON=TFT_YELLOW;
	// 出力
	arcL.displayArc(centerX,centerY+10,ON);
	arcR.displayArc(centerX,centerY+10,ON);
	arcM.displayArc(centerX,centerY+10,ON);
	//display.drawFastHLine(0,centerY+rOUT,320,TFT_RED);
	//display.drawFastHLine(0,centerY-rOUT+8,320,TFT_RED);
}

// ------------------------------ループ------------------------------
void loop() {

	// 経過時間(ms)取得
	unsigned long time = millis();

	// 疑似ウインカーリレー
	if(debugWinkerTime <= time){
		// 出力反転
		digitalWrite(PIN.relay, !digitalRead(PIN.relay));
		debugWinkerTime = debugWinkerTime + WINKER_DURATION;
	}
	
	// 各種モニタリング・更新
	if(monitorTime <= time){
		// 現在のギアポジを取得
		gearPositions.updateStatus();
		// 現在のウインカー状態を取得
		winkers.updateStatus();
		// スイッチ状態取得
		pushSw.updateStatus();
		monitorTime = time + MONITOR_INTERVAL;
	}
	// 温度電圧モニタリング・表示
	if(tempTime <= time){
		sensors_event_t humidity, temp;
		aht.getEvent(&humidity,&temp);
		Serial.print("Temperature:");
		Serial.println(temp.temperature);
		Serial.print("Humidity:");
		Serial.println(humidity.relative_humidity);
		tempDisplay();
		uint16_t raw = ads.readADC_SingleEnded(3);
		String voltage = String((raw * 0.0001875f), 2);
		Serial.print("Voltage:");
		Serial.println(voltage);
		tempTime = time + TEMP_INTERVAL;
	}

	// 時刻表示
	if(timeTime <= time){
		realTimeDisplay();
		timeTime = time + TIME_INTERVAL;
	}
	
	// 各種表示処理
	if(displayTime <= time){
		// デバッグ用スイッチ表示
		displaySwitch(&pushSw);
		// ギア表示
		gearDisplay(gearPositions.getGear());
		// ウインカー点灯状態が切り替わった場合
		if(winkersDisplay() == true && bzzTime == 0 ){
			// ブザーON
			digitalWrite(PIN.buzzer, HIGH);
			//digitalWrite(PIN.buzzer, LOW);
			//analogWrite(PIN.buzzer, 153);
			pixels.setPixelColor(0, pixels.Color(1,1,0));
			pixels.show();
			// 時間設定
			bzzTime = time + BUZZER_DURATION;
		}
		displayTime = time + DISPLAY_INTERVAL;
	}
	//ブザーOFF処理
	if(bzzTime != 0 && bzzTime <= time){
		digitalWrite(PIN.buzzer, LOW);
		//digitalWrite(PIN.buzzer, HIGH);
		//analogWrite(PIN.buzzer, 0);
		pixels.clear();
		pixels.show();
		bzzTime = 0;
	}
}

// -------------------------------------------------------------------
// ------------------------------メソッド------------------------------
// -------------------------------------------------------------------

/**
 * モジュール走査
 */
void scanModules(){
	// モジュールの配列
	Module moduleArr[] = {
		MODULES.ioExp,
		MODULES.thrm0,
		MODULES.thrm1,
		MODULES.adCnv,
		MODULES.rtcMm,
		MODULES.rtcIC
	};

	// 初期表示メッセージ
	setDisplay(&PROP.InitMsg);
	display.println("Hello");
	display.println("");
	delay(500);
	// スキャン処理
	display.println("I2C Module Scanning...");
	for(byte adrs=1; adrs<0x7F; adrs++){
		int moduleIndex = existsModule(adrs, moduleArr, MODULES.size);
		if(moduleIndex == -1){
			continue;
		}
		Wire1.beginTransmission(adrs);
		byte error = Wire1.endTransmission();
		display.setTextColor(TFT_WHITE, TFT_BLACK);
		String nameColon = moduleArr[moduleIndex].name + ":";
		display.print(nameColon);
		uint16_t color = (error==0) ? TFT_GREEN : TFT_RED;
		display.setTextColor(color, TFT_BLACK);
		String msg = (error==0) ? "OK" : "NG";
		display.println(msg);
	}
	display.setTextColor(TFT_WHITE);
	display.println("");
	display.println("done");
	delay(2000);
}

/**
 * ギアポジションの表示処理
 * @param dispChar char型 表示文字列
 * @param tft Adafruit_ST7735クラス ディスプレイ設定
 */
void gearDisplay(char newGear){
	// バッファ文字列
	static char beforeGear = '-';
	// バッファと引数が同じ場合スキップ
	if(beforeGear == newGear){
		return;
	}
	// ディスプレイ設定
	setDisplay(&PROP.Gear);
	// ギア表示更新
	display.print(newGear);
	// バッファ文字列を上書き
	beforeGear = newGear;
}


/**
 * ウインカー表示処理
 * @param winkers Winkers型 ウインカークラス
 * @return isSwitchStatus bool型 左右いずれかが点灯状態が切り替わった場合true
 */
bool winkersDisplay(){
	// バッファ状態
	static bool buffer[2] = {OFF, OFF};
	// 返却用フラグ
	bool isSwitched = false;

	for(int side=LEFT; side<=RIGHT; side++){
		// 左右ウインカー状態を判定
		if(buffer[side] == winkers.getStatus(side)){
			continue;
		}
		// バッファ上書き
		buffer[side] = winkers.getStatus(side);
		// ディスプレイ表示処理
		displayWinker(side,buffer[side]);
		//displayTriangle(triCoords[side], buffer[side]);
		// フラグ立てる
		isSwitched = true;
	}
	return isSwitched;
	return true;
}

/**
 * ウインカー表示処理
 * @param onOff bool型 true...点灯, false...消灯
 * @param onOff bool型 true...点灯, false...消灯
 */
void displayWinker(int side, bool onOff){
	if(side==RIGHT){
		arcL.displayArc(centerX,centerY+10,onOff);
	}
	else{
		arcR.displayArc(centerX,centerY+10,onOff);
	}
}

/**
 * スイッチ動作表示
 * @param Switch スイッチクラス
 */
void displaySwitch(Switch *sw){
	static bool beforeSw = false;
	static bool beforeLong = false;
	static int brightLvMax = sizeof(brightLevel) / sizeof(byte) - 1;
	static int nowBrightLv = 0;
	bool nowSw = sw->getStatus();
	display.setTextSize(3);
	display.setCursor(0, 0);

	// キーダウンの場合
	if(nowSw){
		display.setTextColor(TFT_RED, TFT_BLACK);
		if(beforeSw != nowSw){
			display.print("ON ");
			beforeSw = ON;
		}
		// 長押し
		else if(beforeLong != sw->isLongPress()){
			display.setCursor(200,0);
			display.print("long");
			beforeLong = sw->isLongPress();
		}
	}
	// キーアップの場合
	else{
		if(beforeSw != nowSw){
			display.setTextColor(TFT_BLUE, TFT_BLACK);
			display.print("OFF");
			beforeSw = OFF;
		}
		// 長押し
		if(beforeLong){
			display.setTextColor(TFT_WHITE, TFT_BLACK);
			display.setCursor(200,0);
			display.print("    ");
			beforeLong = false;
		}
		// プッシュ
		else if(sw->isPush()){
			display.setTextColor(TFT_BLUE, TFT_BLACK);
			display.setCursor(100,0);
			nowBrightLv = (nowBrightLv+1) % brightLvMax;
			display.setBrightness(brightLevel[nowBrightLv]);
			display.print(nowBrightLv);
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
void tempDisplay(){
	static int beforeTempx10 = 0;
	// 温度取得(10倍)
	int newTempx10 = lm75.readTemperatureC() * 10;
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
	setDisplay(&PROP.Temp);
	display.setTextColor(TFT_WHITE, TFT_BLACK);
	// 温度が一桁以下の場合、十の位にスペース
	if(10 <= newTempx10 && newTempx10 < 100){
		display.print(' ');
	}
	display.print(int(newTempx10/10));
	display.print('.');
	display.print(int(newTempx10)%10);
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
void realTimeDisplay(){
	// 前回日時
	static uint8_t beforeTime[5] = {13,32,25,60,60};
	// 表示情報配列
	static PrintProperty* printPropArr[5] = {
		&PROP.Month,
		&PROP.Day,
		&PROP.Hour,
		&PROP.Min,
		&PROP.Sec,
	};
	static int itemLen = 5;
	// 時刻用変数
	uint8_t newTime[itemLen] = {0,0,0,0,0};

	// 現在時刻取得
	DateTime now = rtc.now();
	// 秒の値が前回と同じ場合スキップ
	if(beforeTime[SECOND] == now.second()){
		return;
	}
	// 各配列に格納
	int j = 0;
	newTime[j++] = now.month();
	newTime[j++] = now.day();
	newTime[j++] = now.hour();
	newTime[j++] = now.minute();
	newTime[j++] = now.second();

	// 出力処理
	for(int i=0;i<itemLen;i++){
		// 前回と同じ場合スキップ
		if(beforeTime[i] == newTime[i]){
			continue;
		}
		// 表示設定を反映
		setDisplay(printPropArr[i]);
		// 値が1桁の場合は0埋め
		if(newTime[i] < 10){
			display.print('0');
		}
		// 値を出力
		display.print(newTime[i]);
		// 前回日時を更新
		beforeTime[i] = newTime[i];
	}
}
