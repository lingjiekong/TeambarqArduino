/****************************************************************************
   BarqStateMachine.c
****************************************************************************/

/*----------------------------- Includes ----------------------------*/
  // Wifi
#include <ESP8266WiFi.h>
#include <FirebaseArduino.h>
  // Accelerometer and I2C
#include <SFE_MMA8452Q.h> // Includes the SFE_MMA8452Q library
#include <Wire.h> // Must include Wire library for I2C
  // LED Ring
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

/*-------------------------- Module Defines ----------------------------*/
#define LEDPin 13  // LED Ring
#define ButtonPin 12 // Delete Button
#define DebugPin 15 // Blue Init Debug LED

#define AccelSampleTimeLength 100 // 2000 millisecond
#define AccelCounterThreshold 50
#define AccelThreshold 2.5
<<<<<<< HEAD
#define MicrophoneMax 0.9
#define MicrophoneMin 0.0
#define ColorMax 100
#define ColorMin 0
=======
#define TIME_DEBOUNCE 1000
#define TIME_ACCEL_SAMPLE 500
>>>>>>> 3f192a2b5cd62dfc727fcfb89121b1555b9f016d

// Modifed NeoPixel sample for the holiday craft project
// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(16, LEDPin, NEO_GRB + NEO_KHZ800);

/*---------------------------- Module Functions ---------------------------*/

// Event Checkers
static void Check4Add(void);
static void Check4Delete(void);
static void CheckDebounceTimerExpired();

// Functions
static void WifiInit(void);
static void AccelInit(void);
static void ReadMACID(void);
static bool Add2Queue(void);
static bool Delete2Queue(void);
static void Add2Firebase(void);
static void Delete2Firebase(void);

// LED Functions
static void LEDInit(void);
static void LEDService(void);
static void deleteService(void);
static void setRingColor(uint32_t c);
static void flash(void);
static void fadered2blue(void);
static void fadeblue2red(void);

/*---------------------------- Module Variables ---------------------------*/
<<<<<<< HEAD
// everybody needs a state variable, you may need others as well.
// type of state variable should match that of enum in header file
typedef enum {STATEWAIT4SERVICE, STATEONQUEUE} BarqState_t;

// for Accel
=======

typedef enum {STATE_WAITING, STATE_IN_QUEUE} BarqState_t;
>>>>>>> 3f192a2b5cd62dfc727fcfb89121b1555b9f016d
BarqState_t CurrentState;
MMA8452Q accel;
static unsigned long LastAccelSampleTime;
static unsigned long LastTimeDebounce;
static bool Add = false;
static bool Delete = false;
static bool Debouncing_Flag = false;
static bool CurrentButtonPinStatus = false; // put pull down resistor on the circuit
static bool LastButtonPinStatus = false; // put pull down resistor on the cirucit
float Accelz = 0;
int AccelCounter = 0;
static uint8_t MAC_array[6];
static char MAC_char[18];
static String MAC_string;

// for Microphone
double MicrophonValue = 0.00;
char MicrophonePin = A0;

// for LED
static uint32_t off = strip.Color(0,0,0);
static uint32_t blue = strip.Color(0,0,100);
static uint32_t blue_low = strip.Color(0,0,30);
static uint32_t red = strip.Color(100,0,0);
static uint32_t green = strip.Color(0,100,0);
static uint32_t red_high = strip.Color(255,0,0);
static uint32_t red_low = strip.Color(50,0,0);
static uint32_t CalColor;
static bool LEDServiceFlag = false;
/*------------------------------ Module Code ------------------------------*/
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(ButtonPin, INPUT); 
  pinMode(DebugPin, OUTPUT);
  WifiInit();
  AccelInit();
  ReadMACID();
  LEDInit();
  LastAccelSampleTime = millis();
  CurrentState = STATE_WAITING;
  Serial.println("CurrentState = STATE_WAITING");
}

/*------------------------------ Main Loop ------------------------------*/
void loop() {
  
  // Event Checkers
  Check4Add();
  Check4Delete();
  CheckDebounceTimerExpired();

  // Continuous Service
  LEDService();

  // State Machine
  switch (CurrentState) {
    case STATE_WAITING:
      if (true == Add){
        LEDServiceFlag = true;
        Delete = false;
        CurrentState = STATE_IN_QUEUE;
        Serial.println("CurrentState = STATE_IN_QUEUE");
        Add2Firebase();
        //LEDService();
      }
    break;

    case STATE_IN_QUEUE:
      if (true == Delete){
        //CurrentState = STATE_WAITING;
        Serial.println("CurrentState = STATE_WAITING");
        Delete2Firebase();
        //deleteService();
      }
      FirebaseObject object = Firebase.get("0f0f1366-75d7-4a06-bb37-f03efd6ad06a/RunningQueue/5ccf7f006c6c");
      String& json = (String&)object;
      if (json.equals("null")) {
        Serial.println("Deleted");
        CurrentState = STATE_WAITING;
        Serial.println("CurrentState = STATE_WAITING");
        deleteService(); // led fade red to blue
        Delete = false;
      } else {
        //Serial.println("exists");
      }
    break;
  }
}
/*---------------------------- Event Checker ---------------------------*/
static void Check4Add(void){
  if ((millis() - LastAccelSampleTime) < TIME_ACCEL_SAMPLE){
    if (accel.available())
    {
      accel.read();
      float val = accel.cx + accel.cy + accel.cz;
      Serial.print("x: ");
      Serial.print(accel.cx);
      Serial.print(", y: ");
      Serial.print(accel.cy);
      Serial.print(", z: ");
      Serial.println(accel.cz);
      if (val > 8) {
        Serial.print(" >>>> Greater than 8, val: " );
        Serial.println(val);
        delay(1000);
        Add = true;
      }
    }
  } else {
    LastAccelSampleTime = millis();
    Add = false;
  }
}

static void Check4Delete(void){
  if ((digitalRead(ButtonPin) == HIGH)) {
    CurrentButtonPinStatus = true;
    Serial.println("ButtonPin is high"); 
  }else{
    CurrentButtonPinStatus = false;
    Serial.println("ButtonPin is low"); 
  }
  if (CurrentButtonPinStatus != LastButtonPinStatus) { 
    if ((true == CurrentButtonPinStatus) && (false == Debouncing_Flag)) { // if legit Button Press
      Delete = true;
      Debouncing_Flag = true; // Debouncing flag set, ignore subsequent button presses for duration of debounce timer
      Serial.println("Delete Button Pressed");
      LastTimeDebounce = millis();
    } else { // fake button press
      // Do nothing 
    }
  }
  LastButtonPinStatus = CurrentButtonPinStatus;
}

static void CheckDebounceTimerExpired() {
  if (millis() - LastTimeDebounce > TIME_DEBOUNCE) { // 
    Debouncing_Flag = false;
  }
}


/*---------------------------- Private Function ---------------------------*/
static void WifiInit(void){
  Serial.begin(9600);
  // connect to wifi.
  WiFi.begin("Stanford", "");
  //WiFi.begin("SUOC1788-207", "OC1788-207SU57");
  Serial.print("connecting");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("connected: ");
  Serial.println(WiFi.localIP()); 
  Firebase.begin("barq.firebaseIO.com", "");
  Serial.println("End of WifiInit"); 
}

static void AccelInit(void){
  // I2C for accelerometer int
  accel.init();
  Serial.println("End of AccelInit");
  digitalWrite(DebugPin, HIGH);
}

static void LEDInit(void) {
  // LED init
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  setRingColor(off);
}

static void Add2Firebase(void){
  Firebase.set("0f0f1366-75d7-4a06-bb37-f03efd6ad06a/RunningQueue/5ccf7f006c6c/MACid", "5ccf7f006c6c");
}

static void Delete2Firebase(void){
  Firebase.remove("0f0f1366-75d7-4a06-bb37-f03efd6ad06a/RunningQueue/5ccf7f006c6c");
}

static void ReadMACID(void){
  WiFi.macAddress(MAC_array);
  for (int i = 0; i < sizeof(MAC_array); ++i){
    sprintf(MAC_char,"%s%02x:",MAC_char,MAC_array[i]);
  }
  MAC_string = String(MAC_char);
  Serial.println("End of ReadMACID"); 
}

static void deleteService(void) {
  fadered2blue();
  delay(1000);
  setRingColor(off);
  LEDServiceFlag = false;
}

static void LEDService(void) {
<<<<<<< HEAD
  if (LEDServiceFlag == true){
    MicrophonValue = (analogRead(MicrophonePin)/1024.00);
    CalColor = (uint32_t)((((double)ColorMax - (double)ColorMin)/(MicrophoneMax - MicrophoneMin))*(MicrophonValue-MicrophoneMin));
    if (CalColor > 100)
    {
      CalColor = 100;
    }
    if (CalColor < 0)
    {
      CalColor = 0;
    }
    Serial.print("Microphone analog value is: ");
    Serial.println(MicrophonValue);   
    Serial.print("CalColor analog value is: ");
    Serial.println(CalColor);
    setRingColor(strip.Color(CalColor,0,(100-CalColor)));
    delay(100);
  }
}

=======
>>>>>>> 3f192a2b5cd62dfc727fcfb89121b1555b9f016d
//  setRingColor(blue);
//  delay(1000);
//  fadeblue2red();   // fade to red
//  setRingColor(red);
//  delay(1000);
//  flash(); // order ready flash red
//  setRingColor(red);
//  delay(1000);
//  //fadered2blue(); // fade to blue
<<<<<<< HEAD


void setRingColor(uint32_t c) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
  }
  strip.show();
=======
    setRingColor(red);
>>>>>>> 3f192a2b5cd62dfc727fcfb89121b1555b9f016d
}

void flash(void) {
  for (uint16_t i = 0; i < 5; i++) {
    setRingColor(red_high);
    delay(200);
    setRingColor(red_low);
    delay(200);
  }
}

void fadeblue2red(void) {
  uint16_t i, j;
   for (i=0; i<100; i++) {
      for(j=0; j<strip.numPixels(); j++) {
         strip.setPixelColor(j, strip.Color(0+i, 0, 100-i)); // R upto 100 is Red
      }  
   strip.show();
   delay(20);
   }  
}

void fadered2blue(void) {
  uint16_t i, j;
   for (i=0; i<100; i++) {
      for(j=0; j<strip.numPixels(); j++) {
         strip.setPixelColor(j, strip.Color(100-i, 0, 0+i)); // B upto 100 is Blue
      }  
   strip.show();
   delay(10);
   }  
}

<<<<<<< HEAD


// fade_up - fade up to the given color
//void fade_up(int num_steps, int wait, int R, int G, int B) {
//   uint16_t i, j;
//   
//   for (i=0; i<num_steps; i++) {
//      for(j=0; j<strip.numPixels(); j++) {
//         strip.setPixelColor(j, strip.Color(R * i / num_steps, G * i / num_steps, B * i / num_steps));
//      }  
//   strip.show();
//   delay(wait);
//   }  
//} // fade_up


//void fade_down(int num_steps, int wait, int R, int G, int B) {
//   uint16_t i, j;
//   
//   for (i=100; i>1; i--) {
//      for(j=0; j<strip.numPixels(); j++) {
//         strip.setPixelColor(j, strip.Color(R * i / num_steps, G * i / num_steps, B * i / num_steps));
//      }  
//   strip.show();
//   delay(wait);
//   }  
//} // fade_up

=======
void setRingColor(uint32_t c) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
  }
  strip.show();
}
>>>>>>> 3f192a2b5cd62dfc727fcfb89121b1555b9f016d

