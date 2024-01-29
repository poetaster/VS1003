/*
  vs1003_hello
   
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

#include <VS1003.h>

#include <SPI.h>

#include "CircularBufferLib.h"
CircularBuffer<char> circBuffer(1024*24);


/*
template <typename T>
void CircularBuffer<T>::FromArray(T* items, size_t numItems)
{
  _items = new T[numItems];
  _itemsEnd = _items + numItems - 1;

  _firstAccessor = _items;
  _lastAccessor = _items;
  _capacity = numItems;
  _count = 0;

  memmove(_items, items, numItems * sizeof(T));
}
*/
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

#define VS_XCS    9 // 6 Control Chip Select Pin (for accessing SPI Control/Status registers)
#define VS_XDCS   6 // 7 Data Chip Select / BSYNC Pin
#define VS_DREQ   7 // 9 Data Request Pin: Player asks for more data
#define VS_RESET  8 // 8 Reset is active low

// VS10xx SPI pin connections (both boards)
// Provided here for info only - not used in the sketch as the SPI library handles this
#define VS_MOSI   11
#define VS_MISO   12
#define VS_SCK    13
#define VS_SS     10
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
    player.playChunk((char *)bt_wav_header, 44);
}

void printArray(byte* x, int length, char separator)
{
  for (int iCount = 0; iCount < length-1; iCount++)
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
    circBuffer.FromArray(bt_wav_header, len);
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
    if ( circBuffer.Count() > 32 )
    {
      Serial.println("buffer avail");
            
            circBuffer.CopyTo((char *)mp3buff);
            // Actually send the data to the VS1053
            player.playChunk(mp3buff, 32);
            
    } 
}


void read_data_stream(const uint8_t *data, uint32_t length)
{
    //int bytes_read_from_stream = length;
    if ( ( circBuffer.Capacity() - circBuffer.Count() ) > length)
    {
        // If we get -1 here it means nothing could be read from the stream
        if (length > 0)
        {
            // Add them to the circular buffer
            circBuffer.FromArray((char *)data, length); // length seems to be 4096 every time
            //Serial.printf("\nRead %lu bytes", length);
        }
    }
}

// END kitchen


//sinewave
int F = 2;                                                   //frequency of the signal
int Fs = 500;                                                //sampling frequency
int n = 500;                                                 //number of samples
float t;                                                     //Time instance
int sampling_interval;
byte samples[500];                                           // to store the samples



void loop() {
  
  //for (int j = 0; j < 512; j++) {
    //analogWrite(10, samples[j]);
    //circBuffer.Insert((char *)samples[j]);
      for (int n = 0; n < 32; n++)
       {
         mp3buff[n] = samples[n];
       }
       player.playChunk(mp3buff, 32);
    delayMicroseconds(sampling_interval);                      //time interval
  //}
  

  
  //write_to_stream();
  //delayMicroseconds(sampling_interval); 
}


void setup () {

    // initiate a serial port at 57600
     Serial.begin(57600);
     SPI.begin();
    // internal routines                                                                                                                                                                                                                                                                                 
    // initiate a  buffer and a player player
    Serial.println("Hello");
    player.begin();
    delay(200);
    // set maximum output volume
    player.setVolume(0x00);
    delay(200);
    // set the header in the circ buffer
    send_header();
    buffer_start();
    Serial.print("Count:");
    Serial.print(circBuffer.Count());
    Serial.print("\t\tFirst:");
    Serial.print(circBuffer.First());
    Serial.print("\t\tLast:");
    Serial.print(circBuffer.Last());
    Serial.print("\t\tData:");
    printArray(circBuffer.ToArray(), circBuffer.Count(), ',');
    // write it out
    //write_to_stream();
    for (int n = 0; n < 32; n++)
    {
        t = (float) n / Fs;                                       //creating time isntance to find the 500 samples
        samples[n] = (byte) (127.0 * sin(2 * 3.14 * t) + 127.0 ); //calculating the sin value at each time instance
     }
    sampling_interval = 1000000 / (F * n);                      
   //sampling interval Ts = 1/frequency x number of sample (Ts = 1/Fn or Ts = T/n)x1000000 to convert it in µS

}

/*
int16_t my_buf[512];         // each element will hold one sample, which I think is 16 bits in your case
#define CHUNKSIZE 16      // but you want to queue 32 bytes = 16 samples per transmission. Not sure, if that detail is important.
byte buf_write_pos = 0;
byte buf_read_pos = -CHUNKSIZE;

void audioOutput(const AudioOutput f) {
  my_buf[buf_write_pos++] = f.l();      // assuming mono, for simplicity
}

bool canBufferAudioOutput() {
  if ( ((byte) (buf_write_pos - buf_read_pos) > 16) ) {
     buf_read_pos += CHUNKSIZE;
     player.playChunk((char)&my_buf[buf_read_pos], CHUNKSIZE*sizeof(my_buf[0]));
  }
  // fill buffer, until most recent buf_read_pos is hit, again.
  return (buf_write_pos != buf_read_pos);
}

*/


/*
void audioOutput(const AudioOutput f) {
   circBuffer.write((char *)f.l(), sizeof( f.l() ) );
}

// very simple, now:
bool canBufferAudioOutput() {
   while(digitalRead(VS_DREQ) && circBuffer.available()) {
     delayMicroseconds(3); // not sure, if this is really needed, but copied from your code
     int bytesRead = circBuffer.read((char *)mp3buff, 32); // If we didn't read the full 32 bytes, that's a worry
     // Actually send the data to the VS1053
     player.playChunk(mp3buff, bytesRead);
   }
   return (circBuffer.room() >= sizeof(AudioOutputStorage_t));  // room for at least one more sample
}
*/
