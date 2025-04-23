#include <iterator>
#ifndef WINKERS_H_INCLUDE
	#define WINKERS_H_INCLUDE
	#include <Arduino.h>
	#include <Adafruit_PCF8574.h>
	#include "CommonDefine.h"
	#include "IOPin.h"            //IOピン自作クラス

	struct Winker{
		IOPin ioPin;         // IOピンクラス
		bool status = false; // ステータス（ON, OFF）
		bool before = false; // 前回ステータス（ON, OFF）
		int count = 0;       // チャタリング対策用カウンタ
	};

	// ウインカークラス
	class Winkers{
		private:
			Winker winker[2];
		public:
			// コンストラクタ
			Winkers(int pinLeft, int pinRight, Adafruit_PCF8574 *pcf);
			void begin();          // 動作開始
			bool getStatus(int i); // 状態取得
			void updateStatus();   // 状態更新
	};

#endif
