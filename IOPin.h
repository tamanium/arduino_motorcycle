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
			IOPin(int p, char c = '0', Adafruit_PCF8574 *pcf = NULL);
			// ピン読み取り設定
			void begin();
			// 表示値取得
			char getChar();
			// ポジションが自身か判定（表示値を参照渡し）
			bool isLow();
			bool isHIGH();
	};

#endif
