#ifndef SWITCH_H_INCLUDE
#define SWITCH_H_INCLUDE
#include <Arduino.h>
#include <Adafruit_PCF8574.h>

// スイッチクラス
class Switch{
	private:
		int pinSwitch;		// スイッチ読み取りピン
		bool status;	// スイッチ状態
        Adafruit_PCF8574 *pcf;// IOエキスパンダ
	public:
        Switch(int pinSwitch, Adafruit_PCF8574 *pcf);// コンストラクタ
        bool getStatus();  //【Getter】ウインカー状態
		void monitor();// ウインカー状態更新
};

#endif