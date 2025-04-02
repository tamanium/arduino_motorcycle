#ifndef DEFINE_H_INCLUDE
	#define DEFINE_H_INCLUDE

	#include "CommonDefine.h"

	/**
	* フォントサイズ
	**/
	struct Font{
		int HEIGHT = 8;
		int WIDTH = 6;
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
		const int WIDTH = 320;
		const int HEIGHT = 240;
	} OLED;

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
	* bool値に対応する文字列を出力
	* @return trueなら"OK"、falseなら"NG"
	*/
	String OKNGMsg(bool b){
		if(b){	return "OK";}
		else {	return "NG";}
	}
#endif
