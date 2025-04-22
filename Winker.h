#include <iterator>
#ifndef WINKERS_H_INCLUDE
	#define WINKERS_H_INCLUDE
	#include <Arduino.h>
	#include <Adafruit_PCF8574.h>
	#include "CommonDefine.h"
	#include "IOPin.h"            //IOピン自作クラス

	struct Winker{
		IOPin ioPin;
		bool status = false;
		bool before = false;
		int count = 0;
	};

	// ウインカークラス
	class Winkers{
		private:
			Winker winker[2];
			//IOPin pin[2];
			//bool status[2];		// ウインカー状態
		public:
			// コンストラクタ
			Winkers(int pinLeft, int pinRight, Adafruit_PCF8574 *pcf);
			void begin();
			// ウインカー状態取得
			bool getStatus(int i);
			// ウインカー状態更新
			void updateStatus();
	};

#endif
