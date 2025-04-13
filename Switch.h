#ifndef SWITCH_H_INCLUDE
	#define SWITCH_H_INCLUDE
	#include <Arduino.h>
	#include <Adafruit_PCF8574.h>
	#include "IOPin.h"	//入出力ピンクラス
	#include "Const.h"

	// スイッチクラス
	class Switch{
		private:
			IOPin pin;			// 入出力ピンクラス
			bool status;			// スイッチ状態
			bool pushFlag;			// プッシュしたか
			bool longPressFlag;		// 長押ししているか
		public:
			// コンストラクタ
			Switch(int pin, Adafruit_PCF8574 *pcf);
			bool getStatus();       //【Getter】ウインカー状態
			bool isPush();          // プッシュしたか
			bool isLongPress();     // 長押ししているか
			void updateStatus();         // ウインカー状態更新
	};

#endif