#include "Gear.h"

/**
 * デフォルトコンストラクタ
 */
Gear::Gear(){

}

/**
 * コンストラクタ
 *
 * @param pin int型 表示値
 * @param dispChar char型 表示値
 */
Gear::Gear(int pin, char dispChar){
	this->pin = pin;
	this->dispChar = dispChar;
}

/**
 * //ピン読み取り設定
 */
void Gear::begin(Adafruit_PCF8574 *pcf){
	pcf->pinMode(this->pin, INPUT_PULLUP);
}

/**
 * 表示値を取得
 *
 * @return dispChar char型 表示値
 */
char Gear::getChar(){
	return dispChar;
}

/**
 * ポジションが自分自身か判定
 * @return 自分自身がポジションとなっている場合true
 */
bool Gear::isActive(Adafruit_PCF8574 *pcf){
	return pcf->digitalRead(this->pin) == LOW;
}
