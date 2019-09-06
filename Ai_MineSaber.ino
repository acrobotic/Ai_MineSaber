#include "hit.h"
#include "idle_hq.h"
#include "on.h"
#include "off.h"
#include "swing.h"
#include <XT_DAC_Audio.h>
#include <Wire.h>
#include <QMC5883.h>
#include <FastLED.h>
#include <Filter.h>

#define HIT_THRESHOLD 4000
#define SWING_THRESHOLD 1000

XT_Wav_Class hit(hit_new_wav);
XT_Wav_Class idle(idle_wav);
XT_Wav_Class on(on_new_wav);
XT_Wav_Class off(off_new_wav);
XT_Wav_Class swing(swing_new_wav);

XT_DAC_Audio_Class DacAudio(25,0);
XT_Sequence_Class Sequence; 

QMC5883 sensor;
int x, y, z;
unsigned long reading = 0, f_reading = 0, old_f_reading = 0;
uint8_t d_counter = 0;
int d_reading = 0;
bool actionable = false;

ExponentialFilter<unsigned long> reading_filter(5, 0);

#define LED_PIN     5
#define NUM_LEDS    40
#define BRIGHTNESS  180
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];

void setup() {
  Serial.begin(115200);

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);
  mineSaberOn();
  
  sensor.begin();
}

bool first = true;
void loop() {
  sensor.calculate();
  x = sensor.getX()>>9;
  y = sensor.getY()>>9;
  z = sensor.getZ()>>9;
  reading = x*x + y*y + z*z;
  if(first) {
    reading_filter.SetCurrent(reading);
    old_f_reading = reading;
    first = false;
  }
  else
    reading_filter.Filter(reading);
  f_reading = reading_filter.Current();
  Serial.print(reading);
  Serial.print("  ---   ");
  Serial.print(f_reading);
  Serial.print("  ---   ");
  Serial.println(d_reading);
  
  if(++d_counter>10) {
    actionable = true;
    d_reading = abs(f_reading - old_f_reading);
    old_f_reading = f_reading;
    d_counter = 0;
  }
  if((d_reading > HIT_THRESHOLD) && actionable) {
    mineSaberHit();
    actionable = false;
    reading_filter.SetCurrent(reading);
  }
  else if((d_reading > SWING_THRESHOLD) && actionable) {
    mineSaberSwing();
    actionable = false;    
    reading_filter.SetCurrent(reading);
  }
  else
    mineSaberIdle();

  DacAudio.FillBuffer();  
    
  delay(50);
}

void mineSaberOn() {
  Sequence.RemoveAllPlayItems();  
  Sequence.AddPlayItem(&on);
  DacAudio.Play(&Sequence);
  unsigned long t = millis();
  int led = 0;
  while(Sequence.Playing) {
    if((led < NUM_LEDS) && ((millis()-t) > 10)){
      t = millis();
      leds[led] = CRGB::MidnightBlue;
      led++;
      FastLED.show();
    }
    // Show the leds (only one of which is set to white, from above)
    DacAudio.FillBuffer();  
  }
}

void mineSaberOff() {
  Sequence.RemoveAllPlayItems();  
  Sequence.AddPlayItem(&off);
  DacAudio.Play(&Sequence);
  unsigned long t = millis();
  int led = NUM_LEDS-1;
  while(Sequence.Playing) {
    if((led >= 0) && ((millis()-t) > 10)){
      t = millis();
      leds[led] = CRGB::Black;
      led--;
      FastLED.show();
    }
    // Show the leds (only one of which is set to white, from above)
    DacAudio.FillBuffer();  
  }
}

void mineSaberSwing() {
  Sequence.RemoveAllPlayItems();  
  Sequence.AddPlayItem(&swing);
  DacAudio.Play(&Sequence);
  unsigned long t = millis();
  int led = 0;
  while(Sequence.Playing) {
    if((led < NUM_LEDS)){
      t = millis();
      leds[led] = CRGB::Blue;
      led++;
      FastLED.show();
    }
    // Show the leds (only one of which is set to white, from above)
    DacAudio.FillBuffer();  
  }
}

void mineSaberHit() {
  Sequence.RemoveAllPlayItems();  
  Sequence.AddPlayItem(&hit);
  DacAudio.Play(&Sequence);
  unsigned long t = millis();
  int i = 0;
  FastLED.setBrightness(BRIGHTNESS);
  while(Sequence.Playing) {
    if(i++%20)
      fill_solid( leds, NUM_LEDS, CRGB::Cyan);
    else
      fill_solid( leds, NUM_LEDS, CRGB::Blue);
    FastLED.show();
    // Show the leds (only one of which is set to white, from above)
    DacAudio.FillBuffer();  
  }
}

void mineSaberIdle() {
  Sequence.RemoveAllPlayItems();    
  Sequence.AddPlayItem(&idle);
  DacAudio.Play(&Sequence);
  fill_solid( leds, NUM_LEDS, CRGB::MidnightBlue);
  FastLED.show();
}
