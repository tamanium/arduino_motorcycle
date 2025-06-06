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
	if(!antiChattering){
		this->data = data;
		return;
	}
	
	// チャタリング対策する場合
	// 前回値と異なる場合
	if(data != this->beforeData){
		// カウンタリセット・前回値更新
		chatCounter = 0;
		this->beforeData = data;
		return;
	}
	
	// カウンタが規定数を超えた場合
	if(5 < ++chatCounter){
		// 代入
		this->data = data;
	}
}