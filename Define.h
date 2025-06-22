#ifndef DEFINE_H_INCLUDE
	#define DEFINE_H_INCLUDE

	#include "CommonDefine.h"
	#include <LovyanGFX.hpp>

	// モジュール情報
	struct Module{
		char name[12];	// モジュール名
		byte address;	// I2Cアドレス
	};
	
	// モジュール情報用インデックス・サイズ
	enum {
		THERM,
		SPEED,
		RTCMM,
		RTCIC,
		MODULE_NUM
	};

	// モジュール情報配列
	struct Module modules[MODULE_NUM] = {
		{"Thermometer", 0x38},
		{"Attiny1604 ", 0x55},
		{"RTC memory ", 0x50},
		{"RTC IC     ", 0x68}
	};
	
	enum DisplaySize {
		WIDTH = 320,
		HEIGHT = 240,
		CENTER_WIDTH = WIDTH >> 1,
		CENTER_HEIGHT = HEIGHT >> 1
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
	} ;
	
	// 表示設定
	struct Prop {
		int x = 0;                  // x座標
		int y = 0;                  // y座標
		uint8_t size = 1;               // フォント倍率
		const lgfx::v1::IFont* font = &fonts::Font0; // フォント
		uint8_t width = 6;
		uint8_t height = 8;
	};

	/**
	 * 表示設定構造体のコピー
	 *
	 * @param p 表示設定構造体コピー元
	 * @param location RIGHTかUNDERか空欄
	 * @param font フォント
	 */
	Prop propCopy(Prop* p, int location = -1, const lgfx::v1::IFont* font=NULL){
		Prop newP = *p;
		// コピー元に対して右側に配置する場合
		int x = (location == RIGHT) ? p->width  : 0;
		// コピー元に対して下側に配置する場合
		int y = (location == UNDER) ? p->height : 0;
		//
		if(font != NULL){
			newP.font = font;
		}
		newP.x += x;
		newP.y += y;
		return newP;
	}

	/**
	 * 表示設定を右寄せにする
	 */
	void alignRight(Prop* p, int offset = 0){
		p->x = DisplaySize::WIDTH - p->width - 1 - offset;
	}

	/**
	 * 表示設定を下寄せにする
	 *
	 * @param p 表示設定
	 * @param offset 右側オフセットピクセル値
	 */
	void alignBottom(Prop* p, int offset = 0){
		p->y = DisplaySize::HEIGHT - p->height - 1 - offset;
	}

	struct Interval {
		unsigned long time = 0; // 処理実行時刻[ms]
		int interval;           // 処理実行間隔[ms]

		/**
		 * コンストラクタ
		 */ 
		Interval(int interval) :interval(interval){}

		/**
		 * 処理時刻が0かどうか
		 */
		bool isZero(){
			return (this->time == 0);
		}
		/**
		 * 処理時刻に0をセット
		 */
		void setZero(){
			this->time = 0;
		}

		/**
		 * 処理時刻がシステム時刻を超えているか
		 *
		 * @param sysTime システム時刻
		 */
		bool over(unsigned long sysTime){
			if(this->time == 0){
				this->time = sysTime;
				return false;
			}
			return (this->time <= sysTime);
		}

		/**
		 * 処理時刻再設定 
		 */
		void reset(){
			this->time += this->interval;
		}
	};
	
	/**
	 * x座標中央揃え
	 *
	 * @param width フォント横幅
	 */
	int centerHorizontal(int width){
		return (DisplaySize::WIDTH>>1) - (width>>1);
	}


#endif
