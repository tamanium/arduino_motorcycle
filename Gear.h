#ifndef GEAR_H_INCLUDE
	#define GEAR_H_INCLUDE
	#include <Arduino.h>
	#include <Adafruit_PCF8574.h>

	// ギアポジションクラス
	class Gear{
		private:
			int pin;		// 読み取りピン番号
			char dispChar;	// 表示値
		public:
			// コンストラクタ
			Gear();
			Gear(int p, char c);
			// ピン読み取り設定
			void begin(Adafruit_PCF8574 *pcf);
			// 表示値取得
			char getChar();
			// ポジションが自身か判定（表示値を参照渡し）
			bool isActive(Adafruit_PCF8574 *pcf);
	};

#endif
