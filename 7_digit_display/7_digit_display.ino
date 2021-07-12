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
#include <DHT.h>
#include <DHT_U.h>
#include <FastLED.h>
#include <Wire.h>
#include "RTClib.h"
#include <SoftwareSerial.h>
#include "Timer.h"
#include <stdlib.h>

// -- Hardware Constants --
#define NUM_LEDS 86
#define DATA_PIN 5 // D5
#define RX_PIN 8 // D8
#define TX_PIN 9 // D9
#define BD_SE 9600
#define BD_BT 9600
#define DHTPIN 12 //D12
#define DHTTYPE DHT22

// LED digit start num
#define p1 65
#define p2 44
#define p3 21
#define p4 0

// -- Defines Special Variables --
CRGB LED[NUM_LEDS];
RTC_DS3231 rtc; // SDA = A4, SCL = A5
Timer t_clock;
CRGB cOff = CRGB::Black;
CRGB cOn = CRGB(0, 150, 150);
SoftwareSerial BT_Serial(RX_PIN, TX_PIN);
DHT dht(DHTPIN, DHTTYPE);


// -- Define Default Variables --
int i;
char input[9];
char bt_buffer[4];
int color[3];
int mode = 0;

  
void setup() { 
  FastLED.addLeds<WS2812B, DATA_PIN, RGB>(LED, NUM_LEDS);  // GRB ordering is typical
  Serial.begin(BD_SE);
  BT_Serial.begin(BD_BT);
  dht.begin();

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, lets set the time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  bt_buffer[3] = 0;

  t_clock.every(1000, updateDisplay);
  updateDisplay();
  
}

void loop() { 
  t_clock.update();
  check_bt();
}

void check_bt(){

  switch (read_one_byte()) {
    case 'k':
      mode = 0;
      break;
    case 't':
      mode = 1;
      break;
    case 'h':
      mode = 2;
      break;
    case 'c':
    
      delay(100);
      for (i = 0; i < 9; i++) input[i] = read_one_byte();
  
      for (i = 0; i < 9; i++) {
        bt_buffer[0] = input[i++];
        bt_buffer[1] = input[i++];
        bt_buffer[2] = input[i];
        color[i/3] = atoi(bt_buffer);
        Serial.println(color[i/3]);
      }
  
      for (i = 0; i < 3; i++){
        if (color[i] > 255 || color[i] < 0){
          Serial.println("Error!");
          return;
        }
      }
  
      cOn = CRGB(color[1], color[0], color[2]);
  }
  
}

char read_one_byte(){
  
    //from bluetooth to Terminal. 
    if (BT_Serial.available()) 
      return BT_Serial.read(); 
    else {
      return 0;
    }
}

void updateDisplay(){
  switch (mode) {
    case 0:
      displayClock();
      break;
    default:
      displayHT();
      break;
  }
  FastLED.show();

}

void displayClock(){
    DateTime now = rtc.now();
    int h  = now.hour();

    // Set digits
    displaySegments(p1, now.minute() % 10);    
    displaySegments(p2, now.minute() / 10);
    displaySegments(p3, h % 10);    
    displaySegments(p4, (h / 10) == 0 ? 13 : (h / 10));  

    // Blink dots
    LED[42] = (LED[42] == cOff) ? cOn : cOff;
    LED[43] = (LED[43] == cOff) ? cOn : cOff;
}

void displayHT() {

  float temp = mode == 1 ? dht.readTemperature() : dht.readHumidity();
  int p1_val = mode == 1 ? 11 : 12;
  
  if (isnan(temp)) {
    Serial.println("Failed to read from DHT sensor!");
  } else {

    // Set digits
    displaySegments(p1, p1_val);    
    displaySegments(p2, 10);
    displaySegments(p3, ((int)temp) % 10);    
    displaySegments(p4, temp / 10);
  }

  // Disable dots
  LED[42] = cOff;
  LED[43] = cOff;
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
    LED[3*i + 2 + startindex] = ((numbers[number] & 1 << i) == 1 << i) ? cOn : cOff;
  } 
}
