#ifndef STRUCTS_H_INCLUDE
#define STRUCTS_H_INCLUDE

// 表示情報構造体
struct DisplayInfo{
	
	int x;			// カーソル座標x
	int y;			// カーソル座標y
	int bg;			// 背景色
	int textSize;	// テキストサイズ
	
	//コンストラクタ
	DisplayInfo(int x, int y, int bg, int textSize){
		this->x = x;
		this->y = y;
		this->bg = bg;
		this->textSize = textSize;
	};
};
// ギアポジションクラス
class _GearPosition{
	private:
		int pin;		// 読み取りピン番号
		char dispChar;	// 表示値
	
	public:
		// コンストラクタ
		_GearPosition(int pin, char dispChar){
			this->pin = pin;
			this->dispChar = dispChar;
			pinMode(this->pin, INPUT_PULLUP);
		}
		bool isOn(){
			if(digitalRead(pin) == LOW){
				return true;
			}
			return false;
		}
		// 表示値を出力
		char getCharIfOn(){
			// 読み取り結果がLOWだった場合
			if(digitalRead(pin) == LOW){
				return dispChar;
			}
		}
}

// ギアポジション構造体
struct GearPosition{
	
	int pin;		// 読み取りピン番号
	char dispChar;	// 表示文字
	// コンストラクタ
	GearPosition(int pin, char dispChar){
		this->pin = pin;
		this->dispChar = dispChar;
		// ピンモード設定
		pinMode(this->pin, INPUT_PULLUP);
	};
};

// ウインカー構造体
struct Winker{
	
	int pin;		// 読み取りピン番号 
	bool status;	// 状態
	// コンストラクタ
	Winker(int pin){
		this->pin = pin;
		this->status = 0;
		// ピンモード設定
		pinMode(this->pin, INPUT_PULLUP);
	};
	
	readWinker(){
		
	};
};

#endif
