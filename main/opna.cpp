// クラス OPN3L_MDL 宣言
//
class OPNA_MODULE {
  public:
    // コンストラクタ
    OPN3L_MODULE() {
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