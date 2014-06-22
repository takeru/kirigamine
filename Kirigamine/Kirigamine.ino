#include <IrCtrl.h>

int PIN_LED         = 13;
int PIN_IR_IN       =  8; // PB0 ICP1 Counter1
int PIN_IR_OUT      =  3; // PD3 OC2B Timer2

void setup()
{
  Serial.begin(115200);
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_IR_OUT, OUTPUT);
  pinMode(PIN_IR_IN,  INPUT);
  digitalWrite(PIN_IR_IN, HIGH); // pull-up
  IR_initialize();
  Serial.println("Kirigamine");
}

#define BUF_SIZE 100
char buf_data[BUF_SIZE];
int  buf_pos  = 0;

void loop()
{
  digitalWrite(PIN_LED, !digitalRead(PIN_IR_IN));
  ir_recv();

  if(0<Serial.available()){
    char c = Serial.read();
    char n = -1;
    if('0'<=c && c<='9'){
      n = c-'0';
    }else if('a'<=c && c<='f'){
      n = c - 'a' + 10;
    }else if('A'<=c && c<='F'){
      n = c - 'A' + 10;
    }

    if(0<=n){
      if(buf_pos/2<BUF_SIZE){
        if(buf_pos%2==0){
          buf_data[buf_pos/2]  = n << 4;
        }else{
          buf_data[buf_pos/2] |= n;
        }
        buf_pos += 1;
      }else{
        Serial.println("[ERROR] too long data.");
        buf_pos = 0;
      }
    }

    if((c=='\r' || c=='\n') && 1<=buf_pos/2){
      unsigned char cmd = buf_data[0];
      if(cmd==0x01){
        unsigned char repeat  = buf_data[1];
        unsigned char wait_ms = buf_data[2];
        unsigned char fmt     = buf_data[3]; // 2:NEC 4:AEHA 8:SONY
        unsigned char bits    = buf_data[4];
        if((bits+7)/8 == buf_pos/2-5){
          for(; 0<repeat; repeat--){
            delay(wait_ms);
            if(IR_xmit(fmt, (uint8_t*)(buf_data+5), bits)){
              Serial.println("[SEND] OK");
            }else{
              Serial.println("[ERROR] IR_xmit failed.");
            }
          }
        }else{
          Serial.println("[ERROR] invalid ir data.");
        }
      }else if(cmd==0x02){
        unsigned char ain = buf_data[1];
        if(buf_pos/2==2 && 0<=ain && ain<=5){
          int v = analogRead(ain);
          Serial.print("[ANALOGIN");
          Serial.print(ain);
          Serial.print("] ");
          Serial.println(v, DEC);
        }else{
          Serial.println("[ERROR] invalid ain data.");
        }
      }else{
        Serial.println("[ERROR] invalid cmd.");
      }
      buf_pos = 0;
    }
  }
}

void Serial_print_hex(unsigned char c){
  if(c<=0x0F){
    Serial.print(0, HEX);
  }
  Serial.print(c, HEX);
}

void ir_recv(void)
{
  if(IrCtrl.state!=IR_RECVED){
    return;
  }

  uint8_t d, i, l;
  uint16_t a;

  l = IrCtrl.len;
  switch (IrCtrl.fmt) {	/* Which frame arrived? */
#if IR_USE_NEC
  case NEC:	/* NEC format data frame */
    if (l == 32) {	/* Only 32-bit frame is valid */
      Serial.print("[NEC(TODO TEST IT)] 01 01 00 02 20|"); // repeat=1 wait=0ms
      Serial_print_hex(IrCtrl.buff[0]); Serial.print(" ");
      Serial_print_hex(IrCtrl.buff[1]); Serial.print(" ");
      Serial_print_hex(IrCtrl.buff[2]); Serial.print(" ");
      Serial_print_hex(IrCtrl.buff[3]); Serial.println();
      // TODO test it.
    }
    break;
  case NEC|REPT:	/* NEC repeat frame */
    Serial.println("[NEC repeat]");
    // TODO test it.
    break;
#endif
#if IR_USE_AEHA
  case AEHA:		/* AEHA format data frame */
    if ((l >= 48) && (l % 8 == 0)) {	/* Only multiple of 8 bit frame is valid */
      Serial.print("[AEHA] 01 01 00 04 "); // repeat=1 wait=0ms
      Serial_print_hex(l);
      Serial.print("|");
      l /= 8;
      for (i = 0; i < l; i++){
        Serial_print_hex(IrCtrl.buff[i]);
        Serial.print(" ");
      }
      Serial.println();
    }
    break;
  case AEHA|REPT:	/* AEHA format repeat frame */
    Serial.println("[AEHA repeat]");
    // TODO test it.
    break;
#endif
#if IR_USE_SONY
  case SONY:
    switch (l) {	/* Only 12, 15 or 20 bit frames are valid */
    case 12:
      Serial.print("[SONY12(TODO TEST IT)] 01 03 2D 08 0C|"); // repeat=3 wait=45ms
      Serial_print_hex(IrCtrl.buff[0]       ); Serial.print(" ");
      Serial_print_hex(IrCtrl.buff[1] & 0x0F);
      Serial.println("");
      break;
    case 15:
      Serial.print("[SONY15(TODO TEST IT)] 01 03 2D 08 0F|"); // repeat=3 wait=45ms
      Serial_print_hex(IrCtrl.buff[0]       ); Serial.print(" ");
      Serial_print_hex(IrCtrl.buff[1] & 0x7F);
      Serial.println("");
      break;
    case 20:
      Serial.print("[SONY20] 01 03 2D 08 14|"); // repeat=3 wait=45ms
      Serial_print_hex(IrCtrl.buff[0]       ); Serial.print(" ");
      Serial_print_hex(IrCtrl.buff[1]       ); Serial.print(" ");
      Serial_print_hex(IrCtrl.buff[2] & 0x0F);
      Serial.println("");
      break;
    }
    break;
#endif
  }
  IrCtrl.state = IR_IDLE;		/* Ready to receive next frame */
}

