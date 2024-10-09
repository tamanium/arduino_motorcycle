#ifndef GEAR_POSITION_H_INCLUDE
#define GEAR_POSITION_H_INCLUDE

// 表示情報
struct DisplayInfo{
  // カーソル座標x
  int x;
  // カーソル座標y
  int y;
  // 背景色
  int bg;
  // テキストサイズ
  int textSize;
  
  //コンストラクタ
  DisplayInfo(int x, int y, int bg, int textSize){
    this->x = x;
    this->y = y;
    this->bg = bg;
    this->textSize = textSize;
  };
};

// ギアポジション構造体
struct GearPosition{
  int pin;        // 読み取りピン番号
  char dispChar;  // 表示文字

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
  int pin;      // 読み取りピン番号 
  bool status;  // 状態
  
  // コンストラクタ
  Winker(int pin){
    this->pin = pin;
    this->status = 0;
    // ピンモード設定
    pinMode(this->pin, INPUT_PULLUP);
  };
};

#endif
