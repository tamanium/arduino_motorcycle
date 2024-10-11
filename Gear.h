#ifndef GEARPOSITION_H_INCLUDE
#define GEARPOSITION_H_INCLUDE

// ギアポジションクラス
class Gear{
	private:
		int pin;		// 読み取りピン番号
		char dispChar;	// 表示値
	public:
		// コンストラクタ
		Gear(int pin, char dispChar){
			this->pin		= pin;
			this->dispChar	= dispChar;
			pinMode(this->pin, INPUT_PULLUP);
		}
		// 【Getter】表示値
		char getChar();
		// ポジションが自身か判定（表示値を参照渡し）
		bool isActive();
};

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
