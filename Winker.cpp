#include "Winker.h"

/**
 * コンストラクタ
 * @param pinLeft int 左ウインカーピン
 * @param pinRight int 右ウインカーピン
 */
Winkers::Winkers(int pinLeft, int pinRight, Adafruit_PCF8574 *pcf){
	this->pinLeft = pinLeft;
	this->pinRight = pinRight;
	this->statusLR[0] = false;
	this->statusLR[1] = false;
	this->pcf = pcf;
}

/**
 * 【Getter】ウインカー状態を取得
 * @param i int型 インデックス
 * @return bool型 ウインカーが点灯している場合true
 */
bool Winkers::getStatus(int i){
	// 配列数以上の数値の場合false
	if(i < 2){
		return this->statusLR[i];
	}
	return false;
}

/**
 *  ウインカー状態を更新
 */
void Winkers::monitor(){
	// カウンタ(1は左ウインカー、0は右ウインカー)
	static int counter[] = {0, 0};
	// 直前ギア状態(1は左ウインカー、0は右ウインカー)
	static bool bufferStatusLR[] = {false, false};
	// 現在のウインカー状態(1は左ウインカー、0は右ウインカー)
	bool newStatusLR[] = {false, false};

	// ピンを配列化
	int pins[] = {this->pinRight, this->pinLeft};
	// 各ピンを読み取りウインカー状態へセット
	for(int i=0; i<=1; i++){
		if(this->pcf->digitalRead(pins[i]) == HIGH){
			newStatusLR[i] = true;
		}
	
		// 直前状態と取得状態が異なる場合
		if(bufferStatusLR[i] != newStatusLR[i]) {
			// 直前状態を更新・カウンタをリセット
			bufferStatusLR[i] = newStatusLR[i];
			counter[i] = 0;
		}
		// 現在状態と直前状態が異なる場合
		else if(this->statusLR[i] != bufferStatusLR[i]){
			counter[i]++;
		}
		// 5カウント以上の場合
		if(5 <= counter[i]){
			// 現在状態に直前状態を代入し、カウンタをリセット
			this->statusLR[i] = bufferStatusLR[i];
			counter[i] = 0;
		}
	}
}
