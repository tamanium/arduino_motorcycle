#ifndef COMMON_DEFINE_H_INCLUDE
	#define COMMON_DEFINE_H_INCLUDE

	#define ON       true
	#define OFF      false

	#define LIGHT_MODE true
	#define DARK_MODE  false

	#define KEY_DOWN true
	#define KEY_UP   false

	#define LEFT     0
	#define RIGHT    1
	#define OVER     2
	#define UNDER    3

	// インデックス：日時
	enum {
		SECOND,
		MINUTE,
		HOUR,
		DAY,
		MONTH
	};

	// インデックス：モジュールからのデータ
	enum {
		THERM,
		SPEED,
		RTCMM,
		RTCIC,
		MODULE_NUM
	};
	
	/**
	 * IOピン定義
	 */
	enum Pins {
		LED      = 16,
		BUZZER   = 26,

		I2C_SDA  = 14,
		I2C_SCL  = 15,

		SPI_SCLK =  2,
		SPI_MOSI =  3,
		SPI_MISO = -1,
		SPI_DC   =  7,
		SPI_CS   =  6,
		SPI_RST  =  8,
		SPI_BL   =  5,
		SPI_BUSY = -1
	};


#endif