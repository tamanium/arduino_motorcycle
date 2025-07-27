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


// --------------------モード切替用定義--------------------
//#define BUZZER_ON
#define DEBUG_MODE
#define TIMER_SET

// --------------------定数--------------------
// 明るさレベル
const uint8_t brightLevel[] = {
	0x01, 0x10, 0x20, 0x40, 0x80
};

// --------------------列挙型--------------------
// モジュール側も同じ定義
enum {
	INDEX_FREQ,          // パルス周波数
	INDEX_GEARS,         // ギアポジADC値
	INDEX_WINKERS,       // ウインカービット値
	INDEX_SWITCH,        // スイッチビット値
	INDEX_VOLT,          // 電圧ADC値

	DATA_SIZE,           // 配列要素数
	INDEX_A_PART = 0x40, // 電圧と出力パルス以外のデータを要求する値
	INDEX_ALL = 0xFF,    // 全てのデータを要求する値(イラナイかも)
};

// ウインカー値
enum{
	INDICATE_NONE,
	INDICATE_LEFT,
	INDICATE_RIGHT,
	INDICATE_BOTH = INDICATE_LEFT | INDICATE_RIGHT,
};

// --------------------変数--------------------
// センサーからの取得値
int moduleData[DATA_SIZE];
// データクラス：スイッチ
DataClass switchData(true);
// データクラス：ウインカーADC値
DataClass winkersData(false);

// ディスプレイ
LGFX display;
// オンボLED
Adafruit_NeoPixel pixels(1, Pins::LED);
// RTC
RTC_DS1307 rtc;
// 温湿度計
Adafruit_AHTX0 aht;

// フォント色
uint16_t textColor = TFT_WHITE;
// 背景色
uint16_t bgColor = TFT_BLACK;

// 円弧表示情報
struct MeterArcInfo : ArcInfo {
	// スプライト
	LGFX_Sprite sprite;
	// コンストラクタ
	MeterArcInfo(LGFX* display) : sprite(display) {}
	// 初期設定
	void initArc() {
		this->sprite.fillScreen(this->colorBG);
		this->sprite.setPivot(this->x, this->y);
	}
};

// 円弧情報
MeterArcInfo arcM(&display);
ArcInfo arcL;
ArcInfo arcR;

// --------------------プロトタイプ宣言--------------------
void setDisplay(Prop* p, uint16_t color = textColor, uint16_t markColor = 1);
void displayNumberln(int valueInt, byte digiNum, bool spacerZero=false);
void displayNumberln(Prop* p, int valueInt, byte digitNum, bool spacerZero=false);
void displayNumberln(Prop* p, byte valueByte, byte digitNum, bool spacerZero=false);
void displayArcM(ArcInfo* a, int stdX, int stdY, byte sp = 0);

// --------------------インスタンス--------------------
// 表示設定まとめ
struct Props {
	// 時:分
	Prop Clock;
	// 温度
	Prop Temp;
	// 湿度
	Prop Humid;
	// ギア
	Prop Gear;
	// ギアニュートラル
	//Prop Newt;
	// 速度
	Prop Speed;
	// 速度センサカウンタ
	//Prop SpFreqIn;
	// 電圧
	Prop Voltage;
	// デバッグ用値表示
	Prop DebugData;
} props;


// ------------------------------初期設定------------------------------
void setup(void) {
	delay(100);

	// I2C設定
	Wire1.setSDA(Pins::I2C_SDA);
	Wire1.setSCL(Pins::I2C_SCL);
	Wire1.setClock(400000);
	Wire1.begin();

	int offsetY = 50;

	// 各表示情報の初期化
	initSetProps(offsetY);

	// ディスプレイの初期化
	display.init();
	// 明るさ
	display.setBrightness(brightLevel[2]);

	// ブザー連動LED設定
	pixels.begin();
	pixels.setPixelColor(0, pixels.Color(1, 1, 0));
	pixels.show();
	
	// ウインカー設定
	#ifdef BUZZER_ON
		// ピンモード定義
		pinMode(Pins::BUZZER, OUTPUT);
		//3回鳴らす
		for(int i=0;i<3;i++){
			setBuzzer(ON);
			delay(80);
			setBuzzer(OFF);
			delay(80);
		}
	#endif
	// I2C通信スキャン
	scanModules();

	// 各モジュール動作開始
	// RTC
	rtc.begin(&Wire1);
	// 温度計
	aht.begin(&Wire1, 0, modules[THERM].address);

	// 時計合わせ
	#ifdef TIMER_SET
		rtc.adjust(DateTime(F(__DATE__),F(__TIME__))); 
	#endif

	// 現在日時表示
	display.setTextSize(2);
	display.print("Date: ");
	if(modules[RTCMM].active){
		DateTime now = rtc.now();
		display.print(now.year());
		display.print('/');
		display.print(now.month());
		display.print('/');
		display.print(now.day());
		display.print(' ');
		display.print(now.hour());
		display.print(':');
		display.println(now.minute());
	}
	else{
		display.println("----/--/-- --:--");
	}
	// バッテリー電圧表示
	display.print("Batt: ");
	if(modules[SPEED].active){
		// 電圧ADC取得・算出
		// Vcc=5.22, 分圧逆数=3.05, 倍率10 => 係数=159
		byte voltagex10 = (getData(INDEX_VOLT) * 159) / 1023;
		char valueChars[6];
		sprintf(valueChars, "%2d.%01dV", int(voltagex10/10), voltagex10%10);
		display.println(valueChars);
	}
	else{
		display.println("--.-V");
	}
	display.println("");
	delay(2000);
	display.print("3 ");
	delay(1000);
	display.print("2 ");
	delay(1000);
	display.print("1 ");
	delay(1000);
	display.print("Start");
	delay(1000);

	// 各項目の初期表示
	initDisplayProps();
	// スプライト設定
	// 横縦
	int w = (135 - offsetY + 10) * 2;
	int h = w;
	// 弧の幅
	arcM.d = 10;
	arcL.d = 10;
	arcR.d = 10;
	// 弧の内外半径
	arcM.r = ((w-1)>>1) - arcM.d;
	arcL.r = arcM.r + 25;
	arcR.r = arcM.r + 25;
	// 弧の中心座標
	arcM.x = w >> 1;
	arcM.y = h >> 1;
	// arcL.x = CENTER_WIDTH;
	arcL.x = 0;
	arcL.y = CENTER_HEIGHT + 10;
	
	//arcR.x = CENTER_WIDTH;
	arcR.x = DisplaySize::WIDTH-1;
	arcR.y = CENTER_HEIGHT + 10;
	// 角度
	int a0btm = 25;
	int a1top = 37;
	int a1btm = 46;
	arcM.angle0 = 90  + a0btm;
	arcM.angle1 = 90  - a0btm;
	arcL.angle0 = 90  + a1btm;
	arcL.angle1 = 270 - a1top;
	arcR.angle0 = arcL.angle0 + 180;
	arcR.angle1 = arcL.angle1 - 180;
	// 色
	arcM.colorON = TFT_GREEN;
	arcL.colorON = TFT_YELLOW;
	arcR.colorON = TFT_YELLOW;
	// 大きさ
	arcM.sprite.createSprite(w, h);
	// 弧の中心・背景色
	arcM.initArc();

	arcL.h = 50;
	arcL.w = 40;
	arcR.h = 50;
	arcR.w = 40;
	// 補助線
	//display.drawFastHLine(0,CENTER_Y-rOUT+7,320,TFT_RED);
	//display.drawFastHLine(0,CENTER_Y+rOUT+6,320,TFT_RED);
}


// ------------------------------ループ------------------------------
void loop() {
	
	static Interval intervalDisplay(20);    // 表示用時間
	static Interval intervalMonitor(3);     // 各種読み取り用時間
	static Interval intervalTemp(20000);    // 温度表示用時間
	static Interval intervalVoltage(60000); // 電圧表示用時間
	static Interval intervalTime(30);       // 時刻表示用時間
	static Interval intervalBzz(100);       // ブザー用時間

	// 経過時間(ms)取得
	unsigned long time = millis();

	// 各種モニタリング・更新
	if (intervalMonitor.over(time)) {
		// 周波数、ギアポジ、ウインカー、スイッチの値を取得
		getDataA();
		switchData.setData(moduleData[INDEX_SWITCH]);

		// デバッグモード表示
		#ifdef DEBUG_MODE
			static int loopTimeMax = 0;
			static byte countLoop = 0;
			static unsigned long beforeTime = 0;

			if(70 < ++countLoop){
				loopTimeMax = 0;
				countLoop = 0;
			}

			int loopTime = (int)(time - beforeTime);
			if(100<loopTime){
				loopTime = 99;
			}
			if(loopTimeMax < loopTime){
				loopTimeMax = loopTime;
			}

			beforeTime = time;
			
			setDisplay(&props.DebugData, textColor);
			display.println("");
			display.print("loop :");
			// 表示値を取得
			char valueStr[6] = "";
			sprintf(valueStr, "%2d-%2d", loopTime, loopTimeMax);
			display.println(valueStr);
			display.print("FreqI:");
			displayNumberln(moduleData[INDEX_FREQ], 5);
			display.print("vltAD:");
			displayNumberln(moduleData[INDEX_VOLT], 5);
			display.print("wnkAD:");
			displayNumberln(moduleData[INDEX_WINKERS], 5);
			display.print("geaAD:");
			
			for(int i=0; i<5; i++){
				valueStr[i] = (moduleData[INDEX_GEARS] & (1<<i)) ? '1' : '0';
			}
			display.println(valueStr);
		#endif
		intervalMonitor.reset();
	}

	// 温度モニタリング・表示
	if (intervalTemp.over(time)) {
		#ifdef DEBUG_MODE
			setDisplay(&props.DebugData, textColor);
			display.println("temp");
		#endif
		displayTemp();
		intervalTemp.reset();
	}

	// 電圧モニタリング・表示
	if (intervalVoltage.over(time)) {
		#ifdef DEBUG_MODE
			setDisplay(&props.DebugData, textColor);
			display.println("vold");
		#endif
		displayVoltage();
		intervalVoltage.reset();
	}

	// 時刻表示
	if (intervalTime.over(time)) {
		#ifdef DEBUG_MODE
			setDisplay(&props.DebugData, textColor);
			display.println("time");
		#endif
		displayRealTime();
		intervalTime.reset();
	}

	// 各種表示処理
	if (intervalDisplay.over(time)) {
		#ifdef DEBUG_MODE
			setDisplay(&props.DebugData, textColor);
			display.println("spd ");
		#endif
		// 速度表示
		displaySpeed();
		// デバッグ用スイッチ表示
		displaySwitch();
		#ifdef DEBUG_MODE
			setDisplay(&props.DebugData, textColor);
			display.println("gear");
		#endif
		// ギア表示
		displayGear();
		#ifdef DEBUG_MODE
			setDisplay(&props.DebugData, textColor);
			display.println("wnkr");
		#endif
		// ウインカー表示・ブザー出力
		if (displayWinkers() && intervalBzz.isZero()) {
			#ifdef DEBUG_MODE
				setDisplay(&props.DebugData, textColor);
				display.println("bzOn");
			#endif
			setBuzzer(ON);
			intervalBzz.reset();
		}
		intervalDisplay.reset();
	}

	//ブザーOFF処理
	if (!intervalBzz.isZero() && intervalBzz.over(time)) {
		#ifdef DEBUG_MODE
			setDisplay(&props.DebugData, textColor);
			display.println("bzOf");
		#endif
		setBuzzer(OFF);
		intervalBzz.setZero();
	}
}

// -------------------------------------------------------------------
// ------------------------------メソッド-----------------------------
// -------------------------------------------------------------------

/**
 * 表示（ウインカー向け）
 */
void displayArcW(ArcInfo* a, bool onOff) {
	// 弧描画 on,offで色変更
	// display.fillArc(a->x, a->y, a->r + a->d, a->r, a->angle0,a-> angle1, onOff ? a->colorON : bgColor);
	// display.fillRect(a->opt,0,CENTER_WIDTH,8, onOff ? a->colorON : bgColor);
	display.fillTriangle(a->x, a->y, 
	a->x < DisplaySize::CENTER_WIDTH ? 
	a->x+a->h : a->x-a->h,
	a->y+a->w/2,
	a->x < DisplaySize::CENTER_WIDTH ? 
	a->x+a->h : a->x-a->h,
	a->y-a->w/2,
	onOff ? a->colorON : bgColor);
}

/**
 * 表示（メーター向け）
 */
void displayArcM(MeterArcInfo* a, int stdX, int stdY, byte sp) {
	// 前回速度
	static byte beforeSp = 0xFF;

	// 初期処理
	if(beforeSp == 0xFF){
		// 弧描画（薄緑）
		a->sprite.fillArc(a->x, a->y, a->r + a->d, a->r, a->angle0, a->angle1, 0x01e0);
		a->sprite.pushRotateZoom(stdX, stdY, 0, 1, 1, a->colorBG);
		beforeSp = 0;
		return;
	}
	// 速度が同じ場合、スキップ
	if(beforeSp == sp){
		return;
	}
	// 速さに対する弧の角度算出
	int angleSp = (360 - a->angle0 + a->angle1) * sp / 120;
	int newAngle0 = a->angle0;
	int newAngle1 = a->angle1;
	uint16_t newColor = a->colorON;
	if(beforeSp < sp){
		// 速度が上がった場合
		newAngle1 = a->angle0 + angleSp;
	}
	else{
		// 速度が下がった場合
		newAngle0 = a->angle0 + angleSp;
		// 色を薄緑に変更
		newColor = 0x01e0;
	}
	// 出力
	a->sprite.fillArc(a->x, a->y, a->r + a->d, a->r, newAngle0, newAngle1, newColor);
	a->sprite.pushRotateZoom(stdX, stdY, 0, 1, 1, a->colorBG);
	beforeSp = sp;
}

/**
 * 表示情報の初期化
 */
void initSetProps(int offsetY){
	// 時:分
	props.Clock.font = &fonts::Font4;
	setPropWH(&props.Clock, "00");
	props.Clock.y += 25;

	// 温度
	props.Temp = propCopy(&props.Clock);
	setPropWH(&props.Temp, "00 c");
	alignRight(&props.Temp, 3);

	// 湿度
	props.Humid = propCopy(&props.Temp, UNDER);
	setPropWH(&props.Humid, "00%");
	alignRight(&props.Humid);

	// 速度
	props.Speed.y = 10 + offsetY;
	props.Speed.font = &fonts::Font7;
	setPropWH(&props.Speed, "00");
	props.Speed.x = centerHorizontal(props.Speed.width);

	// 速度単位
	Prop SpUnit = propCopy(&props.Speed, UNDER, &fonts::Font2);
	setPropWH(&SpUnit, "km/h");

	// ギア
	props.Gear = propCopy(&SpUnit, UNDER);
	props.Gear.y += 10;
	props.Gear.font = &fonts::Font8;
	setPropWH(&props.Gear, "0");
	props.Gear.x = centerHorizontal(props.Gear.width);

	// ギアニュートラル
	//props.Newt = propCopy(&props.Gear);
	//setPropWH(&props.Newt, "N");
	//props.Newt.x = centerHorizontal(props.Newt.width);

	// スピードセンサIN
	//props.SpFreqIn = propCopy(&SpUnit, UNDER, &fonts::Font7);
	//setPropWH(&props.SpFreqIn, "0000");
	//props.SpFreqIn.x = centerHorizontal(props.SpFreqIn.width);

	// 電圧
	props.Voltage.font = &fonts::Font4;
	setPropWH(&props.Voltage, "00.0V");
	alignRight(&props.Voltage);
	alignBottom(&props.Voltage);

	// デバッグ用表示
	setPropWH(&props.DebugData, "00");
	alignBottom(&props.DebugData, props.DebugData.height * 8);
}

/**
 * 初期表示表示
 */
void initDisplayProps(){
	// 画面リセット
	display.fillScreen(bgColor);
	// 湿度
	displayString(&props.Humid, "00%");
	// 電圧
	displayString(&props.Voltage, "00.0V");
	
	// 速度単位
	Prop SpUnit = propCopy(&props.Speed, UNDER, &fonts::Font2);
	setPropWH(&SpUnit, "km/h");
	SpUnit.x = centerHorizontal(SpUnit.width);
	displayString(&SpUnit, "km/h");

	// 温度単位
	Prop TempUnit = propCopy(&props.Temp);
	setPropWH(&TempUnit, "c");
	alignRight(&TempUnit, 3);
	displayString(&TempUnit, "c");       // 温度単位
	display.fillCircle(303, props.Temp.y + 6, 3, textColor);
	display.fillCircle(303, props.Temp.y + 6, 1, bgColor);
	
	// スピードセンサIN単位
	//Prop SpFreqInUnit = propCopy(&props.SpFreqIn, UNDER, &fonts::Font2);
	//setPropWH(&SpFreqInUnit, "Hz");
	//SpFreqInUnit.x = centerHorizontal(SpFreqInUnit.width);

	//displayString(&SpFreqInUnit, "Hz");  // パルス周波数単位
}

/**
 * ブザー鳴らす
 */
void setBuzzer(bool isOn){
	// ブザーON
	#ifdef BUZZER_ON
		digitalWrite(Pins::BUZZER, isOn);
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
 * ディスプレイ表示設定
 * @param p 表示設定
 */
void setDisplay(Prop* p, uint16_t color, uint16_t markColor) {
	// 描画位置
	display.setCursor(p->x, p->y);
	// テキスト倍率
	display.setTextSize(p->size);
	// フォント色指定
	display.setTextColor(color, (markColor == 1) ? bgColor : markColor);
	// フォントセット
	display.setFont(p->font);
}

/**
 * フォントの縦横を設定
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

	// 初期表示メッセージ
	display.fillScreen(bgColor);
	display.setCursor(0,0);
	display.setTextSize(1);
	display.setTextColor(textColor, bgColor);
	display.println("Hello");
	display.println("");
	delay(500);
	// スキャン処理
	display.println("I2C Module Scanning...");
	for (int i=0; i<MODULE_NUM; i++) {
		Wire1.beginTransmission(modules[i].address);
		byte error = Wire1.endTransmission();
		display.setTextColor(textColor, bgColor);
		display.print(modules[i].name);
		display.print(':');
		display.setTextColor((error == 0) ? TFT_GREEN : TFT_RED, TFT_BLACK);
		display.println((error == 0) ? "OK" : "NG");
		modules[i].active = (error == 0);
	}
	display.setTextColor(textColor);
	display.println("");
	display.println("-- done --");
	display.println("");
}

/**
 * ギアポジションの表示処理
 */
void displayGear() {
	// フォントが対応するまで'N'を'0'、'0'を'-'に置き換え
	const char NEWTRAL = '0';
	const char OUT_GEAR = '-';

	static char before = NEWTRAL;
	char gearArr[5] = {'3', '2', NEWTRAL, '4', '1'};
	char gear = NEWTRAL;
	// 現在のギアポジを取得
	for(int i=0; i<5; i++){
		if(moduleData[INDEX_GEARS] & (1<<i)){
			gear = gearArr[i];
			break;
		}
	}
	// 前回と同じ場合、スキップ
	if(gear == before){
		return;
	}
	// 0の場合（ギア抜け）
	else if(gear == OUT_GEAR){
		// グレーで前回ギアを表示
		setDisplay(&props.Gear, TFT_DARKGREY);
		gear = before;
	}
	// 0～4の場合
	else{
		setDisplay(&props.Gear, (gear!=NEWTRAL) ? textColor : TFT_GREEN);
	}
	// 表示
	display.print(gear);
	// 前回値に今回値を代入
	before = gear;
}

/**
 * ウインカー表示処理
 */
bool displayWinkers() {
	// 前回値
	static byte beforeStatus = INDICATE_NONE;
	// 閾値
	int thresholdArr[] = {480, 650, 900};

	// 現在値取得
	int nowData = moduleData[INDEX_WINKERS];
		#ifdef DEBUG_MODE
			setDisplay(&props.DebugData, textColor);
			display.println("wnkr1");
		#endif
	byte nowStatus = INDICATE_NONE;

	// ハザード機能つけたら以下コメント解除
	//if(nowData < thresholdArr[0]){
	//	nowStatus = INDICATE_BOTH;
	//}
	//else if (nowData < thresholdArr[1]){
	if (nowData < thresholdArr[1]){
		nowStatus = INDICATE_LEFT;
	}
	else if(nowData < thresholdArr[2]){
		nowStatus = INDICATE_RIGHT;
	}

	// 前回値と同じ場合、処理終了
	if (nowStatus == beforeStatus) {
		return false;
	}

	#ifdef DEBUG_MODE
		setDisplay(&props.DebugData, textColor);
		display.println("wnkr2");
	#endif
	//左ウインカーの表示
	bool onOff = ((nowStatus & INDICATE_LEFT) == INDICATE_LEFT);
	displayArcW(&arcL, onOff);
	
	#ifdef DEBUG_MODE
		setDisplay(&props.DebugData, textColor);
		display.println("wnkr3");
	#endif
	//右ウインカーの表示
	onOff = ((nowStatus & INDICATE_RIGHT) == INDICATE_RIGHT);

	#ifdef DEBUG_MODE
		setDisplay(&props.DebugData, textColor);
		display.println("wnkr3");
	#endif
	displayArcW(&arcR, onOff);

	beforeStatus = nowStatus;
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
	display.setCursor(0, DisplaySize::HEIGHT - 8*2 - 1);

	// キーダウンの場合
	if (nowSw) {
		display.setTextColor(TFT_RED, bgColor);
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
			display.setTextColor(TFT_BLUE, bgColor);
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
	//static int beforeFreq = -1;
	// 前回スピード
	static byte beforeSpeed = 0xFF;

	// 周波数表示
	//if (moduleData[INDEX_FREQ] != beforeFreq) {
	//	displayNumberln(&props.SpFreqIn, moduleData[INDEX_FREQ], 4, true);
	//	beforeFreq = moduleData[INDEX_FREQ];
	//}
	// 速度表示
	byte speed = byte(moduleData[INDEX_FREQ] * 2 / 25);
	if(100 <= speed){
		speed = 99;
	}
	if (speed != beforeSpeed) {
		displayNumberln(&props.Speed, speed, 2, true);
		displayArcM(&arcM, CENTER_WIDTH, CENTER_HEIGHT + 10, speed);
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
	moduleData[INDEX_VOLT] = adcValue;
	// 電圧算出
	// Vcc=5.22, 分圧逆数=3.05, 倍率10 => 係数=159
	byte voltagex10 = (adcValue * 159) / 1023;
	if (voltagex10 != beforeVoltagex10) {
		// 電圧表示
		setDisplay(&props.Voltage, textColor);
		char valueChars[5];
		sprintf(valueChars, "%2d.%01d", int(voltagex10/10), voltagex10%10);
		display.print(valueChars);
		//valueChars[0] = '.';
		//valueChars[1] = '0' + voltagex10%10;
		//display.print(valueChars);
		beforeVoltagex10 = voltagex10;
	}
}

/**
 * 温度・湿度表示
 */
void displayTemp() {
	static byte beforeTemp = 0;
	static byte beforeHumid = 0;
	// 温度取得
	sensors_event_t humidity, temp;
	aht.getEvent(&humidity, &temp);
	byte newTemp = byte(temp.temperature);
	if (beforeTemp != newTemp) {
		displayNumberln(&props.Temp, newTemp, 2);
		beforeTemp = newTemp;
	}
	byte newHumid = byte(humidity.relative_humidity);
	if (beforeHumid != newHumid) {
		displayNumberln(&props.Humid, newHumid, 2);
		beforeHumid = newHumid;
	}
}
/**
 * 時分(秒)表示
 */
void displayRealTime(){
	// 時刻データ数
	const int itemLen = 3;
	// 前回日時
	static uint8_t beforeTime[itemLen] = { 60, 60, 60};
	// 現在時刻取得
	DateTime now = rtc.now();
	if(now.minute() != beforeTime[1]){
		// 現在時刻を配列化
		uint8_t newTime[itemLen] = {
			now.second(),
			now.minute(),
			now.hour()
		};
		// 表示設定セット
		setDisplay(&props.Clock);
		// 表示更新分の文字列
		char valueChars[8];
		// 表示文字列取得
		sprintf(valueChars, "%02d:%02d", newTime[2],newTime[1]);
		// 表示
		display.print(valueChars);
		// 前回値を更新
		memcpy(beforeTime, newTime, itemLen);
	}
}

/**
 * 月日時分秒表示（未使用）
 */
void displayRealDateTime(){
	// 時刻データ数
	const int itemLen = 5;
	// 前回日時
	static uint8_t beforeTime[itemLen] = { 60, 60, 60, 60, 60 };
	// 現在時刻取得
	DateTime now = rtc.now();
	// 秒の値が前回と同じ場合スキップ
	if (beforeTime[0] == now.second()) {
		return;
	}
	// 現在時刻を配列化
	uint8_t newTime[itemLen] = {
		now.second(),
		now.minute(),
		now.hour(),
		now.day(),
		now.month()
	};

	// 表示更新分の文字列
	String resultStr = "";
	// 表示情報セット
	setDisplay(&props.Clock, textColor);
	// 初期処理
	if(60 <= beforeTime[0]){
		char defaultStr[12];
		sprintf(defaultStr, "%02d:%02d:%02d",newTime[2],newTime[1],newTime[0]);
		display.print(defaultStr);
		// 前回日時を更新
		memcpy(beforeTime, newTime, itemLen);
		return;
	}
	// 表示処理
	for(int i=0; i<3; i++){
		// 0以外の場合
		if(newTime[i] != 0){
			// 10の倍数なら2桁とも。そうでなければ1桁だけ
			byte value = (newTime[i]%10 == 0) ? newTime[i] : newTime[i]%10;
			resultStr = String(value) + resultStr;
			break;
		}
		// 0の場合
		resultStr = "00" + resultStr;
		if(i!=3-1){
			resultStr = ":" + resultStr;
		}
	}
	// offsetの値を文字数から文字幅に変換
	String templateClock = "00:00:00";
	int offset = resultStr.length();
	templateClock.remove(templateClock.length()-offset, offset);
	offset = display.textWidth(templateClock);
	// カーソルセット
	display.setCursor(props.Clock.x + offset, props.Clock.y);
	// 値出力　
	display.print(resultStr);
}

/**
 * 表示の設定・文字の表示
 *
 * @param p Prop型 表示設定
 * @param str 文字列
 */
void displayString(Prop* p, String str){
	setDisplay(p, textColor);
	display.print(str);
}

/**
 * 値の表示（左0埋め）開発用
 * 
 * @param p Prop型 表示設定
 * @param valueLong long型 表示値
 * @param digitNum int型 表示桁数
 */
void displayNumberln(Prop* p, int valueInt, byte digitNum, bool spacerZero) {
	setDisplay(p, textColor);
	displayNumberln(valueInt, digitNum, spacerZero);
}

/**
 * 値の表示（左0埋め）
 * 
 * @param p Prop型 表示設定
 * @param valueByte byte型 表示値
 * @param digitNum int型 表示桁数
 */
void displayNumberln(Prop* p, byte valueByte, byte digitNum, bool spacerZero) {
	setDisplay(p, textColor);
	displayNumberln(int(valueByte), digitNum, spacerZero);
}

/**
 * 値の改行表示（左0埋め）開発用
 * 
 * @param p Prop型 表示設定
 * @param digitNum int型 表示桁数
 * @param spacerZero 0埋めするかどうか
 */
void displayNumberln(int valueInt, byte digitNum, bool spacerZero){
	// 表示指定子を設定
	String _format = (spacerZero) ? "%0" : "%";
	_format += char('0' + digitNum);
	_format += 'd';
	const char* format = _format.c_str();
	// 表示値を取得
	char valueStr[5] = "";
	sprintf(valueStr, format, valueInt);
	// 表示
	display.println(valueStr);
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
		moduleData[INDEX_FREQ]    = (Wire1.read() << 8) | Wire1.read();
		moduleData[INDEX_GEARS]   = (Wire1.read() << 8) | Wire1.read();
		moduleData[INDEX_WINKERS] = (Wire1.read() << 8) | Wire1.read();
		moduleData[INDEX_SWITCH]  = (Wire1.read() << 8) | Wire1.read();
	}
}

/**
 * モジュール(ATTINY1604)へのリクエスト
 *
 * @param reg int型 送信レジスト値
 * @param numByte int型 データ容量(byte)
 */
void requestSpeedModule(int reg, int numByte){
	Wire1.beginTransmission(modules[SPEED].address);
	Wire1.write(reg);
	Wire1.endTransmission(false);
	Wire1.requestFrom(modules[SPEED].address, numByte);
}