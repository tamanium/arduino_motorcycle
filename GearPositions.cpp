#include "GearPositions.h"

/**
 * コンストラクタ
 *
 * @param nowGear char型 表示値
 */
GearPositions::GearPositions(int *pins, int len, Adafruit_PCF8574 *pcf){
	for(int i=0; i<len; i++){
		char c = 'N';
		if(0<i){
			c = '0'+i;
		}
		this->gears[i] = Gear(pins[i], c);
	}
	this->numOfGear = len;
	this->nowGear = '-';
	this->pcf = pcf;
}

/**
 * 表示値を取得
 *
 * @return nowGear char型 表示値
 */
char GearPositions::getGear(){
	return this->nowGear;
}

/**
 * 現在のギア表示値を更新
 */
void GearPositions::monitor(){
	// カウンタ・直前ギア
	static int counter = 0;
	static char bufferGear = '-';
	// 現在のギア表示値を宣言
	char newGear = '-';
	// ギア配列でループし、現在のギア表示値を取得
	for(int i=0; i<this->numOfGear; i++){
		if(this->gears[i].isActive(this->pcf) == true){
			newGear = this->gears[i].getChar();
			break;
		}
	}
	
	// 直前ギアと取得ギアが異なる場合
	if(bufferGear != newGear){
		// 直前ギアを上書き、カウンタリセット
		bufferGear = newGear;
		counter = 0;
	}
	// 現在ギアと直前ギアが異なる場合
	else if(this->nowGear != bufferGear){
		counter++;
	}
	
	// 5カウント以上の場合
	if(5 <= counter){
		// 現在ギアに直前ギアを代入し、カウンタをリセット
		this->nowGear = bufferGear;
		counter = 0;
	}
}
