// --------------------自作クラス・ピン定義--------------------
//#include "Winker.h"
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

char  realTime[6] = " 0:00";
unsigned long realTimeLong = 0;

// --------------------インスタンス--------------------
Adafruit_ST7735 tft = Adafruit_ST7735(&SPI, TFT_CS, TFT_DC, TFT_RST);

void setup(void) {
  
  Serial.begin(9600);

  //ピンモード設定
  pinMode(POS1_PIN, INPUT_PULLUP);
  pinMode(POS2_PIN, INPUT_PULLUP);
  pinMode(POS3_PIN, INPUT_PULLUP);
  pinMode(POS4_PIN, INPUT_PULLUP);
  pinMode(POSN_PIN, INPUT_PULLUP);
  pinMode(WNK_RIGHT, INPUT_PULLUP);
  pinMode(WNK_LEFT, INPUT_PULLUP);

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
  unsigned long realTotalSec = time/1000;

  //int realSec = realTotalSec%60;
  //int realMins = realTotalSec/60;

  int _realTimeMin1 = (realTotalSec%60)%10;
  int _realTimeMin2 = (realTotalSec%60)/10;
  int _realTimeHour1 = (realTotalSec/60)%10;
  int _realTimeHour2 = (realTotalSec/60)/10;
  char _realTime[6] = "  :  ";
  _realTime[4] = '0'+_realTimeMin1;
  _realTime[3] = '0'+_realTimeMin2;
  _realTime[1] = '0'+_realTimeHour1;
  _realTime[0] = '0'+_realTimeHour2;
  


  // フォント設定
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  
  for(int i=4; i>=0; i--){
    if(realTime[i] != _realTime[i]){
      tft.fillRect(6*2*i, 128-8*2, 6*2, 8*2, ST7735_BLACK);
      tft.setCursor(6*2*i, 128-8*2);
      tft.print(_realTime[i]);
      realTime[i] = _realTime[i];
    }
  }

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
