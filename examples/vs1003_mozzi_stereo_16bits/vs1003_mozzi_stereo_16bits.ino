/*
  from vs1003_hello & and the mozzi examples
   
  A simple MP3 flow player to test if board and library are working together.
  The following sketch should say "Hello" every 0.5s :)
  Created 2012-04-05 by Andrey Karpov
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <avr/io.h>

#include <Arduino.h>

#include "printf.h"
#include <VS1003.h>

/*
 * VS1003 development board connected by it's header pins the following way:
 *
 * GND  - GND
 * XDCS - D6
 * DREQ - D7
 * XRES - D8
 * XCS  - D9
 * SCLK - D13
 * SI   - D11
 * SO   - D12
 * GND  - GND
 * 5V   - 5V
 */
VS1003 player(9, 6, 7, 8); // cs_pin, dcs_pin, dreq_pin, reset_pin

#include <MozziGuts.h>
#include <Oscil.h>
#include <tables/cos2048_int8.h> // table for Oscils to play
#include <SPI.h>

#define CONTROL_RATE 256 // Hz, powers of 2 are most reliable

// Synthesis part
Oscil<COS2048_NUM_CELLS, AUDIO_RATE> aCos1(COS2048_DATA);
Oscil<COS2048_NUM_CELLS, AUDIO_RATE> aCos2(COS2048_DATA);
Oscil<COS2048_NUM_CELLS, CONTROL_RATE> kEnv1(COS2048_DATA);
Oscil<COS2048_NUM_CELLS, CONTROL_RATE> kEnv2(COS2048_DATA);


// External audio output parameters
#define WS_pin 5   // channel select pin for the DAC
//#define AUDIO_BIAS 0  // this DAC works on 0-centered signals

// The buffer size 64 seems to be optimal. At 32 and 128 the sound might be brassy.
uint8_t mp3buff[64];


void audioOutput(const AudioOutput f) // f is a structure containing both channels

{

/* Note:
 *  the digital writes here can be optimised using portWrite if more speed is needed
 */
 
  player.playChunk(f.l(), sizeof(f.l()));
  //uint8_t bytesread = f ;//f.read(mp3buff, 64);
  //player.playChunk(mp3buff, bytesread);
  //player.playChunk(f, 64);
  /*
  digitalWrite(WS_pin, LOW);  //select Right channel
  SPI.transfer16(f.r());

  digitalWrite(WS_pin, HIGH);  // select Left channel
  SPI.transfer16(f.l());
  */
}


void setup() {
  
  // initiate SPI
    SPI.begin();
    // initiate a serial port at 57600
    Serial.begin(57600);
    // internal routines                                                                                                                                         
    printf_begin();                                                                                                                                                            
    printf_P(PSTR(__FILE__ "\r\n"));                                                                                                                                           
  // initiate a player
    player.begin();
  // set maximum output volume
    player.setVolume(0x00);

  aCos1.setFreq(440.f);
  aCos2.setFreq(220.f);
  kEnv1.setFreq(0.25f);
  kEnv2.setFreq(0.30f);
  startMozzi(CONTROL_RATE);
}



// Carry enveloppes
int env1, env2;

void updateControl() {
  env1 = kEnv1.next();
  env2 = kEnv2.next();
}


AudioOutput_t updateAudio() {
  return StereoOutput::from16Bit(aCos1.next() * env1, aCos2.next() * env2);
}


void loop() {
  audioHook();
}
