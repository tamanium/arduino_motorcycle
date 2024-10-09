// --------------------自作クラス・ピン定義--------------------
//#include "Winker.h"
#include "GearPosition.h"
#include "PinDefine.h"

// --------------------ライブラリ--------------------
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#include <SPI.h>

// --------------------定数--------------------
#define ON false
#define OFF true

#define posInterval  200
#define wnkInterval  100
#define bzzInterbal  200

#define timeFontSize 2

// --------------------ピン定義--------------------
#define TFT_MOSI  3
#define TFT_SCLK  2
#define TFT_CS    6
#define TFT_RST   8
#define TFT_DC    7

// ギアポジション入力ピン
#define pos1  9
#define pos2  10
#define pos3  11
#define pos4  12
#define posN  13

GearPosition gearPos[6] = {
  GearPosition(posN, 'N'),
  GearPosition(pos1, '1'),
  GearPosition(pos2, '2'),
  GearPosition(pos3, '3'),
  GearPosition(pos4, '4')
};

// ウインカー対応ピン
#define wnkRight 15
#define wnkLeft 14

// ビープ音を発生させるピン
#define bzzPin 29


// --------------------変数--------------------
char exPos = '-';

unsigned long startTime = 0;
unsigned long posTime = 0;
unsigned long wnkTime = 0;
unsigned long bzzTime = 1;

bool wnkRightStatus = false;
bool wnkLeftStatus = false;

bool isWnkRight = false;
bool isWnkLeft = false;

char  nowTime[6] = " 0:00";
unsigned long realTimeLong = 0;

// --------------------インスタンス--------------------
Adafruit_ST7735 tft = Adafruit_ST7735(&SPI, TFT_CS, TFT_DC, TFT_RST);
/*
GearPosition gearPos1 = GearPosition(pos1, '1');
GearPosition gearPos1 = GearPosition(pos2, '2');
GearPosition gearPos1 = GearPosition(pos3, '3');
GearPosition gearPos1 = GearPosition(pos4, '4');
GearPosition gearPos1 = GearPosition(posN, 'N');
*/
void setup(void) {
  
  Serial.begin(9600);

  //ピンモード設定
  int len = sizeof(gearPos) / sizeof(GearPosition);
  for(int i=0; i<len i++){
    pinMode(gearPos[i].pin, INPUT_PULLUP);
  }
  /*
  pinMode(POS1_PIN, INPUT_PULLUP);
  pinMode(POS2_PIN, INPUT_PULLUP);
  pinMode(POS3_PIN, INPUT_PULLUP);
  pinMode(POS4_PIN, INPUT_PULLUP);
  pinMode(POSN_PIN, INPUT_PULLUP);
  pinMode(WNK_RIGHT, INPUT_PULLUP);
  pinMode(WNK_LEFT, INPUT_PULLUP);
  */

  //SPI接続設定
  SPI.setTX(TFT_MOSI);
  SPI.setSCK(TFT_SCLK);

  // ディスプレイ初期化
  tft.initR(INITR_BLACKTAB);
  // 画面向き
  tft.setRotation(3);
  // 黒背景で表示リセット
  tft.fillScreen(ST77XX_BLACK);
  
  // 初期表示
  tft.setCursor(0, 0);
  tft.setTextColor(ST77XX_GREEN);
  tft.setTextSize(3);
  tft.setTextWrap(true);
  tft.print("hello");
  delay(2000);
  
  // ギアポジション表示開始
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(8*5+4, 8*8);
  tft.setTextSize(3);
  tft.print("gear");

  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(8);
  tft.setCursor(8*7+4, 0);
  tft.print(exPos);

  // 時計表示開始
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setCursor(0,128-8*2);
  tft.print("00:00");

/*
  tone(tonePin, 220);
  delay(100);
  noTone(tonePin);
  delay(150);
  tone(tonePin, 220);
  delay(100);
  noTone(tonePin);
  delay(150);
  */

  //起動時の時間を取得
  startTime = millis();
  posTime = startTime + posInterval;
  wnkTime = startTime + wnkInterval;
}

void loop() {
  unsigned long time = millis() - startTime;
  // --------------------時計機能--------------------
  // システム時間(秒)取得
  unsigned long newTimeSec = time/1000;

  //int realSec = realTotalSec%60;
  //int realMins = realTotalSec/60;



  // --------------------経過時間表示処理--------------------
  // 表示char配列作成(arduinoでは array[n+1]で定義
  char newTime[6] = "  :  ";
  // 経過時間を各桁で文字列化
  newTime[4] = '0' + (newTimeSec%60)%10;
  newTime[3] = '0' + (newTimeSec%60)/10;
  newTime[1] = '0' + (newTimeSec/60)%10;
  newTime[0] = '0' + (newTimeSec/60)/10;
  
  // フォント設定
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(timeFontSize);

  for(int i=4; i>=0; i--){
    // 値が同じ場合処理スキップ
    if(nowTime[i] == newTime[i]){
      continue;
    }
    // 該当表示をクリア
    tft.fillRect(6*timeFontSize*i, 128-8*timeFontSize, 6*timeFontSize, 8*timeFontSize, ST7735_BLACK);
    // カーソル設定
    tft.setCursor(6*timeFontSize*i, 128-8*timeFontSize);
    // 数値を表示
    tft.print(newTime[i]);
    // 表示データを格納
    realTime[i] = newTime[i];
  }

  // --------------------経過時間表示処理--------------------
  if(digitalRead(wnkLeft) == LOW){
    if(wnkLeftStatus == OFF){
      tft.setCursor(160-6*3, 120);
      if(bzzTime == 0){
        bzzTime = time + bzzInterbal;
        tft.setTextSize(1);
        tft.print("Zzz");
      }
    }
    wnkLeftStatus = ON;
  }
  else{
    if(wnkLeftStatus == ON){
      tft.setCursor(160-6*3, 120);
      if(bzzTime == 0){
        bzzTime = time + bzzInterbal;
        tft.setTextSize(1);
        tft.print("Zzz");
      }
    }
    wnkLeftStatus = OFF;
  }

  if(digitalRead(wnkRight) == LOW){
    if(wnkRightStatus == OFF){
      tft.setCursor(160-6*3, 120);
      if(bzzTime == 0){
        bzzTime = time + bzzInterbal;
        tft.setTextSize(1);
        tft.print("Zzz");
      }
    }
    wnkRightStatus = ON;
  }
  else{
    if(wnkRightStatus == ON){
      tft.setCursor(160-6*3, 128-8);
      if(bzzTime == 0){
        bzzTime = time + bzzInterbal;
        tft.setTextSize(1);
        tft.print("Zzz");
      }
    }
    wnkRightStatus = OFF;
  }

  if(bzzTime > 0 && bzzTime <= time ){
    tft.fillRect(160-6*3, 120, 6*3, 8, ST77XX_BLACK);
    bzzTime = 0;
  }
   

  // ギアポジ表示処理
  //if(posTime <= time ){
    // ギアポジション取得
    char pos = getPos();

    if(pos != exPos){
      tft.fillRect(8*7+4,0,6*8,8*8,ST77XX_BLACK);
      tft.setCursor(8*7+4, 0);
      tft.setTextSize(8);
      tft.print(pos);
      exPos = pos;
    }
    posTime += posInterval;  
  //}
    if(wnkRightStatus == ON){
      if(isWnkRight == false){
        tft.fillTriangle(160-32, 0, 160-32, 62, 160-0, 31, ST7735_YELLOW);
        isWnkRight = true;
      }
    }
    else if(isWnkRight == true){
      tft.fillRect(160-32, 0, 32, 63, ST77XX_BLACK);
      isWnkRight = false;
    }

    if(wnkLeftStatus == ON){
      if(isWnkLeft == false){
        tft.fillTriangle(31, 0, 31, 62, 0, 31, ST7735_YELLOW);
        isWnkLeft = true;
      }
    }
    else if(isWnkLeft == true){
      tft.fillRect(0, 0, 32, 63, ST77XX_BLACK);
      isWnkLeft = false;
    }

  wnkTime += wnkInterval;
}

//ギアポジション取得
char getPos(){
  // ギアポジ表示値
  char pos = '-';
  // ギアポジ取得
  if(digitalRead(pos1) == LOW){
    pos = '1';
  }
  else if(digitalRead(pos2) == LOW){
    pos = '2';
  }
  else if(digitalRead(pos3) == LOW){
    pos = '3';
  }
  else if(digitalRead(pos4) == LOW){
    pos = '4';
  }
  else if(digitalRead(posN) == LOW){
    pos = 'N';
  }
  return pos;
}


void testdrawtext(char *text, uint16_t color) {
  tft.setCursor(0, 0);
  tft.setTextColor(color);
  tft.setTextWrap(true);
  tft.print(text);
}
