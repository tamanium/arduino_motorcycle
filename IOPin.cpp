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
	this->status = OFF;
	this->dispChar = dispChar;
	if(pcf != null){
		pcf->pinMode(this->pin, INPUT_PULLUP);
	}
	else{
		pinMode(this->pin, INPUT_PULLUP);
	}
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
 * ポジションが自分自身か判定
 * @return 自分自身がポジションとなっている場合true
 */
bool IOPin::isLow(){
	if(pcf != null){
		return (pcf->digitalRead(this->pin) == LOW);
	}
	return (digitalRead(this->pin) == LOW);
}
