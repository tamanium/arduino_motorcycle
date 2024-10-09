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
GearPosition::GearPosition(int p, char c){
  // 読み取りピンを定義
  pin = p;
  // 表示文字定義
  word = c;
  // ピンモード定義
  pinMode(pin, INPUT_PULLUP);
}
/**
 * 状態読み取り
 * LOWの場合true
 */
bool GearPosition::getStatus(){
  if(digitalRead(pin) == LOW){
    return true;
  }
  return false;
}
/**
 * 対応char取得
 */
char GearPosition::getWord(){
  return word;
}
#endif
