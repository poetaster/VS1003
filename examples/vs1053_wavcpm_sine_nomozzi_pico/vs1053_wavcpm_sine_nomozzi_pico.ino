#include <Arduino.h>
#include "Oscillator.h"
#include <VS1053Driver.h>
#include <SPI.h>

/*
 * from pschatz' wiki
  SCK  18
  (MI)SO  16
  (MO)SI  19
  (X)CS 17
  (X)DCS  9
  DREQ  10
  (X)RST  11
  VBUS
  GND
*/
#define VS1053_CS     17
#define VS1053_DCS    9
#define VS1053_DREQ   10

#define VS_XCS    17 // 6 Control Chip Select Pin (for accessing SPI Control/Status registers)
#define VS_XDCS   9 // 7 Data Chip Select / BSYNC Pin
#define VS_DREQ   10 // 9 Data Request Pin: Player asks for more data
#define VS_RESET  11 // 8 Reset is active low

#define VOLUME  100 // volume level 0-100

VS1053 player(VS1053_CS, VS1053_DCS, VS1053_DREQ, VS_RESET, SPI);

// VS10xx SPI pin connections (both boards)
// Provided here for info only - not used in the sketch as the SPI library handles this
#define VS_MOSI   5
#define VS_MISO   6
#define VS_SCK    4
#define VS_SS     10


// not used
//#include "CircularBufferLib.h"
#include <CircularBuffer.hpp>
CircularBuffer<char, 256> circBuffer;

// Oscillators object declarationSCI_AUDATA,16384 nano
float sRate = 48000.0f;
Oscillator ossy = Oscillator(sRate, Oscillator::sine);

// ideas from https://github.com/jeroenlukas/KitchenRadio/blob/master/src/kr_bluetoothsink.cpp

//cbuf circBuffer(1024 * 24); //64);
char readBuffer[1000] __attribute__((aligned(4)));

uint8_t mp3buff[32]; // vs1053 likes 32 bytes at a time

unsigned char bt_wav_header[44] = {
  0x52, 0x49, 0x46, 0x46, // RIFF
  0xFF, 0xFF, 0xFF, 0xFF, // size
  0x57, 0x41, 0x56, 0x45, // WAVE
  0x66, 0x6d, 0x74, 0x20, // fmt
  0x10, 0x00, 0x00, 0x00, // subchunk1size
  0x01, 0x00,             // audio format - pcm
  0x02, 0x00,             // numof channels
  0x80, 0xbb, 0x00, 0x00, //, //samplerate 44k1: 0x44, 0xac, 0x00, 0x00       48k: 48000: 0x80, 0xbb, 0x00, 0x00,
  0x10, 0xb1, 0x02, 0x00, //byterate
  0x04, 0x00,             // blockalign
  0x10, 0x00,             // bits per sample - 16
  0x64, 0x61, 0x74, 0x61, // subchunk3id -"data"
  0xFF, 0xFF, 0xFF, 0xFF  // subchunk3size (endless)
};

void send_header() {
  player.playChunk(bt_wav_header, 44);
}


//sinewave
int F = 2;                                                   //frequency of the signal
int Fs = 512;                                                //sampling frequency
int n = 512;                                                 //number of samples
float t;                                                     //Time instance
int sampling_interval;
byte samples[512];                                           // to store the samples
int offset = 0;
byte buff[512];
int bufset = 0;
int count = 0;
size_t items = 32;
unsigned long nexttick;

void loop() {
  
  float f = ossy.process()+500.0f;
  //Serial.println(f);
  mp3buff[0]= f;
  player.playChunk(mp3buff,1);
  
  /*
  mp3buff[count] = f;
  count++;
  if (count == 31) {
      player.playChunk(mp3buff, 32);
      count = 0;
  }
  
  offset += 32;
  if (offset > 512) {
    offset = 0 ;
  }
*/
}

void setup () {

  // initiate a serial port at 57600
  Serial.begin(115200);
  SPI.begin();

  ossy.setFrequency(240.0f);
  // internal routines
  // initiate a  buffer and a player player
  Serial.println("Hello");
  player.begin();
  // set maximum output volume
  player.setVolume(80);
  delay(20);
  // set the header in the circ buffer
  send_header();

//primitive sine
  for (int n = 0; n < 512; n++)
  {

    t = (float) n / Fs;                                       //creating time isntance to find the 500 samples
    samples[n] = (byte) (127.0 * sin(2 * 3.1415926535 * t) + 127.0 ); //calculating the sin value at each time instance
  }

  sampling_interval = 1000000 / (F * n);
  
  //sampling interval Ts = 1/frequency x number of sample (Ts = 1/Fn or Ts = T/n)x1000000 to convert it in µS
  nexttick = millis();

}


// unused funcs
void printArray(byte* x, int length, char separator)
{
  for (int iCount = 0; iCount < length - 1; iCount++)
  {
    Serial.print(x[iCount]);
    Serial.print(separator);
  }
  Serial.print(x[length - 1]);
  Serial.println();
}

void buffer_start()
{
  //circBuffer.clear();
  delay(100);
  // this header  is needed to get the 1053 to treat the pcm data as a stream
  size_t len = *(&bt_wav_header + 1) - bt_wav_header;
  Serial.println(len);
  //circBuffer.FromArray(bt_wav_header, len);
  //circBuffer.write((char *)bt_wav_header, 44);
  delay(100);
}

// Function to copy 'len' elements from 'src' to 'dst'
void copy(char* src, char* dst, int len) {
  memcpy(dst, src, sizeof(src[0])*len);
}


void write_to_stream()
{
  Serial.println("buffer begin");
  if ( circBuffer.size() > 32 )
  {
    Serial.println("buffer avail");

    circBuffer.copyToArray((char *)mp3buff);
    // Actually send the data to the VS1053
    player.playChunk(mp3buff, 32);

  }
}


void read_data_stream(const uint8_t *data, uint32_t length)
{
  //int bytes_read_from_stream = length;
  if ( ( circBuffer.capacity - circBuffer.size() ) > length)
  {
    // If we get -1 here it means nothing could be read from the stream
    if (length > 0)
    {
      // Add them to the circular buffer
      //circBuffer.FromArray((char *)data, length); // length seems to be 4096 every time
      //Serial.printf("\nRead %lu bytes", length);
    }
  }
}

// END kitchen
