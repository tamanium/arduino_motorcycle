#include "IOPin.h"

/**
 * デフォルトコンストラクタ
 */
IOPin::IOPin(){}

/**
 * コンストラクタ
 *
 * @param pin ピン番号
 * @param dispChar char型 表示値
 * @param pcf IOエキスパンダクラス
 */
IOPin::IOPin(int pin, char dispChar, Adafruit_PCF8574 *pcf){
	this->pin = pin;
	this->dispChar = dispChar;
	this->pcf = pcf;
}

/**
 * 動作開始
 *
 * @param mode OUTPUT, INPUT, INPUT_PULLUP
 */
void IOPin::begin(int mode){
	if(pcf != NULL){
		pcf->pinMode(this->pin, mode);
		return;
	}
	pinMode(this->pin, mode);
}

/**
 * 表示値を取得
 *
 * @return dispChar char型 表示値
 */
char IOPin::getChar(){
	return this->dispChar;
}
/**
 * 読み取り値がLOWか判別
 * @return LOWの場合true
 */
bool IOPin::isHigh(){
	if(pcf != NULL){
		return (pcf->digitalRead(this->pin) == HIGH);
	}
	return (digitalRead(this->pin) == HIGH);
}
/**
 * 読み取り値がLOWか判別
 * @return LOWの場合true
 */
bool IOPin::isLow(){
	return !isHigh();
}
