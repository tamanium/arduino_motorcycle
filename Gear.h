#ifndef GEAR_H_INCLUDE
#define GEAR_H_INCLUDE

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

/**
 * デフォルトコンストラクタ
 */
Gear::Gear(){
  
}

/**
 * コンストラクタ
 * @param pin int型 表示値
 * @param dispChar char型 表示値
 */
Gear::Gear(int pin, char dispChar){
  this->pin = pin;
  this->dispChar = dispChar;
  pinMode(this->pin, INPUT_PULLUP);
}

/**
 * 【Getter】表示値
 * @return dispChar char型 表示値
 */
char Gear::getChar(){
	return dispChar;
}

/**
 * ポジションが自分自身か判定
 * @return 自分自身がポジションとなっている場合true
 */
bool Gear::isActive(){
	if(digitalRead(pin) == LOW){
		return true;
	}
	return false;
}

#endif
