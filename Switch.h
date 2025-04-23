#ifndef SWITCH_H_INCLUDE
	#define SWITCH_H_INCLUDE
	#include <Arduino.h>
	#include "CommonDefine.h"
	#include <Adafruit_PCF8574.h> //IOエキスパンダクラス
	#include "IOPin.h"            //IOピン自作クラス

	// スイッチクラス
	class Switch{
		private:
			IOPin swPin;        // IOピン自作クラス
			bool status;        // スイッチ状態
			bool pushFlag;      // プッシュしたか
			bool longPressFlag; // 長押ししているか
		public:
			// コンストラクタ
			Switch(int pin, Adafruit_PCF8574 *pcf);
			void begin();        // 動作開始
			bool getStatus();    // ウインカー状態取得
			bool isPush();       // プッシュしたか
			bool isLongPress();  // 長押ししているか
			void updateStatus(); // ウインカー状態更新
	};

#endif