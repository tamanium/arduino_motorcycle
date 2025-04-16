#ifndef IOPIN_H_INCLUDE
	#define IOPIN_H_INCLUDE
	#include <Arduino.h>
	#include <Adafruit_PCF8574.h>
	#include "CommonDefine.h"
	
	// ギアポジションクラス
	class IOPin{
		private:
			int pin;		// 読み取りピン番号
			boolean status;	// ステータス
			char dispChar;	// 表示値
		public:
			// コンストラクタ
			//IOPin();
			//IOPin(int p, char c, Adafruit_PCF8574 *pcf);
			// 表示値取得
			char getChar();
			// ポジションが自身か判定（表示値を参照渡し）
			bool isActive(Adafruit_PCF8574 *pcf);
	};

#endif
