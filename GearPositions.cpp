#include "GearPositions.h"

/**
 * コンストラクタ
 *
 * @param nowGear char型 表示値
 */
GearPositions::GearPositions(int *pins, int gearNum, Adafruit_PCF8574 *pcf){
	for(int i=0; i<gearNum; i++){
		// 0ならN、1以上ならその数字
		char c = (i==0) ? 'N' : '0'+i;
		this->gears[i] = IOPin(pins[i], pcf, c);
	}
	this->GearNum = gearNum;
	this->nowGear = '0';
}

/**
 * 動作開始
 */
void GearPositions::begin(){
	for(IOPin p : this->gears){
		p.begin(INPUT_PULLUP);
	}
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
void GearPositions::updateStatus(){
	// カウンタ・直前ギア
	static int counter = 0;
	static char bufferGear = '0';
	// 現在のギア表示値を宣言
	char newGear = '0';
	// ギア配列でループし、現在のギア表示値を取得
	for(IOPin ioPin : this->gears){
		if(ioPin.isLow()){
			newGear = ioPin.getChar();
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
