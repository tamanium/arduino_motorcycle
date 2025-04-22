#include "Winker.h"

/**
 * コンストラクタ
 * @param pinLeft int 左ウインカーピン
 * @param pinRight int 右ウインカーピン
 */
Winkers::Winkers(int pinLeft, int pinRight, Adafruit_PCF8574 *pcf){
	this->winker[LEFT].pin = Winker(pinLeft, pcf);
	this->winker[RIGHT].pin = Winker(pinRight, pcf);
}

/**
 * 動作開始
 */
void Winkers::begin(){
	this->winker[LEFT].pin.begin(INPUT_PULLUP);
	this->winker[RIGHT].pin.begin(INPUT_PULLUP);
}

/**
 * 【Getter】ウインカー状態を取得
 * @param i int型 インデックス
 * @return bool型 ウインカーが点灯している場合true
 */
bool Winkers::getStatus(int i){
	return this->winker[i].status;
}

/**
 *  ウインカー状態を更新
 */
void Winkers::updateStatus(){
	// カウンタ(1は左ウインカー、0は右ウインカー)
	//static int counter[] = {0, 0};
	// 直前ウインカー状態(1は左ウインカー、0は右ウインカー)
	//static bool buffer[] = {OFF, OFF};
	// 現在のウインカー状態(1は左ウインカー、0は右ウインカー)
	bool now[] = {OFF, OFF};

	// 各ピンを読み取りウインカー状態へセット
	for(int i=0; i<=1; i++){
		Winker* w = &this->winker[i];
		now[i] = w->pin.isHigh();
		// 直前状態と取得状態が異なる場合
		if(w->before != now[i]) {
			// 直前状態を更新・カウンタをリセット
			w->buffer = now[i];
			w->counter = 0;
		}
		// 現在状態と直前状態が異なる場合
		else if(w->status != w->before){
			w->counter++;
		}
		// 5カウント以上の場合
		if(5 <= w->counter){
			// 現在状態に直前状態を代入し、カウンタをリセット
			w->status = w->before;
			w->counter = 0;
		}
	}
}
