#ifndef WINKER_H_INCLUDE
#define WINKER_H_INCLUDE

class Winker{

  private:
    // ウインカー状態読み取りピン
    int wnkPin;
    // ウインカー点灯状態
    bool exStatus;

  public:
    Winker(int pin);
};
/**
 *コンストラクタ
 */
Winker::Winker(int pin){
  // ウインカー状態読み取りピンを定義
  wnkPin = pin;
  // ウインカー点灯状態を定義（デフォルト値：false）
  exStatus = false;
}

#endif
