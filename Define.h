#ifndef DEFINE_H_INCLUDE
	#define DEFINE_H_INCLUDE

	#include "CommonDefine.h"
	#include <LovyanGFX.hpp>

	// ディスプレイ関連値
	enum DisplaySize {
		WIDTH = 320,
		HEIGHT = 240,
		CENTER_WIDTH = WIDTH >> 1,
		CENTER_HEIGHT = HEIGHT >> 1
	};

	// モジュール情報
	struct Module{
		char name[12]; // モジュール名
		byte address;  // I2Cアドレス
		bool active; // 通信可能かどうか
	};
	
	// モジュール情報配列
	struct Module modules[MODULE_NUM] = {
		{"Thermometer", 0x38, false},
		{"Attiny1604 ", 0x55, false},
		{"RTC memory ", 0x50, false},
		{"RTC IC     ", 0x68, false}
	};
	
	/**
	 * 円弧表示情報（基底）
	 */
	struct ArcInfo {
		int x; // 円弧中心x座標
		int y; // 円弧中心y座標
		int r; // 内径
		int d; // 厚さ
		int angle0; // 角度0
		int angle1; // 角度1
		uint16_t colorON; // 色
		uint16_t colorBG = TFT_BLUE; // 透過色
	};

	/**
	 * 表示設定
	 */
	struct Prop {
		int x = 0; // x座標
		int y = 0; // y座標
		uint8_t size = 1; // フォント倍率
		const lgfx::v1::IFont* font = &fonts::Font0; // フォント
		uint8_t width = 6;  // 表示文字列の幅（例外あり）
		uint8_t height = 8; // 表示文字列の高さ（例外あり）
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
		if(location == RIGHT){
			newP.x += p->width;
		}
		// コピー元に対して下側に配置する場合
		else if(location == UNDER){
			newP.y += p->height;
		}

		if(font != NULL){
			newP.font = font;
		}
		return newP;
	}

	/**
	 * 処理実行時刻管理
	 */
	struct Interval {
		// 処理実行時刻[ms]
		unsigned long time = 0;
		// 処理実行間隔[ms]
		int interval;

		 // コンストラクタ
		Interval(int interval) :interval(interval){}
		// 処理時刻が0かどうか
		bool isZero(){
			return (this->time == 0);
		}
		 // 処理時刻に0をセット
		void setZero(){
			this->time = 0;
		}

		/**
		 * 処理時刻がシステム時刻を超えているか
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
	
	// -------------------------------------------------------------------
	// ------------------------------座標関係------------------------------
	// -------------------------------------------------------------------
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
	/**
	 * x座標中央揃え
	 *
	 * @param width フォント横幅
	 */
	int centerHorizontal(int width){
		return CENTER_WIDTH - (width>>1);
	}

#endif
