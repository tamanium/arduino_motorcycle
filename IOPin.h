<<<<<<< HEAD
#ifndef IOPin_H_INCLUDE
	#define IOPin_H_INCLUDE
	#include <Arduino.h>
	#include <Adafruit_PCF8574.h>
	#include "Const.h"

=======
#ifndef IOPIN_H_INCLUDE
	#define IOPIN_H_INCLUDE
	#include <Arduino.h>
	#include <Adafruit_PCF8574.h>
	#include "CommonDefine.h"
	
>>>>>>> 9b9aa296a482cf02e06a2cd6729ddd5e2860e12f
	// ギアポジションクラス
	class IOPin{
		private:
			int pin;		// 読み取りピン番号
			boolean status;	// ステータス
			char dispChar;	// 表示値
<<<<<<< HEAD
			Adafruit_PCF8574 *pcf;	//IOエキスパンダ
		public:
			// コンストラクタ
			IOPin();
			IOPin(int pin, char c = '0', Adafruit_PCF8574 *pcf=NULL);
			IOPin(int pin, Adafruit_PCF8574 *pcf=NULL);
			char getChar();	// 表示値取得
			bool isLow();	// 読取値がLOWかどうか
			bool isHIGH();	//読取値がHIGHかどうか
=======
		public:
			// コンストラクタ
			IOPin();
			IOPin(int p, char c = '0');
			// ピン読み取り設定
			void begin(Adafruit_PCF8574 *pcf);
			void begin(int pin);
			// 表示値取得
			char getChar();
			// ポジションが自身か判定（表示値を参照渡し）
			bool isActive(Adafruit_PCF8574 *pcf);
>>>>>>> 9b9aa296a482cf02e06a2cd6729ddd5e2860e12f
	};

#endif
