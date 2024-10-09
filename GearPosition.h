#ifndef GEAR_POSITION_H_INCLUDE
#define GEAR_POSITION_H_INCLUDE

struct GearPosition{
  // 読み取りピン番号  
  int pin;
  // 表示文字
  char dispChar;
  GearPosition(int pin, char dispChar){
    this->pin = pin;
    this->dispChar = dispChar;
  }
}

#endif
