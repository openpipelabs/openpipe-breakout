/**
 * Copyright (c) 2012 Xulio Coira <xulioc@gmail.com>. All rights reserved.
 *
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
 
 /******************************************************************************
OPENPIPE BREAKOUT AUDIO CODEC:
This Arduino sketch allows Openpipe Breakout [1] connected to an Arduino UNO 
using Audio Codec Shield from openmusiclabs [2] to generate bagpipes sounds 
based on sound samples (44100 Hz @ 16bit).

You could generate your own fingering tables with provided fingerings.py script
You could also generate your own sound samples with provided samples.py script

For more electronics bagpipes info please visit openpipe.cc
Happy OpenPiping!!!

[1] http://openpipe.cc/products/openpipe-breakout-board/
[2] http://www.openmusiclabs.com/projects/codec-shield/

******************************************************************************/
 
 // SELECT HERE WICH INSTRUMENT TO USE
//#define GAITA_GALEGA
#define GHB
//#define SACKPIPA

// DISABLE DRONE COMMENTING THE FOLLOWING LINE
//#define ENABLE_DRONE

// setup codec parameters
// must be done before #includes
#define SAMPLE_RATE 44 // 44.1kHz sample rate in accordance with samples.py
#define ADCS 0

#include <Wire.h>     // Wiring two-wire library for I2C
#include <SPI.h>
#include <AudioCodec.h>
#include "mpr121.h"   // MPR121 register definitions
#include "fingerings.h"
#include "samples.h"

// THE FOLLOWING LINES ASSOCAITES FINGERINGS AND SOUND SAMPLES FOR EVERY INSTRUMENT
#ifdef GAITA_GALEGA
  #define FINGERING FINGERING_GAITA_GALEGA
  #define INSTRUMENT INSTRUMENT_GAITA_GALEGA
#endif

#ifdef GHB
  #define FINGERING FINGERING_GREAT_HIGHLAND_BAGPIPE  
  #define INSTRUMENT INSTRUMENT_GHB
#endif

#ifdef SACKPIPA
  #define FINGERING FINGERING_SACKPIPA  
  #define INSTRUMENT INSTRUMENT_SACKPIPA
#endif


// GLOBAL VARIABLES

int ledPin = 13;

// create data variables for audio transfer
// even though there is no input needed, the codec requires stereo data
int left_in = 0; // in from codec (LINE_IN)
int right_in = 0;
int left_out = 0; // out to codec (HP_OUT)
int right_out = 0;

uint16_t fingers;
uint16_t control;
uint8_t previous_note,note;
unsigned long * fingering_table;
uint8_t previous_sample,sample, drone_sample;
uint16_t sample_index, drone_index;
uint16_t drone_sample_length;

sample_t* samples_table;

void setup()
{
  Serial.begin(115200);
  Serial.write("SETUP...\r\n");
  
  fingering_table=fingerings[FINGERING].table;
  samples_table=instruments[INSTRUMENT].samples;
  
  drone_sample=note_to_sample(fingering_table[2]);
  drone_sample_length=samples_table[drone_sample].len;
  
  Serial.write("FINGERING: ");
  Serial.write(fingerings[FINGERING].name);
  Serial.write("\r\n");
  Serial.write("INSTRUMENT: ");
  Serial.write(instruments[INSTRUMENT].name); 
  Serial.write("\r\n");
  
  Serial.print("DRONE NOTE: ");
  Serial.print(fingering_table[2],DEC);
  Serial.print(" DRONE SAMPLE: ");
  Serial.print(drone_sample,DEC);
  Serial.print(" LENGTH: ");
  Serial.println(drone_sample_length,DEC);
  
  /*
  previous_sample=0;
  int i;
  int16_t tmp;
  for (i=0; i< samples_table[previous_sample].len; i++){
    tmp=samples_table[previous_sample].sample[i];
    tmp = pgm_read_word_near(((int16_t*)samples_table[previous_sample].sample) + i);
    Serial.println(tmp);
  }
    */

  pinMode(ledPin,OUTPUT);
  
  Wire.begin();
  mpr121QuickConfig();
  
  previous_note=0xFF;
  sample_index=0;
  drone_index=0;
  
  AudioCodec_init(); // setup codec and microcontroller registers
  
}

void loop()
{

  static int previous_fingers=0;
  static int previous_control=0;
  int previous_note=0xFF;
  
  read_fingers();
  if (fingers!=previous_fingers || control!=previous_control){

    //TODO: add every electrode in fingerss & control
    /*
    for (int i=8; i>=0; i--){
      if (fingers&(1<<i)) Serial.print("*");
      else Serial.print("O");
    }
    Serial.print(" ");
    for (int i=0; i<1; i++){
      if (control&(1<<i)) Serial.print("*");
      else Serial.print("O");
    }
    */

    previous_fingers=fingers;
    previous_control=control;
    
    if (control&1){
      note=fingers_to_note(fingers);
      sample=note_to_sample(note);
      
      /*
      Serial.print(" NOTE: ");
      Serial.print(note);
      Serial.print(" SAMPLE: ");
      Serial.print(sample);
      Serial.println();
      */
      
    }else{
      sample=0xFF;
      Serial.println(" SILENCE");
    }      
  }
  return;
}


uint16_t read_fingers(void){

  char buffer[32];
  int i=0;

  // READ MPR121
  Wire.beginTransmission(0x5A);
  Wire.write((uint8_t)0);
  Wire.requestFrom(0x5A, 18);
  while(Wire.available()){ 
    buffer[i] = Wire.read();
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

  // READ RIGHT THUMB CONTROL ELECTRODES
  control=(buffer[0]&(1<<5))>>5;

  return fingers;
}


/* Search note in fingering table based on fingers position */
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

void mpr121Write(unsigned char address, unsigned char data)
{
  Wire.beginTransmission(0x5A);  // begin communication with the MPR121 on I2C address 0x5A 
  Wire.write(address);            // sets the register pointer
  Wire.write(data);               // sends data to be stored
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
  // Set ELE_CFG to 0x00 to return to standby mode
  mpr121Write(ELE_CFG, 0x0C);	// Enables all 12 Electrodes

  // Section F
  // Enable Auto Config and auto Reconfig
  mpr121Write(ATO_CFG0, 0x0B);
  mpr121Write(ATO_CFGU, 0xC9);	// USL = (Vdd-0.7)/vdd*256 = 0xC9 @3.3V
  mpr121Write(ATO_CFGL, 0x82);	// LSL = 0.65*USL = 0x82 @3.3V
  mpr121Write(ATO_CFGT, 0xB5);  // Target = 0.9*USL = 0xB5 @3.3V

}


// timer1 interrupt routine - all data processed here
//ISR(TIMER1_COMPA_vect, ISR_NAKED) { // dont store any registers
ISR(TIMER1_COMPA_vect) { // dont store any registers

  // &'s are necessary on data_in variables
  AudioCodec_data(&left_in, &right_in, left_out, right_out);
  
  if (sample==0xFF  && sample_index==0){
    left_out = 0; // put sinusoid out on left channel
    right_out = 0; // put inverted version out on right chanel
    return;
  }
  
  if (previous_sample!=sample && sample_index==0){
    previous_sample=sample;
    //sample_index=0;
  }
  
  if (sample_index==samples_table[previous_sample].len){
    sample_index=0;
  }else{
    sample_index++;
  }
  
  int16_t out;
  out=0;
  
#ifdef ENABLE_DRONE

  // LOOP DRONE SAMPLE
  if (drone_index==drone_sample_length){
    drone_index=0;
  }else{
    drone_index++;
  }
  
  // MIX NOTE AND DRONE SAMPLES
  out=pgm_read_word_near(((int16_t*)samples_table[drone_sample].sample) + drone_index)/32;
  out+=pgm_read_word_near(((int16_t*)samples_table[previous_sample].sample) + sample_index)/4;
#else
  out = pgm_read_word_near(((int16_t*)samples_table[previous_sample].sample) + sample_index);
#endif
  
  left_out = out;
  right_out = -out;
  
   //reti(); // dont forget to return from the interrupt
  
}
