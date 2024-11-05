#ifndef GEARPOSITIONS_H_INCLUDE
#define GEARPOSITIONS_H_INCLUDE
#include <Adafruit_PCF8574.h>
#include "Gear.h"	//ギアクラス

// ギアポジションクラス
class GearPositions{
	private:
		Gear gears[5];	// ギアクラス配列
		char nowGear;	// 現在のギア表示値
		int numOfGear;	// ギア個数
        Adafruit_PCF8574 *pcf;// IOエキスパンダポインタ
	public:
		GearPositions(int *pins, int len, Adafruit_PCF8574 *pcf);  // コンストラクタ
        void setPCF(Adafruit_PCF8574 *pcf);//ピン読み取り設定
        void begin(uint8_t i2c_addr = PCF8574_I2CADDR_DEFAULT, TwoWire *wire = &Wire);
		char getGear();  // 【Getter】表示値
		void monitor(Adafruit_PCF8574 *pcf);  // 現在のギア表示値を更新
		void monitor();  // 現在のギア表示値を更新
};

#endif
