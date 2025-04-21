#ifndef IOPIN_H_INCLUDE
	#define IOPIN_H_INCLUDE
	#include <Arduino.h>
	#include <Adafruit_PCF8574.h>
	#include "CommonDefine.h"
	
	// ギアポジションクラス
	class IOPin{
		private:
			int pin;               // 読取ピン番号
			int mode;              // 入出力モード
			char dispChar;         // 表示値
			Adafruit_PCF8574 *pcf; // IOエキスパンダクラス
		public:
			IOPin();                                     // コンストラクタ
			IOPin(int p, Adafruit_PCF8574 *pcf, char c='0'); // コンストラクタ
			void begin(int mode = INPUT_PULLUP);         // 動作開始
			char getChar();                              // 表示値取得
			bool isHigh();                               // 読取値がHIGHかどうか
			bool isLow();                                // 読取値がLOWかどうか
	};

#endif
