#ifndef WINKERS_H_INCLUDE
#define WINKERS_H_INCLUDE

// ウインカークラス
class Winkers{
	private:
		int pinLeft;		// 左ウインカー読み取りピン
		int pinRight;		// 右ウインカー読み取りピン
		byte statusLR;		// ウインカー状態 
	public:
		// コンストラクタ
		Winkers(int pinLeft, int pinRight){
			this->pinLeft = pinLeft;
			this->pinRight = pinRight;
			status = B00;
			pinMode(this->pinLeft, INPUT_PULLUP);
			pinMode(this->pinRight, INPUT_PULLUP);
		}
		//【Getter】ウインカー状態
		bool getStatus();
};

/**
 * 【Getter】左ウインカー状態を取得
 * @return bool型 ウインカーが点灯している場合true
 */
bool Winkers::getStatusLeft(){
	if((this->statusLR >>1)&1 == 1){
		return true;
	}
	return false;
}

/**
 * 【Getter】右ウインカー状態を取得
 * @return bool型 ウインカーが点灯している場合true
 */
bool WInkers::getStatusRight(){
	if(this->statusLR&1 == 1){
		return true;
	}
	return false;
}

void Winkers::monitor(){
	// カウンタ(2桁目は左ウインカー、1桁目は右ウインカー)
	static byte counter = 0x00;
	// 直前ギア状態(2桁目は左ウインカー、1桁目は右ウインカー)
	static byte bufferStatusLR = B00;
	// 現在のウインカー状態(2桁目は左ウインカー、1桁目は右ウインカー)
	byte newStatusLR = B00;
	
	if(digitalRead(pinLeft) == HIGH){
		newStatusLR |= B10; 
	}
	if(digitalRead(pinRight) == HIGH){
		newStatusLR |= B01;
	}
	
	// 左直前状態と左取得状態が異なる場合
	if((bufferStatusLR | B10) != (newStatusLR | B10)) {
		// 左ビットを更新・左カウンタを加算
		bufferStatusLR &= newStatusLR | B01;
		counter += 0x10;
	}
	// 左現在状態と左直前状態が異なる場合
	else if((this->statusLR | B10) != (bufferStatusLR | B10)){
		counter += 0x10;
	}
	//  5カウント以上の場合
	if(5 <= (counter>>8)){
		// 左現在状態に左直前状態を代入し、左カウンタをリセット
		this->status |= B10;
		counter &= 0x0F;
	}
	
	// 右直前状態と右取得状態が異なる場合
	if((bufferStatusLR | B01) != (newStatusLR | B01)) {
		// 右ビットを更新・右カウンタを加算
		bufferStatusLR &= newStatusLR | B10;
		counter += 0x01;
	}
	// 右現在状態と右直前状態が異なる場合
	else if((this->statusLR| B10) != (bufferStatusLR | B10)){
		counter += 0x01;
	}
	//  5カウント以上の場合
	if(5 <= (counter&0x0F)){
		// 右現在状態に右直前状態を代入し、右カウンタをリセット
		this->status |= B01;
		counter &= 0xF0;
	}
}
#endif
