/*
  vs1003_hello
   
  A simple MP3 flow player to test if board and library are working together.
  The following sketch should say "Hello" every 0.5s :)
  Created 2012-04-05 by Andrey Karpov
*/

#include <VS1003.h>
//#include "cbuf.h"
#include <SPI.h>

/// before including Mozzi.h, configure external audio output mode:
#include "MozziConfigValues.h"  // for named option values
#define MOZZI_AUDIO_MODE MOZZI_OUTPUT_EXTERNAL_TIMED
#define MOZZI_AUDIO_CHANNELS MOZZI_MONO
#define CONTROL_RATE 256 // Hz, powers of 2 are most reliable

#include <Mozzi.h>
#include <Oscil.h> // oscillator template
#include <tables/sin2048_int8.h> // sine table for oscillator
// use: Oscil <table_size, update_rate> oscilName (wavetable), look in .h file of table #included above
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> aSin(SIN2048_DATA);

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

/*
void buffer_start()
{
  Serial.println("buffer begin");
    circBuffer.flush();

    delay(100);
    // this header  is needed to get the 1053 to treat the pcm data as a stream
    circBuffer.write((char *)bt_wav_header, 44);
    delay(100);
    Serial.println(circBuffer.room());
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

// END kitchen

*/





void setup () {

    // initiate a serial port at 57600
     Serial.begin(9600);
     SPI.begin();
    // internal routines                                                                                                                                                                                                                                                                                 
    // initiate a  buffer and a player player
    Serial.println("Hello");
    player.begin();
    player.setVolume(0x00);
    //player.startSong();

    delay(200);
    // set the header in the circ buffer
    //send_header();
    //buffer_start();
    // write it out
    //write_to_stream();
    startMozzi(CONTROL_RATE); // :)
    aSin.setFreq(440); // set the frequency
}


void updateControl(){
  // put changing controls in here
}

AudioOutput_t updateAudio(){
  return MonoOutput::from8Bit(aSin.next()); // return an int signal centred around 0
}


int16_t my_buf[256];         // each element will hold one sample, which I think is 16 bits in your case
#define CHUNKSIZE 16      // but you want to queue 32 bytes = 16 samples per transmission. Not sure, if that detail is important.
byte buf_write_pos = 0;
byte buf_read_pos = -CHUNKSIZE;


void audioOutput(const AudioOutput f) {
  
  my_buf[buf_write_pos++] = f.l();      // assuming mono, for simplicity
  
  if ( ((byte) (buf_write_pos - buf_read_pos) > 16) ) {
     buf_read_pos += CHUNKSIZE;
     player.playChunk((char *) &my_buf[buf_read_pos], CHUNKSIZE*sizeof(my_buf[0]));
  }
  
}

void loop() {
   // internally, sdi_send_buffer just sends one byte at the time. Duplicate that here, but avoiding the wait.

  // fill buffer, until most recent buf_read_pos is hit, again.
  //return (buf_write_pos != buf_read_pos);

   audioHook();  // needed in every Mozzi sketch
}
/*
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
