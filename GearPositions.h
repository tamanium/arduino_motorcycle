#ifndef GEARPOSITIONS_H_INCLUDE
#define GEARPOSITIONS_H_INCLUDE

#include "Gear.h"	//ギアクラス

// ギアポジションクラス
class GearPositions{
	private:
		Gear Gears[5];	// ギアクラス配列
		char nowGear;	// 現在のギア表示値
		int numOfGear;	// ギア個数
	public:
		// コンストラクタ
		GearPositions(int &pins, int len){
			for(int i=0; i<len; i++){
				char _char = '0';
				if(i==0){
					_char = 'N'
				}
				else{
					_char += i;
				}
				this->Gear[i] = Gear(pins[i], _char);
			}
			this->numOfGear = len;
			this->nowGear = '-';
		}
};

/**
 * 【Getter】表示値
 * @return nowGear char型 表示値
 */
void GearPositions::getGear(){
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
	char newChar = '-'
	// ギア配列でループし、現在のギア表示値を取得
	for(int i=0; i<this->numOfGear; i++){
		if(Gears[i].isActive() == true){
			newChar = Gears[i].getChar();
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
		this->nowChar = bufferChar;
		counter = 0;
	}
}

#endif
