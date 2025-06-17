#ifndef DURATION_H_INCLUDE
	#define DURATION_H_INCLUDE
	#include <Arduino.h>
	
	// データクラス
	class Duration{
		private:
			unsigned long time;    // 時刻[ms]
			int interval; // 間隔[ms]
		public:
			Duration(int interval);  // コンストラクタ
			bool over(unsigned long sysTime); // データ取得
			void reset(); // データセット
	};
	
	/**
	 * コンストラクタ
	 */
	Duration::Duration(int interval){
		this->time = 0;
		this->interval = interval;
	}
	
	/**
	 * 時刻が引数時刻を超えたかどうか
	 */
	bool Duration::over(unsigned long sysTime){
		// 初期処理
		if (this->time == 0){
			this->time = sysTime;
			return false;
		}
		return (sysTime < this->time);
	}

	/**
	 * 次回実行時刻へリセット
	 */
	void Duration::reset(){
		this->time += this->interval;
	}

#endif
