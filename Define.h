#ifndef DEFINE_H_INCLUDE
	#define DEFINE_H_INCLUDE

	#include "CommonDefine.h"
	#include <LovyanGFX.hpp>

	/**
	 * フォントサイズ
	 */
	struct Font{
		int WIDTH = 6;
		int HEIGHT = 8;
	} FONT;

	/**
	 * モジュール情報
	 */
	struct Module{
		String name;		// モジュール名
		byte address;		// I2Cアドレス
		bool disabled = true;// 使用可能か
	};

	// モジュール構造体
	struct Modules{
		Module thmst = {"Thermometer ", 0x38};
		Module speed = {"Speed Sensor", 0x55};
		Module rtcMm = {"RTC memory  ", 0x50};
		Module rtcIC = {"RTC IC      ", 0x68};
	} MODULES;
	
	/**
	 * ディスプレイ定義
	 */
	struct Display{
		const int WIDTH = 320;
		const int HEIGHT = 240;
	}OLED;

	/**
	 * IOピン定義
	 */
	struct Pins{
		const int LED = 16;
		const int buzzer = 26;     // ブザー
		const int relay = 0;       // ダミーリレー
		struct I2c{                // I2C通信
			const int scl = 15;
			const int sda = 14;
		} I2C;
		struct Spi{
			const int SCLK = 2;
			const int MOSI = 3;
			const int MISO = -1;
			const int DC = 7;
			const int CS = 6;
			const int RST = 8;
			const int BL = 5;

		} SPI;
	} PINS;
	
	// 表示設定
	struct Prop {
		int x = 0;                  // x座標
		int y = 0;                  // y座標
		int size = 1;               // フォント倍率
		const lgfx::v1::IFont* font = &fonts::Font0; // フォント
		int width = 6;
		int height = 8;
	};
	
	/**
	 * x座標出力（画面右端原点、左向き）
	 */
	int fromRight(int x){
		return OLED.WIDTH-x-1;
	}

	/**
	 * y座標出力（画面下底原点、上向き）
	 */
	int fromBottom(int y){
		return OLED.HEIGHT-y-1;
	}
	/**
	 * x座標中央揃え
	 *
	 * @param fontWidth フォント横幅
	 * @param size 文字数
	 */
	int centerHorizontal(int width){
		return (OLED.WIDTH>>1) - (width>>1);
	}


#endif
