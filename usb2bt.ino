#include <hidboot.h>
#include <usbhub.h>

#include "btkeytab.h"
#include "btpins.h"

#define LEDPIN 13

enum pk_ident{
  PK_DN, PK_UP
};

void setNeededPinsToOutput();
void modRowAndColumn(uint8_t oem, int8_t *row, int8_t *col);
void modPinNoForRowAndColumn(int8_t row, int8_t col, int8_t *prow, int8_t *pcol);
void outputToBT(uint8_t key, MODIFIERKEYS mod, char ch, enum pk_ident ident);
void outputToBTControl(MODIFIERKEYS mod);

void setNeededPinsToOutput()
{
  int i;

  pinMode(LEDPIN, OUTPUT);

  for(i=0; i<=7; i++)
    pinMode(table_pinno_row[i], OUTPUT);
  for(i=0; i<=14; i++)
    pinMode(table_pinno_col[i], OUTPUT);

  pinMode(PORT_CTRL, OUTPUT);
  pinMode(PORT_ALT, OUTPUT);
  pinMode(PORT_SHIFT, OUTPUT);
  pinMode(PORT_GUI, OUTPUT);

  return;
}

void modRowAndColumn(uint8_t oem, int8_t *row, int8_t *col)
{
  if((ch<0)||(ch>0xE7)){
    *row=-1;
    *col=-1;
  }else{
    *row=table_normal_keys[ch][0];
    *col=table_normal_keys[ch][1];
  }
  
  return;
}

void modPinNoForRowAndColumn(int8_t row, int8_t col, int8_t *prow, int8_t *pcol)
{
  if((row<0)||(row>7)||(col<0)||(col>14)){
    *prow=-1;
    *pcol=-1;
  }else{
    *prow=table_pinno_row[row];
    *pcol=table_pinno_col[col];
  }

  return;
}

void outputToBT(uint8_t key, MODIFIERKEYS mod, char ch, enum pk_ident ident)
{
  int8_t r, c;
  static int8_t pr, pc;

  Serial.print("I:outputToBt is called (char=");
  Serial.print((uint8_t)ch);
  Serial.print("(");
  Serial.print(ch);
  Serial.println("))");

  if(ch>='A' && ch<='Z'){
    Serial.println("I:outputToBT:ch is assumed to be lower.");
    ch-='A'-'a';
  }

  modRowAndColumn(key, &r, &c);

  if((r!=-1)&&(c!=-1)){
    modPinNoForRowAndColumn(r, c, &pr, &pc);
    if((pr!=-1)&&(pc!=-1)){
      digitalWrite(LEDPIN, LOW);
      Serial.print("I:outputToBT:(pr,pc,ident)=(");
      Serial.print(pr);
      Serial.print(",");
      Serial.print(pc);
      Serial.print(",");
      Serial.print(ident==PK_DN?"DN":"UP");
      Serial.println(")");
      digitalWrite(pr, ident==PK_DN?HIGH:LOW);
      digitalWrite(pc, ident==PK_DN?HIGH:LOW);
    }else{
      Serial.println("W:Row or column is undefined in modPinNoForRowAndColumn.");
      digitalWrite(LEDPIN, HIGH); /* means error */
    }
  }else{
    Serial.println("W:Row or column is undefined in modRowAndColumn.");
    digitalWrite(LEDPIN, HIGH); /* means error */
  }

  return;
}

void outputToBTControl(MODIFIERKEYS mod)
{
  /* note that mod.bm* are all 1 bit */
  if(( mod.bmLeftCtrl )||( mod.bmRightCtrl ))
    digitalWrite(PORT_CTRL, HIGH);
  else
    digitalWrite(PORT_CTRL, LOW);
  if(( mod.bmLeftShift )||( mod.bmRightShift ))
    digitalWrite(PORT_SHIFT, HIGH);
  else
    digitalWrite(PORT_SHIFT, LOW);
  if(( mod.bmLeftAlt )||( mod.bmRightAlt ))
    digitalWrite(PORT_ALT, HIGH);
  else
    digitalWrite(PORT_ALT, LOW);
  if(( mod.bmLeftGUI )||( mod.bmRightGUI ))
    digitalWrite(PORT_GUI, HIGH);
  else
    digitalWrite(PORT_GUI, LOW);
}

class KbdRptParser : public KeyboardReportParser
{
        void PrintKey(uint8_t mod, uint8_t key, char ch, enum pk_ident ident);

protected:
        virtual void OnControlKeysChanged(uint8_t before, uint8_t after);

	virtual void OnKeyDown	(uint8_t mod, uint8_t key);
	virtual void OnKeyUp	(uint8_t mod, uint8_t key);
	virtual void OnKeyPressed(uint8_t key);
};

void KbdRptParser::PrintKey(uint8_t m, uint8_t key, char ch, enum pk_ident ident)
{
    MODIFIERKEYS mod;
    *((uint8_t*)&mod) = m;
    Serial.print((mod.bmLeftCtrl   == 1) ? "C" : " ");
    Serial.print((mod.bmLeftShift  == 1) ? "S" : " ");
    Serial.print((mod.bmLeftAlt    == 1) ? "A" : " ");
    Serial.print((mod.bmLeftGUI    == 1) ? "G" : " ");

    Serial.print(" >");
    PrintHex<uint8_t>(key, 0x80);
    Serial.print("< ");

    Serial.print((mod.bmRightCtrl   == 1) ? "C" : " ");
    Serial.print((mod.bmRightShift  == 1) ? "S" : " ");
    Serial.print((mod.bmRightAlt    == 1) ? "A" : " ");
    Serial.println((mod.bmRightGUI    == 1) ? "G" : " ");

    outputToBT(key, mod, ch, ident);
};

void KbdRptParser::OnKeyDown(uint8_t mod, uint8_t key)
{
    Serial.print("DN ");
    uint8_t c = OemToAscii(mod, key);
    PrintKey(mod, key, (char)c, PK_DN);
   
    if (c)
        OnKeyPressed(c);
}

void KbdRptParser::OnControlKeysChanged(uint8_t before, uint8_t after) {

    MODIFIERKEYS beforeMod;
    *((uint8_t*)&beforeMod) = before;

    MODIFIERKEYS afterMod;
    *((uint8_t*)&afterMod) = after;

    if (beforeMod.bmLeftCtrl != afterMod.bmLeftCtrl) {
        Serial.println("LeftCtrl changed");
    }
    if (beforeMod.bmLeftShift != afterMod.bmLeftShift) {
        Serial.println("LeftShift changed");
    }
    if (beforeMod.bmLeftAlt != afterMod.bmLeftAlt) {
        Serial.println("LeftAlt changed");
    }
    if (beforeMod.bmLeftGUI != afterMod.bmLeftGUI) {
        Serial.println("LeftGUI changed");
    }

    if (beforeMod.bmRightCtrl != afterMod.bmRightCtrl) {
        Serial.println("RightCtrl changed");
    }
    if (beforeMod.bmRightShift != afterMod.bmRightShift) {
        Serial.println("RightShift changed");
    }
    if (beforeMod.bmRightAlt != afterMod.bmRightAlt) {
        Serial.println("RightAlt changed");
    }
    if (beforeMod.bmRightGUI != afterMod.bmRightGUI) {
        Serial.println("RightGUI changed");
    }
    
    outputToBTControl(afterMod);
}

void KbdRptParser::OnKeyUp(uint8_t mod, uint8_t key)
{
    Serial.print("UP ");
    uint8_t c = OemToAscii(mod, key);
    PrintKey(mod, key, (char)c, PK_UP);
}

void KbdRptParser::OnKeyPressed(uint8_t key)
{
    Serial.print("ASCII: ");
    Serial.println((char)key);
};

USB     Usb;
//USBHub     Hub(&Usb);
HIDBoot<HID_PROTOCOL_KEYBOARD>    HidKeyboard(&Usb);

uint32_t next_time;

KbdRptParser Prs;

void fancy_test()
{
  int i;

  for(i=0; i<=7; i++){
    digitalWrite(table_pinno_row[i], HIGH);
    delay(50);
    digitalWrite(table_pinno_row[i], LOW);
    delay(50);
  }
  for(i=0; i<=14; i++){
    digitalWrite(table_pinno_col[i], HIGH);
    delay(50);
    digitalWrite(table_pinno_col[i], LOW);
    delay(50);
  }
  digitalWrite(PORT_CTRL, HIGH);
  delay(50);
  digitalWrite(PORT_CTRL, LOW);
  delay(50);
  digitalWrite(PORT_ALT, HIGH);
  delay(50);
  digitalWrite(PORT_ALT, LOW);
  delay(50);
  digitalWrite(PORT_SHIFT, HIGH);
  delay(50);
  digitalWrite(PORT_SHIFT, LOW);
  delay(50);
  digitalWrite(PORT_GUI, HIGH);
  delay(50);
  digitalWrite(PORT_GUI, LOW);

  return;
}

void setup()
{
  setNeededPinsToOutput();
  fancy_test();

    Serial.begin( 115200 );
    /* in this program we use serial port only for debug */
    //while (!Serial); // Wait for serial port to connect - used on Leonardo, Teensy and other boards with built-in USB CDC serial connection
    Serial.println("Start");

    if (Usb.Init() == -1){
        Serial.println("Fatal error:");
        Serial.println("OSC did not start.");
	while(1){
		digitalWrite(LEDPIN, HIGH);
		delay(200);
		digitalWrite(LEDPIN, LOW);
		delay(200);
	}
    }
    delay( 200 );

    next_time = millis() + 5000;

    HidKeyboard.SetReportParser(0, (HIDReportParser*)&Prs);
}

void loop()
{
    Usb.Task();
}

