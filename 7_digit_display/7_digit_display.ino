/*
 * 
 *  Copyright (c) 2021 Marcus Toftås
 *  
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *  
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *  
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 * 
 * 
 * Code inspired and influenced and partly borrowed from Leon van der Beukel's 7-digit display clock.
 * Source: https://github.com/leonvandenbeukel/3D-7-Segment-Digital-Clock
 * 
 */

//  -- Used Packages --
#include <FastLED.h>
#include <Wire.h>
#include "RTClib.h"
#include <SoftwareSerial.h>
#include "Timer.h"

// -- Hardware Constants --
#define NUM_LEDS 86
#define DATA_PIN 5

// -- Defines Special Variables --
CRGB LED[NUM_LEDS];
RTC_DS3231 rtc;
Timer t_clock;
CRGB cOff = CRGB::Black;
CRGB cOn = CRGB::Green;


// -- Define Default Variables --
int i;
String btBuffer;

  
void setup() { 
  FastLED.addLeds<WS2812B, DATA_PIN, RGB>(LED, NUM_LEDS);  // GRB ordering is typical
  Serial.begin(9600);

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, lets set the time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  t_clock.every(1000, DisplayTime);
  DisplayTime();
  
}

void loop() { 
  t_clock.update();
}

void DisplayTime(){
    DateTime now = rtc.now();
      
    int h  = now.hour();
    int hl = (h / 10) == 0 ? 13 : (h / 10);
    int hr = h % 10;
    int ml = now.minute() / 10;
    int mr = now.minute() % 10;
  
    displaySegments(65, mr);    
    displaySegments(44, ml);
    displaySegments(21, hr);    
    displaySegments(0, hl);  
    displayDots(0);  
    FastLED.show();
}

void displayDots(int dotMode) {

  LED[42] = (LED[42] == cOff) ? cOn : cOff;
  LED[43] = (LED[43] == cOff) ? cOn : cOff;
  FastLED.show();
  // dotMode: 0=Both on, 1=Both Off, 2=Bottom On, 3=Blink
  //  switch (dotMode) {
  //    case 0:
  //      LED[14] = colorMODE == 0 ? colorCRGB : colorCHSV;
  //      LED[15] = colorMODE == 0 ? colorCRGB : colorCHSV; 
  //      break;
  //    case 1:
  //      LED[14] = colorOFF;
  //      LED[15] = colorOFF; 
  //      break;
  //    case 2:
  //      LED[14] = colorOFF;
  //      LED[15] = colorMODE == 0 ? colorCRGB : colorCHSV; 
  //      break;
  //    case 3:
  //      LED[14] = (LED[14] == colorOFF) ? (colorMODE == 0 ? colorCRGB : colorCHSV) : colorOFF;
  //      LED[15] = (LED[15] == colorOFF) ? (colorMODE == 0 ? colorCRGB : colorCHSV) : colorOFF;
  //      FastLED.show();  
  //      break;
  //    default:
  //      break;    
  //  }
}

void displaySegments(int startindex, int number) {

  byte numbers[] = {
    0b00111111, // 0    
    0b00000110, // 1
    0b01011011, // 2
    0b01001111, // 3
    0b01100110, // 4
    0b01101101, // 5
    0b01111101, // 6
    0b00000111, // 7
    0b01111111, // 8
    0b01101111, // 9   
    0b01100011, // º              10
    0b00111001, // C(elcius)      11
    0b01011100, // º lower        12
    0b00000000, // Empty          13
    0b01110001, // F(ahrenheit)   14
  };

  for (int i = 0; i < 7; i++) {
    LED[3*i + startindex] = ((numbers[number] & 1 << i) == 1 << i) ? cOn : cOff;
    // LED[3*i + 1 + startindex] = ((numbers[number] & 1 << i) == 1 << i) ? cOn : cOff;
    // LED[3*i + 2 + startindex] = ((numbers[number] & 1 << i) == 1 << i) ? cOn : cOff;
  } 
}
