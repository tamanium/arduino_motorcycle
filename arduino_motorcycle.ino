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
#include "FontGear55x75.h" // 自作フォント

const lgfx::v1::RLEfont FontG = { chrtbl_fg, widtbl_fg, 0, chr_hgt_fg, baseline_fg };

// --------------------モード切替用定義--------------------
#define BUZZER_ON
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
struct ArcInfo : ShapeInfo {
	// 角度0,1
	int angle0;
	int angle1;
};

// 各種図形情報
ArcInfo arcM;
ShapeInfo triangleL;
ShapeInfo triangleR;

// --------------------プロトタイプ宣言--------------------
void setDisplay(Prop* p, uint16_t color = textColor, uint16_t markColor = 1);
void displayNumberln(int valueInt, byte digiNum, bool spacerZero=false);
void displayNumberln(Prop* p, int valueInt, byte digitNum, bool spacerZero=false);

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
		
		char valueChars[6];
		sprintf(valueChars, "%02d/%02d", now.month(), now.month());
		display.print(valueChars);
		display.print(' ');
		sprintf(valueChars, "%02d:%02d", now.hour(), now.minute());
		display.print(valueChars);
	}
	else{
		display.println("----/--/-- --:--");
	}
	display.println("");
	// バッテリー電圧表示
	display.print("Batt: ");
	if(modules[SPEED].active){
		// 電圧ADC取得・算出
		// Vcc=5.22, 分圧逆数=3.05, 倍率10 => 係数=159
		byte voltagex10 = (getData(INDEX_VOLT) * 159) / 1023;
		char valueChars[6];
		sprintf(valueChars, "%02d.%01dV", int(voltagex10/10), voltagex10%10);
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
	// 横縦
	int w = (135 - offsetY + 10) * 2;
	// 弧の幅
	arcM.d = 10;
	// 弧の内外半径
	arcM.r = ((w-1)>>1) - arcM.d;
	// 弧の中心座標
	arcM.x = CENTER_WIDTH;
	arcM.y = CENTER_HEIGHT + 10;
	// 角度
	int a0btm = 25;
	arcM.angle0 = 90 + a0btm;
	arcM.angle1 = 90 - a0btm;
	// 色
	arcM.colorON = TFT_GREEN;

	// 底辺
	triangleL.d = 40;
	triangleR.d = 40;
	// 高さ
	triangleL.r = 50;
	triangleR.r = 50;
	// 外角の座標
	triangleL.x = 0;
	triangleL.y = CENTER_HEIGHT + 10;
	triangleR.x = DisplaySize::WIDTH-1;
	triangleR.y = CENTER_HEIGHT + 10;
	// 色
	triangleL.colorON = TFT_YELLOW;
	triangleR.colorON = TFT_YELLOW;
	// 左右の定数
	triangleL.LR = INDICATE_LEFT;
	triangleR.LR = INDICATE_RIGHT;
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
	static Interval intervalTime(1000);       // 時刻表示用時間
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
			sprintf(valueStr, "%2d-%2d", loopTime%100, loopTimeMax%100);
			display.println(valueStr);
			display.print("FreqI:");
			displayNumberln(moduleData[INDEX_FREQ], 5);
			display.print("vltAD:");
			displayNumberln(moduleData[INDEX_VOLT], 5);
			display.print("wnkAD:");
			displayNumberln(moduleData[INDEX_WINKERS]%100000, 5);
			display.print("geaAD:");
			
			for(int i=0; i<5; i++){
				valueStr[i] = (moduleData[INDEX_GEARS] & (1<<i)) ? '1' : '0';
			}
			display.println(valueStr);
		#endif
		intervalMonitor.reset(time);
	}

	// 温度モニタリング・表示
	if (intervalTemp.over(time)) {
		#ifdef DEBUG_MODE
			setDisplay(&props.DebugData, textColor);
			display.println("temp");
		#endif
		displayTemp();
		intervalTemp.reset(time);
	}

	// 電圧モニタリング・表示
	if (intervalVoltage.over(time)) {
		#ifdef DEBUG_MODE
			setDisplay(&props.DebugData, textColor);
			display.println("vold");
		#endif
		displayVoltage();
		intervalVoltage.reset(time);
	}

	// 時刻表示
	if (intervalTime.over(time)) {
		#ifdef DEBUG_MODE
			setDisplay(&props.DebugData, textColor);
			display.println("time");
		#endif
		displayRealTime();
		intervalTime.reset(time);
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
			intervalBzz.reset(time);
		}
		intervalDisplay.reset(time);
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
void displayArcW(ShapeInfo* s, byte nowStatus){
	bool onOff = ((nowStatus & s->LR) == s->LR);
	// on,offで色変更
	// xyは外側の点の座標、rは高さ、dは底辺長さ
	display.fillTriangle(s->x, s->y,
	s->x < DisplaySize::CENTER_WIDTH ? s->x+s->r : s->x-s->r,
	s->y+s->d/2,
	s->x < DisplaySize::CENTER_WIDTH ? s->x+s->r : s->x-s->r,
	s->y-s->d/2,
	onOff ? s->colorON : bgColor);
}

void displayMeter(byte sp){
	// 前回速度
	static byte beforeSp = 0xFF;
	// 速度が同じ場合、スキップ
	if(beforeSp == sp){
		return;
	}
	// 速さに対する弧の角度算出
	int angleSp = (360 - arcM.angle0 + arcM.angle1) * sp / 120;
	int newAngle0 = arcM.angle0;
	int newAngle1 = arcM.angle1;
	uint16_t newColor = arcM.colorON;
	if(beforeSp < sp){
		// 速度が上がった場合
		newAngle1 = arcM.angle0 + angleSp;
	}
	else{
		// 速度が下がった場合
		newAngle0 = arcM.angle0 + angleSp;
		// 色を薄緑に変更
		newColor = 0x01e0;
	}
	// 出力
	display.fillArc(arcM.x, arcM.y, arcM.r + arcM.d, arcM.r, newAngle0, newAngle1, newColor);
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
	props.Gear.font = &FontG;
	setPropWH(&props.Gear, "0");
	props.Gear.x = centerHorizontal(props.Gear.width);

	// ギアニュートラル
	//props.Newt = propCopy(&props.Gear);
	//setPropWH(&props.Newt, "N");
	//props.Newt.x = centerHorizontal(props.Newt.width);

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
	
	// メーターの弧描画（薄緑）
	display.fillArc(arcM.x, arcM.y, arcM.r + arcM.d, arcM.r, arcM.angle0, arcM.angle1, 0x01e0);
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
	const char NEWTRAL = 'N';
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
	if(gear == OUT_GEAR){
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
	#ifdef DEBUG_MODE
		setDisplay(&props.DebugData, textColor);
		display.println("wnkr3");
	#endif
	displayArcW(&triangleL, nowStatus);
	
	//右ウインカーの表示
	#ifdef DEBUG_MODE
		setDisplay(&props.DebugData, textColor);
		display.println("wnkr4");
	#endif
	displayArcW(&triangleR, nowStatus);

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

	// キーアップ・ダウン状態が変更された場合
	if(nowSw != beforeSw){
		// 文字色
		display.setTextColor(nowSw ? TFT_RED : TFT_BLUE, bgColor);
		// 表示文字
		display.print(nowSw ? "ON  " : "OFF ");
		beforeSw = nowSw;
	}
}

/**
 * スピード表示
 */
void displaySpeed(){
	// 速度算出
	byte speed = byte(moduleData[INDEX_FREQ] * 2 / 25);
	if(100 <= speed){
		speed = 99;
	}
	// 速度出力
	displayNumberln(&props.Speed, speed, 2, true);
	// メーター表示
	displayMeter(speed);
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
		sprintf(valueChars, "%02d.%1d", int(voltagex10/10), voltagex10%10);
		display.print(valueChars);
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
 * 時分表示
 */
void displayRealTime(){
	// 時刻データ数
	const int itemLen = 2;
	// 前回日時
	static uint8_t beforeTime[itemLen] = { 60, 60};
	// 現在時刻取得
	DateTime now = rtc.now();
	if(now.minute() != beforeTime[0]){
		// 現在時刻を配列化
		uint8_t newTime[itemLen] = {
			now.minute(),
			now.hour()
		};
		// 表示設定セット
		setDisplay(&props.Clock);
		// 表示更新分の文字列
		char valueChars[8];
		// 表示文字列取得
		sprintf(valueChars, "%02d:%02d", newTime[1],newTime[0]);
		// 表示
		display.print(valueChars);
		// 前回値を更新
		memcpy(beforeTime, newTime, itemLen);
	}
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
 * 値の改行表示（左0埋め）開発用
 * 
 * @param p Prop型 表示設定
 * @param digitNum int型 表示桁数
 * @param spacerZero 0埋めするかどうか
 */
void displayNumberln(int value, byte digitNum, bool spacerZero){
	// 最大5桁+null終端
	char valueStr[6];
	if (spacerZero) {
		sprintf(valueStr, "%0*d", digitNum, value);
	} else {
		sprintf(valueStr, "%*d", digitNum, value);
	}
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