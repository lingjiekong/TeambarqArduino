/****************************************************************************
 Module
   BarqStateMachine.c
 Revision
   1.0.1
 Description
   This is a template file for implementing flat state machines under the
   Gen2 Events and Services Framework.
 Notes
 History
 When           Who     What/Why
 -------------- ---     --------
 01/15/12 11:12 jec      revisions for Gen2 framework
 11/07/11 11:26 jec      made the queue static
 10/30/11 17:59 jec      fixed references to CurrentEvent in RunTemplateSM()
 10/23/11 18:20 jec      began conversion from SMTemplate.c (02/20/07 rev)
****************************************************************************/

/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/

// Wifi Module
#include <ESP8266WiFi.h>
#include <FirebaseArduino.h>
// Accelerometer and UART Module
#include <Wire.h> // Must include Wire library for I2C
#include <SFE_MMA8452Q.h> // Includes the SFE_MMA8452Q library
#include <Adafruit_NeoPixel.h>

#ifdef __AVR__
  #include <avr/power.h>
#endif


/*----------------------------- Module Defines ----------------------------*/

#define LEDPin 13  // the digital pin the LED ring data line is connected to
#define DebugPin 15
#define ButtonPin 12
#define AccelSampleTimeLength 500 // 2000 millisecond
#define AccelCounterThreshold 50
#define AccelThreshold 2.5


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
/* prototypes for private functions for this machine.They should be functions
   relevant to the behavior of this state machine
*/

// Event Checkers
static void Check4Add(void);
static void Check4Delete(void);

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
static void fade_down(int num_steps, int wait, int R, int G, int B);
static void fade_up(int num_steps, int wait, int R, int G, int B);
static void setRingColor(uint32_t c);
static void flash(void);
static void fadered2blue(void);
static void fadeblue2red(void);

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well.
// type of state variable should match that of enum in header file
typedef enum {STATEWAIT4SERVICE, STATEONQUEUE} BarqState_t;
BarqState_t CurrentState;
MMA8452Q accel;
unsigned long LastAccelSampleTime;
float Accelz = 0;
int AccelCounter = 0;
int counterflipped = 0;
static bool Add = false;
static bool Delete = false;
static bool CurrentButtonPinStatus = false; // put pull down resistor on the circuit
static bool LastButtonPinStatus = false; // put pull down resistor on the cirucit
static uint8_t MAC_array[6];
static char MAC_char[18];
static String MAC_string;

uint32_t off = strip.Color(0,0,0);
uint32_t blue = strip.Color(0,0,100);
uint32_t blue_low = strip.Color(0,0,30);
uint32_t red = strip.Color(100,0,0);
uint32_t green = strip.Color(0,100,0);
uint32_t red_high = strip.Color(255,0,0);
uint32_t red_low = strip.Color(50,0,0);

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************/

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
  CurrentState = STATEWAIT4SERVICE;
  Serial.println("CurrentState = STATEWAIT4SERVICE");
  //Serial.println("MMA8452Q Test Code!");
}

void loop() {
  // Event Checker
  Check4Add();
  Check4Delete();

  // State Machine
  switch (CurrentState) {
    case STATEWAIT4SERVICE:
      if (true == Add){
        Delete = false;
        CurrentState = STATEONQUEUE;
        Serial.println("CurrentState = STATEONQUEUE");
        Add2Firebase();
        LEDService();
      }
    break;

    case STATEONQUEUE:
      if (true == Delete){
        CurrentState = STATEWAIT4SERVICE;
        Serial.println("CurrentState = STATEWAIT4SERVICE");
        Delete2Firebase();
      }
      FirebaseObject object = Firebase.get("0f0f1366-75d7-4a06-bb37-f03efd6ad06a/RunningQueue/5ccf7f006c6c");
      String& json = (String&)object;
      if (json.equals("null")) {
        Serial.println("Deleted");
        CurrentState = STATEWAIT4SERVICE;
        Serial.println("CurrentState = STATEWAIT4SERVICE");
        deleteService(); // led fade red to blue
      } else {
        Serial.println("exists");
      }
    break;
      /*Serial.println("JSON: " + json);
      if (json.equals("null")) {
        Serial.println("Json is string literal null");
      } else if (json == NULL) {
        Serial.println("Json is actually null");
      }*/
      
      /*if (Firebase.get("b9da2970-e73c-4700-a166-5d4de4955e1b/RunningQueue/5ccf7f006c6c") == NULL) {
          Serial.println("Order was deleted from firebase"); 
          CurrentState = STATEWAIT4SERVICE;
          Serial.println("CurrentState = STATEWAIT4SERVICE");
          fadered2blue();
          delay(1000);
          setRingColor(strip.Color(0,100,0));
      }*/
  }
}
/*---------------------------- Event Checker ---------------------------*/
static void Check4Add(void){
  if ((millis() - LastAccelSampleTime) < AccelSampleTimeLength){
    if (accel.available())
    {
      // First, use accel.read() to read the new variables:
      accel.read();
      Accelz = accel.cz;
      if (Accelz > AccelThreshold){
        AccelCounter++;
      }
    }
  }else{
    LastAccelSampleTime = millis();
    //Serial.print("The total coutner is: ");
    //Serial.println(AccelCounter);
    if (AccelCounter > AccelCounterThreshold){
      if (counterflipped % 2 == 0) {
        Add = true;
      } else {
        Delete = true;
      }
      counterflipped++;
    }else{
      Add = false;
    }
    AccelCounter = 0;
  }
}

static void Check4Delete(void){
  if (digitalRead(ButtonPin) == HIGH){
    CurrentButtonPinStatus = true;
    //Serial.println("ButtonPin is high"); 
  }else{
    CurrentButtonPinStatus = false;
    //Serial.println("ButtonPin is low"); 
  }
  if ((false == LastButtonPinStatus) && (true == CurrentButtonPinStatus)){
    Delete = true;
    //Delete2Firebase();
    Serial.println("Delete value from firebase"); 
  }else{
    Delete = false;
  }
  LastButtonPinStatus = CurrentButtonPinStatus;
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
  //LEDService();
}

static void Delete2Firebase(void){
  Firebase.remove("0f0f1366-75d7-4a06-bb37-f03efd6ad06a/RunningQueue/5ccf7f006c6c");
  deleteService();
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
}
static void LEDService(void) {
  setRingColor(blue);
  delay(1000);
  fadeblue2red();   // fade to red
  setRingColor(red);
  delay(1000);
  flash(); // order ready flash red
  setRingColor(red);
  delay(1000);
  //fadered2blue(); // fade to blue
}


void setRingColor(uint32_t c) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
  }
  strip.show();
}

void flash(void) {
  for (uint16_t i = 0; i < 10; i++) {
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
         strip.setPixelColor(j, strip.Color(0+i, 0, 100-i));
      }  
   strip.show();
   delay(20);
   }  
}

void fadered2blue(void) {
  uint16_t i, j;
   for (i=0; i<100; i++) {
      for(j=0; j<strip.numPixels(); j++) {
         strip.setPixelColor(j, strip.Color(100-i, 0, 0+i));
      }  
   strip.show();
   delay(10);
   }  
}

// fade_up - fade up to the given color
void fade_up(int num_steps, int wait, int R, int G, int B) {
   uint16_t i, j;
   
   for (i=0; i<num_steps; i++) {
      for(j=0; j<strip.numPixels(); j++) {
         strip.setPixelColor(j, strip.Color(R * i / num_steps, G * i / num_steps, B * i / num_steps));
      }  
   strip.show();
   delay(wait);
   }  
} // fade_up


void fade_down(int num_steps, int wait, int R, int G, int B) {
   uint16_t i, j;
   
   for (i=100; i>1; i--) {
      for(j=0; j<strip.numPixels(); j++) {
         strip.setPixelColor(j, strip.Color(R * i / num_steps, G * i / num_steps, B * i / num_steps));
      }  
   strip.show();
   delay(wait);
   }  
} // fade_up


