#ifndef WINKERS_H_INCLUDE
#define WINKERS_H_INCLUDE
#include <Arduino.h>
#include <Adafruit_PCF8574.h>

// ウインカークラス
class Winkers{
	private:
		int pinLeft;		// 左ウインカー読み取りピン
		int pinRight;		// 右ウインカー読み取りピン
		bool statusLR[2];	// ウインカー状態 
	public:
		Winkers(int pinLeft, int pinRight);// コンストラクタ
        void begin(Adafruit_PCF8574 *pcf);
		bool getStatusLeft();   //【Getter】左ウインカー状態
		bool getStatusRight();  //【Getter】右ウインカー状態
        bool getStatus(int i);  //【Getter】ウインカー状態
		void monitor(Adafruit_PCF8574 *pcf);
};

#endif
