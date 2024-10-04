// --------------------自作クラス--------------------
#include "Winker.h"

// --------------------ピン定義--------------------
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
unsigned long bzzTime = 0;

bool wnkRightStatus = false;
bool wnkLeftStatus = false;

// --------------------インスタンス--------------------
Adafruit_ST7735 tft = Adafruit_ST7735(&SPI, TFT_CS, TFT_DC, TFT_RST);

void setup(void) {

  pinMode(pos1, INPUT_PULLUP);
  pinMode(pos2, INPUT_PULLUP);
  pinMode(pos3, INPUT_PULLUP);
  pinMode(pos4, INPUT_PULLUP);
  pinMode(posN, INPUT_PULLUP);

  pinMode(wnkRight, INPUT_PULLUP);
  pinMode(wnkLeft, INPUT_PULLUP);

  SPI.setTX(TFT_MOSI);
  SPI.setSCK(TFT_SCLK);

  // 初期化
  tft.initR(INITR_BLACKTAB);
  // 画面向き：右に90度回転
  tft.setRotation(1);
  // 黒背景で表示リセット
  tft.fillScreen(ST77XX_BLACK);
  
  tft.setCursor(0, 0);
  tft.setTextColor(ST77XX_GREEN);
  tft.setTextSize(3);
  tft.setTextWrap(true);
  tft.print("hello");

  delay(2000); // Pause for 2 seconds
  
  tft.fillScreen(ST77XX_BLACK);

  tft.setCursor(0, 8*3);

  tft.setTextSize(1);
  tft.print("gear");

  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(3);
  tft.setCursor(0, 0);
  tft.print(exPos);

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

  if(digitalRead(wnkLeft) == LOW){
    if(wnkLeftStatus == OFF){
      tft.setCursor(0, 50+8*3);
      if(bzzTime == 0){
        bzzTime = time + bzzInterbal;
        tft.print("Zzz");
      }
    }
    wnkLeftStatus = ON;
  }
  else{
    if(wnkLeftStatus == ON){
      tft.setCursor(0, 50+8*3);
      if(bzzTime == 0){
        bzzTime = time + bzzInterbal;
        tft.print("Zzz");
      }
    }
    wnkLeftStatus = OFF;
  }

  if(digitalRead(wnkRight) == LOW){
    if(wnkRightStatus == OFF){
      tft.setCursor(0, 50+8*3);
      if(bzzTime == 0){
        bzzTime = time + bzzInterbal;
        tft.print("Zzz");
      }
    }
    wnkRightStatus = ON;
  }
  else{
    if(wnkRightStatus == ON){
      tft.setCursor(0, 50+8*3);
      if(bzzTime == 0){
        bzzTime = time + bzzInterbal;
        tft.print("Zzz");
      }
    }
    wnkRightStatus = OFF;
  }

  if(bzzTime > 0 && bzzTime <= time ){
    tft.fillRect(0, 50+8*3, 6*3*3, 8*3, ST77XX_BLACK);
    bzzTime = 0;
  }
   

  // ギアポジ表示処理
  //if(posTime <= time ){
    // ギアポジション取得
    char pos = getPos();

    if(pos != exPos){
      tft.fillRect(0,0,6*3,8*3,ST77XX_BLACK);
      tft.setCursor(0, 0);
      tft.print(pos);
      exPos = pos;
    }
    posTime += posInterval;  
  //}

  // ウインカー表示処理
  //if(wnkTime <= time){
    
      if(wnkRightStatus == ON){
        tft.setCursor(0, 50);
        tft.print('<');
      }
      else{
        tft.fillRect(0, 50, 6*3, 8*3, ST77XX_BLACK);
      }

      if(wnkLeftStatus == ON){
        tft.setCursor(6*3, 50);
        tft.print('>');
      }
      else{
        tft.fillRect(6*3, 50, 6*3, 8*3, ST77XX_BLACK);
      }

    wnkTime += wnkInterval;
  //}

  //delay(200);
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
