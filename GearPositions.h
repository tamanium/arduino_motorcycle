#ifndef GEARPOSITIONS_H_INCLUDE
	#define GEARPOSITIONS_H_INCLUDE
	#include <Adafruit_PCF8574.h>
	#include "Gear.h"	//ギアクラス

	// ギアポジションクラス
	class GearPositions{
		private:
			Gear gears[5];			// ギアクラス配列
			char nowGear;			// 現在のギア表示値
			int GearNum;			// ギア個数
			Adafruit_PCF8574 *pcf;	// IOエキスパンダポインタ
		public:
			// コンストラクタ
			GearPositions(int *pins, int len, Adafruit_PCF8574 *pcf);  
			// 表示値取得
			char getGear();
			// ギア状態更新
			void updateStatus();
	};

#endif
