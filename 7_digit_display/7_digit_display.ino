/*
 * 
 *  Copyright (c) 2021 Marcus Toftås
 *  
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following c_onditions:
 *  
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *  
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF c_onTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN c_onNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
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

// -- Hardware c_onstants --
#define NUM_LEDS 86                   // Number of LEDs in clock

#define DATA_PIN 6                    // D6

#define RX_PIN 8                      // D8
#define TX_PIN 9                      // D9

#define BD_SE 9600                    // Baudrate local serial when debugging
#define BD_BT 9600                    // Baudrate BT-communication

#define DHTTYPE DHT22                 // Temp and humidity hardware
#define DHTPIN 12                     // D12

// LED digit start num
// LED number that corresponds to the respective 1st, 2nd, 3rd and 4th digit
#define P1 65
#define P2 44
#define P3 21
#define P4 0

// LED initial color
#define RED 150
#define GREEN 0
#define BLUE 150


// -- Defines Special Variables --
CRGB LED[NUM_LEDS];
CRGB c_off = CRGB::Black;
CRGB c_saved = CRGB(GREEN, RED, BLUE);
CRGB c_on = c_saved;

RTC_DS3231 rtc;                       // SDA = A4, SCL = A5

Timer t_clock;                        // Automatically updates clock face w. time

SoftwareSerial bt_serial(RX_PIN, TX_PIN);

DHT dht(DHTPIN, DHTTYPE);


// -- Define Default Variables --
int i;
char input[9];
char bt_buffer[4];
char time_read[3];
int time_set[4] = {07, 00, 22, 00};   // Default time to be awake
int color[3] = {RED, GREEN, BLUE};    // Saved LED color
int enabled[3] = {1, 0, 1};           // Saved LED states
int mode = 0;                         // State, time, temp, humid

  
void setup() { 
  FastLED.addLeds<WS2812B, DATA_PIN, RGB>(LED, NUM_LEDS);  // GRB ordering is typical
  Serial.begin(BD_SE);
  bt_serial.begin(BD_BT);
  dht.begin();

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, lets set the time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  // Sets "extra" array position to 0 to avoid errors
  bt_buffer[3] = 0;
  time_read[2] = 0;

  t_clock.every(1000, display_refresh);
  display_refresh();
  
}

void loop() { 
  t_clock.update();
  read_bt();
}

void read_bt(){

  switch (read_one_byte()) {
    // Clock Mode
    case 'k':
      mode = 0;
      break;
    // Temperature Mode
    case 't':
      mode = 1;
      break;
    // Humidity Mode
    case 'h':
      mode = 2;
      break;
    // Help
    case 'q':
      print_help();
      break;
    // Change Color
    case 'c':
      read_bt_color();
      break;
    // Set sleep time
    case 'z':
      read_bt_time();
      break;
    // Change active LED's
    case 'l':
      read_bt_light();
      break;
    // Set new clock time
    case 'r':
      set_time();
      break;
    // Ask system for info
    case '?':
      delay(100);
      switch (read_one_byte()) {
        case 'k':
          print_time();
          break;
        case 't':
          print_temp();
          break;
        case 'h':
          print_humid();
          break;
        case 'c':
          print_color();
          break;
        case 'z':
          print_awake();
          break;
        case 'l':
          print_enabled();
          break;
      }
     break;
  }
}

// Prints help info
void print_help(){
  bt_serial.println(F("- List of current functions - \n"));
  bt_serial.println(F("- 'q' - Prints list of all commands"));
  bt_serial.println(F("- 'k' - Sets clock to display current time"));
  bt_serial.println(F("- 't' - Sets clock to display current temperature"));
  bt_serial.println(F("- 'h' - Sets clock to display current humidity"));
  bt_serial.println(F("- 'cxxxyyyzzz' - Sets the display color to R(xxx),G(yyy),B(zzz)"));
  bt_serial.println(F("- 'zxxxxyyyy' - Disables lights outside of time xxxx:yyyy"));
  bt_serial.println(F("- '?X' - Replace X with k, t, h, c or z to get systems current respective value"));
  bt_serial.println(F("- 'rhhmmss' - Sets time module to hh:mm:ss"));
}

// Prints temperature to BT-controller
void print_temp(){
  bt_serial.print(F("Current Temperature: "));
  bt_serial.print(dht.readTemperature());
  bt_serial.println("c");
}

// Prints humidity to BT-controller
void print_humid(){
  bt_serial.print(F("Current Humidity: "));
  bt_serial.print(dht.readHumidity());
  bt_serial.println("%");
}

// Prints time to BT-controller
void print_time(){
    char msg[24];
    DateTime now = rtc.now();
    sprintf(msg, "Current System Time: %02d:%02d:%02d", now.hour(), now.minute(), now.second());
    bt_serial.println(msg);

}

// Prints sleep/awake times to BT-controller
void print_awake(){
  char msg[25];
  sprintf(msg, "Time Awake: %02d:%02d - %02d:%02d", time_set[0], time_set[1], time_set[2], time_set[3]);
  bt_serial.println(msg);
}

// Prints enables/disabled LED's
void print_enabled(){
  char msg[25];
  sprintf(msg, "Lights enabled: %01d, %01d, %01d", enabled[0], enabled[1], enabled[2]);
  bt_serial.println(msg);
}

// Prints diode color to BT-controller
void print_color(){
  char msg[34];
  sprintf(msg, "Print Color Set To: %d, %d, %d", color[0], color[1], color[2]);
  bt_serial.println(msg);
}

// Reads input real time from the BT-Device and sets clock accordingly as hhmmss
void set_time(){
  delay(100);
  int input_num[3]; 
  for (i = 0; i < 3; i++) {
    input_num[i] = get_num(2);
    if (input_num[i] >= 60 or input_num[i] < 0) return;  
  }
  if (input_num[0] > 24) return;
  rtc.adjust(DateTime(2021, 01, 01, input_num[0], input_num[1], input_num[2]));
   
}

// Converts n chars into integer ab. i.e, a=1, b=2 returns 12
int get_num(int a){
  int ret = 0;
  for(int j = a; j > 0; j--) ret += pow(10,j - 1)*(read_one_byte() - '0');
  return ret;
}


// Read input time from the BT-Device
// Sets sleep time through: zXXXXXXXX, reads the 8 "X"'s and converts it into hhmm_1 and hhmm_2
void read_bt_time(){
  delay(100);
  for (i = 0; i < 4; i++) time_set[i] = get_num(2); 

}

// Read input color from the BT-Device
// Sets diode color thorugh: cXXXXXXXXX, reads the 9 "X"'s and converts it into RRR, GGG and BBB RGB-codes
void read_bt_color(){
  delay(100);
  for (i = 0; i < 3; i++) {
    color[i] = get_num(3) + 1;
    // Checks if inputed values are valid, sets it to 0 else
    if (color[i] > 255 || color[i] < 0){
      color[i] = 0;
      bt_serial.println(F("Error! Color val outside of range, setting to 0"));
    }
  }

  c_saved = CRGB(color[1], color[0], color[2]);
  print_color();
  
}

// Read input light number from the BT-Device
// Enables LEDS on/off through: lXXX, reads the 0 "X"'s and converts to 0 or 1 to enable/disable lights
void read_bt_light(){
  delay(100);
  for (i = 0; i < 3; i++) {
    char temp = read_one_byte();
    if (temp == '0') enabled[i] = 0;
    else if (temp == '1') enabled[i] = 1;
  }
}


// Read one byte from the BT-stream
char read_one_byte(){
    if (bt_serial.available()) return bt_serial.read(); 
    else return 0;
}

// Updates display every second with correct mode
void display_refresh(){

  // Check if awake/asleep
  set_awake();
  switch (mode) {
    case 0:
      display_clock();
      break;
    default:
      display_ht();
      break;
  }
  FastLED.show();
}

// Enables/Disables lights according to sleep schedule
void set_awake(){
  DateTime now = rtc.now();

  int current_time = now.hour()*60 + now.minute();   
  int awake = time_set[0]*60 + time_set[1];
  int sleep = time_set[2]*60 + time_set[3];
  
  if (current_time < awake or current_time >= sleep) c_on = c_off;
  else c_on = c_saved;
  
}

// Checks and sets time segments
void display_clock(){

    DateTime now = rtc.now();
    int h  = now.hour();

    // Set digits
    display_segments(P1, now.minute() % 10);    
    display_segments(P2, now.minute() / 10);
    display_segments(P3, h % 10);    
    display_segments(P4, (h / 10) == 0 ? 13 : (h / 10));  

    // Blink dots
    LED[42] = (LED[42] == c_off) ? c_on : c_off;
    LED[43] = (LED[43] == c_off) ? c_on : c_off;
}

// Checks and sets humidity segments
void display_ht() {

  float temp = mode == 1 ? dht.readTemperature() : dht.readHumidity();
  int p1_val = mode == 1 ? 11 : 12;
  
  if (isnan(temp)) {
    Serial.println("Failed to read from DHT sensor!");
  } else {

    // Set digits
    display_segments(P1, p1_val);    
    display_segments(P2, 10);
    display_segments(P3, ((int)temp) % 10);    
    display_segments(P4, temp / 10);
  }

  // Disable dots
  LED[42] = c_off;
  LED[43] = c_off;
}

// Lights up specified segments
void display_segments(int startindex, int number) {

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
    if (enabled[0] == 1) LED[3*i + startindex] = ((numbers[number] & 1 << i) == 1 << i) ? c_on : c_off;
    else LED[3*i + startindex] = c_off;
    if (enabled[1] == 1) LED[3*i + 1 + startindex] = ((numbers[number] & 1 << i) == 1 << i) ? c_on : c_off;
    else LED[3*i + 1 + startindex] = c_off;
    if (enabled[2] == 1) LED[3*i + 2 + startindex] = ((numbers[number] & 1 << i) == 1 << i) ? c_on : c_off;
    else LED[3*i + 2 + startindex] = c_off;
  } 
}
