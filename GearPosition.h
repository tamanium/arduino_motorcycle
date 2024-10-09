#ifndef GEAR_POSITION_H_INCLUDE
#define GEAR_POSITION_H_INCLUDE

class GearPosition{
  private:
    // 読み取りピン
    int pin;
    // 表示文字
    char word;

  public:
    GearPosition(int pin, char word);
};
/**
 *コンストラクタ
 */
GearPosition::GearPosition(int pin, char word){
  // 読み取りピンを定義
  this->pin = pin;
  // 表示文字定義
  this->word = word;
}

#endif
