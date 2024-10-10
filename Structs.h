#ifndef STRUCTS_H_INCLUDE
#define STRUCTS_H_INCLUDE

#define ON true;
#define OFF false;

// ギアポジションクラス
class _GearPosition{
	private:
		int pin;		// 読み取りピン番号
		char dispChar;	// 表示値
	public:
		// コンストラクタ
		_GearPosition(int pin, char dispChar){
			this->pin		= pin;
			this->dispChar	= dispChar;
			pinMode(this->pin, INPUT_PULLUP);
		}
		// 【Getter】表示値
		char getChar(){
			return dispChar;
		}
		// ポジションが自身か判定（表示値を参照渡し）
		bool isActive(){
			if(digitalRead(pin) == LOW){
				return true;
			}
			return false;
		}
};

class Winker{
	private:
		int pin;			// 読み取りピン番号
		bool status;		// ウインカー状態
		byte statusHist;	// ウインカー状態履歴 
	public:
		// コンストラクタ
		Winker(int pin){
			this->pin = pin;
			status = OFF;
			statusHolder = 0;
			pinMode(this->pin, INPUT_PULLUP);
		}
		//【Getter】ウインカー状態
		bool getStatus(){
			return status;
		}
		// エッジを取得(チャタリング対策済)
		bool getEdge(bool &status){
			byte newStatus = 0;
			bool returnBool = false;
			// ピンの状態を取得
			if(digitalRead(pin) == HIGH){
				newStatus = 1;
			}
			// 右に1シフトさせて最下位ビットに上位結果を代入
			statusHist = (statusHist << 1) | newStatus;
			// 仮変数にウインカー状態履歴を代入
			byte tmp = statusHist;
			// 状態ONの履歴をカウント
			int count;
			for(byte count=0; tmp!=0; tmp&=tmp-1){
				count++;
			}
			// OFFで直近8回中6回以上ONの場合
			if(status == OFF && 6 <= count){
				this->status = ON;
				returnBool = true;
			}
			// ONで直近8回中2回以下ONの場合
			else if(status == ON && count <= 2){
				this->status = OFF;
				returnBool = true;
				
			}
			status = this->status;
			return returnBool;
		}
}

// ギアポジション構造体
struct GearPosition{
	int pin;		// 読み取りピン番号
	char dispChar;	// 表示文字
	
	// コンストラクタ
	GearPosition(int pin, char dispChar){
		this->pin		= pin;
		this->dispChar	= dispChar;
		// ピンモード設定
		pinMode(this->pin, INPUT_PULLUP);
	};
};

#endif
