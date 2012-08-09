/*

  OPENPIPE BREAKOUT MIDI for Arduino Music Instrument Shield
  https://www.sparkfun.com/products/10587
  
  OPENPIPE BREAKOUT is a MPR121 based touch controller with a flute-like HMI
  
  Copyright (c) 2012 Xulio Coira <xulioc@gmail.com>
  
 */


#include <Wire.h>     // Wiring two-wire library for I2C
#include "mpr121.h"   // MPR121 register definitions
#include <SoftwareSerial.h>
SoftwareSerial mySerial(2, 3); //Soft TX on 3, we don't use RX in this code

//byte note = 0; //The MIDI note value to be played
byte resetMIDI = 4; //Tied to VS1053 Reset line
byte ledPin = 13; //MIDI traffic inidicator
int instrument = 0;

#define DEBOUNCE_DELAY 10

#define MIDI_DEFAULT_CHANNEL    1

#define MIDI_COMMAND_NOTE_OFF       0x80
#define MIDI_COMMAND_NOTE_ON        0x90
#define MIDI_COMMAND_SOUNDS_OFF     0xB0

/* The format of the message to send via serial */
typedef union {
    struct {
	uint8_t command;
	uint8_t channel;
	uint8_t data2;
	uint8_t data3;
    } msg;
    uint8_t raw[4];
} t_midiMsg;


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
  59,
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
  
#define OP_CONTROL_ENABLED_FLAG  (1<<0)

// GLOBALS
unsigned char fingers;  // stores last read finger position
unsigned char control;  // stores last read control flags

unsigned char playing;

int note, previous_note;

//char touch_previous;
//unsigned char fingers;
//unsigned char fingers_previous;
//unsigned char enabled;


int MIDI_sounds_off(void){
   t_midiMsg midiMsg;
   midiMsg.msg.command = MIDI_COMMAND_SOUNDS_OFF;
   midiMsg.msg.channel = MIDI_DEFAULT_CHANNEL;
   midiMsg.msg.data2   = 120;
   midiMsg.msg.data3   = 0;	/* Velocity */
   
   Serial.write(midiMsg.raw, sizeof(midiMsg));
}

int note_on(int note, int vel){
    t_midiMsg midiMsg;
    
    midiMsg.msg.command = MIDI_COMMAND_NOTE_ON;
    midiMsg.msg.channel = MIDI_DEFAULT_CHANNEL;
    midiMsg.msg.data2   = note;
    midiMsg.msg.data3   = vel;	/* Velocity */
    
    Serial.write(midiMsg.raw, sizeof(midiMsg));
}

int note_off(int note, int vel){
    t_midiMsg midiMsg;
    
    midiMsg.msg.command = MIDI_COMMAND_NOTE_OFF;
    midiMsg.msg.channel = MIDI_DEFAULT_CHANNEL;
    midiMsg.msg.data2   = note;
    midiMsg.msg.data3   = vel;	/* Velocity */
    
    Serial.write(midiMsg.raw, sizeof(midiMsg));
}

/* Search note in fingering table based on fingers position */
int OP_finger_to_note(unsigned int fingers_position){
  
  int i;
  //int base;
  int note=0;
  unsigned long tmp;
  unsigned long fingers;
  
  fingers=fingers_position;
  
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
          // Add base note
          note += fingering_table[0];
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

void OP_read_fingers(void){
  
  int i;
  char buffer[32];

  // READ MPR121
  Wire.beginTransmission(0x5A);
  //Wire.write(0x00);
  Wire.requestFrom(0x5A, 18);
  while(Wire.available())
  { 
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
             
  control=0;
  //RIGHT THUMB
  if ( buffer[0]&(1<<5) ) control |= OP_CONTROL_ENABLED_FLAG;
  
}


//Send a MIDI note-on message.  Like pressing a piano key
//channel ranges from 0-15
void noteOn(byte channel, byte note, byte attack_velocity) {
  talkMIDI( (0x90 | channel), note, attack_velocity);
}

//Send a MIDI note-off message.  Like releasing a piano key
void noteOff(byte channel, byte note, byte release_velocity) {
  talkMIDI( (0x80 | channel), note, release_velocity);
}

void soundsOff(byte channel){
  talkMIDI( (MIDI_COMMAND_SOUNDS_OFF | channel), 120, 0);
}

//Plays a MIDI note. Doesn't check to see that cmd is greater than 127, or that data values are less than 127
void talkMIDI(byte cmd, byte data1, byte data2) {
  digitalWrite(ledPin, HIGH);
  mySerial.write(cmd);
  mySerial.write(data1);

  //Some commands only have one data byte. All cmds less than 0xBn have 2 data bytes 
  //(sort of: http://253.ccarh.org/handout/midiprotocol/)
  if( (cmd & 0xF0) <= 0xB0)
    mySerial.write(data2);

  digitalWrite(ledPin, LOW);
}




void setup()
{
    
  Serial.begin(115200);
  
  //Setup soft serial for MIDI control
  mySerial.begin(31250);
  
  //Reset the VS1053
  pinMode(resetMIDI, OUTPUT);
  digitalWrite(resetMIDI, LOW);
  delay(100);
  digitalWrite(resetMIDI, HIGH);
  delay(100);
  
  talkMIDI(0xB0, 0x07, 120); //0xB0 is channel message, set channel volume to near max (127)

  talkMIDI(0xB0, 0, 0); //Select the bank of really fun sounds

  //For this bank 0x78, the instrument does not matter, only the note
  talkMIDI(0xC0, 57, 0); //Set instrument number. 0xC0 is a 1 data byte command

 
  // Setup MPR121
  Wire.begin();
  mpr121QuickConfig();
  
  playing=0;
  previous_note=0;
}

void loop()
{ 
  // UPDATE FINGERS
  OP_read_fingers();
  
  if (! control&OP_CONTROL_ENABLED_FLAG){
    if (playing){
      //MIDI_sounds_off();
      soundsOff(0);
      previous_note=0;
    }
    playing=0;
    return;
  }else{
    if (!playing){
      noteOn(0, 36, 64);
      playing=1;
    }
  }
  note= OP_finger_to_note(fingers);
  if (note != previous_note){
    //note_off(previous_note, 127);
    noteOff(0, previous_note, 127);
    previous_note=note;
    if (note!=0xFF){
      noteOn(0, note, 127);
      //note_on(note, 127);
    }else{
      // DO NOTHING ???
    }
  } 
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




