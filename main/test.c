//
// 簡易 S98 Player for Arduino Uno with GMC-MOD01
// IO高速版(Uno専用）
// Programmed by ponzu0147
//
// 【本プログラムの動作に必要なライブラリ】
//　・TimerCounterライブラリ（https://github.com/rcmolina/MaxDuino_v1.54）
//  ・SdFatライブラリ
//  ・SPIライブラリ
//
// 【参考】
//  ・YMF288変換基板（http://gimic.jp/index.php?GMC-MOD01%E8%AA%AC%E6%98%8E）
//　・S98v3フォーマット（http://www.vesta.dti.ne.jp/~tsato/soft_s98v3.html）
//  ・手のひらサイズのS98プレイヤーを作る（http://fcneet.hatenablog.com/entry/2015/04/14/221719）
//　・S98プレイヤを作ってみる（http://risky-safety.org/zinnia/sdl/works/fmgen/s98sdl/）
//　・YM2608 OPNA アプリケーションマニュアル
//
#include <SPI.h>
#include <SdFat.h>
#include <TimerOne.h>
//#include "TimerCounter.h"

//TimerCounter Timer1;              // preinstatiate

/*
unsigned short TimerCounter::pwmPeriod = 0;
unsigned char TimerCounter::clockSelectBits = 0;
void (*TimerCounter::isrCallback)() = NULL;

ISR(TIMER1_OVF_vect)
{
  Timer1.isrCallback();
}
*/
#include <Wire.h>                       //I2C ライブラリ

unsigned int startADR = 0x0000;
unsigned int endADR = 0x1FFF;           // アドレス上限指定 (24LC64なら0x1FFF)
unsigned int ADR;
byte data;
char ascii[16];

// GMC-MOD01関連Pin定義
//
// Pin 0, 1  はシリアル通信用に予約（今回は未使用）
// Pin 10-13 はSDカードへのSPIアクセス用に予約
// WE端子は回路上でLow（GND）に固定
// IRQ端子は未使用（Open）
//
static const int data_pins[] = { 2, 3, 4, 5, 6, 7, 8, 9 }; // D0-D7
static const int RST = 14;                           // Reset
static const int A0_ = 15;                           // A0
static const int A1_ = 16;                           // A1
static const int WR = 17;                           // Read Enable
static const int CS = 18;                           // Chip Select

// SDカード CS Pin定義
// 10pin以外には設定しないが吉
//
const int chipSelect = 10; // SDカードCS pin

// S98ファイルヘッダ構造体
//
struct S98Header {
  char Magic[3];
  char Format[1];
  unsigned long Timer;
  unsigned long Timer2;
  unsigned long Compress;
  unsigned long NamePtr;
  unsigned long DataPtr;
  unsigned long LoopPtr;
  unsigned long DeviceCount;
  unsigned long DeviceInfo;
};

// クラス OPN3L_MDL 宣言
//
class OPN3L_MDL {
  public:
    // コンストラクタ
    OPN3L_MDL() {
    }
    // モジュールリセット
    void reset() {
      digitalWrite(RST, LOW);  // モジュールリセット
      delayMicroseconds(100); // min. 25us
      digitalWrite(RST, HIGH);
      delay(100);
    }
    // レジスタライト
    void reg_write(unsigned char ifadr, unsigned char adr, unsigned char dat) {
      law_write(ifadr, adr, dat);
      switch (adr) { // データライト後のWait
        case 0x28: { // FM Address 0x28
            delayMicroseconds(25);  // min: 24us wait
            break;
          }
        case 0x10: { // RHYTHM Address 0x10
            delayMicroseconds(23);  // min: 22us wait
            break;
          }
        default: { // Other Address
            delayMicroseconds(2);  // min.1.9us wait
          }
      }
    }
  private:

    // データバスセット
    void write_data(unsigned char dat) {
      PORTD = (PORTD & 0B00000011) | (dat << 2); // ポートD2-D7へ出力データセット
      PORTB = (PORTB & 0B11111100) | (dat >> 6); // ポートD8-D9へ出力データセット
    }

    void law_write(unsigned char ifadr, unsigned char adr, unsigned char dat) {
      PORTC &= ~_BV(1);      // A0=Low
      if (ifadr)
        PORTC |= _BV(2);     // A1=High
      else
        PORTC &= ~_BV(2);    // A1=Low
      write_data(adr);       // Address set
      PORTC &= ~_BV(4);      // CS=Low
      delayMicroseconds(1);  // min: 200ns wait
      PORTC |= _BV(4);       // CS=High

      delayMicroseconds(2);  //  min: 1.9us wait

      PORTC |= _BV(1);       // A0=High
      write_data(dat);       // Data set
      PORTC &= ~_BV(4);      // CS=Low
      delayMicroseconds(1);  // min: 200ns wait
      PORTC |= _BV(4);       // CS=High
    }
};

// グローバル変数
//
OPN3L_MDL mdl;               // GMC-MOD01制御クラス
SdFat SD;
SdFile file;
SdFile root;
SdFile dir;
SdFile dataFile;               // SDカードアクセスクラス
S98Header header;            // S98形式ヘッダ格納用構造体
unsigned char dat;
volatile int loop_count = 0; // Sync waitカウント用変数
volatile boolean f_flag, p_flag, r_flag;
volatile boolean pause = 0;
volatile int rep=1;

//ファイル名を記憶しておくリスト変数
char tmp[32];
char** nameList;
//リスト内に含まれるファイル数
int numList;
int pos = 0;

void attachTimerOne()
{
  if (header.Timer == 0)
    Timer1.attachInterrupt(S98Play, 990 * 10);
  else{
    Timer1.attachInterrupt(S98Play, 980 * header.Timer );
  }
}

// Setup関数
//
void setup() {

  Wire.begin();
  Serial.begin(9600);
  int i;

  for (uint8_t i = 0; i < 8; i++){// Arduino Pin初期化
    pinMode(data_pins[i], OUTPUT);
  }
  pinMode(A0_, OUTPUT);
  pinMode(A1_, OUTPUT);
  pinMode(RD, OUTPUT);
  pinMode(CS, OUTPUT);
  pinMode(RST, OUTPUT);

  digitalWrite(RD, HIGH);  // RD端子はHigh固定
  digitalWrite(CS, HIGH);

  mdl.reset();  // GMC-MOD01リセット

  //  SDカードアクセス初期化
  pinMode(SS, OUTPUT);
  SD.begin(chipSelect);

  root.open("/");  //SDカードのルートフォルダ
  String listTemp = "";
//  file.openNext(&root, O_RDONLY);//System Volum
  while (file.openNext(&root, O_RDONLY)) {
    file.getName(tmp,32);
    String name = tmp;
    if(name !="System Volume Information"){
    //Serial.println(name);
      listTemp += name;
      listTemp += ",";
      }

    file.close();
  }

  if (listTemp.length() > 0) {
//    Serial.println(listTemp);

    //リストの要素数を数え上げる

    for (int i = 0; i < listTemp.length(); i++) {
      i = listTemp.indexOf(',', i);  //コンマの位置を探す
      numList++;
    }

    //リストの初期化
    nameList = new char* [numList];

    for (int i = 0; i < numList; i++) {
      //カンマの位置を見つけ、
      int index = listTemp.indexOf(',');
      String temp = String(listTemp.substring(0, index));
      nameList[i] = new char[temp.length() + 1];
      temp.toCharArray(nameList[i], temp.length() + 1);
      Serial.println(nameList[i]);
      listTemp.remove(0, index + 1);
    }

    Serial.println(numList);
  }
  else {
    //ファイルが見つからなかった場合は強制終了
    return;
  }
  // ファイルを開く
  if(!dataFile.open(nameList[pos])){pos=pos+1;}
  //if(!dataFile.open("ed3r_98.s98")){pos=pos+1;}
  
//  Serial.println("test");
  Serial.print(nameList[pos]);

  // S98ヘッダ情報取得
  dataFile.seekSet(0x0);
  dataFile.read(&header, sizeof(header));
  //Serial.println(sizeof(header));
  
  // データ先頭へシーク
  dataFile.seekSet(header.DataPtr);
  //Serial.println(header.DataPtr);

  // タイマ割込宣言
  // 1sync期間毎に割込を掛けるよう設定
  Timer1.initialize();
  if (header.Timer == 0)
    Timer1.attachInterrupt(S98Play, 980 * 10);
  else
    Timer1.attachInterrupt(S98Play, 990 * header.Timer );
}

// Loop関数
//
void loop() {
  int value;
//  float volt;
//  value = analogRead(A7);
  value = analogRead(A5);
  //Serial.println(value);
  //Serial.println(vvolt);

char var = Serial.read();

  switch (var) {
    case 'f':
      //hoge(var);<-数値を送って何かをさせる処理
      Serial.println(var);
      f_flag = 1;
      break;
    case 'r':
      //hoge(var);
      Serial.println(var);
      r_flag = 1;
      break;
    case 'p':
      //hoge(var);
      Serial.println(var);
      p_flag = 1;
      break;
  }

  if ( f_flag | r_flag ) {
    return;
  }
  else if ( p_flag ){
    if(!pause){
      pause = 0;
    }
  }
  else {
    if ( value < 550) {
      f_flag = 1;
    }
    else if ( value < 700) {
      p_flag = 1;
    }
    else if ( value < 800) {
      r_flag = 1;
    }
  }
  delay(10000);
}

// S98データ演奏関数
//
void S98Play() {
  int n, s, i;
  unsigned char devadr, adr, dat;
  unsigned char *p;
   
  
   // Sync wait中はwaitカウントを減じて関数を抜ける
    if (loop_count != 0) {
    loop_count--;
    return;
    }

   

  do {
    // 再生フラグ割り込み判定
    if (f_flag | p_flag | r_flag) {
         devadr = 0xFD;
         rep =1; //リピートフラグ再設定
      if (f_flag) {
        pos = pos + 1;
        if (pos == numList)
          pos = 0;
        //f_flag=0;
        Serial.println("FF..");
      }
      else if (p_flag) {
        Serial.println("Rewind");
        
      }
      else if (r_flag) {
/*        if (pos == 0)
          pos = numList;
        pos = pos - 1;
        //r_flag=0;*/
        Serial.println("RR..");

      }
    }
      else {
        //Serial.println("data satart");
        dataFile.read(&devadr,1); 
      
    }
    
    //dataFile.read(&devadr,1); 
      
    //Serial.println((unsigned char)devadr,HEX);
    switch (devadr) {
      case 0x00: { // SSG/FM共通部/リズム/FM ch1-3
          dataFile.read(&adr, 1);
          dataFile.read(&dat, 1);
          mdl.reg_write(0B0, adr, dat);
          break;
        }
      case 0x01: { // FM ch4-6
          dataFile.read(&adr, 1);
          dataFile.read(&dat, 1);
          mdl.reg_write(0B1, adr, dat);
          break;
        }
      case 0xFF: { // 1 sync wait
          loop_count = 0;
          return;
        }
      case 0xFE: { // n sync wait
          n = s = 0; i = 0;
          do {
            ++i;
            dataFile.read(&dat, 1);
            n |= (dat & 0x7f) << s;
            s += 7;
          } while (dat & 0x80);
          n += 2;
          loop_count = n - 1;
          return;
        }
      case 0xFD: { // データ末尾
            Serial.println(rep);
            delayMicroseconds(3000);
            dataFile.close();
            PORTC &= ~_BV(4);
            delayMicroseconds(1000);
            PORTC |= _BV(4);
            //dataFile = dir.openNextFile();
            //dataFile = SD.open("hoge2.s98");
          }
          if (r_flag|f_flag) {
            r_flag = 0;
            f_flag = 0;
          }
          else if(p_flag){
            if(pause)return;
            else{
              p_flag=0;
              pause=0;
            }
          }
          else {
            pos += 1;
            if ( pos == numList) {
              pos = 0;
            }
          }
          
          Serial.println(nameList[pos]);
          if (dataFile.open(nameList[pos]))
          {
            //dataFile.rewind();
            dataFile.openNext(&root,O_RDONLY);
          }
          // S98ヘッダ情報取得
          dataFile.seekSet(0x0);
          dataFile.read(&header, sizeof(header));


          // データ先頭へシーク
          dataFile.seekSet(header.DataPtr);
          Timer1.detachInterrupt();
          attachTimerOne();
          return;
      
    }
  } while (1);
}