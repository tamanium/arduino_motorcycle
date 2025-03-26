#include "Switch.h"

/**
 * コンストラクタ
 *
 * @param pinLeft スイッチピン
 * @param *pcf IOエキスパンダクラス
 */
Switch::Switch(int pin, Adafruit_PCF8574 *pcf){
    this->pin = pin;			//スイッチピン定義
    this->status = false;			//初期ステータス：キーアップ
    this->pushFlag = false;			//プッシュフラグ
    this->longPressFlag = false;	//長押しフラグ
    this->pcf = pcf;				//IOエキスパンダ
}

/**
 * 【Getter】スイッチ状態を取得
 * @return スイッチがonの場合true
 */
bool Switch::getStatus(){
	return this->status;
}

/**
 * プッシュされたか取得
 *
 * @return プッシュされた場合true返却(フラグをfalseに戻す)
 */
bool Switch::isPush(){
	bool returnBool = this->pushFlag;
	if(returnBool){
		this->pushFlag = false;
	}
	return returnBool;
}

/**
 * 長押しされているか取得
 *
 * @return 長押しされている場合true
 */
bool Switch::isLongPress(){
    return this->longPressFlag;
}

/**
 *  ウインカー状態を更新
 */
void Switch::monitor(){
	// カウンタ
	static int counter = 0;
	// 直前状態
	static bool bufferStatus = false;
	// 現在状態
	bool newStatus = false;
	// 各ピンを読み取りウインカー状態へセット
	if(this->pcf->digitalRead(this->pin) == LOW){
		newStatus = true;
	}

	// 直前状態と取得状態が異なる場合
	if(bufferStatus != newStatus) {
		// 直前状態を更新・カウンタをリセット
		bufferStatus = newStatus;
		counter = 0;
	}
	// 現在状態と直前状態が異なる場合、またはキーダウンが持続している場合
    else if(this->status != bufferStatus || newStatus == true){
		counter++;
	}

	// キーダウンで200カウント以上の場合
	if(200 <= counter && this->longPressFlag == false){
		// 長押しフラグ
		this->longPressFlag = true;
		counter = 0;
		return;
	}
	// 5カウント以上の場合
	if(5 <= counter){
		// 現在状態に直前状態を代入
		this->status = bufferStatus;
		// キーアップの場合
		if(this->status == false){
			counter = 0;
			// 長押しフラグリセット
			this->longPressFlag = false;
			// プッシュフラグオン
			this->pushFlag = true;
		}
		// キーダウンの場合
		else{
			// プッシュフラグリセット
			this->pushFlag = false;
		}
	}

}