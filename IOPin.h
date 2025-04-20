#ifndef IOPIN_H_INCLUDE
	#define IOPIN_H_INCLUDE
	#include <Arduino.h>
	#include <Adafruit_PCF8574.h>
	#include "CommonDefine.h"
	
	// ギアポジションクラス
	class IOPin{
		private:
			int pin;               // 読み取りピン番号
			char dispChar;         // 表示値
			Adafruit_PCF8574 *pcf; // IOエキスパンダ
		public:
			// コンストラクタ
			IOPin();
			IOPin(int p, char c, Adafruit_PCF8574 *pcf);
			// 動作開始
			begin(int mode);
			// 表示値取得
			char getChar();
			// 読み取り
			bool isHigh();
			bool isLow();
	};

#endif
