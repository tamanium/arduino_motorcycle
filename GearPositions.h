#ifndef GEARPOSITIONS_H_INCLUDE
#define GEARPOSITIONS_H_INCLUDE
#include "Gear.h"	//ギアクラス

// ギアポジションクラス
class GearPositions{
	private:
		Gear gears[5];	// ギアクラス配列
		char nowGear;	// 現在のギア表示値
		int numOfGear;	// ギア個数
	public:
		GearPositions(int *pins, int len);  // コンストラクタ
        void begin(Adafruit_PCF8574 *pcf);//ピン読み取り設定
		char getGear();  // 【Getter】表示値
		void monitor(Adafruit_PCF8574 *pcf);  // 現在のギア表示値を更新
};

#endif
