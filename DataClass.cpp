#include "DataClass.h"
	
/**
 * コンストラクタ
 */
DataClass::DataClass(bool isAntiChat){
	this->data = -7;
	this->beforeData = -7;
	this->chatCounter = 0;
	this->antiChattering = isAntiChat;
}

int DataClass::getData(){
	return this->data;
}

void DataClass::setData(int data){
	// チャタリング対策しない場合
	if(antiChattering){
		// 前回値と異なる場合
		if(data != this->beforeData){
			// カウンタリセット・前回値更新
			chatCounter = 0;
			this->beforeData = data;
			return;
		}
		// カウントおよびカウント値を評価
		if(++chatCounter < 3){
			// カウント3未満の場合何もしない
			return;
		}
	}
	// 値更新
	this->data = data;
}