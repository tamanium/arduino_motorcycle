#include "Winker.h"

/**
 * コンストラクタ
 * @param pinLeft int 左ウインカーピン
 * @param pinRight int 右ウインカーピン
 */
Winkers::Winkers(int pinLeft, int pinRight, Adafruit_PCF8574 *pcf){
	this->pin[LEFT] = pinLeft;
	this->pin[RIGHT] = pinRight;
	this->status[LEFT] = OFF;
	this->status[RIGHT] = OFF;
	this->pcf = pcf;
}

/**
 * 【Getter】ウインカー状態を取得
 * @param i int型 インデックス
 * @return bool型 ウインカーが点灯している場合true
 */
bool Winkers::getStatus(int i){
	return this->statusLR[i];
}

/**
 *  ウインカー状態を更新
 */
void Winkers::updateStatus(){
	// カウンタ(1は左ウインカー、0は右ウインカー)
	static int counter[] = {0, 0};
	// 直前ウインカー状態(1は左ウインカー、0は右ウインカー)
	static bool buffer[] = {OFF, OFF};
	// 現在のウインカー状態(1は左ウインカー、0は右ウインカー)
	bool now[] = {OFF, OFF};

	// ピンを配列化
	int pins[] = {this->pinRight, this->pinLeft};
	// 各ピンを読み取りウインカー状態へセット
	for(int i=0; i<=1; i++){
			now[i] = (this->pcf->digitalRead(pins[i]) == ON);
		// 直前状態と取得状態が異なる場合
		if(buffer[i] != now[i]) {
			// 直前状態を更新・カウンタをリセット
			buffer[i] = now[i];
			counter[i] = 0;
		}
		// 現在状態と直前状態が異なる場合
		else if(this->status[i] != buffer[i]){
			counter[i]++;
		}
		// 5カウント以上の場合
		if(5 <= counter[i]){
			// 現在状態に直前状態を代入し、カウンタをリセット
			this->status[i] = buffer[i];
			counter[i] = 0;
		}
	}
}
