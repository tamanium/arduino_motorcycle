// --------------------ライブラリ--------------------
#include <Adafruit_GFX.h>               // 画面出力
#include <SPI.h>                        // SPI通信
#include <RTClib.h>                     // 時計機能
#include <Adafruit_PCF8574.h>           // IOエキスパンダ
#include <Adafruit_AHTX0.h>             // 温湿度計
#include <Adafruit_ADS1X15.h>           // ADコンバータ
#include <Adafruit_NeoPixel.h>          // オンボLED

// --------------------自作クラス・ピン定義--------------------
#include "Define.h"			// 値定義
#include "GearPositions.h"	// ギアポじかんかんかつ
#include "Winker.h"			// ウインカークラス
#include "Switch.h"			// スイッチクラス
#include "MyLovyanGFX.h"	// ディスプレイ設定

//#define BUZZER_ON

// --------------------定数--------------------
// 各じかんかんかつ
const int MONITOR_INTERVAL = 3;		//ms
const int DISPLAY_INTERVAL = 15;	//ms
const int TEMP_INTERVAL    = 2000;	//ms
const int VOLT_INTERVAL    = 2000;	//ms
const int TIME_INTERVAL    = 30;	//ms
const int BUZZER_DURATION  = 50;	//ms
const int WINKER_DURATION  = 380;	//ms
const int SP_DURATION = 500;        //ms

// 中心座標
const int CENTER_X = OLED.WIDTH>>1;
const int CENTER_Y = OLED.HEIGHT>>1;

// --------------------変数--------------------
unsigned long displayTime = 0; // 表示用時間
unsigned long monitorTime = 0; // 各種読み取り用時間
unsigned long tempTime = 0;    // 温度表示用時間
unsigned long voltageTime = 0; // 電圧表示用時間
unsigned long timeTime = 0;    // 時刻表示用時間
unsigned long bzzTime = 0;     // ブザー用時間
unsigned long spTime = 0;      // 速度センサ時間

// シフトポジション配列
int gears[] = {
	PINS.IOEXP.POS.nwt,
	PINS.IOEXP.POS.low,
	PINS.IOEXP.POS.sec,
	PINS.IOEXP.POS.thi,
	PINS.IOEXP.POS.top
};
// 明るさレベル
byte brightLevel[] = {
	0x01,
	0x08,
	0x18,
	0x38,
	0x80
};

// ディスプレイ
LGFX display;

// 円弧表示情報
struct arcInfo{
	LGFX_Sprite sprite;   // スプライト
	//LGFX_Sprite spriteBg; // 背景スプライト
	int x;                // 円弧中心x座標
	int y;                // 円弧中心y座標
	int r;                // 内径
	int d;                // 厚さ
	int angle0;           // 角度0
	int angle1;           // 角度1
	uint16_t colorON;     // 色
	uint16_t colorBG = TFT_BLUE; // 透過色
	/**
	 *  コンストラクタ
	 */
	arcInfo(LGFX *display) : sprite(display){}
	/**
	 * 初期設定　
	 */
	void initArc(){
		sprite.fillScreen(TFT_BLUE);
		sprite.setPivot(x,y);
	}
	/**
	 * 表示(ウインカー向け)
	 */
	void displayArcW(int stdX, int stdY, bool onOff){
		// on,offで色変更
		uint16_t color = onOff ? colorON : TFT_BLACK;
		// 弧描画
		sprite.fillArc(x,y,r+d,r,angle0,angle1,color);
		// 出力
		sprite.pushRotateZoom(stdX,stdY,0,1,1,colorBG);
	}

	/**
	 * 表示（メーター向け）
	 */
	void displayArcM(int stdX, int stdY, byte sp = 0){
		// 弧描画（緑）
		sprite.fillArc(x,y,r+d,r,angle0,angle1,colorON);
		
		if(sp < 99 ){
			// 速さに対する弧の角度算出
			int angleSp = (360-angle0+angle1)* (100-sp)/100;
			int newAngle0 = angle1 - angleSp;
			if(angleSp <= angle1){
				newAngle0 += 360;
			}
			// 弧描画（黒）
			sprite.fillArc(x,y,r+d-2,r+2,newAngle0+1,angle1-1,TFT_BLACK);
		}
		// 出力
		sprite.pushRotateZoom(stdX,stdY,0,1,1,colorBG);
	}

};
// 円弧情報
arcInfo arcM(&display);
arcInfo arcL(&display);
arcInfo arcR(&display);

// --------------------インスタンス--------------------
// 表示設定まとめ
struct Props{
	Prop Month;    // 月
	Prop Day;      // 日
	Prop Hour;     // 時
	Prop Min;      // 分
	Prop Sec;      // 秒
	Prop Temp;     // 温度
	Prop TempUnit; // 「℃」
	Prop Humid;    // 湿度
	Prop Gear;     // ギア
	Prop Newt;     // ギアニュートラル
	Prop Speed;    // 速度
	Prop SpUnit;   // 「km/h」
	Prop InitMsg;  // 初期表示
	Prop SpFreqIn; // 速度センサカウンタ
	Prop SpFreqInUnit; // 「Hz」
	Prop Voltage;    // 電圧
	Prop DebugData;   // デバッグ用値表示
} props;

// オンボLED
Adafruit_NeoPixel pixels(1, PINS.LED);
// RTC
RTC_DS1307 rtc;
// IOエキスパンダ
Adafruit_PCF8574 pcf;
// ADコンバータ
Adafruit_ADS1X15 ads;
// 温湿度計
Adafruit_AHTX0 aht;
// ギアポジション
GearPositions gearPositions(gears, sizeof(gears)/sizeof(int), &pcf);
// ウインカー
Winkers winkers(PINS.IOEXP.WNK.left, PINS.IOEXP.WNK.right, &pcf);
// スイッチ
Switch pushSw(PINS.IOEXP.sw, &pcf);

// ------------------------------初期設定------------------------------
void setup(void) {
	delay(500);
	// デバッグ用シリアル設定
	Serial.begin(9600);
	// I2C設定
	Wire1.setSDA(PINS.I2C.sda);
	Wire1.setSCL(PINS.I2C.scl);
	Wire1.setClock(400000);
	Wire1.begin();
	
	int offsetY = 50;
	
	// 月
	props.Month.font = &fonts::Font4;
	setPropWH(&props.Month,"00/");

	// 日
	props.Day = {
		props.Month.x + props.Month.width,
		props.Month.y,
		props.Month.size,
		props.Month.font
	};
	setPropWH(&props.Day, "00  ");

	// 時間
	props.Hour = {
		props.Day.x + props.Day.width,
		props.Month.y,
		props.Month.size,
		props.Month.font
	};
	setPropWH(&props.Hour,"00:");

	// 分
	props.Min = {
		props.Hour.x + props.Hour.width,
		props.Hour.y,
		props.Hour.size,
		props.Hour.font
	};
	setPropWH(&props.Min,"00:");

	// 秒
	props.Sec = {
		props.Min.x + props.Min.width,
		props.Hour.y,
		props.Hour.size,
		props.Hour.font
	};
	setPropWH(&props.Sec,"00");

	// 温度
	props.Temp = {
		0, 
		0,
		props.Hour.size,
		props.Hour.font
	};
	setPropWH(&props.Temp,"00 c");
 	props.Temp.x = fromRight(props.Temp.width)-3;

	// 温度単位
	props.TempUnit = {
		0,
		0,
		props.Temp.size,
		props.Temp.font
	};
	setPropWH(&props.TempUnit, "c");
	props.TempUnit.x = fromRight(props.TempUnit.width)-3;

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
		140+offsetY,
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
		10+offsetY,
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
	setPropWH(&props.SpUnit,"km/h");
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
	setPropWH(&props.SpFreqInUnit,"Hz");
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
	setPropWH(&props.DebugData,"00");
	props.DebugData.y = fromBottom(props.DebugData.height*8);

	// 初期表示メッセージ
	props.InitMsg = {
		0, 0, 2
	};
	// 初期情報表示
	// ディスプレイの初期化
	display.init();
	// 明るさ
	display.setBrightness(brightLevel[2]);

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
	ads.setGain(GAIN_TWOTHIRDS);
	ads.begin(MODULES.adCnv.address, &Wire1); // ADコンバータ
	#ifdef BUZZER_ON
		pinMode(PINS.buzzer, OUTPUT);              // ウインカー音
		digitalWrite(PINS.buzzer, LOW);
	#endif
	aht.begin(&Wire1,0,MODULES.thmst.address);// 温度計

	//rtc.adjust(DateTime(F(__DATE__),F(__TIME__))); // 時計合わせ

	
	display.fillScreen(TFT_BLACK);         // 画面リセット
	setDisplay(&props.Gear, "0");          // ギアポジション表示開始
	setDisplay(&props.Speed, "00");        // 速度
	setDisplay(&props.SpUnit, "km/h");     // 速度単位
	setDisplay(&props.Hour, "00:00:00");   // 時間
	setDisplay(&props.Month, "00/00");     // 日付
	setDisplay(&props.Temp, "00");         // 温度
	setDisplay(&props.Humid, "00%");       // 湿度
	setDisplay(&props.SpFreqIn, "0000");   // パルス周波数
	setDisplay(&props.SpFreqInUnit, "Hz"); // パルス周波数単位
	setDisplay(&props.Voltage, "00.0V");   // 電圧
	setDisplay(&props.DebugData);          // デバッグ用表示
	setDisplay(&props.TempUnit, "c");      // 温度単位
	display.fillCircle(306-3,6,3,TFT_WHITE);
	display.fillCircle(306-3,6,1,TFT_BLACK);
	// スプライト設定
	// 横縦
	//int w = (props.Speed.y + 60 - offsetY+10) * 2;
	int w = (75 + 60 - offsetY+10) * 2;
	int h = w;
	// 弧の幅
	arcM.d = 15;
	arcL.d = 10;
	arcR.d = 10;
	// 弧の内外半径
	int rOUT = (w-1)>>1;
	arcM.r = rOUT - arcM.d;
	arcL.r = rOUT - arcL.d + 25;
	arcR.r = rOUT - arcR.d + 25;
	// 弧の中心座標
	arcM.x = w>>1;
	arcM.y = h>>1;
	arcL.x = arcL.r+arcL.d+5;
	arcL.y = arcM.y;
	arcR.x = 0;
	arcR.y = arcM.y;
	// 角度
	int a0btm = 20;
	int a1top = 37;
	int a1btm = 41;
	arcM.angle0 = 90  + a0btm;
	arcM.angle1 = 90  - a0btm;

	arcL.angle0 = 90  + a1btm;
	arcL.angle1 = 270 - a1top;

	arcR.angle0 = 270 + a1top;
	arcR.angle1 = 90  - a1btm;

	// 大きさ
	arcM.sprite.createSprite(w,h);
	arcL.sprite.createSprite(arcL.x,h);
	arcR.sprite.createSprite(arcL.x,h);
	// 色
	arcM.colorON=TFT_GREEN;
	arcL.colorON=TFT_YELLOW;
	arcR.colorON=TFT_YELLOW;

	// 弧の中心・背景色
	arcM.initArc();
	arcL.initArc();
	arcR.initArc();
	// 出力
	arcM.displayArcM(CENTER_X,CENTER_Y+10);
	// 補助線
	//display.drawFastHLine(0,CENTER_Y-rOUT+7,320,TFT_RED);
	//display.drawFastHLine(0,CENTER_Y+rOUT+6,320,TFT_RED);
}


// ------------------------------ループ------------------------------
void loop() {

	// 経過時間(ms)取得
	unsigned long time = millis();
	static int pulseFreq = 0;
	static int beforePulseFreq = 0;
	static byte speed = 0;
	static byte beforeSpeed = 0;


	// 各種モニタリング・更新
	if(monitorTime <= time){
		// パルス周波数取得
		pulseFreq = getData(0x00);
		// 速度算出
		speed = byte(pulseFreq/10);
		if(100 <= speed){
			speed = 99;
		}
		// 現在のギアポジを取得
		gearPositions.updateStatus();
		// 現在のウインカー状態を取得
		winkers.updateStatus();
		// スイッチ状態取得
		pushSw.updateStatus();
		monitorTime += MONITOR_INTERVAL;
	}

	// 温度電圧モニタリング・表示
	if(tempTime <= time){
		displayTemp();

		tempTime += TEMP_INTERVAL;
	}

	if(voltageTime <= time){
		displayVoltage();

		voltageTime += VOLT_INTERVAL;
	}

	// 時刻表示
	if(timeTime <= time){
		displayRealTime();
		timeTime += TIME_INTERVAL;
	}
	
	// 各種表示処理
	if(displayTime <= time){
		// 速度カウンタ表示
		if(pulseFreq != beforePulseFreq){
			displayNumber(&props.SpFreqIn, pulseFreq, 4);
			beforePulseFreq = pulseFreq;
		}
		// 速度表示
		if(speed != beforeSpeed){
			displayNumber(&props.Speed, speed, 2);
			arcM.displayArcM(CENTER_X, CENTER_Y+10, speed);
			beforeSpeed = speed;
		}
		// デバッグ用スイッチ表示
		displaySwitch(&pushSw);
		// ギア表示
		gearDisplay(gearPositions.getGear());
		// ウインカー点灯状態が切り替わった場合
		if(displayWinkers() == true && bzzTime == 0 ){
			// ブザーON
			#ifdef BUZZER_ON
				digitalWrite(PINS.buzzer, HIGH);
			#endif
			pixels.setPixelColor(0, pixels.Color(1,1,0));
			pixels.show();
			// 時間設定
			bzzTime += BUZZER_DURATION;
		}
		displayTime += DISPLAY_INTERVAL;
	}

	//ブザーOFF処理
	if(bzzTime != 0 && bzzTime <= time){
		#ifdef BUZZER_ON
			digitalWrite(PINS.buzzer, LOW);
		#endif
		pixels.clear();
		pixels.show();
		bzzTime = 0;
	}
}

// -------------------------------------------------------------------
// ------------------------------メソッド------------------------------
// -------------------------------------------------------------------

/**
 * i2cモジュールのアドレスから接続中モジュールの有無を取得
 *
 * @param adrs i2cモジュールのアドレス
 * @param arr モジュール配列
 * @param size i2cモジュール数
 * @return モジュール配列のインデックス
 */
int existsModule(byte adrs, Module* arr, int size){
	for(int i=0; i<size; i++){
		if(arr[i].address == adrs){
			return i;
		}
	}
	return -1;
}

/**
 * ディスプレイ表示設定0
 *
 * @param p 表示設定
 */
void setDisplay(Prop* p){
	display.setCursor(p->x,p->y);   //描画位置
	display.setTextSize(p->size);   //テキスト倍率
	display.setTextColor(TFT_WHITE, TFT_BLACK); //フォント色...白
	display.setFont(p->font);
}

/**
 * ディスプレイ表示設定1
 *
 * @param p 表示設定
 * @param value 表示文字列
 */
void setDisplay(Prop* p, String value){
	setDisplay(p);
	display.print(value);
}

/**
 * フォントの縦横を設定
 *
 * @param p 表示設定
 * @param str 文字列
 */
void setPropWH(Prop* p, String str){
	display.setFont(p->font);
	display.setTextSize(p->size);
	p->width = display.textWidth(str);
	p->height = display.fontHeight();
}

/**
 * モジュールスキャン
 */
void scanModules(){
	// モジュールの配列
	Module moduleArr[] = {
		MODULES.ioExp,
		MODULES.thmst,
		MODULES.adCnv,
		MODULES.rtcMm,
		MODULES.speed,
		MODULES.rtcIC
	};

	// 初期表示メッセージ
	setDisplay(&props.InitMsg);
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
	display.println("---done---");
	delay(2000);
}

/**
 * ギアポジションの表示処理
 *
 * @param newChar char型 表示文字列
 */
void gearDisplay(char newGear){
	// バッファ文字列
	static char beforeGear = '0';
	// バッファと引数が同じ場合スキップ
	if(beforeGear == newGear){
		return;
	}
	// ニュートラルの場合
	if(newGear == 'N'){
		setDisplay(&props.Newt);
		display.setTextColor(TFT_GREEN, TFT_BLACK);
	}
	// 1～4の場合
	else{
		setDisplay(&props.Gear);
	}
	// ディスプレイ設定
	// ギア抜けorペダル踏み込み中の場合
	if(newGear == '0'){
		if(beforeGear == 'N'){
			setDisplay(&props.Newt);
		}
		// グレーで前回ギアを表示
		display.setTextColor(TFT_DARKGREY);
		display.print(beforeGear);
	}
	else{
		// ギア表示更新
		display.print(newGear);
	}
	// バッファ文字列を上書き
	beforeGear = newGear;
}


/**
 * ウインカー表示処理
 */
bool displayWinkers(){
	// バッファ状態
	static bool before[2] = {OFF, OFF};
	// 返却用フラグ
	bool isSwitched = false;

	arcInfo* arcArr[2] = {&arcL, &arcR};

	for(int side=LEFT; side<=RIGHT; side++){
		// 左右ウインカー状態を判定
		if(before[side] == winkers.getStatus(side)){
			continue;
		}
		// バッファ上書き
		before[side] = winkers.getStatus(side);
		// ディスプレイ表示処理
		arcArr[side]->displayArcW(CENTER_X,CENTER_Y+10,!before[side]);
		// フラグ立てる
		isSwitched = true;
	}
	return isSwitched;
}

/**
 * スイッチ動作表示
 * @param sw スイッチクラス
 */
void displaySwitch(Switch *sw){
	static bool beforeSw = false;
	static bool beforeLong = false;
	static byte brightIndex = 0;

	bool nowSw = sw->getStatus();
	display.setFont(NULL);
	display.setTextSize(2);
	display.setCursor(0, fromBottom(8*2));

	// キーダウンの場合
	if(nowSw){
		display.setTextColor(TFT_RED, TFT_BLACK);
		if(beforeSw != nowSw){
			display.print("ON  ");
			beforeSw = ON;
		}
		// 長押し
		else if(beforeLong != sw->isLongPress()){
			display.print("long");
			beforeLong = sw->isLongPress();
		}
	}
	// キーアップの場合
	else{
		if(beforeSw != nowSw){
			display.setTextColor(TFT_BLUE, TFT_BLACK);
			display.print("OFF ");
			beforeSw = OFF;
		}
		// 長押し判定だった場合
		if(beforeLong){
			beforeLong = false;
		}
		// プッシュ
		else if(sw->isPush()){
			display.setTextColor(TFT_BLUE, TFT_BLACK);
			display.setCursor((6*2)*4,fromBottom(8*2));
			brightIndex = (++brightIndex) % (sizeof(brightLevel) / sizeof(byte));
			display.setBrightness(brightLevel[brightIndex]);
			display.print(brightIndex);
			beforeSw = OFF;
		}
	}
}

/**
 * 電圧表示
 */
void displayVoltage(){
	// 前回電圧値
	static byte beforeVoltagex10 = 0;
	// 電圧ADC値取得
	int adcValue = getData(0x02);
	// 電圧算出
	// Vcc=5.22, 分圧逆数=3.05, 倍率10 => 係数=159
	byte voltagex10 = (adcValue * 159) / 1023;
	if(voltagex10 != beforeVoltagex10){
		// 電圧表示
		setDisplay(&props.Voltage);
		display.print(voltagex10/100);
		display.print((voltagex10/10)%10);
		display.print('.');
		display.print(voltagex10%10);

		
		setDisplay(&props.DebugData);
		display.println(adcValue);
		display.println(voltagex10/10);
		display.print(voltagex10);

		beforeVoltagex10 = voltagex10;
	}
}

/**
 * 温度表示
 */
void displayTemp(){
	//static int beforeTempx10 = 0;
	static byte beforeTemp = 0;
	//static int beforeHumid = 0;
	static byte beforeHumid = 0;
	// 温度取得
	sensors_event_t humidity, temp;
	aht.getEvent(&humidity,&temp);
	// 3桁分だけ取得
	//int newTempx10 = (int)(temp.temperature * 10)%1000;
	int newTempInt = (byte)(temp.temperature);
	// 0以下の場合は0に
	//if(newTempx10 <= 0){
	//	newTempx10 = 0;
	//}
	if(newTempInt < 0){
		newTempInt = 0;
	}
	else if(100 <= newTempInt ){
		newTempInt = 99;
	}
	byte newTemp = byte(newTempInt);
	if(beforeTemp != newTemp){
		displayNumber(&props.Temp, newTemp, 2);
		beforeTemp = newTemp;
	}

	// 前回温度と同じ場合、スキップ
	/*
	if(beforeTempx10 != newTempx10){
		setDisplay(&props.Temp);
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
	*/

	//int newHumid = (int)humidity.relative_humidity%100;
	//if(newHumid < 0){
	//	newHumid = 0;
	//}
	int newHumidInt = (int)humidity.relative_humidity%100;
	if(newHumidInt < 0){
		newHumidInt = 0;
	}
	else if(100 <= newHumidInt ){
		newHumidInt = 99;
	}
	byte newHumid = byte(newHumidInt);
	if(beforeHumid != newHumid){
		displayNumber(&props.Humid, newHumid, 2);
		beforeHumid = newHumid;
	}
	/*
	// 前回温度と同じ場合、スキップ
	if(beforeHumid != newHumid){
		setDisplay(&props.Humid);
		display.setTextColor(TFT_WHITE, TFT_BLACK);
		// 温度が一桁以下の場合、十の位にスペース
		if(1 <= newHumid && newHumid < 10){
			display.print(' ');
		}
		display.print(int(newHumid));
		// 保持変数を更新
		beforeHumid = newHumid;
	}
	*/
}

/**
 * 現在時刻表示処理
 */
void displayRealTime(){
	// 前回日時
	static uint8_t beforeTime[5] = {13,32,25,60,60};
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
	uint8_t newTime[itemLen] = {0,0,0,0,0};

	// 現在時刻取得
	DateTime now = rtc.now();
	// 秒の値が前回と同じ場合スキップ
	if(beforeTime[SECOND] == now.second()){
		return;
	}
	// 各配列に格納
	newTime[MONTH]  = now.month();
	newTime[DAY]    = now.day();
	newTime[HOUR]   = now.hour();
	newTime[MINUTE] = now.minute();
	newTime[SECOND] = now.second();

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

/**
 * 値の表示（左0埋め）
 * 
 * @param p Prop型 表示設定
 * @param valueByte byte型 表示値
 * @param digitNum int型 表示桁数
 */
void displayNumber(Prop* p, byte valueByte, int digitNum){
	// 表示設定
	setDisplay(p);
	// 速度周波数表示
	for(int d = pow(10, digitNum-1); 1 < d; d /= 10 ){
		if(valueByte / d == 0){
			display.print('0');
		}
		else{
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
void displayNumber(Prop* p, int valueInt, int digitNum){
	// 表示設定
	setDisplay(p);
	// 速度周波数表示
	for(int d = pow(10, digitNum-1); 1 < d; d /= 10 ){
		if(valueInt / d == 0){
			display.print('0');
		}
		else{
			break;
		}
	}
	display.print(valueInt);
}

/**
 * スピードセンサ(ATTINY85)からデータ取得
 *
 * @param reg byte型 データ種類
 */
int getData(byte reg){
	// 返却用変数
	int result = -1;
	
	if(0x02 < reg){
		return -1;
	}
	Wire1.beginTransmission(MODULES.speed.address);
	Wire1.write(reg);
	Wire1.endTransmission(false);
	Wire1.requestFrom(MODULES.speed.address, 2);
	if(Wire1.available() == 2){
		result = (Wire1.read()<<8) | Wire1.read();
	}
	return result;
}