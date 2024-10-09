#ifndef GEAR_POSITION_H_INCLUDE
#define GEAR_POSITION_H_INCLUDE

struct GearPosition{
  int pin;        // 読み取りピン番号
  char dispChar;  // 表示文字
  /**
   * コンストラクタ
  */
  GearPosition(int pin, char dispChar){
    this->pin = pin;
    this->dispChar = dispChar;
  }
}

#endif
