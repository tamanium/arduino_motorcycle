#ifndef DATA_CLASS_H_INCLUDE
	#define DATA_CLASS_H_INCLUDE
	#include <Arduino.h>
	
	// データクラス
	class DataClass{
		private:
			int data;            // データ
			int beforeData;      // 前回データ
			bool antiChattering; // チャタリング対策するか否か
			uint8_t chatCounter; // チャタリング対策用カウンタ
		public:
			// コンストラクタ
			DataClass(bool isAntiChat = false);
			// データ取得
			int getData();
			// データセット
			void setData(int data);
			// 前回値と同じかどうか
			bool isEqual();
	};
#endif
