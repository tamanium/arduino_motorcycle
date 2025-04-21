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
IOPin::IOPin(int pin, Adafruit_PCF8574 *pcf, char dispChar){
	this->pin = pin;
	this->mode = INPUT_PULLUP;
	this->dispChar = dispChar;
	this->pcf = pcf;
}

/**
 * 動作開始
 *
 * @param mode OUTPUT(0x00), INPUT(0x01), INPUT_PULLUP(0x02)
 */
void IOPin::begin(int mode){
	this->mode = mode;
	if(pcf != NULL){
		pcf->pinMode(this->pin, this->mode);
		return;
	}
	pinMode(this->pin, this->mode);
}

/**
 * 表示値を取得
 *
 * @return 表示値
 */
char IOPin::getChar(){
	return this->dispChar;
}

/**
 * 読み取り値がHIGHか判別
 * @return HIGHの場合true
 */
bool IOPin::isHigh(){
	if(pcf != NULL){
		return pcf->digitalRead(this->pin);
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
