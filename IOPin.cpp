#include "IOPin.h"

/**
 * デフォルトコンストラクタ
 */
//IOPin::IOPin(){}

/**
 * コンストラクタ
 *
 * @param pin int型 表示値
 * @param dispChar char型 表示値
 
IOPin::IOPin(int pin, char dispChar, Adafruit_PCF8574 *pcf){
	this->pin = pin;
	this->status = OFF;
	this->dispChar = dispChar;
	pcf->pinMode(this->pin, INPUT_PULLUP);
}
*/
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
bool IOPin::isActive(Adafruit_PCF8574 *pcf){
	return pcf->digitalRead(this->pin) == LOW;
}
