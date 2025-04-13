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
 * ポジションが自分自身か判定
 * @return 自分自身がポジションとなっている場合true
 */
bool IOPin::isLow(){
	return (this->pcf->digitalRead(this->pin) == LOW);
}

bool IOPin::isHIGH(){
	return (this->pcf->digitalRead(this->pin) == HIGH);
}
