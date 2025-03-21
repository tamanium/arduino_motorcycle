#include "Switch.h"

/**
 * コンストラクタ
 * @param pinLeft スイッチピン
 * @param *pcf IOエキスパンダクラス
 */
Switch::Switch(int pinSwitch, Adafruit_PCF8574 *pcf){
	this->pinSwitch = pinSwitch;
	this->status = false;
    this->pcf = pcf;
}

/**
 * 【Getter】スイッチ状態を取得
 * @return スイッチがonの場合true
 */
bool Switch::getStatus(){
    return this->status;
}

/**
 *  ウインカー状態を更新
 */
void Winkers::monitor(){
	// カウンタ
	static int counter = 0;
	// 直前状態
	static bool bufferStatus = false;
	// 現在状態
	bool newStatus = false;
    // 各ピンを読み取りウインカー状態へセット
		if(this->pcf->digitalRead(pinSwitch) == HIGH){
			newStatus = true;
		}

    // 直前状態と取得状態が異なる場合
	if(bufferStatus != newStatus) {
		// 直前状態を更新・カウンタをリセット
		bufferStatus = newStatus;
		counter = 0;
	}
	// 現在状態と直前状態が異なる場合
	else if(this->status != bufferStatus){
		counter++;
	}
	// 5カウント以上の場合
	if(5 <= counter){
		// 現在状態に直前状態を代入し、カウンタをリセット
		this->status = bufferStatus;
		counter = 0;
	}
}