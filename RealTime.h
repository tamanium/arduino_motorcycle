#ifndef REALTIME_H_INCLUDE
	#define REALTIME_H_INCLUDE
	#include <Arduino.h>
	#include <Adafruit_PCF8574.h>
	#include "CommonDefine.h"

	// ウインカークラス
	class RealTime{
		private:
			uint8_t realTimeBuf[5];			// 左ウインカー読み取りピン
			int pinRight;			// 右ウインカー読み取りピン
			bool statusLR[2];		// ウインカー状態
			Adafruit_PCF8574 *pcf;	// IOエキスパンダ
		public:
			// コンストラクタ
			Winkers(int pinLeft, int pinRight, Adafruit_PCF8574 *pcf);
			// ウインカー状態取得
			RealTime(lib
			// 時刻更新
			void update();
			// 画面表示
			void output();
	};

#endif