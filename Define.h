#ifndef DEFINE_H_INCLUDE
#define DEFINE_H_INCLUDE

#define ON true
#define OFF false

#define DOWN true
#define UP false

#define LEFT 0
#define RIGHT 1

#define MONTH	0
#define DAY		1
#define HOUR	2
#define MINUTE	3
#define SECOND	4

/**
 * モジュール情報
*/
struct Module{
	String name;
	byte address;
};

struct Modules{
	int size = 5;
	Module ioExp = {"IO Expander ", 0x27};
	Module therm = {"Thermometer ", 0x48};
	Module adCnv = {"AD Converter", 0x4A};
	Module rtcMm = {"RTC memory  ", 0x50};
	Module rtcIC = {"RTC IC      ", 0x68};
} MODULES;
/**
 * ディスプレイ定義
 */
struct Display{
	const int width = 320;
	const int height = 240;
} DISPLAY_INFO;

/**
 * IOピン定義
 */
struct Pin{
	const int buzzer = 27;	// ブザー
	const int relay = 0;	// ダミーリレー
	struct I2c{				// I2C通信
		const int scl = 15;
		const int sda = 14;
	} I2C;
	struct Spi{				// SPI通信
		const int mosi = 3;
		const int sclk = 2;
		const int bl   = 5;
		const int cs   = 6;
		const int dc   = 7;
		const int rst  = 8;
	} SPI;
	struct IoExp{			// IOエキスパンダのピン定義
		const int sw = 3;	//  スイッチ
		struct Pos{			//  ギアポジ
			const int nwt = 0;
			const int low = 1;
			const int sec = 2;
			const int thi = 4;
			const int top = 5;
 		} POS;
		struct Wnk{			//  ウインカー
			const int left = 7;
			const int right = 6;
		} WNK;
	} IOEXP;
} PIN;

#endif
