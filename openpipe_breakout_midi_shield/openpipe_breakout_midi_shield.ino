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
OPENPIPE BREAKOUT & MIDI-USB SHIELD:
This Arduino sketch allows Openpipe Breakout [1] connected to an Arduino UNO 
using MIDI-USB Shield [2] to send MIDI commands to computer or Apple/Android
tablets.

You could generate your own fingering tables with provided fingerings.py script

For more electronics bagpipes info please visit openpipe.cc
Happy OpenPiping!!!

[1] http://openpipe.cc/products/openpipe-breakout-board/
[2] http://openpipe.cc/products/midi-usb-shield/

******************************************************************************/
 
 // SELECT HERE WICH FINGERING TO USE
//#define FINGERING FINGERING_GREAT_HIGHLAND_BAGPIPE
#define FINGERING FINGERING_GAITA_GALEGA
//#define FINGERING FINGERING_UILLEANN_PIPE
//#define FINGERING FINGERING_SACKPIPA

#include <Wire.h>
#include "mpr121.h"   // MPR121 register definitions
#include "fingerings.h"
#include <MIDI.h>

// GLOBAL VARIABLES

uint16_t fingers;
uint16_t control;
uint8_t previous_note,note;
int previous_fingers, previous_control;
unsigned long * fingering_table;
boolean noteoff;

#define LED 13   		// LED pin on Arduino board

void setup()
{
  
  pinMode(LED, OUTPUT);
  MIDI.begin(4);            	// Launch MIDI with default options
				// input channel is set to 4
  fingering_table=fingerings[FINGERING].table;
  
  Wire.begin();
  mpr121QuickConfig();
  
  previous_note=0xFF;
  previous_fingers=0xFF;
  previous_control=0xFF;
  MIDI.sendProgramChange(66,1);

  
}

void loop()
{
  
  read_fingers();
  if (fingers!=previous_fingers || control!=previous_control){

    //TODO: add every electrode in fingerss & control

    previous_fingers=fingers;
    
    if (control&1){
      note=fingers_to_note(fingers);
      noteoff=true;
      if (note!=previous_note){
         MIDI.sendNoteOff(previous_note,127,1);   // Stop the note
         MIDI.sendNoteOn(note,127,1);   // Start the note
         previous_note=note;
      }
      
    }else{
       if (noteoff){
         MIDI.sendNoteOff(note,0,1);   // Stop the note
         noteoff = false;
       }   
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

