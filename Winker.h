#ifndef WINKERS_H_INCLUDE
#define WINKERS_H_INCLUDE
#include <Arduino.h>
#include <Adafruit_PCF8574.h>

// ウインカークラス
class Winkers{
	private:
		int pinLeft;			// 左ウインカー読み取りピン
		int pinRight;			// 右ウインカー読み取りピン
		bool statusLR[2];		// ウインカー状態
		Adafruit_PCF8574 *pcf;	// IOエキスパンダ
	public:
		// コンストラクタ
		Winkers(int pinLeft, int pinRight, Adafruit_PCF8574 *pcf);
		// ウインカー状態取得
		bool getStatus(int i);
		// ウインカー状態更新
		void monitor();
};

#endif
