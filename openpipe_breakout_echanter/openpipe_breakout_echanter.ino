/**
 * Copyright (c) 2012 Xulio Coira <xulioc@gmail.com>. All rights reserved.
 *
 * This file is part of openpipe_breakout_echanter
 * 
 * openpipe_breakout_echanter is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * openpipe_breakout_echanter is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with openpipe_breakout_echanter.  If not, see <http://www.gnu.org/licenses/>.
 */

/******************************************************************************
OPENPIPE BREAKOUT ECHANTER:
This Arduino sketch, inspired on echanter project (http://www.echanter.com/),
allows Openpipe Breakout (http://openpipe.cc/products/openpipe-breakout-board/)
connected to an Arduino UNO to generate bagpipes sounds, 
based on sound samples (44100 Hz @ 8bit), using PWM output.
Only one speaker/headphone is needed, in addition to the Arduino and the Openpipe Breakout.
The speaker must be connected to pins 11 and GND.

You could generate your own fingering tables with provided fingerings.py script
You could also generate your own sound samples with provided samples.py script

For more electronics bagpipes info please visit openpipe.cc
Happy OpenPiping!!!

******************************************************************************/


#include <Wire.h>     	// I2C LIBRARY FOR MPR121
#include "mpr121.h"   	// MPR121 register definitions
#include "fingerings.h"	// FINGERING TABLES
#include "samples.h"	// SOUND SAMPLES

// SELECT HERE WICH INSTRUMENT TO USE
#define GAITA_GALEGA
//#define GAITA_ASTURIANA
//#define GHB

// DISABLE DRONE COMMENTING THE FOLLOWING LINE
//#define ENABLE_DRONE

// THE FOLLOWING LINES ASSOCAITES FINGERINGS AND SOUND SAMPLES FOR EVERY INSTRUMENT
#ifdef GAITA_GALEGA
  #define FINGERING FINGERING_GAITA_GALEGA
  #define INSTRUMENT INSTRUMENT_GAITA_GALEGA
#endif

#ifdef GAITA_ASTURIANA
  #define FINGERING FINGERING_GAITA_ASTURIANA
  #define INSTRUMENT INSTRUMENT_GAITA_ASTURIANA
#endif

#ifdef GHB
  #define FINGERING FINGERING_GREAT_HIGHLAND_BAGPIPE  
  #define INSTRUMENT INSTRUMENT_GHB
#endif

#define DEBOUNCE_DELAY 10
#define SAMPLE_RATE 44100	// THIS PARAMETER MUST MATCH THE CORRESPONDING ONE IN samples.py

/* AUDIO/VISUAL OUTPUT PINS */
int LED = 13;
int speakerPin = 11;

// GLOBAL VARIABLES
uint16_t fingers;
uint16_t control;
uint8_t previous_note,note;
uint8_t previous_sample,sample, drone_sample;
uint8_t sample_index, drone_index;
uint16_t drone_sample_length;
unsigned long * fingering_table;
sample_t* samples_table;

int timestamp;

void setup()
{
  Serial.begin(115200);
  Serial.println("SETUP...");
  Serial.print("FINGERING: ");
  Serial.println(fingerings[FINGERING].name);
  fingering_table=fingerings[FINGERING].table;
  Serial.print("INSTRUMENT: ");
  Serial.println(instruments[INSTRUMENT].name);
  samples_table=instruments[INSTRUMENT].samples;
  
  Serial.print("DRONE NOTE: ");
  Serial.print(fingering_table[2],DEC);
  Serial.print(" DRONE SAMPLE: ");
  drone_sample=note_to_sample(fingering_table[2]);
  Serial.print(drone_sample,DEC);
  drone_sample_length=samples_table[drone_sample].len;
  Serial.print(" LENGTH: ");
  Serial.println(drone_sample_length,DEC);
  

  pinMode(LED,OUTPUT);
  
  pinMode(A3,OUTPUT);
  pinMode(A2,OUTPUT);
  
  digitalWrite(A3, LOW); //GND
  digitalWrite(A2, HIGH);   //VCC
  

  Wire.begin();
  mpr121QuickConfig();
  startPlayback();

  previous_note=0xFF;
  sample_index=0;
  drone_index=0;
  previous_sample=0;
  
  // UNCOMMENT FOR SOUND SAMPLE DEBUG OUTPUT
  /*
  int i;
  int16_t tmp;
  for (i=0; i< samples_table[previous_sample].len; i++){
    tmp=samples_table[previous_sample].sample[i];
    tmp = pgm_read_byte_near(((uint8_t*)samples_table[previous_sample].sample) + i);
    Serial.println(tmp);
  }
  */
     
}

void loop()
{

  static int previous_fingers=0;
  static int previous_control=0;
  int previous_note=0xFF;
  
  read_fingers(); // read MPR121 electrodes in openpipe board
  
  if (fingers!=previous_fingers || control!=previous_control){

    //TODO: add every electrode in fingerss & control
    for (int i=8; i>=0; i--){
      if (fingers&(1<<i)) Serial.print("*");
      else Serial.print("O");
    }
    Serial.print(" ");
    for (int i=2; i>=0; i--){
      if (control&(1<<i)) Serial.print("*");
      else Serial.print("O");
    }

    previous_fingers=fingers;
    previous_control=control;
    
    if (control&1){
       note=fingers_to_note(fingers);
      sample=note_to_sample(note);
      Serial.print(" NOTE: ");
      Serial.print(note);
      Serial.print(" SAMPLE: ");
      Serial.print(sample);
      Serial.println();
    }else{
      sample=0xFF;
      Serial.println(" SILENCE");
    }      
  }
    
    //TODO: DEBOUNCE

  return;
}

// read MPR121 electrodes and sort them in openpipe board order
// updates global 'fingers' and 'control' variables
// returns 'fingers'
uint16_t read_fingers(void){

  char buffer[32];
  int i=0;
  unsigned int tmp;
  
  digitalWrite(LED, HIGH);

  // READ MPR121
  Wire.beginTransmission(0x5A);
  Wire.write((uint8_t)0);
  Wire.requestFrom(0x5A, 18);
  while(Wire.available()){ 
    buffer[i] = Wire.read();
    i++;
    if (i>18) break;
  }
  Wire.endTransmission();
  
  digitalWrite(LED, LOW);

  // SORT MPR121 ELECTRODES
  fingers=   ((buffer[0]&(1<<0))>>0) | 
    ((buffer[0]&(1<<1))>>0) |
    ((buffer[0]&(1<<2))>>0) |
    ((buffer[0]&(1<<4))>>1) |
    ((buffer[0]&(1<<7))>>3) |
    ((buffer[0]&(1<<6))>>1) |
    ((buffer[1]&(1<<2))<<4) |
    ((buffer[1]&(1<<1))<<6) |
    ((buffer[1]&(1<<3))<<5);

  // READ RIGHT THUMB CONTROL ELECTRODES
  control=(buffer[0]&(1<<5))>>5;
  control|=(buffer[0]&(1<<3))>>2;
  control|=(buffer[1]&(1<<0))<<2;
  
  
  //tmp=(unsigned char)buffer[(0*2)+0x04] | (unsigned char)(buffer[(0*2)+0x05]<<8);
  //Serial.print((unsigned char)buffer[0x05], HEX);
  //Serial.print((unsigned char)buffer[0x04], HEX);
  //Serial.print(" ");
  //Serial.println(tmp, DEC);

  return fingers;
}


// search note in fingering table based on fingers position
// returns MIDI note if found, 0xFF otherwise
int fingers_to_note(uint16_t fingers_position){

  int i;
  int note=0;
  unsigned long tmp;
  unsigned long fingers;
  int base;

  fingers=fingers_position;

  base=fingering_table[0];
  i=2;
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
        return base+note;
      }
    }
    i++;
  }
  // FINGERING NOT FOUND
  return 0xFF;
}

// search sound sample index based on MIDI note
// return sample index if found, 0xFF otherwise
int note_to_sample(int note){
  int i=0;
  while(samples_table[i].note!=0xFF){
    if (samples_table[i].note==note){
      return i;
    }
    i++;
  }
  return 0xFF;
}

// read one MPR121 register at 'address'
// return the register content
char mpr121Read(unsigned char address)
{
  char data;

  Wire.beginTransmission(0x5A);  // begin communication with the MPR121 on I2C address 0x5A
  Wire.write(address);            // sets the register pointer
  Wire.requestFrom(0x5A, 1);     // request for the MPR121 to send you a single byte

  // check to see if we've received the byte over I2C
  if(1 <= Wire.available())
  {
    data = Wire.read();
  }

  Wire.endTransmission();        // ends communication

  return data;  // return the received data
}

// write one MPR121 register with 'data' at 'address'
void mpr121Write(unsigned char address, unsigned char data)
{
  Wire.beginTransmission(0x5A);  // begin communication with the MPR121 on I2C address 0x5A 
  Wire.write(address);            // sets the register pointer
  Wire.write(data);               // sends data to be stored
  Wire.endTransmission();        // ends communication
}

// configure all MPR121 registers as described in AN3944
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

  // Section F
  // Enable Auto Config and auto Reconfig @3.3V
  mpr121Write(ATO_CFG0, 0x0B);
  mpr121Write(ATO_CFGU, 0xC9);	// USL = (Vdd-0.7)/vdd*256
  mpr121Write(ATO_CFGL, 0x83);	// LSL = 0.65*USL
  mpr121Write(ATO_CFGT, 0xB5);  // Target = 0.9*USL

}

// configure PWM for sound generation
void startPlayback()
{
  pinMode(speakerPin, OUTPUT);

  // Set up Timer 2 to do pulse width modulation on the speaker
  // pin.

  // Use internal clock (datasheet p.160)
  ASSR &= ~(_BV(EXCLK) | _BV(AS2));

  // Set fast PWM mode  (p.157)
  TCCR2A |= _BV(WGM21) | _BV(WGM20);
  TCCR2B &= ~_BV(WGM22);

  // Do non-inverting PWM on pin OC2A (p.155)
  // On the Arduino this is pin 11.
  TCCR2A = (TCCR2A | _BV(COM2A1)) & ~_BV(COM2A0);
  TCCR2A &= ~(_BV(COM2B1) | _BV(COM2B0));

  // No prescaler (p.158)
  TCCR2B = (TCCR2B & ~(_BV(CS12) | _BV(CS11))) | _BV(CS10);

  // Set initial pulse width to the first sample.
  OCR2A = 0;

  // Set up Timer 1 to send a sample every interrupt.
  cli();

  // Set CTC mode (Clear Timer on Compare Match) (p.133)
  // Have to set OCR1A *after*, otherwise it gets reset to 0!
  TCCR1B = (TCCR1B & ~_BV(WGM13)) | _BV(WGM12);
  TCCR1A = TCCR1A & ~(_BV(WGM11) | _BV(WGM10));

  // No prescaler (p.134)
  TCCR1B = (TCCR1B & ~(_BV(CS12) | _BV(CS11))) | _BV(CS10);

  // Set the compare register (OCR1A).
  // OCR1A is a 16-bit register, so we have to do this with
  // interrupts disabled to be safe.
  OCR1A = F_CPU / SAMPLE_RATE;    // 16e6 / 8000 = 2000

  // Enable interrupt when TCNT1 == OCR1A (p.136)
  TIMSK1 |= _BV(OCIE1A);

  sei();
}

// stop PWM sound
void stopPlayback()
{
  // Disable playback per-sample interrupt.
  TIMSK1 &= ~_BV(OCIE1A);

  // Disable the per-sample timer completely.
  TCCR1B &= ~_BV(CS10);

  // Disable the PWM timer.
  TCCR2B &= ~_BV(CS10);

  digitalWrite(speakerPin, LOW);
}

/* PWM AUDIO CODE : This is called at SAMPLE_RATE Hz to load the next sample. */
ISR(TIMER1_COMPA_vect) {
  
  // STOP SOUND
  if (!(control&1)){
   	OCR2A=0;
    return;
  }
  
  // PLAY PREVIOUS SAMPLE IF THE CURRENT ONE IS NOT FOUND
  if (sample==0xFF){
    //OCR2A=0;
    //return;
    sample=previous_sample;
  }
  
  // WAIT FOR THE SAMPLE TO FINISH IN ORDER TO AVOID 'CLICKS'
  if (previous_sample!=sample && sample_index==0){
    previous_sample=sample;
    //sample_index=0;
  }
  
  // LOOP SAMPLE
  if (sample_index==samples_table[previous_sample].len){
    sample_index=0;
  }else{
    sample_index++;
  }
  
#ifdef ENABLE_DRONE

  // LOOP DRONE SAMPLE
  if (drone_index==drone_sample_length){
    drone_index=0;
  }else{
    drone_index++;
  }
  
  // MIX NOTE AND DRONE SAMPLES
  int16_t out;
  out=0;
  out=pgm_read_byte_near(((uint8_t*)samples_table[drone_sample].sample) + drone_index)*2;
  out+=pgm_read_byte_near(((uint8_t*)samples_table[previous_sample].sample) + sample_index)*8;
  out=out>>4;
  OCR2A=out;
#else
  // UPDATE PWM
  OCR2A=pgm_read_byte_near(((uint8_t*)samples_table[previous_sample].sample) + sample_index);
#endif

}

