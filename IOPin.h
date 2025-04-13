#ifndef IOPin_H_INCLUDE
	#define IOPin_H_INCLUDE
	#include <Arduino.h>
	#include <Adafruit_PCF8574.h>
	#include "Const.h"

	// ギアポジションクラス
	class IOPin{
		private:
			int pin;		// 読み取りピン番号
			boolean status;	// ステータス
			char dispChar;	// 表示値
			Adafruit_PCF8574 *pcf;	//IOエキスパンダ
		public:
			// コンストラクタ
			IOPin();
			IOPin(int pin, char c = '0', Adafruit_PCF8574 *pcf=NULL);
			IOPin(int pin, Adafruit_PCF8574 *pcf=NULL);
			char getChar();	// 表示値取得
			bool isLow();	// 読取値がLOWかどうか
			bool isHIGH();	//読取値がHIGHかどうか
	};

#endif
