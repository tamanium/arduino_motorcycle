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
const int SP_DURATION = 500;  //ms
int spPulseDuration = 1000;       //ms

// 中心座標
const int centerX = OLED.WIDTH>>1;
const int centerY = OLED.HEIGHT>>1;

// --------------------変数--------------------
unsigned long displayTime = 0; // 表示処理
unsigned long monitorTime = 0; // 各種読み取り
unsigned long tempTime = 0;    // 温度測定にて使用
unsigned long timeTime = 0;    // 時刻測定
unsigned long bzzTime = 0;     // ブザー
unsigned int pulseTotalCounter = 0;     // 速度センサーカウンタ
unsigned long spTime = 0;      // 速度センサ時間 
unsigned long spPulseSwitchTime = 0; // 速度パルスH/L切り替え
unsigned long spPulseChangeTime = 0; // 速度パルス周波数切り替え

unsigned int spPulseFreq[] = {
	10,  50,
	100, 150,
	200, 250,
	300, 350,
	400, 450,
	500, 550,
	600, 650,
	700, 750,
	800, 850,
	900, 950,
	1000
};
unsigned int spPulseSize = 21;

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
	LGFX_Sprite sprite; // スプライト
	int x;              // 円弧中心x座標
	int y;              // 円弧中心y座標
	int r;              // 内径
	int d;              // 厚さ
	int angle0;         // 角度0
	int angle1;         // 角度1
	int angle2;         // 角度2
	int angle3;         // 角度3
	uint16_t colorON;   // 色
	uint16_t colorBG = TFT_BLUE; // 透過色
	/**
	 *  コンストラクタ
	 */
	arcInfo(LGFX *display) : sprite(display){}
	/**
	 * 表示
	 */
	void displayArc(int stdX, int stdY, bool onOff, bool isWinker = false){
		sprite.fillScreen(TFT_BLUE);
		// on,offで色変更
		uint16_t color = onOff ? colorON : TFT_BLACK;
		// 弧の中心軸
		sprite.setPivot(x,y);
		// 弧描画
		sprite.fillArc(x,y,r+d,r,angle0,angle1,color);
		if(isWinker){
			sprite.fillArc(x,y,r+d+20,r+20,angle2,angle3,color);
		}
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
struct Props{
	Prop Month;    // 月
	Prop Day;      // 日
	Prop Hour;     // 時
	Prop Min;      // 分
	Prop Sec;      // 秒
	Prop Temp;     // 温度
	Prop TempUnit; // 温度単位
	Prop Humid;    // 湿度
	Prop Gear;     // ギア
	Prop Newt;     // ギアニュートラル
	Prop Speed;    // 速度
	Prop SpUnit;   // 速度単位
	Prop InitMsg;  // 初期表示：「hello」
	Prop SpSensor; // 速度センサカウンタ
	Prop SpFreq;     // 速度センサ周波数
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

/**
 * ディスプレイ表示設定
 *
 * @param p 表示設定
 * @param isTrans 透過させるか
 */
void setDisplay(Prop* p, String value =""){
	display.setCursor(p->x,p->y);   //描画位置
	display.setTextSize(p->size);   //テキスト倍率
	display.setTextColor(TFT_WHITE, TFT_BLACK); //フォント色...白
	display.setFont(p->font);
	if(value != ""){
		display.print(value);
	}
}

/**
 * フォントの縦横を設定
 *
 * @param p 表示設定
 * @param size フォント倍率
 */
void setPropWH(Prop* p, String str = "0"){
	display.setFont(p->font);
	display.setTextSize(p->size);
	p->width = display.textWidth(str);
	p->height = display.fontHeight();
}

// ------------------------------初期設定------------------------------
void setup(void) {
	// デバッグ用シリアル設定
	Serial.begin(9600);
	// I2C設定
	Wire1.setSDA(PINS.I2C.sda);
	Wire1.setSCL(PINS.I2C.scl);
	Wire1.begin();// いらないけど明示しておく
	// ここは外部でpulldownする
	pinMode(PINS.intrpt, INPUT_PULLUP);

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
	setPropWH(&props.Temp,"00.0 c");
 	props.Temp.x = fromRight(props.Temp.width)-3;

	// 温度単位
	props.TempUnit = {
		0,
		0,
		props.Temp.size,
		props.Temp.font
	};
	setPropWH(&props.TempUnit, "c");
	props.TempUnit.x = fromRight(props.TempUnit.width);

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
	setPropWH(&props.Gear);
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
	props.Speed = {
		0,
		30+offsetY,
		1,
		&fonts::Font8
	};
	setPropWH(&props.Speed, "00");
	props.Speed.x = centerHorizontal(props.Speed.width);

	// 速度単位
	props.SpUnit = {
		0,
		110+offsetY,
		1,
		&fonts::Font4
	};
	setPropWH(&props.SpUnit,"km/h");
	props.SpUnit.x = centerHorizontal(props.SpUnit.width);

	// 速度センサーパルス数
	props.SpSensor.size=1;
	setPropWH(&props.SpSensor,"00000");
	props.SpSensor.x = fromRight(props.SpSensor.width);
	props.SpSensor.y = fromBottom(props.SpSensor.height)-1;

	// 速度センサー周波数
	props.SpFreq.size = 1;
	setPropWH(&props.SpFreq,"0000Hz");
	props.SpFreq.x = fromRight(props.SpFreq.width);
	props.SpFreq.y = props.SpSensor.y - props.SpFreq.height;

	// 初期表示メッセージ
	props.InitMsg = {
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
	pinMode(PINS.buzzer, OUTPUT);              // ウインカー音
	aht.begin(&Wire1,0,MODULES.thmst.address);// 温度計
	digitalWrite(PINS.buzzer, LOW);
	//rtc.adjust(DateTime(F(__DATE__),F(__TIME__))); // 時計合わせ

	// 画面リセット
	display.fillScreen(TFT_BLACK);
	// ギアポジション表示開始
	setDisplay(&props.Gear, "0");
	// 速度
	setDisplay(&props.Speed, "00");
	// 速度単位
	setDisplay(&props.SpUnit, "km/h");
	// 時間
	setDisplay(&props.Hour, "00:00:00");
	// 日付
	setDisplay(&props.Month, "00/00");
	// 温度
	setDisplay(&props.Temp, "00.0");
	// 温度単位
	setDisplay(&props.TempUnit, "c");
	display.fillCircle(306,6,3,TFT_WHITE);
	display.fillCircle(306,6,1,TFT_BLACK);
	// 湿度
	setDisplay(&props.Humid, "00%");
	// 速度センサカウンタ
	setDisplay(&props.SpSensor, "00000");
	// 速度センサ周波数
	setDisplay(&props.SpFreq, "0000Hz");

	// スプライト設定
	// 横縦
	int w = (props.Speed.y + 60 - offsetY+10) * 2;
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
	arcL.x = arcL.r+arcL.d+40+3*arcL.d;
	arcL.y = arcM.y;
	arcR.x = 0;
	arcR.y = arcM.y;
	// 角度
	int a0btm = 20;
	int a1top = 34;
	int a1btm = 38;
	int a2top = 44;
	int a2btm = 47;
	arcM.angle0 = 90  + a0btm;
	arcM.angle1 = 90  - a0btm;

	arcL.angle0 = 90  + a1btm;
	arcL.angle1 = 270 - a1top;

	arcL.angle2 = 90  + a2btm;
	arcL.angle3 = 270 - a2top;

	arcR.angle0 = 270 + a1top;
	arcR.angle1 = 90  - a1btm;

	arcR.angle2 = 270 + a2top;
	arcR.angle3 = 90  - a2btm;
	// 大きさ
	arcM.sprite.createSprite(w,h);
	arcL.sprite.createSprite(arcL.r+40+3*arcL.d+1,h);
	arcR.sprite.createSprite(arcR.r+40+3*arcR.d+1,h);
	// 色
	arcM.colorON=TFT_GREEN;
	arcL.colorON=TFT_YELLOW;
	arcR.colorON=TFT_YELLOW;
	// 出力
	arcM.displayArc(centerX,centerY+10,ON);
	// 補助線
	//display.drawFastHLine(0,centerY-rOUT+7,320,TFT_RED);
	//display.drawFastHLine(0,centerY+rOUT+6,320,TFT_RED);
	// 速度パルス
	tone(PINS.spPulse, spPulseFreq[2]);
	// 割り込み処理
	attachInterrupt(digitalPinToInterrupt(PINS.intrpt), interruptMethod, RISING);
}

// ------------------------------ループ------------------------------
void loop() {

	// 経過時間(ms)取得
	unsigned long time = millis();
	static int pulseIndex = 0;

	// 速度パルス切り替え
	if(spPulseChangeTime <= time){
		pulseIndex = (++pulseIndex) % spPulseSize;
		tone(PINS.spPulse, spPulseFreq[pulseIndex]);
		spPulseChangeTime = time + 5000;
	}

	// 速度計算・表示
	if(spTime <= time){
		displayFreq(spTime);
		spTime = time + SP_DURATION;
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
		displayTemp();
		uint16_t raw = ads.readADC_SingleEnded(3);
		String voltage = String((raw * 0.0001875f), 2);
		Serial.print("Voltage:");
		Serial.println(voltage);
		tempTime = time + TEMP_INTERVAL;
	}

	// 時刻表示
	if(timeTime <= time){
		displayRealTime();
		timeTime = time + TIME_INTERVAL;
	}
	
	// 各種表示処理
	if(displayTime <= time){
		// 速度カウンタ表示
		displaySpeed();
		// デバッグ用スイッチ表示
		displaySwitch(&pushSw);
		// ギア表示
		gearDisplay(gearPositions.getGear());
		// ウインカー点灯状態が切り替わった場合
		if(displayWinkers() == true && bzzTime == 0 ){
			// ブザーON
			digitalWrite(PINS.buzzer, HIGH);
			//digitalWrite(PINS.buzzer, LOW);
			//analogWrite(PINS.buzzer, 153);
			pixels.setPixelColor(0, pixels.Color(1,1,0));
			pixels.show();
			// 時間設定
			bzzTime = time + BUZZER_DURATION;
		}
		displayTime = time + DISPLAY_INTERVAL;
	}
	//ブザーOFF処理
	if(bzzTime != 0 && bzzTime <= time){
		digitalWrite(PINS.buzzer, LOW);
		//digitalWrite(PINS.buzzer, HIGH);
		//analogWrite(PINS.buzzer, 0);
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
		MODULES.thmst,
		MODULES.adCnv,
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
 * @param winkers Winkers型 ウインカークラス
 * @return isSwitchStatus bool型 左右いずれかが点灯状態が切り替わった場合true
 */
bool displayWinkers(){
	// バッファ状態
	static bool buffer[2] = {OFF, OFF};
	// 返却用フラグ
	bool isSwitched = false;

	arcInfo* arcArr[2] = {&arcL, &arcR};

	for(int side=LEFT; side<=RIGHT; side++){
		// 左右ウインカー状態を判定
		if(buffer[side] == winkers.getStatus(side)){
			continue;
		}
		// バッファ上書き
		buffer[side] = winkers.getStatus(side);
		// ディスプレイ表示処理
		arcArr[side]->displayArc(centerX,centerY+10,!buffer[side],true);
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
	static int brightLvMax = sizeof(brightLevel) / sizeof(byte);
	static int nowBrightLv = 0;
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
			nowBrightLv = (++nowBrightLv) % brightLvMax;
			display.setBrightness(brightLevel[nowBrightLv]);
			display.print(nowBrightLv);
			beforeSw = OFF;
		}
	}
}

/**
 * 温度表示
 * 
 */
void displayTemp(){
	static int beforeTempx10 = 0;
	static int beforeHumid = 0;
	// 温度取得
	sensors_event_t humidity, temp;
	aht.getEvent(&humidity,&temp);
	int newTempx10 = temp.temperature * 10;
	// 100度以上の場合は99.9度(変数では999)に修正
	if(1000 <= newTempx10){
		newTempx10 = 999;
	}
	else if(newTempx10 <= 0){
		newTempx10 = 0;
	}
	// 前回温度と同じ場合、スキップ
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
	int newHumid = humidity.relative_humidity;
	if(100 <= newHumid){
		newHumid = 99;
	}
	else if(newHumid <= 0){
		newHumid = 0;
	}
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
}

/**
 * 現在時刻表示処理
 *
 * @param totalSec long型 経過時間(秒)
 * @param tft Adafruit_ST7735クラス ディスプレイ設定
 * @param dispInfo 表示文字情報構造体 文字の座標と大きさ
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

/**
 * 速度センサカウンタの表示
 */
void displaySpeed(){
	// 表示設定
	setDisplay(&props.SpSensor);
	// 速度カウンタ表示
	displayNumber(&props.SpSensor, pulseTotalCounter, 5);
}

/**
 * 速度センサ周波数の表示
 */
void displayFreq(unsigned long spTime){
	// 前回単位時間当たりパルス数
	static unsigned int beforeCounter = 0;
	// 前回システム時間
	static unsigned long beforeSpTime;
	// 前回速度周波数
	static int beforeFreq = 0;
	// 前回速度
	static byte beforeSpeed = 0;
	// 今回速度周波数
	unsigned int freq = 0;

	if(beforeCounter != 0 && beforeSpTime != 0){
		// 前回単位時間当たりパルス数取得
		unsigned long pulseCounter = pulseTotalCounter - beforeCounter;
		// 単位時間取得
		unsigned long pulseTime = spTime - beforeSpTime;
		// 周波数取得
		freq = pulseCounter * 1000 / pulseTime;
		// 表示設定
		setDisplay(&props.SpFreq);
		// 速度周波数表示
		displayNumber(&props.SpFreq, freq, 4);

		// 速度換算（100km/h超の場合でも3桁目非表示）
		byte speed = (freq / 10) % 100;
		// 前回速度と比較
		if(speed != beforeSpeed){
			// 速度表示
			displayNumber(&props.Speed, speed, 2);
			beforeSpeed = speed;
		}
		beforeFreq = freq;
	}
	// カウンタ
	beforeCounter = pulseTotalCounter;
	// システム時間
	beforeSpTime = spTime;
}

/**
 * 割り込み処理
 */
void interruptMethod(){
	// カウンタインクリメント
	pulseTotalCounter++;
	if(100000 <= pulseTotalCounter){
		pulseTotalCounter = 0;
	}
}

/**
 * 値の表示（右0埋め）
 * 
 * @param p Prop型 表示設定
 * @param valueByte byte型 表示値
 * @param digitNum int型 表示桁数
 */
void displayNumber(Prop* p, byte valueByte, int digitNum){
	// 表示設定
	setDisplay(p);
	// 速度周波数表示
	for(int digit = pow(10, digitNum-1); 1 < digit; digit /= 10 ){
		if(valueByte / digit == 0){
			display.print('0');
		}
		else{
			break;
		}
	}
	display.print(valueByte);
}

/**
 * 値の表示（右0埋め）
 * 
 * @param p Prop型 表示設定
 * @param valueLong long型 表示値
 * @param digitNum int型 表示桁数
 */
void displayNumber(Prop* p, unsigned int valueInt, int digitNum){
	// 表示設定
	setDisplay(p);
	// 速度周波数表示
	for(int digit = pow(10, digitNum-1); 1 < digit; digit /= 10 ){
		if(valueInt / digit == 0){
			display.print('0');
		}
		else{
			break;
		}
	}
	display.print(valueInt);
}