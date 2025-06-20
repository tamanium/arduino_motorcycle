#ifndef DATA_CLASS_H_INCLUDE
	#define DATA_CLASS_H_INCLUDE
	#include <Arduino.h>
	
	// データクラス
	class DataClass{
		private:
			int data;            // データ
			int beforeData;      // 前回データ
			bool antiChattering; // チャタリング対策するか否か
			uint8_t chatCounter;    // チャタリング対策用カウンタ 
		public:
			DataClass(bool isAntiChat = false);         // コンストラクタ
			int getData();       // データ取得
			void setData(int data); // データセット
			bool isEqual();      // 前回値と同じか
	};
#endif
