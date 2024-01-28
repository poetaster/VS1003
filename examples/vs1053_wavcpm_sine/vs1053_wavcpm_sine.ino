/*
  vs1003_hello
   
  A simple MP3 flow player to test if board and library are working together.
  The following sketch should say "Hello" every 0.5s :)
  Created 2012-04-05 by Andrey Karpov
*/


#include <VS1003.h>
#include "cbuf.h"

#include <MozziGuts.h>
#include <Oscil.h> // oscillator template
#include <tables/sin2048_int8.h> // sine table for oscillator
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

// ideas from https://github.com/jeroenlukas/KitchenRadio/blob/master/src/kr_bluetoothsink.cpp

cbuf circBuffer(1024 * 24); //64);
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

int16_t my_buf[256];         // each element will hold one sample, which I think is 16 bits in your case
#define CHUNKSIZE 16      // but you want to queue 32 bytes = 16 samples per transmission. Not sure, if that detail is important.
byte buf_write_pos = 0;
byte buf_read_pos = -CHUNKSIZE;


// use: Oscil <table_size, update_rate> oscilName (wavetable), look in .h file of table #included above
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> aSin(SIN2048_DATA);

// use #define for CONTROL_RATE, not a constant
#define CONTROL_RATE 64 // Hz, powers of 2 are most reliable


void setup () {

    // initiate a serial port at 57600
    Serial.begin(9600);
    // internal routines                                                                                                                                                                                                                                                                                 
    // initiate a  buffer and a player player
    player.begin();
    delay(200);
    // set maximum output volume
    player.setVolume(0x00);
    delay(200);
    // set the header in the circ buffer
    buffer_start();
    // write it out
    write_to_stream();
    
    
    startMozzi(CONTROL_RATE); // :)
    aSin.setFreq(440); // set the frequency
}

void audioOutput(const AudioOutput f) {
   // This function will be called once per audio sample generated.
  //my_buf[buf_write_pos++] = (char *)f.l();      // assuming mono, for simplicity

    // Add them to the circular buffer
    if (circBuffer.room() > sizeof( f.l() ))
    {
            // Add them to the circular buffer
            circBuffer.write((char *)f.l(), sizeof( f.l() ) );

    }
}

bool canBufferAudioOutput() {
  
   // This function shall return true, whenever there is room for another audio sample.
   Serial.println("buffer begin");
   return write_to_stream();
   
  /*if ( ((byte) (buf_write_pos - buf_read_pos) > 16) ) {
     buf_read_pos += CHUNKSIZE;
     player.playChunk(&my_buf[buf_read_pos], CHUNKSIZE*sizeof(my_buf[0]) );
  }
  // fill buffer, until most recent buf_read_pos is hit, again.
  return (buf_write_pos != buf_read_pos);
  */
}

void loop() {
  audioHook(); // required here
}

void send_header() {
    player.playChunk((char *)bt_wav_header, 44);
}


void buffer_start()
{
    circBuffer.flush();
    delay(100);
    // this header  is needed to get the 1053 to treat the pcm data as a stream
    circBuffer.write((char *)bt_wav_header, 44);
    delay(100);
}

bool write_to_stream()
{
  Serial.println("buffer begin");
    if (circBuffer.available())
    {
      Serial.println("buffer avail");
            
            int bytesRead = circBuffer.read((char *)mp3buff, 32);
            // If we didn't read the full 32 bytes, that's a worry
            if (bytesRead != 32)
            {
                Serial.println("Only read n bytes from  circular buffer");
            }
            // Actually send the data to the VS1053
            player.playChunk(mp3buff, bytesRead);
            
    } else {
      return 0;
    }
    return 1;
}

void read_data_stream(const uint8_t *data, uint32_t length)
{
    //int bytes_read_from_stream = length;
    if (circBuffer.room() > length)
    {
        // If we get -1 here it means nothing could be read from the stream
        if (length > 0)
        {
            // Add them to the circular buffer
            circBuffer.write((char *)data, length); // length seems to be 4096 every time
            //Serial.printf("\nRead %lu bytes", length);
        }
    }
}

// end kitchen radio
