#ifndef GEARPOSITIONS_H_INCLUDE
	#define GEARPOSITIONS_H_INCLUDE
	#include <Adafruit_PCF8574.h>
	#include "IOPin.h"	//ギアクラス

	// ギアポジションクラス
	class GearPositions{
		private:
			IOPin gears[5]; // ギアクラス配列
			char nowGear;   // 現在のギア表示値
			int GearNum;    // ギア個数
		public:
			// コンストラクタ
			GearPositions(int *pins, int len, Adafruit_PCF8574 *pcf);  
			begin();
			// 表示値取得
			char getGear();
			// ギア状態更新
			void updateStatus();
	};

#endif
