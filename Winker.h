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
        Adafruit_PCF8574 pcf;// IOエキスパンダ
	public:
		Winkers(int pinLeft, int pinRight);// コンストラクタ
        void begin(Adafruit_PCF8574 *pcf);
        void begin(uint8_t i2c_addr = PCF8574_I2CADDR_DEFAULT, TwoWire *wire = &Wire);
		bool getStatusLeft();   //【Getter】左ウインカー状態
		bool getStatusRight();  //【Getter】右ウインカー状態
        bool getStatus(int i);  //【Getter】ウインカー状態
		void monitor(Adafruit_PCF8574 *pcf);// ウインカー状態更新
		void monitor();// ウインカー状態更新
};

#endif
