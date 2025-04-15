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
<<<<<<< HEAD
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
=======
 */
IOPin::IOPin(int pin, char dispChar){
	this->pin = pin;
	this->status = OFF;
	this->dispChar = dispChar;
}

/**
 * //ピン読み取り設定
 */
void IOPin::begin(Adafruit_PCF8574 *pcf){
	pcf->pinMode(this->pin, INPUT_PULLUP);
>>>>>>> 9b9aa296a482cf02e06a2cd6729ddd5e2860e12f
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
<<<<<<< HEAD
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
=======
 * ポジションが自分自身か判定
 * @return 自分自身がポジションとなっている場合true
 */
bool IOPin::isActive(Adafruit_PCF8574 *pcf){
	return pcf->digitalRead(this->pin) == LOW;
>>>>>>> 9b9aa296a482cf02e06a2cd6729ddd5e2860e12f
}
