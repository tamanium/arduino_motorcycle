#ifndef GEARPOSITIONS_H_INCLUDE
	#define GEARPOSITIONS_H_INCLUDE
	#include <Adafruit_PCF8574.h> //IOエキスパンダクラス
	#include "IOPin.h"            //IOピン自作クラス

	// ギアポジションクラス
	class GearPositions{
		private:
			IOPin gears[5]; // ギアクラス配列
			char nowGear;   // 現在のギア表示値
			int GearNum;    // ギア個数
		public:
			// コンストラクタ
			GearPositions(int *pins, int gearNum, Adafruit_PCF8574 *pcf);
			void begin();        // 動作開始
			char getGear();      // 表示値取得
			void updateStatus(); // ギア状態更新
	};

#endif
