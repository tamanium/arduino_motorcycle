// --------------------ライブラリ--------------------
#include <Adafruit_GFX.h>       // 画面出力
#include <SPI.h>                // SPI通信
#include <RTClib.h>             // 時計機能
#include <Adafruit_AHTX0.h>     // 温湿度計
#include <Adafruit_NeoPixel.h>  // オンボLED

// --------------------自作クラス・ピン定義--------------------
#include "Define.h"      // 値定義
#include "MyLovyanGFX.h" // ディスプレイ設定
#include "DataClass.h"   // データ処理クラス

//#define BUZZER_ON
#define DEBUG_MODE

// --------------------定数--------------------
// 各時間間隔(ms)
const int MONITOR_INTERVAL = 3;
const int DISPLAY_INTERVAL = 15;
const int TEMP_INTERVAL    = 2000;
const int VOLT_INTERVAL    = 2000;
const int TIME_INTERVAL    = 30;
const int BUZZER_DURATION  = 50;

// 中心座標
const int CENTER_X = OLED.WIDTH >> 1;
const int CENTER_Y = OLED.HEIGHT >> 1;

// ウインカー値
enum{
	INDICATE_NONE,
	INDICATE_LEFT,
	INDICATE_RIGHT,
	INDICATE_BOTH = INDICATE_LEFT | INDICATE_RIGHT,
};

// モジュール側も同じ定義
enum {
	INDEX_FREQ,
	INDEX_GEARS,
	INDEX_WINKERS,
	INDEX_SWITCH,
	INDEX_VOLT,
	INDEX_PULSE,
	DATA_SIZE,
	INDEX_A_PART = 0x40,
	INDEX_ALL    = 0xFF,
};

// 明るさレベル
byte brightLevel[] = {
	0x01,
	0x08,
	0x18,
	0x38,
	0x80
};

// --------------------変数--------------------
unsigned long displayTime = 0; // 表示用時間
unsigned long monitorTime = 0; // 各種読み取り用時間
unsigned long tempTime    = 0; // 温度表示用時間
unsigned long voltageTime = 0; // 電圧表示用時間
unsigned long timeTime    = 0; // 時刻表示用時間
unsigned long bzzTime     = 0; // ブザー用時間

int moduleData[DATA_SIZE];     // センサーからの取得値

// ディスプレイ
LGFX display;

// 円弧表示情報
struct arcInfo {
	LGFX_Sprite sprite;           // スプライト
	int x;                        // 円弧中心x座標
	int y;                        // 円弧中心y座標
	int r;                        // 内径
	int d;                        // 厚さ
	int angle0;                   // 角度0
	int angle1;                   // 角度1
	uint16_t colorON;             // 色
	uint16_t colorBG = TFT_BLUE;  // 透過色
	/**
	 *  コンストラクタ
	 */
	arcInfo(LGFX* display) : sprite(display) {}
	/**
	 * 初期設定　
	 */
	void initArc() {
		sprite.fillScreen(TFT_BLUE);
		sprite.setPivot(x, y);
	}
	/**
	 * 表示(ウインカー向け)
	 */
	void displayArcW(int stdX, int stdY, bool onOff) {
		// 弧描画 on,offで色変更
		sprite.fillArc(x, y, r + d, r, angle0, angle1, onOff ? colorON : TFT_BLACK);
		// 出力
		sprite.pushRotateZoom(stdX, stdY, 0, 1, 1, colorBG);
	}

	/**
	 * 表示（メーター向け）
	 */
	void displayArcM(int stdX, int stdY, byte sp = 0) {
		static byte beforeSp = 0xFF;                // 前回速度

		// 初期処理
		if(beforeSp == 0xFF){
			// 弧描画（薄緑）
			sprite.fillArc(x, y, r + d, r, angle0, angle1, 0x01e0);
			sprite.pushRotateZoom(stdX, stdY, 0, 1, 1, colorBG);
			beforeSp = 0;
			return;
		}
		// 速度が同じ場合、スキップ
		if(beforeSp == sp){
			return;
		}
		// 速さに対する弧の角度算出
		int angleSp = (360 - angle0 + angle1) * sp / 100;
		int newAngle0 = angle0;
		int newAngle1 = angle1;
		int newColor = colorON;
		if(beforeSp < sp){
			// 速度が上がった場合
			newAngle1 = angle0 + angleSp;
		}
		else{
			// 速度が下がった場合
			newAngle0 = angle0 + angleSp;
			// 色を薄緑に変更
			newColor = 0x01e0;
		}
		// 出力
		sprite.fillArc(x, y, r + d, r, newAngle0, newAngle1, newColor);
		sprite.pushRotateZoom(stdX, stdY, 0, 1, 1, colorBG);
		beforeSp = sp;
	}
};

// 円弧情報
arcInfo arcM(&display);
arcInfo arcL(&display);
arcInfo arcR(&display);

// データクラス(チャタリング対策あり)
DataClass switchData(true);
DataClass winkersData(false);

// --------------------インスタンス--------------------
// 表示設定まとめ
struct Props {
	Prop Month;        // 月
	Prop Day;          // 日
	Prop Hour;         // 時
	Prop Min;          // 分
	Prop Sec;          // 秒
	Prop Temp;         // 温度
	Prop TempUnit;     // 「℃」
	Prop Humid;        // 湿度
	Prop Gear;         // ギア
	Prop Newt;         // ギアニュートラル
	Prop Speed;        // 速度
	Prop SpUnit;       // 「km/h」
	Prop InitMsg;      // 初期表示
	Prop SpFreqIn;     // 速度センサカウンタ
	Prop SpFreqInUnit; // 「Hz」
	Prop Voltage;      // 電圧
	Prop DebugData;    // デバッグ用値表示
} props;

// オンボLED
Adafruit_NeoPixel pixels(1, PINS.LED);
// RTC
RTC_DS1307 rtc;
// 温湿度計
Adafruit_AHTX0 aht;

// ------------------------------初期設定------------------------------
void setup(void) {
	delay(100);
	// デバッグ用シリアル設定
	Serial.begin(9600);
	// I2C設定
	Wire1.setSDA(PINS.I2C.sda);
	Wire1.setSCL(PINS.I2C.scl);
	Wire1.setClock(400000);
	Wire1.begin();

	int offsetY = 50;

	// 時間
	props.Hour = {
		0,
		0,
		1,
		&fonts::Font4
	};
	setPropWH(&props.Hour, "00:");

	// 分
	props.Min = {
		props.Hour.x + props.Hour.width,
		props.Hour.y,
		props.Hour.size,
		props.Hour.font
	};
	setPropWH(&props.Min, "00:");

	// 秒
	props.Sec = {
		props.Min.x + props.Min.width,
		props.Hour.y,
		props.Hour.size,
		props.Hour.font
	};
	setPropWH(&props.Sec, "00");

	// 月
	props.Month = {
		0,
		props.Hour.y + props.Hour.height,
		1,
		&fonts::Font4
	};
	setPropWH(&props.Month, "00/");

	// 日
	props.Day = {
		props.Month.x + props.Month.width,
		props.Month.y,
		props.Month.size,
		props.Month.font
	};
	setPropWH(&props.Day, "00");

	// 温度
	props.Temp = {
		0,
		0,
		props.Hour.size,
		props.Hour.font
	};
	setPropWH(&props.Temp, "00 c");
	props.Temp.x = fromRight(props.Temp.width) - 3;

	// 温度単位
	props.TempUnit = {
		0,
		0,
		props.Temp.size,
		props.Temp.font
	};
	setPropWH(&props.TempUnit, "c");
	props.TempUnit.x = fromRight(props.TempUnit.width) - 3;

	// 湿度
	props.Humid = {
		props.Temp.x,
		props.Temp.height,
		1,
		&fonts::Font4
	};
	setPropWH(&props.Humid, "00%");
	props.Humid.x = fromRight(props.Humid.width);

	// ギア
	props.Gear = {
		0,
		140 + offsetY,
		1,
		&fonts::DejaVu56
	};
	setPropWH(&props.Gear, "0");
	props.Gear.x = centerHorizontal(props.Gear.width);

	// ギアニュートラル
	props.Newt = {
		0,
		props.Gear.y,
		1,
		&fonts::DejaVu56
	};
	setPropWH(&props.Newt, "N");
	props.Newt.x = centerHorizontal(props.Newt.width);

	// 速度
	/*
	props.Speed = {
		0,
		30+offsetY,
		1,
		&fonts::Font8
	};
	*/
	props.Speed = {
		0,
		10 + offsetY,
		1,
		&fonts::Font7
	};
	setPropWH(&props.Speed, "00");
	props.Speed.x = centerHorizontal(props.Speed.width);

	// 速度単位
	/*
	props.SpUnit = {
		0,
		110+offsetY,
		1,
		&fonts::Font4
	};
	*/
	props.SpUnit = {
		0,
		props.Speed.y + props.Speed.height,
		1,
		&fonts::Font2
	};
	setPropWH(&props.SpUnit, "km/h");
	props.SpUnit.x = centerHorizontal(props.SpUnit.width);

	// スピードセンサIN
	props.SpFreqIn = {
		0,
		props.SpUnit.y + props.SpUnit.height,
		1,
		&fonts::Font7
	};
	setPropWH(&props.SpFreqIn, "0000");
	props.SpFreqIn.x = centerHorizontal(props.SpFreqIn.width);

	// スピードセンサIN単位
	props.SpFreqInUnit = {
		0,
		props.SpFreqIn.y + props.SpFreqIn.height,
		1,
		&fonts::Font2
	};
	setPropWH(&props.SpFreqInUnit, "Hz");
	props.SpFreqInUnit.x = centerHorizontal(props.SpFreqInUnit.width);

	// 電圧
	props.Voltage = {
		0,
		0,
		1,
		&fonts::Font4
	};
	setPropWH(&props.Voltage, "00.0V");
	props.Voltage.x = fromRight(props.Voltage.width);
	props.Voltage.y = fromBottom(props.Voltage.height);

	// デバッグ用表示
	setPropWH(&props.DebugData, "00");
	props.DebugData.y = fromBottom(props.DebugData.height * 9);

	// 初期表示メッセージ
	props.InitMsg = {
		0, 0, 2
	};

	// ディスプレイの初期化
	display.init();
	// 明るさ
	display.setBrightness(brightLevel[2]);

	// ブザー連動LED設定
	pixels.begin();
	pixels.setPixelColor(0, pixels.Color(1, 1, 0));
	pixels.show();

	// I2C通信スキャン
	scanModules();
	// 各モジュール動作開始
	rtc.begin(&Wire1);                         // RTC
	#ifdef BUZZER_ON
		pinMode(PINS.buzzer, OUTPUT);  // ウインカー音
		digitalWrite(PINS.buzzer, LOW);
	#endif
	aht.begin(&Wire1, 0, MODULES.thmst.address);  // 温度計

	//rtc.adjust(DateTime(F(__DATE__),F(__TIME__))); // 時計合わせ

	display.fillScreen(TFT_BLACK);          // 画面リセット
	setDisplay(&props.Gear, "0");           // ギアポジション表示開始
	setDisplay(&props.Speed, "00");         // 速度
	setDisplay(&props.SpUnit, "km/h");      // 速度単位
	setDisplay(&props.Hour, "00:00:00");    // 時間
	setDisplay(&props.Month, "00/00");      // 日付
	setDisplay(&props.Temp, "00");          // 温度
	setDisplay(&props.Humid, "00%");        // 湿度
	setDisplay(&props.SpFreqIn, "0000");    // パルス周波数
	setDisplay(&props.SpFreqInUnit, "Hz");  // パルス周波数単位
	setDisplay(&props.Voltage, "00.0V");    // 電圧
	setDisplay(&props.DebugData);           // デバッグ用表示
	setDisplay(&props.TempUnit, "c");       // 温度単位
	display.fillCircle(306 - 3, 6, 3, TFT_WHITE);
	display.fillCircle(306 - 3, 6, 1, TFT_BLACK);
	// スプライト設定
	// 横縦
	//int w = (props.Speed.y + 60 - offsetY+10) * 2;
	int w = (75 + 60 - offsetY + 10) * 2;
	int h = w;
	// 弧の幅
	arcM.d = 10;
	arcL.d = 10;
	arcR.d = 10;
	// 弧の内外半径
	int rOUT = (w - 1) >> 1;
	arcM.r = rOUT - arcM.d;
	arcL.r = rOUT - arcL.d + 25;
	arcR.r = rOUT - arcR.d + 25;
	// 弧の中心座標
	arcM.x = w >> 1;
	arcM.y = h >> 1;
	arcL.x = arcL.r + arcL.d + 5;
	arcL.y = arcM.y;
	arcR.x = 0;
	arcR.y = arcM.y;
	// 角度
	int a0btm = 20;
	int a1top = 37;
	int a1btm = 41;
	arcM.angle0 = 90 + a0btm;
	arcM.angle1 = 90 - a0btm;
	arcL.angle0 = 90 + a1btm;
	arcL.angle1 = 270 - a1top;
	arcR.angle0 = 270 + a1top;
	arcR.angle1 = 90 - a1btm;
	// 大きさ
	arcM.sprite.createSprite(w, h);
	arcL.sprite.createSprite(arcL.x, h);
	arcR.sprite.createSprite(arcL.x, h);
	// 色
	arcM.colorON = TFT_GREEN;
	arcL.colorON = TFT_YELLOW;
	arcR.colorON = TFT_YELLOW;
	// 弧の中心・背景色
	arcM.initArc();
	arcL.initArc();
	arcR.initArc();
	// 出力
	arcM.displayArcM(CENTER_X, CENTER_Y + 10);
	// 補助線
	//display.drawFastHLine(0,CENTER_Y-rOUT+7,320,TFT_RED);
	//display.drawFastHLine(0,CENTER_Y+rOUT+6,320,TFT_RED);
}


// ------------------------------ループ------------------------------
void loop() {

	// 経過時間(ms)取得
	unsigned long time = millis();

	// 各種モニタリング・更新
	if (monitorTime <= time) {
		// 周波数、ギアポジ、ウインカー、スイッチの値を取得
		getDataA();
		switchData.setData(moduleData[INDEX_WINKERS]>>2);
		winkersData.setData(moduleData[INDEX_WINKERS] & INDICATE_BOTH);

		// デバッグモード表示
		#ifdef DEBUG_MODE
			static int loopTimeMax = 0;
			static byte countLoop = 0;
			static unsigned long beforeTime = 0;

			int freqOut = getData(INDEX_PULSE);

			if(70 < ++countLoop){
				loopTimeMax = 0;
				countLoop = 0;
			}

			int loopTime = (int)(time - beforeTime);
			if(loopTimeMax < loopTime){
				loopTimeMax = loopTime;
			}

			beforeTime = time;
			
			setDisplay(&props.DebugData);
			display.print("loop :");
			displayNumberln(loopTime, ' ', 4);
			display.print("loopM:");
			displayNumberln(loopTimeMax, ' ', 4);
			display.print("FreqI:");
			displayNumberln(moduleData[INDEX_FREQ], ' ', 4);
			display.print("FreqO:");
			displayNumberln(freqOut, ' ', 4);
			display.print("vltAD:");
			displayNumberln(moduleData[INDEX_VOLT], ' ', 4);
			display.print("wnkAD:");
			displayNumberln(moduleData[INDEX_WINKERS], ' ', 4);
			display.print("geaAD:");
			displayNumberln(moduleData[INDEX_GEARS], ' ', 4);
		#endif
		monitorTime += MONITOR_INTERVAL;
	}

	// 温度モニタリング・表示
	if (tempTime <= time) {
		displayTemp();
		tempTime += TEMP_INTERVAL;
	}

	// 電圧モニタリング・表示
	
	if (voltageTime <= time) {
		displayVoltage();
		voltageTime += VOLT_INTERVAL;
	}

	// 時刻表示
	if (timeTime <= time) {
		displayRealTime();
		timeTime += TIME_INTERVAL;
	}

	// 各種表示処理
	if (displayTime <= time) {
		// 速度表示
		displaySpeed();
		// デバッグ用スイッチ表示
		displaySwitch();
		// ギア表示
		displayGear();
		// ウインカー表示
		bool isChangedWinkers = displayWinkers();
		// ブザー出力
		if (isChangedWinkers == true && bzzTime == 0) {
			setBuzzer(ON);
			bzzTime = time + BUZZER_DURATION;
		}
		displayTime += DISPLAY_INTERVAL;
	}

	//ブザーOFF処理
	if (bzzTime != 0 && bzzTime <= time) {
		setBuzzer(OFF);
		bzzTime = 0;
	}
}

// -------------------------------------------------------------------
// ------------------------------メソッド------------------------------
// -------------------------------------------------------------------

void setBuzzer(bool isOn){
	// ブザーON
	#ifdef BUZZER_ON
		digitalWrite(PINS.buzzer, !isOn);
	#endif
	if(isOn){
		pixels.setPixelColor(0, pixels.Color(1, 1, 0));
	}
	else{
		pixels.clear();
	}
	pixels.show();
}
/**
 * ディスプレイ表示設定0
 *
 * @param p 表示設定
 */
void setDisplay(Prop* p) {
	display.setCursor(p->x, p->y);               //描画位置
	display.setTextSize(p->size);                //テキスト倍率
	display.setTextColor(TFT_WHITE, TFT_BLACK);  //フォント色...白
	display.setFont(p->font);
}

/**
 * ディスプレイ表示設定1
 *
 * @param p 表示設定
 * @param value 表示文字列
 */
void setDisplay(Prop* p, String value) {
	setDisplay(p);
	display.print(value);
}

/**
 * フォントの縦横を設定
 *
 * @param p 表示設定
 * @param str 文字列
 */
void setPropWH(Prop* p, String str) {
	display.setFont(p->font);
	display.setTextSize(p->size);
	p->width = display.textWidth(str);
	p->height = display.fontHeight();
}

/**
 * モジュールスキャン
 */
void scanModules() {
	// モジュールの配列
	Module moduleArr[] = {
		MODULES.thmst,
		MODULES.speed,
		MODULES.rtcMm,
		MODULES.rtcIC
	};

	// 初期表示メッセージ
	setDisplay(&props.InitMsg);
	display.println("Hello");
	display.println("");
	delay(500);
	// スキャン処理
	display.println("I2C Module Scanning...");
	for (Module  module : moduleArr) {
		Wire1.beginTransmission(module.address);
		byte error = Wire1.endTransmission();
		display.setTextColor(TFT_WHITE, TFT_BLACK);
		display.print(module.name + ":");
		display.setTextColor((error == 0) ? TFT_GREEN : TFT_RED, TFT_BLACK);
		display.println((error == 0) ? "OK" : "NG");
	}
	display.setTextColor(TFT_WHITE);
	display.println("");
	display.println("-- done --");
	delay(2000);
}

/**
 * ギアポジションの表示処理
 */
void displayGear() {
	static char before = '0';
	//0, 234, 456, 658, 847
	int thresholdArr[6] = {0, 117, 345, 557, 753, 935};
	char gearArr[6] = {'0', '3', '2', '4', '1', 'N'};
	char gear = '0';
	for(int i=0; i<6; i++){
		if(moduleData[INDEX_GEARS] < thresholdArr[i]){
			gear = gearArr[i];
			break;
		}
	}
	if(gear == before){
		return;
	}
	if(gear == 'N'){
		setDisplay(&props.Newt);
		display.setTextColor(TFT_GREEN, TFT_BLACK);
		display.print(gear);
	}
	else if(gear == '0'){
		Prop* prop = (before == 'N') ? &props.Newt : &props.Gear;
		/*
		if(before == 'N'){
			setDisplay(&props.Newt);
		}
		else{
			setDisplay(&props.Gear);
		}
		*/
		setDisplay(prop);
		// グレーで前回ギアを表示
		display.setTextColor(TFT_DARKGREY);
		display.print(before);
	}
	else{
		setDisplay(&props.Gear);
		display.print(gear);
	}
	before = gear;
}

/**
 * ウインカー表示処理
 */
bool displayWinkers() {
	// 前回値
	static byte beforeData = INDICATE_NONE;
	// 円弧表示配列
	static arcInfo* arcArr[2] = { &arcL, &arcR };
	// 定数配列
	const byte indicateArr[2] = { INDICATE_LEFT, INDICATE_RIGHT };

	// 現在値取得
	byte nowData = (byte)(winkersData.getData());
	// 前回値と同じ場合、処理終了
	if (nowData == beforeData) {
		return false;
	}

	for(int side = LEFT; side<=RIGHT;side++){
		if((nowData & indicateArr[side]) != (beforeData & indicateArr[side])){
			// ディスプレイ表示処理
			arcArr[side]->displayArcW(CENTER_X, CENTER_Y + 10, (nowData & indicateArr[side]) == indicateArr[side]);
		}
	}

	beforeData = nowData;
	return true;
}

/**
 * スイッチ動作表示
 * @param sw スイッチクラス
 */
void displaySwitch() {
	static bool beforeSw = false;
	//static bool beforeLong = false;
	//static byte brightIndex = 0;

	//bool nowSw = sw->getStatus();
	bool nowSw = switchData.getData();
	display.setFont(NULL);
	display.setTextSize(2);
	display.setCursor(0, fromBottom(8 * 2));

	// キーダウンの場合
	if (nowSw) {
		display.setTextColor(TFT_RED, TFT_BLACK);
		if (beforeSw != nowSw) {
			display.print("ON  ");
			beforeSw = ON;
		}
		// 長押し
		//else if (beforeLong != sw->isLongPress()) {
		//	display.print("long");
		//	beforeLong = sw->isLongPress();
		//}
	}
	// キーアップの場合
	else {
		if (beforeSw != nowSw) {
			display.setTextColor(TFT_BLUE, TFT_BLACK);
			display.print("OFF ");
			beforeSw = OFF;
		}
		// 長押し判定だった場合
		//if (beforeLong) {
		//	beforeLong = false;
		//}
		// プッシュ
		// else if (sw->isPush()) {
		// 	display.setTextColor(TFT_BLUE, TFT_BLACK);
		// 	display.setCursor((6 * 2) * 4, fromBottom(8 * 2));
		// 	brightIndex = (++brightIndex) % (sizeof(brightLevel) / sizeof(byte));
		// 	display.setBrightness(brightLevel[brightIndex]);
		// 	display.print(brightIndex);
		// 	beforeSw = OFF;
		// }
	}
}

/**
 * スピード表示
 */
void displaySpeed(){
	// 前回パルス周波数
	static int beforePulseFreq = 0;
	// 前回スピード
	static byte beforeSpeed = 0;

	// 速度カウンタ表示
	if (moduleData[INDEX_FREQ] != beforePulseFreq) {
		displayNumber(&props.SpFreqIn, moduleData[INDEX_FREQ], 4);
		beforePulseFreq = moduleData[INDEX_FREQ];
	}
	// 速度表示
	byte speed = byte(moduleData[INDEX_FREQ] / 10);
	if(100 <= speed){
		speed = 99;
	}
	if (speed != beforeSpeed) {
		displayNumber(&props.Speed, speed, 2);
		arcM.displayArcM(CENTER_X, CENTER_Y + 10, speed);
		beforeSpeed = speed;
	}
}

/**
 * 電圧表示
 */
void displayVoltage() {
	// 前回電圧値
	static byte beforeVoltagex10 = 0;
	// 電圧ADC値取得
	int adcValue = getData(INDEX_VOLT);
	// 電圧算出
	// Vcc=5.22, 分圧逆数=3.05, 倍率10 => 係数=159
	byte voltagex10 = (adcValue * 159) / 1023;
	if (voltagex10 != beforeVoltagex10) {
	// 電圧表示
	setDisplay(&props.Voltage);

	display.print(voltagex10 / 100);
	display.print((voltagex10 / 10) % 10);
	display.print('.');
	display.print(voltagex10 % 10);

	setDisplay(&props.DebugData);
	display.setCursor(props.DebugData.x, props.DebugData.y + props.DebugData.height * 7);
	display.print("vltAD:");
	displayNumberln(adcValue, ' ', 4);
	
	beforeVoltagex10 = voltagex10;
	}
}

/**
 * 温度表示
 */
void displayTemp() {
	static byte beforeTemp = 0;
	static byte beforeHumid = 0;
	// 温度取得
	sensors_event_t humidity, temp;
	aht.getEvent(&humidity, &temp);
	int newTempInt = (int)(temp.temperature);
	if (newTempInt < 0) {
		newTempInt = 0;
	} else if (100 <= newTempInt) {
		newTempInt = 99;
	}
	byte newTemp = byte(newTempInt);
	if (beforeTemp != newTemp) {
		displayNumber(&props.Temp, newTemp, 2);
		beforeTemp = newTemp;
	}
	int newHumidInt = (int)humidity.relative_humidity % 100;
	if (newHumidInt < 0) {
		newHumidInt = 0;
	} else if (100 <= newHumidInt) {
		newHumidInt = 99;
	}
	byte newHumid = byte(newHumidInt);
	if (beforeHumid != newHumid) {
		displayNumber(&props.Humid, newHumid, 2);
		beforeHumid = newHumid;
	}
}

/**
 * 現在時刻表示処理
 */
void displayRealTime() {
	// 前回日時
	static uint8_t beforeTime[5] = { 13, 32, 25, 60, 60 };
	// 表示情報配列
	static Prop* printPropArr[5] = {
		&props.Month,
		&props.Day,
		&props.Hour,
		&props.Min,
		&props.Sec,
	};
	static int itemLen = 5;
	// 時刻用変数
	uint8_t newTime[itemLen] = { 0, 0, 0, 0, 0 };

	// 現在時刻取得
	DateTime now = rtc.now();
	// 秒の値が前回と同じ場合スキップ
	if (beforeTime[SECOND] == now.second()) {
		return;
	}
	// 各配列に格納
	newTime[MONTH] = now.month();
	newTime[DAY] = now.day();
	newTime[HOUR] = now.hour();
	newTime[MINUTE] = now.minute();
	newTime[SECOND] = now.second();

	// 出力処理
	for (int i = 0; i < itemLen; i++) {
		// 前回と同じ場合スキップ
		if (beforeTime[i] == newTime[i]) {
			continue;
		}
		// 表示設定を反映
		setDisplay(printPropArr[i]);
		// 値が1桁の場合は0埋め
		if (newTime[i] < 10) {
			display.print('0');
		}
		// 値を出力
		display.print(newTime[i]);
		// 前回日時を更新
		beforeTime[i] = newTime[i];
	}
}

/**
 * 値の表示（左0埋め）
 * 
 * @param p Prop型 表示設定
 * @param valueByte byte型 表示値
 * @param digitNum int型 表示桁数
 */
void displayNumber(Prop* p, byte valueByte, int digitNum) {
	// 表示設定
	setDisplay(p);
	// 速度周波数表示
	for (int d = pow(10, digitNum - 1); 1 < d; d /= 10) {
		if (valueByte / d == 0) {
			display.print('0');
		} else {
			break;
		}
	}
	display.print(valueByte);
}

/**
 * 値の表示（左0埋め）開発用
 * 
 * @param p Prop型 表示設定
 * @param valueLong long型 表示値
 * @param digitNum int型 表示桁数
 */
void displayNumber(Prop* p, int valueInt, int digitNum) {
  // 表示設定
  setDisplay(p);
  // 表示
  displayNumberln(valueInt, '0', digitNum);
}

void displayNumberln(int valueInt, char spacer, int digiNum){
	// 表示
	for (int d = pow(10, digiNum - 1); 1 < d; d /= 10) {
		if (valueInt / d == 0) {
			display.print(spacer);
		} else {
			break;
		}
	}
	display.println(valueInt);
}

/**
 * モジュール(ATTINY1604)からデータ取得
 *
 * @param reg byte型 データ種類
 */
int getData(byte reg) {
	// 返却用変数
	requestSpeedModule(reg, 2);
	int result = -1;
	if (Wire1.available() == 2) {
		result = (Wire1.read() << 8) | Wire1.read();
	}
	return result;
}

/**
 * モジュール(ATTINY1604)から一部まとめてデータ取得
 */
void getDataA(){
	// 周波数、ギアポジ、ウインカー、スイッチの順に取得
	requestSpeedModule(INDEX_A_PART, 8);
	if(Wire1.available() == 8) {
		moduleData[INDEX_FREQ] = (Wire1.read() << 8) | Wire1.read();
		moduleData[INDEX_GEARS] = (Wire1.read() << 8) | Wire1.read();
		moduleData[INDEX_WINKERS] = (Wire1.read() << 8) | Wire1.read();
		moduleData[INDEX_SWITCH] = (Wire1.read() << 8) | Wire1.read();
	}
}

/**
 * モジュール(ATTINY1604)へのリクエスト
 *
 * @param reg int型 送信レジスト値
 * @param numByte int型 データ容量(byte)
 */
void requestSpeedModule(int reg, int numByte){
	Wire1.beginTransmission(MODULES.speed.address);
	Wire1.write(reg);
	Wire1.endTransmission(false);
	Wire1.requestFrom(MODULES.speed.address, numByte);
}