#include "IOPin.h"

/**
 * デフォルトコンストラクタ
 */
IOPin::IOPin(){}

/**
 * コンストラクタ
 *
 * @param pin int型 表示値
 * @param dispChar char型 表示値
 * @param Adafruit_PCF8574 IOエキスパンダクラス
 */
IOPin::IOPin(int pin, char dispChar, Adafruit_PCF8574 *pcf){
	this->pin = pin;
	this->status = false;
	this->dispChar = dispChar;
	this->pcf = pcf;
	//if(pcf != NULL){
		pcf->pinMode(this->pin, INPUT_PULLUP);
	//}
	//else{
	//	pinMode(pin, INPUT_PULLUP);
	//}
}

/**
 * コンストラクタ
 *
 * @param pin int型 表示値
 * @param Adafruit_PCF8574 IOエキスパンダクラス
 */
IOPin::IOPin(int pin, Adafruit_PCF8574 *pcf){
	IOPin(pin, '0', pcf);
}

/**
 * 表示値を取得
 *
 * @return dispChar char型 表示値
 */
char IOPin::getChar(){
	return dispChar;
}

/**
 * 読み取り値がLOWかどうか
 * @return 読み取り値がLOWの場合場合true
 */
bool IOPin::isLow(){
	if(pcf != NULL){
		return (this->pcf->digitalRead(this->pin)==LOW);
	}
	return (digitalRead(this->pin)==LOW);
}

/**
 * 読み取り値がHIGHかどうか
 * @return 読み取り値がHIGHの場合場合true
 */
bool IOPin::isHIGH(){
	if(pcf != NULL){
		return (this->pcf->digitalRead(this->pin)==HIGH);
	}
	return (digitalRead(this->pin)==HIGH);
}
