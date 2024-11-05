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
 *
void Winkers::begin(Adafruit_PCF8574 *pcf){
	pcf->pinMode(this->pinLeft, INPUT_PULLUP);
	pcf->pinMode(this->pinRight, INPUT_PULLUP);
}
*/
/**
 * 通信開始
 * @param i2c_addr uint8_t型  IOエキスパンダ
 * @param *wire TwoWireクラス IOエキスパンダ
void Winkers::begin(uint8_t i2c_addr, TwoWire *wire){
    this->pcf.begin(i2c_addr, wire);
	this->pcf.pinMode(this->pinLeft, INPUT_PULLUP);
	this->pcf.pinMode(this->pinRight, INPUT_PULLUP);
}
 */
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
 * 【Getter】左ウインカー状態を取得
 * @return bool型 ウインカーが点灯している場合true
bool Winkers::getStatusLeft(){
	return this->statusLR[1];
}
 */

/**
 * 【Getter】右ウインカー状態を取得
 * @return bool型 ウインカーが点灯している場合true
bool Winkers::getStatusRight(){
	return this->statusLR[0];
}
 */
/**
 *  ウインカー状態を更新
 * @param *pcf Adafruit_PCF8574型 IOエキスパンダ
void Winkers::monitor(Adafruit_PCF8574 *pcf){
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
		if(pcf->digitalRead(pins[i]) == HIGH){
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
 */
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
