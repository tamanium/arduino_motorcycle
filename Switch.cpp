#include "Switch.h"

/**
 * コンストラクタ
 *
 * @param pinLeft スイッチピン
 * @param *pcf IOエキスパンダクラス
 */
Switch::Switch(int pin, Adafruit_PCF8574 *pcf){
	this->swPin = IOPin(pin, pcf);
	this->status = KEY_UP;       //初期ステータス：キーアップ
	this->pushFlag = false;	     //プッシュフラグ
	this->longPressFlag = false; //長押しフラグ
}

/**
 * 動作開始
 */
void Switch::begin(){
	this->swPin.begin(INPUT_PULLUP);
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
void Switch::updateStatus(){
	
	static int counter = 0;               // カウンタ
	static bool beforeStatus = KEY_UP;    // 前回状態
	bool newStatus = this->swPin.isLow(); // 現在状態

	// 前回状態と現在状態が異なる場合
	if(beforeStatus != newStatus) {
		// 前回状態を更新・カウンタをリセット
		beforeStatus = newStatus;
		counter = 0;
	}
	// 保持状態と前回状態が異なる場合、またはキーダウンが持続している場合
	else if(this->status != beforeStatus || newStatus == KEY_DOWN){
		// カウントアップ
		counter++;
	}

	// キーダウンで200カウント以上の場合
	if(this->status == KEY_DOWN && 200 <= counter){
		// 長押しフラグON・プッシュフラグOFF
		this->longPressFlag = true;
		this->pushFlag = false;
		return;
	}

	// 5カウント以上の場合
	if(5 <= counter){
		// 現在状態に直前状態を代入
		this->status = beforeStatus;
		// キーアップの場合
		if(this->status == KEY_UP){
			// カウンタリセット
			counter = 0;
			// 長押しフラグオフの場合
			if(!this->longPressFlag){
				// プッシュフラグオン
				this->pushFlag = true;
			}
		}
		// キーダウンの場合
		else{
			// プッシュフラグリセット
			this->pushFlag = false;
		}
		// 長押しフラグリセット
		this->longPressFlag = false;
	}
}