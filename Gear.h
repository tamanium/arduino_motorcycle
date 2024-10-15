#ifndef GEAR_H_INCLUDE
#define GEAR_H_INCLUDE
#include <Arduino.h>

// ギアポジションクラス
class Gear{
	private:
		int pin;		// 読み取りピン番号
		char dispChar;	// 表示値
	public:
		Gear();// コンストラクタ
		Gear(int p, char c);// コンストラクタ
		char getChar();// 【Getter】表示値
		bool isActive();// ポジションが自身か判定（表示値を参照渡し）
};

#endif
