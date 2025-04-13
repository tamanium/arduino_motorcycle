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
 */
IOPin::IOPin(int pin, char dispChar, Adafruit_PCF8574 *pcf){
	this->pin = pin;
	this->status = false;
	this->dispChar = dispChar;
	this->pcf = pcf;
}

/**
 * //ピン読み取り設定
 */
void IOPin::begin(){
	pcf->pinMode(this->pin, INPUT_PULLUP);
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
	return (this->pcf->digitalRead(this->pin) == LOW);
}

/**
 * 読み取り値がHIGHかどうか
 * @return 読み取り値がHIGHの場合場合true
 */
bool IOPin::isHIGH(){
	return (this->pcf->digitalRead(this->pin) == HIGH);
}
