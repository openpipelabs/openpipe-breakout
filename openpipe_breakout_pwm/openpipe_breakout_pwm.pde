/*

  OPENPIPE BREAKOUT PWM for Arduino Mega
  
  OPENPIPE BRAKOUNT is a MPR121 based touch controller with a flute-like HMI
  
  Copyright (c) 2012 Xulio Coira <xulioc@gmail.com>
  
  Based on: 
  http://www.prizepony.us/2010/10/using-a-mpr121-with-arduino/
  http://www.arcfn.com/2009/07/secrets-of-arduino-pwm.html
  
 */


#include <Wire.h>     // Wiring two-wire library for I2C
#include "mpr121.h"   // MPR121 register definitions

#define DEBOUNCE_DELAY 10


/* FINERING TABLE
 - 32 bit words
 - First word is base note. This constant value will be added to the note value found in table.
 - Last word indicates table end (0xFFFFFFFF).
 - Words with 1 on MSB indicate finger position
   - 16 MSBs indicate finger position (1: finger is ON, 0: finger is OFF) (MSB is not used so 15 bit positions allowed at most)
   - 16 LSBs indicate finger mask (1: finger position is relevant, 0: finger position does not care)
 - Words with 0 on MSB indicate note
    - 16 MSBs indicate note
    - 16 LSBs indicate pith change
 - Search algorythm is as follows:
    - Read note base from first word
    - Find finger position (with 1 on MSB) that matches finger and mask
    - Find note (with 0 on MSB) following the finger position
    - Add base note to found note value
    - Return note
*/ 
   
/* GALICIAN BAGPIPE FINGERING (ALPHA) */
const unsigned long fingering_table[]={
  0x00000000,
  0x80FFFFFF, 0x00000000,
  0x80FEFFFF, 0x01000000,
  0x80FDFFFF, 0x02000000,
  0x80FCFFFF, 0x03000000,
  0x80FAFFFE, 0x04000000,
  0x80F8FFFC, 0x05000000,
  0x80F0FFF8, 0x06000000,
  0x80E8FFF8, 0x07000000,
  0x80E0FFF0, 0x08000000,
  0x80D0FFF0, 0x09000000,
  0x80C0FFF0, 0x0A000000,
  0x80A0FFF0, 0x0B000000,
  0x8080FFF0, 0x807FFFFF, 0x0C000000,
  0x8000FFF0, 0x807EFFFF, 0x80BEFFFF, 0x0D000000,
  0x807CFFFF, 0x0F000000,
  0x8078FFFF, 0x11000000,
  0xFFFFFFFF};


/* Table used for converting note value to PWM register value */
int notes[]={249, 235, 222, 209, 198, 186, 176, 166, 157, 148, 140, 132, 124, 117, 111, 105, 99, 93};

char touch_previous;
unsigned char fingers;
unsigned char fingers_previous;
unsigned char enabled;
int timestamp;

/* Search note in fingering table based on fingers position */
int OP_finger_to_note(unsigned int fingers_position){
  
  int i;
  //int base;
  int note=0;
  unsigned long tmp;
  unsigned long fingers;
  
  fingers=fingers_position;
  
  //base=fingering_table[0];
  i=1;
  while(fingering_table[i]!=0xFFFFFFFF){
      tmp=fingering_table[i];
      // Seach fingering word (1 on MSB)
      if (tmp&(0x80000000)){
        // Clean MSB
        tmp&=~(0x80000000);  
        // Fingering and mask matches? 
        if ( (fingers&(tmp&0xFFFF))==(tmp>>16)){
          //Serial.print("POSITION FOUND");
          // Jump over following fingering positions (if they exist)
          while (fingering_table[i]&(0x80000000)){
             //Serial.print(".");
            i++;
          }
          // Read note
          note= (fingering_table[i]>>24) & 0xFF;
          //Serial.print("NOTE");
          //Serial.println(note, DEC);
          return note;
        }
    }
    i++;
  }
  // FINGERING NOT FOUND
  return 0xFF;
}


void setup()
{
  Serial.begin(57600); 
  
  /* SETUP PWM */
  pinMode(9,OUTPUT);
  pinMode(10,OUTPUT);
  TCCR2A = _BV(COM2A0) | _BV(COM2B1) | _BV(WGM20);
  TCCR2B = _BV(WGM22) | _BV(CS22);
  OCR2B = 10;

  Wire.begin();
  mpr121QuickConfig();
  
  touch_previous=0;
  timestamp=0;
}

void loop()
{
  char buffer[32];
  int i=0;
  int note_now;
  int note;
  
  
  // READ MPR121
  Wire.beginTransmission(0x5A);
  Wire.send(0);
  Wire.requestFrom(0x5A, 18);
  
  while(Wire.available())
  { 
    buffer[i] = Wire.receive();
    i++;
  }
  Wire.endTransmission();
  
  // SORT MPR121 ELECTRODES
  fingers=   ((buffer[0]&(1<<0))>>0) | 
             ((buffer[0]&(1<<1))>>0) |
             ((buffer[0]&(1<<2))>>0) |
             ((buffer[0]&(1<<4))>>1) |
             ((buffer[0]&(1<<7))>>3) |
             ((buffer[0]&(1<<6))>>1) |
             ((buffer[1]&(1<<2))<<4) |
             ((buffer[1]&(1<<1))<<6);
             
 // ENALBLED if right thumb is ON
 enabled=buffer[0]&(1<<5);
 
 if (!enabled){
   OCR2A = 0;  /* silence PWM */
   fingers_previous=fingers_previous+1;
   return; /* exit loop */
 }

  // Some sort of filtering      
  if (fingers != fingers_previous){
    
    if ( millis()-timestamp < DEBOUNCE_DELAY)
    {
      return;
    }
    
    timestamp=millis();
    fingers_previous=fingers;
    
    
    //Serial.print("FINGER ");
    //Serial.println(fingers, HEX);
    //Serial.print("TOUCH STATUS ");
    //Serial.println(buffer[0], DEC);
    
    // Search note in fingering table
    note= OP_finger_to_note(fingers);
    
    if (note!=0xFF){
      // Update PWM register
      OCR2A = notes[note];
    }else{
      // Silence PWM
      OCR2A = 0;
    }
  
  }else{
    timestamp=millis();
  }  
}

char mpr121Read(unsigned char address)
{
  char data;

  Wire.beginTransmission(0x5A);  // begin communication with the MPR121 on I2C address 0x5A
  Wire.send(address);            // sets the register pointer
  Wire.requestFrom(0x5A, 1);     // request for the MPR121 to send you a single byte

  // check to see if we've received the byte over I2C
  if(1 <= Wire.available())
  {
    data = Wire.receive();
  }

  Wire.endTransmission();        // ends communication

  return data;  // return the received data
}

void mpr121Write(unsigned char address, unsigned char data)
{
  Wire.beginTransmission(0x5A);  // begin communication with the MPR121 on I2C address 0x5A 
  Wire.send(address);            // sets the register pointer
  Wire.send(data);               // sends data to be stored
  Wire.endTransmission();        // ends communication
}

// MPR121 Quick Config
// This will configure all registers as described in AN3944
// Input: none
// Output: none

void mpr121QuickConfig(void)
{
  // Section A
  // This group controls filtering when data is > baseline.
  mpr121Write(MHD_R, 0x01);
  mpr121Write(NHD_R, 0x01);
  mpr121Write(NCL_R, 0x00);
  mpr121Write(FDL_R, 0x00);

  // Section B
  // This group controls filtering when data is < baseline.
  mpr121Write(MHD_F, 0x01);
  mpr121Write(NHD_F, 0x01);
  mpr121Write(NCL_F, 0xFF);
  mpr121Write(FDL_F, 0x02);

  // Section C
  // This group sets touch and release thresholds for each electrode
  mpr121Write(ELE0_T, TOU_THRESH);
  mpr121Write(ELE0_R, REL_THRESH);
  mpr121Write(ELE1_T, TOU_THRESH);
  mpr121Write(ELE1_R, REL_THRESH);
  mpr121Write(ELE2_T, TOU_THRESH);
  mpr121Write(ELE2_R, REL_THRESH);
  mpr121Write(ELE3_T, TOU_THRESH);
  mpr121Write(ELE3_R, REL_THRESH);
  mpr121Write(ELE4_T, TOU_THRESH);
  mpr121Write(ELE4_R, REL_THRESH);
  mpr121Write(ELE5_T, TOU_THRESH);
  mpr121Write(ELE5_R, REL_THRESH);

  mpr121Write(ELE6_T, TOU_THRESH);
  mpr121Write(ELE6_R, REL_THRESH);
  mpr121Write(ELE7_T, TOU_THRESH);
  mpr121Write(ELE7_R, REL_THRESH);
  mpr121Write(ELE8_T, TOU_THRESH);
  mpr121Write(ELE8_R, REL_THRESH);
  mpr121Write(ELE9_T, TOU_THRESH);
  mpr121Write(ELE9_R, REL_THRESH);
  mpr121Write(ELE10_T, TOU_THRESH);
  mpr121Write(ELE10_R, REL_THRESH);
  mpr121Write(ELE11_T, TOU_THRESH);
  mpr121Write(ELE11_R, REL_THRESH);

  // Section D
  // Set the Filter Configuration
  // Set ESI2
  //mpr121Write(FIL_CFG, 0x04);
  mpr121Write(FIL_CFG, 0x00);

  // Section E
  // Electrode Configuration
  // Enable 6 Electrodes and set to run mode
  // Set ELE_CFG to 0x00 to return to standby mode
  mpr121Write(ELE_CFG, 0x0C);	// Enables all 12 Electrodes
  //mpr121Write(ELE_CFG, 0x06);		// Enable first 6 electrodes

  // Section F
  // Enable Auto Config and auto Reconfig
  //mpr121Write(ATO_CFG0, 0x0B);
   	//mpr121Write(ATO_CFGU, 0xC9);	// USL = (Vdd-0.7)/vdd*256 = 0xC9 @3.3V
   	//mpr121Write(ATO_CFGL, 0x82);	// LSL = 0.65*USL = 0x82 @3.3V
   	//mpr121Write(ATO_CFGT, 0xB5);  // Target = 0.9*USL = 0xB5 @3.3V

}




