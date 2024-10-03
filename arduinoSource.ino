
// ビープ音を発生させるピン
#define bzzPin 29

// ギアポジション入力ピン
#define pos1 1;
#define pos2 2;
#define pos3 3;
#define pos4 4;
#define posN 5;

// ウインカー対応ピン
#define wnkRight 10
#define wnkLeft  11


void setup(){
  // ギアポジション入力ピン
  pinMode(pos1, INPUT_PULLUP);
  pinMode(pos2, INPUT_PULLUP);
  pinMode(pos3, INPUT_PULLUP);
  pinMode(pos4, INPUT_PULLUP);
  pinMode(posN, INPUT_PULLUP);
}

void loop(){
  // ギアポジションの表示値取得
  char dispChar = getPosChar();
  
}

// ギアポジションの取得
char getPosChar(){
  char dispChar = '-';
  if((digitalRead(pos1) == LOW){
    dispChar = '1';
  }
  else if((digitalRead(pos1) == LOW){
    dispChar = '2;
  }
  else if((digitalRead(pos1) == LOW){
    dispChar = '3';
  }
  else if((digitalRead(pos1) == LOW){
    dispChar = '4';
  }
  else if((digitalRead(pos1) == LOW){
    dispChar = 'N';
  }
  return dispChar;
}

bool getWinkerStatus(winkerPin){
  if(digitalRead(winkerPin) == HIGH){
    return true;
  }
  return flse;
}
