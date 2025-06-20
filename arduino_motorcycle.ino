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

// --------------------プロトタイプ宣言--------------------
void displayNumberln(int valueInt, byte digiNum, bool spacerZero=false);
void displayNumberln(Prop* p, int valueInt, byte digitNum, bool spacerZero=false);
void displayNumberln(Prop* p, byte valueByte, byte digitNum, bool spacerZero=false);

// --------------------定数--------------------
// 明るさレベル
const uint8_t brightLevel[] = {
	0x01, 0x08, 0x18, 0x38, 0x80
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

int moduleData[DATA_SIZE];             // センサーからの取得値
DataClass switchData(true);            //データクラス：スイッチ
DataClass winkersData(false);          //データクラス：ウインカーADC値

LGFX display;                          // ディスプレイ
Adafruit_NeoPixel pixels(1, Pins::LED); // オンボLED
RTC_DS1307 rtc;                        // RTC
Adafruit_AHTX0 aht;                    // 温湿度計

uint16_t fontColor = TFT_WHITE;
uint16_t bgColor = TFT_BLACK;

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
		// 前回速度
		static byte beforeSp = 0xFF;

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
		uint16_t newColor = colorON;
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

// --------------------インスタンス--------------------
// 表示設定まとめ
struct Props {
	Prop Clock;        // 時:分:秒
	Prop Date;         // 月/日
	Prop Temp;         // 温度
	Prop Humid;        // 湿度
	Prop Gear;         // ギア
	Prop Newt;         // ギアニュートラル
	Prop Speed;        // 速度
	Prop SpUnit;       // 「km/h」
	Prop SpFreqIn;     // 速度センサカウンタ
	Prop Voltage;      // 電圧
	Prop DebugData;    // デバッグ用値表示
	Prop TempUnit;     // 「℃」
	//Prop SpFreqInUnit; // 「Hz」
} props;

// ------------------------------初期設定------------------------------
void setup(void) {
	delay(100);
	
	Serial.begin(9600);         // デバッグ用シリアル設定
	Wire1.setSDA(Pins::I2C_SDA); // I2C設定
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

	// I2C通信スキャン
	scanModules();
	// 各モジュール動作開始
	rtc.begin(&Wire1);                         // RTC
	#ifdef BUZZER_ON
		pinMode(PIN::BUZZER, OUTPUT);  // ウインカー音
		digitalWrite(PIN::BUZZER, LOW);
	#endif
	aht.begin(&Wire1, 0, modules[THERM].address);  // 温度計

	//rtc.adjust(DateTime(F(__DATE__),F(__TIME__))); // 時計合わせ

	// 各項目の初期表示
	initDisplayProps();

	setDisplay(&props.DebugData, fontColor);              // デバッグ用表示
	// スプライト設定
	// 横縦
	//int w = (props.Speed.y + 60 - offsetY+10) * 2;
	int w = (135 - offsetY + 10) * 2;
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
	arcM.displayArcM(CENTER_WIDTH, CENTER_HEIGHT + 10);
	// 補助線
	//display.drawFastHLine(0,CENTER_Y-rOUT+7,320,TFT_RED);
	//display.drawFastHLine(0,CENTER_Y+rOUT+6,320,TFT_RED);

}


// ------------------------------ループ------------------------------
void loop() {
	
	static Interval intervalDisplay(20);   // 表示用時間
	static Interval intervalMonitor(3);    // 各種読み取り用時間
	static Interval intervalTemp(2000);    // 温度表示用時間
	static Interval intervalVoltage(2000); // 電圧表示用時間
	static Interval intervalTime(30);      // 時刻表示用時間
	static Interval intervalBzz(50);       // ブザー用時間

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
			if(loopTimeMax < loopTime){
				loopTimeMax = loopTime;
			}

			beforeTime = time;
			
			setDisplay(&props.DebugData, fontColor);
			display.print("loop :");
			displayNumberln(loopTime, 4);
			display.print("loopM:");
			displayNumberln(loopTimeMax, 4);
			display.print("FreqI:");
			displayNumberln(moduleData[INDEX_FREQ], 4);
			display.print("vltAD:");
			displayNumberln(moduleData[INDEX_VOLT], 4);
			display.print("wnkAD:");
			displayNumberln(moduleData[INDEX_WINKERS], 4);
			display.print("geaAD:");
			displayNumberln(moduleData[INDEX_GEARS], 4);
		#endif
		intervalMonitor.reset();
	}

	// 温度モニタリング・表示
	if (intervalTemp.over(time)) {
		displayTemp();
		intervalTemp.reset();
	}

	// 電圧モニタリング・表示
	if (intervalVoltage.over(time)) {
		displayVoltage();
		intervalVoltage.reset();
	}

	// 時刻表示
	if (intervalTime.over(time)) {
		displayRealTime();
		intervalTime.reset();
	}

	// 各種表示処理
	if (intervalDisplay.over(time)) {
		// 速度表示
		displaySpeed();
		// デバッグ用スイッチ表示
		displaySwitch();
		// ギア表示
		displayGear();
		// ウインカー表示・ブザー出力
		if (displayWinkers() && intervalBzz.isZero()) {
			setBuzzer(ON);
			intervalBzz.reset();
		}
		intervalDisplay.reset();
	}

	//ブザーOFF処理
	if (!intervalBzz.isZero() && intervalBzz.over(time)) {
		setBuzzer(OFF);
		intervalBzz.setZero();
	}
}

// -------------------------------------------------------------------
// ------------------------------メソッド------------------------------
// -------------------------------------------------------------------

/**
 * 表示情報の初期化
 */
void initSetProps(int offsetY){
	// 時:分:秒
	props.Clock.font = &fonts::Font4;
	setPropWH(&props.Clock, "00:00:00");

	// 月/日
	props.Date = propCopy(&props.Clock, UNDER); 
	setPropWH(&props.Clock, "00/00");

	// 温度
	props.Temp = propCopy(&props.Clock);
	setPropWH(&props.Temp, "00 c");
	alignRight(&props.Temp, 3);

	// 湿度
	props.Humid = propCopy(&props.Temp, UNDER);
	setPropWH(&props.Humid, "00%");
	alignRight(&props.Humid);
	// ギア
	props.Gear.y = 140 + offsetY;
	props.Gear.font = &fonts::DejaVu56;
	setPropWH(&props.Gear, "0");
	props.Gear.x = centerHorizontal(props.Gear.width);

	// ギアニュートラル
	props.Newt = propCopy(&props.Gear);
	setPropWH(&props.Newt, "N");
	props.Newt.x = centerHorizontal(props.Newt.width);

	// 速度
	props.Speed.y = 10 + offsetY;
	props.Speed.font = &fonts::Font7;
	setPropWH(&props.Speed, "00");
	props.Speed.x = centerHorizontal(props.Speed.width);

	// 速度単位
	props.SpUnit = propCopy(&props.Speed, UNDER);
	props.SpUnit.font = &fonts::Font2;
	setPropWH(&props.SpUnit, "km/h");
	props.SpUnit.x = centerHorizontal(props.SpUnit.width);

	// スピードセンサIN
	props.SpFreqIn = propCopy(&props.SpUnit, UNDER);
	props.SpFreqIn.font = &fonts::Font7;
	setPropWH(&props.SpFreqIn, "0000");
	props.SpFreqIn.x = centerHorizontal(props.SpFreqIn.width);

	// 電圧
	props.Voltage.font = &fonts::Font4;
	setPropWH(&props.Voltage, "00.0V");
	alignRight(&props.Voltage);
	alignBottom(&props.Voltage);

	// デバッグ用表示
	setPropWH(&props.DebugData, "00");
	alignBottom(&props.DebugData, props.DebugData.height * 8);
}

void initDisplayProps(){
	display.fillScreen(bgColor);          // 画面リセット
	//displayString(&props.Gear, "0");           // ギアポジション表示開始
	//displayString(&props.Speed, "00");         // 速度
	//displayString(&props.Clock, "00:00:00");   // 時間
	//displayString(&props.Temp, "00");          // 温度
	displayString(&props.Humid, "00%");        // 湿度
	//displayString(&props.SpFreqIn, "0000");    // パルス周波数
	displayString(&props.Voltage, "00.0V");    // 電圧
	displayString(&props.SpUnit, "km/h");      // 速度単位

	// 温度単位
	Prop TempUnit = propCopy(&props.Temp);
	setPropWH(&TempUnit, "c");
	alignRight(&TempUnit, 3);
	
	// スピードセンサIN単位
	Prop SpFreqInUnit = propCopy(&props.SpFreqIn, UNDER);
	SpFreqInUnit.font = &fonts::Font2;
	setPropWH(&SpFreqInUnit, "Hz");
	SpFreqInUnit.x = centerHorizontal(SpFreqInUnit.width);

	displayString(&SpFreqInUnit, "Hz");  // パルス周波数単位
	displayString(&TempUnit, "c");       // 温度単位
	display.fillCircle(306 - 3, 6, 3, fontColor);
	display.fillCircle(306 - 3, 6, 1, bgColor);
}

void setBuzzer(bool isOn){
	// ブザーON
	#ifdef BUZZER_ON
		digitalWrite(PIN::BUZZER, !isOn);
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
void setDisplay(Prop* p, uint16_t color) {
	display.setCursor(p->x, p->y);               //描画位置
	display.setTextSize(p->size);                //テキスト倍率
	display.setTextColor(color, bgColor);  //フォント色...白
	display.setFont(p->font);
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

	// 初期表示メッセージ
	display.fillScreen(bgColor);
	display.setCursor(0,0);
	display.setTextSize(2);
	display.setTextColor(fontColor, bgColor);
	display.println("Hello");
	display.println("");
	delay(500);
	// スキャン処理
	display.println("I2C Module Scanning...");
	for (Module module : modules) {
		Wire1.beginTransmission(module.address);
		byte error = Wire1.endTransmission();
		display.setTextColor(fontColor, bgColor);
		display.print(module.name);
		display.print(':');
		display.setTextColor((error == 0) ? TFT_GREEN : TFT_RED, TFT_BLACK);
		display.println((error == 0) ? "OK" : "NG");
	}
	display.setTextColor(fontColor);
	display.println("");
	display.println("-- done --");
	display.println("");
	display.print("3 ");
	delay(1000);
	display.print("2 ");
	delay(1000);
	display.print("1 ");
	delay(1000);
	display.print("Start");
	delay(500);
}

/**
 * ギアポジションの表示処理
 */
void displayGear() {
	static char before = '0';
	char gearArr[5] = {'N', '1', '2', '3', '4'};
	char gear = '0';
	// 現在のギアポジを取得
	for(int i=0; i<5; i++){
		if(!(moduleData[INDEX_GEARS] & (1<<i))){
			gear = gearArr[i];
			break;
		}
	}
	// 前回と同じ場合、スキップ
	if(gear == before){
		return;
	}
	// Nの場合
	if(gear == 'N'){
		setDisplay(&props.Newt, TFT_GREEN);
	}
	// 0の場合（ギア抜け）
	else if(gear == '0'){
		Prop* prop = (before == 'N') ? &props.Newt : &props.Gear;
		// グレーで前回ギアを表示
		setDisplay(prop, TFT_DARKGREY);
		gear = before;
	}
	// 1～4の場合
	else{
		setDisplay(&props.Gear, fontColor);
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

	//左ウインカーの表示
	bool onOff = ((nowStatus & INDICATE_LEFT) == INDICATE_LEFT);
	arcL.displayArcW(CENTER_WIDTH, CENTER_HEIGHT + 10, onOff);
	//右ウインカーの表示
	onOff = ((nowStatus & INDICATE_RIGHT) == INDICATE_RIGHT);
	arcR.displayArcW(CENTER_WIDTH, CENTER_HEIGHT + 10, onOff);

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
	static int beforeFreq = 0;
	// 前回スピード
	static byte beforeSpeed = 0;

	// 周波数表示
	if (moduleData[INDEX_FREQ] != beforeFreq) {
		displayNumberln(&props.SpFreqIn, moduleData[INDEX_FREQ], 4, true);
		beforeFreq = moduleData[INDEX_FREQ];
	}
	// 速度表示
	byte speed = byte(moduleData[INDEX_FREQ] / 10);
	if(100 <= speed){
		speed = 99;
	}
	if (speed != beforeSpeed) {
		displayNumberln(&props.Speed, speed, 2, true);
		arcM.displayArcM(CENTER_WIDTH, CENTER_HEIGHT + 10, speed);
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
		setDisplay(&props.Voltage, fontColor);

		String volStr = String(voltagex10/100)
							+ String((voltagex10/10)%10)
							+ "."
							+ String(voltagex10%10);
		display.print(volStr);

		// デバッグモード表示
		#ifdef DEBUG_MODE
			setDisplay(&props.DebugData, fontColor);
			display.setCursor(props.DebugData.x, props.DebugData.y + props.DebugData.height * 7);
			display.print("vltAD:");
			displayNumberln(adcValue, 4);
		#endif
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

void displayRealTime(){
	// 時刻データ数
	const int itemLen = 5;
	// 前回日時
	static uint8_t beforeTime[itemLen] = { 60, 60, 60, 60, 60 };
	// 現在時刻取得
	DateTime now = rtc.now();
	uint8_t newTime[itemLen] = {
		now.second(),
		now.minute(),
		now.hour(),
		now.day(),
		now.month()
	};
	// 秒の値が前回と同じ場合スキップ
	if (beforeTime[0] == newTime[0]) {
		return;
	}

	// 表示情報セット
	setDisplay(&props.Clock, fontColor);
	// 表示更新分の文字列
	String resultStr = "";

	if(60 <= beforeTime[0]){
		char defaultStr[9];	
		sprintf(defaultStr, "%02d:%02d:%02d",newTime[2],newTime[1],newTime[0]);
		display.print(defaultStr);
		// 前回日時を更新
		memcpy(beforeTime, newTime, itemLen);
		return;
	}

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
	setDisplay(p, fontColor);
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
	setDisplay(p, fontColor);
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
	setDisplay(p, fontColor);
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
	Wire1.beginTransmission(modules[SPEED].address);
	Wire1.write(reg);
	Wire1.endTransmission(false);
	Wire1.requestFrom(modules[SPEED].address, numByte);
}


