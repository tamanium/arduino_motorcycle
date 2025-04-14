// --------------------ライブラリ--------------------
#include <Adafruit_GFX.h>               // 画面出力
#include <Adafruit_ST7789.h>            // ディスプレイ
#include <SPI.h>                        // SPI通信
#include <RTClib.h>                     // 時計機能
#include <Adafruit_PCF8574.h>           // IOエキスパンダ
#include <Temperature_LM75_Derived.h>   // 温度計
#include <Adafruit_ADS1X15.h>           // ADコンバータ
#include <Adafruit_NeoPixel.h>          // オンボLED

// --------------------自作クラス・ピン定義--------------------
#include "Define.h"			// 値定義
#include "GearPositions.h"	// ギアポジションクラス
#include "Winker.h"			// ウインカークラス
#include "Switch.h"         // スイッチクラス

#include <LovyanGFX.hpp>
class LGFX : public lgfx::LGFX_Device{

// 接続するパネルの型にあったインスタンスを用意します。
lgfx::Panel_ST7789  _panel_instance;
// パネルを接続するバスの種類にあったインスタンスを用意します。
lgfx::Bus_SPI       _bus_instance;   // SPIバスのインスタンス
// バックライト制御が可能な場合はインスタンスを用意します。(必要なければ削除)
lgfx::Light_PWM     _light_instance;

public:
  LGFX(void){
    { // バス制御の設定を行います。
      auto cfg = _bus_instance.config();    // バス設定用の構造体を取得します。
      // SPIバスの設定
      cfg.spi_host = 0;     // 使用するSPIを選択
      cfg.spi_mode = 0;             // SPI通信モードを設定 (0 ~ 3)
      cfg.freq_write = 40000000;    // 送信時のSPIクロック (最大80MHz, 80MHzを整数で割った値に丸められます)
      cfg.freq_read  = 20000000;    // 受信時のSPIクロック
      cfg.pin_sclk = 2;            // SPIのSCLKピン番号を設定
      cfg.pin_mosi = 3;            // SPIのMOSIピン番号を設定
      cfg.pin_miso = -1;            // SPIのMISOピン番号を設定 (-1 = disable)
      cfg.pin_dc   = 7;            // SPIのD/Cピン番号を設定  (-1 = disable
      _bus_instance.config(cfg);    // 設定値をバスに反映します。
      _panel_instance.setBus(&_bus_instance);      // バスをパネルにセットします。
    }

    { // 表示パネル制御の設定を行います。
      auto cfg = _panel_instance.config();    // 表示パネル設定用の構造体を取得します。
      cfg.pin_cs = 6;  // CSが接続されているピン番号   (-1 = disable)
      cfg.pin_rst = 8;  // RSTが接続されているピン番号  (-1 = disable)
      cfg.pin_busy = -1;  // BUSYが接続されているピン番号 (-1 = disable)
      cfg.panel_width      =   240;  // 実際に表示可能な幅
      cfg.panel_height     =   320;  // 実際に表示可能な高さ
      cfg.offset_rotation  =     0;  // 回転方向の値のオフセット 0~7 (4~7は上下反転)
      cfg.invert           = true;  // パネルの明暗が反転してしまう場合 trueに設定
      _panel_instance.config(cfg);
    }

    { // バックライト制御の設定を行います。（必要なければ削除）
      auto cfg = _light_instance.config();    // バックライト設定用の構造体を取得します。
      cfg.pin_bl = 5;              // バックライトが接続されているピン番号
      cfg.invert = false;           // バックライトの輝度を反転させる場合 true
      cfg.freq   = 44100;           // バックライトのPWM周波数
      cfg.pwm_channel = 7;          // 使用するPWMのチャンネル番号
      _light_instance.config(cfg);
      _panel_instance.setLight(&_light_instance);  // バックライトをパネルにセットします。
    }
    setPanel(&_panel_instance); // 使用するパネルをセットします。
  }
};

// 準備したクラスのインスタンスを作成します。
LGFX display;

// --------------------定数--------------------
const int MONITOR_INTERVAL = 5;		//ms
const int DISPLAY_INTERVAL = 30;	//ms
const int TEMP_INTERVAL    = 2000;	//ms
const int TIME_INTERVAL    = 30;	//ms
const int BUZZER_DURATION  = 50;	//ms
const int WINKER_DURATION  = 380;	//ms

// フォントの寸法
const int GEAR_SIZE   = 24;

// --------------------変数--------------------
unsigned long displayTime = 0;	// 表示処理
unsigned long monitorTime = 0;	// 各種読み取り
unsigned long tempTime = 0;		// 温度測定にて使用
unsigned long timeTime = 0;		// 時刻測定
unsigned long bzzTime = 0;
unsigned long debugWinkerTime  = 0;	//疑似ウインカー

// 保持用char配列
//uint16_t timeItems[4] = {0, 0, 0, 0};
// シフトポジション配列
int gears[] = {PIN.IOEXP.POS.nwt,
				PIN.IOEXP.POS.low,
				PIN.IOEXP.POS.sec,
				PIN.IOEXP.POS.thi,
				PIN.IOEXP.POS.top};
// 明るさレベル
byte brightLevel[] = {50, 100, 250};

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
	return ST77XX_RED;
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
// 表示座標
TriangleLocation triCoords[2] = {
	{50, 34, 50, 160+14, 0, 80+24},
	{fromRight(50), 34, fromRight(50), 160+14, fromRight(0), 80+24}
};
// 表示設定
struct PrintProperty {
	int x = 0;						// x座標
	int y = 0;						// y座標
	int size = 1;					// フォント倍率	（デフォルト：1）
	uint16_t color = ST77XX_WHITE;	// フォント色	（デフォルト：白）
	const GFXfont* font = NULL;		// フォント		（デフォルト：デフォルトフォント
};
// 表示設定まとめ
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
// 電圧表示：座標と文字倍率
DispInfo voltDispInfo = {0, 0, 3};
Adafruit_NeoPixel pixels(1, PIN.LED);	//ブザーがわりのオンボLED
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
	// 描画位置
	tft.setCursor(p->x, p->y);
	// テキスト倍率
	tft.setTextSize(p->size);
	// テキスト色
	if(isTrans){
		tft.setTextColor(p->color);
	}
	else{
		tft.setTextColor(p->color, ST77XX_BLACK);
	}
	// テキストフォント
	tft.setFont(p->font);
}

// ------------------------------初期設定------------------------------
void setup(void) {
	// デバッグ用シリアル設定
	Serial.begin(9600);

	// I2C設定
	Wire1.setSDA(PIN.I2C.sda);
	Wire1.setSCL(PIN.I2C.scl);
	Wire1.begin();// いらないけど明示しておく

	// SPI1設定
	SPI.setTX(PIN.SPI.mosi);
	SPI.setSCK(PIN.SPI.sclk);

	// ディスプレイの初期化
	display.init();
	display.setTextSize((std::max(display.width(), display.height()) + 0xFF) >> 8);
	display.fillScreen(TFT_BLACK);

	display.setFont(&fonts::Font0);
	display.setCursor(0,0);
	display.print("Hello");
	display.println("hello");


	// 表示文字情報
	int dateSize = 2;
	// 月
	PRINT_PROP.Month = {
		int(FONT.WIDTH * dateSize * 2.5),
		fromBottom(FONT.HEIGHT * 2 + FONT.HEIGHT * dateSize + FONT.HEIGHT * dateSize),
		dateSize
	};
	// 日
	PRINT_PROP.Day = {
		int(FONT.WIDTH * dateSize * 5.5),
		PRINT_PROP.Month.y,
		dateSize
	};
	int timeSize = 3;
	// 時間
	PRINT_PROP.Hour = {
		0,
		fromBottom(FONT.HEIGHT * timeSize),
		timeSize
	};
	// 分
	PRINT_PROP.Min = {
		FONT.WIDTH * timeSize * 3,
		PRINT_PROP.Hour.y,
		timeSize
	};
	// 秒
	PRINT_PROP.Sec = {
		FONT.WIDTH * timeSize * 6,
		PRINT_PROP.Hour.y,
		timeSize
	};
	// 温度
	PRINT_PROP.Temp = {
		fromRight(FONT.WIDTH * timeSize * 5), 
		fromBottom(FONT.HEIGHT * timeSize), 
		timeSize
	};
	// ギア
	PRINT_PROP.Gear = {
		centerHorizontal(6, 1),
		140,
		6
	};
	// 速度
	PRINT_PROP.Speed = {
		centerHorizontal(10, 2),
		30,
		10
	};
	// 速度単位
	PRINT_PROP.SpUnit = {
		centerHorizontal(2, 4),
		110,
		2
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

	// ブザー連動LED設定
	pixels.begin();
	pixels.setPixelColor(0, pixels.Color(1,1,0));
	pixels.show();

  /*
	// ディスプレイ明るさ設定(0-255)
	analogWrite(PIN.SPI.bl, brightLevel[0]);

	// ディスプレイ初期化・画面向き・画面リセット
	tft.init(OLED.HEIGHT, OLED.WIDTH);
	tft.setRotation(3);
	tft.fillScreen(ST77XX_BLACK);

	// 初期表示
	setProp(&PRINT_PROP.InitMsg);
	tft.println("hello");
	delay(2000);

	pixels.clear();
	pixels.show();

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
		int moduleIndex = existsModule(adrs, moduleArr, MODULES.size);
		if(moduleIndex == -1){
			continue;
		}
		moduleArr[moduleIndex].disabled = (moduleIndex == -1);
		String name = moduleArr[moduleIndex].name;
		tft.print(name + " : ");
		tft.setTextColor(OKNGColor(error == 0));
		tft.println(OKNGMsg(error == 0));
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
	//digitalWrite(PIN.buzzer, HIGH);
	digitalWrite(PIN.buzzer, LOW);
	//analogWrite(PIN.buzzer, 51);
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
*/
}

// ------------------------------ループ------------------------------
void loop() {
	/*

	// 経過時間(ms)取得
	unsigned long time = millis();
	
	// 疑似ウインカーリレー
	if(debugWinkerTime <= time){
		// 出力反転
		digitalWrite(PIN.relay, !digitalRead(PIN.relay));
		debugWinkerTime += WINKER_DURATION;
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
		tempDisplay();
		tempTime = time + TEMP_INTERVAL;
		uint16_t raw = ads.readADC_SingleEnded(3);
		String voltage = String((raw * 0.0001875f), 2);
		Serial.print("Voltage:");
		Serial.println(voltage);
	}

	// 時刻表示
	if(timeTime <= time){
		realTimeDisplay();
		timeTime = time + TIME_INTERVAL;
	}
	
	// 各種表示処理
	if(displayTime <= time){
		// デバッグ用スイッチ表示
		displaySwitch(&pushSw, &tft);
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
	*/
}

// -------------------------------------------------------------------
// ------------------------------メソッド------------------------------
// -------------------------------------------------------------------

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
	setProp(&PRINT_PROP.Gear);
	// ギア表示更新
	tft.print(newGear);
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
		displayTriangle(triCoords[side], buffer[side]);
		// フラグ立てる
		isSwitched = true;
	}
	return isSwitched;
}
/**
 * 三角形表示処理
 * @param coord TriangleLocation型 
 * @param status bool型 true...点灯, false...消灯
 */
void displayTriangle(TriangleLocation coord, bool status){
	// 文字色宣言（初期値は黒）
	uint16_t color = ST77XX_BLACK;
	// 条件falseの場合は文字色変更
	if(status == false){
		color = ST77XX_YELLOW;
	}
	// 図形表示（BLACKの場合は削除）
	tft.fillTriangle(coord.x1, coord.y1,
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
	tft->setTextSize(3);
	tft->setCursor(0, 0);

	// 前回と状態が異なる場合
	/*
	if(beforeSw != nowSw){
		// 表示リセット
		tft->setTextColor(ST77XX_BLACK);
		String msg ="OFF";
		if(beforeSw){
			msg = "ON";
		}
		tft->print(msg);
		//　前回状態を更新
		beforeSw = nowSw;
		tft->setCursor(0, 0);
	}
	*/
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
		if(beforeSw != nowSw){
			tft->setTextColor(ST77XX_BLUE, ST77XX_BLACK);
			tft->print("OFF");
			beforeSw = OFF;
		}
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
	setProp(&PRINT_PROP.Temp);
	// 温度が一桁以下の場合、十の位にスペース
	if(10 <= newTempx10 && newTempx10 < 100){
		tft.print(' ');
	}
	tft.print(int(newTempx10/10));
	tft.print('.');
	tft.print(int(newTempx10)%10);
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
		&PRINT_PROP.Month,
		&PRINT_PROP.Day,
		&PRINT_PROP.Hour,
		&PRINT_PROP.Min,
		&PRINT_PROP.Sec,
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
		setProp(printPropArr[i]);
		// 値が1桁の場合は0埋め
		if(newTime[i] < 10){
			tft.print('0');
		}
		// 値を出力
		tft.print(newTime[i]);
		// 前回日時を更新
		beforeTime[i] = newTime[i];
	}
}
