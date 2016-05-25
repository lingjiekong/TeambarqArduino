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

#define AccelSampleTimeLength 1000 // 2000 millisecond
#define AccelCounterThreshold 100
#define AccelThreshold 2.5
#define TIME_DEBOUNCE 1000
#define TIME_LEDUPDATE 500
#define TIME_FLASHRED 100

#define AccelSampleTimeLength 1000 // 2000 millisecond
#define AccelCounterThreshold 100
#define AccelThreshold 2.5
#define TIME_DEBOUNCE 1000
#define TIME_LEDUPDATE 500
#define TIME_FLASHRED 100
#define AccelZCounterThreshold 4
#define AccelZRangeThreshold 1

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
static void CheckDebounceTimerExpired(void);
static void CheckDeletefromTablet(void);
static void CheckQueuePosition(void);
static void Check4Start(void);

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
static void FlashRedService(void);
static void fadered2blue(void);
static void fadeblue2red(void);
static void fadeblue2pink(void);
static void fadepink2red(void);
static void fadeblue2blue(void);
static void Set2Blue(void);
static void Set2Pink(void);
uint32_t Wheel(byte WheelPos);

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well.
// type of state variable should match that of enum in header file

// for Accel
typedef enum {STATE_INIT, STATE_WAITING, STATE_IN_QUEUE} BarqState_t;
BarqState_t CurrentState;
MMA8452Q accel;
static unsigned long LastAccelSampleTime;
static unsigned long LastLEDSampleTime;
static unsigned long LastTimeDebounce;
static unsigned long LastTime;
static unsigned long LastFlashRed;
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
static uint8_t AccelZCounter;
static double AccelZ;
static double AccelZMax = 0;
static double AccelZMin = 4;
static double AccelZRange;
static uint8_t AccelZSample = 0;

// for LED
static uint32_t off = strip.Color(0,0,0);
static uint32_t blue = strip.Color(0,0,100);
static uint32_t blue_low = strip.Color(0,0,30);
static uint32_t red = strip.Color(100,0,0);
static uint32_t green = strip.Color(0,100,0);
static uint32_t red_high = strip.Color(255,0,0);
static uint32_t red_low = strip.Color(50,0,0);
static bool LEDServiceFlag = true;
static uint16_t i_LED = 0;
static uint16_t j_Cycle = 0;
static uint8_t FlashRedCounter = 0;
static uint8_t CurrentQueuePos = 0;
static uint8_t LastQueuePos = 0;
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
  //Serial.print("Time to run through the framework is: ");
  //Serial.println(millis() - LastTime);
  //LastTime = millis();
  
  // State Machine
  switch (CurrentState) {
    case STATE_INIT:
     // Event Checkers
     CheckDebounceTimerExpired();
     Check4Start();
     if (true == CurrentButtonPinStatus){
      CurrentState = STATE_WAITING;
      CurrentButtonPinStatus = false;
     }
    break;
    
    case STATE_WAITING:
     // Event Checkers
      Check4Add();
      if (true == Add){
        LEDServiceFlag = false;
        Delete = false;
        CurrentState = STATE_IN_QUEUE;
        Serial.println("CurrentState = STATE_IN_QUEUE");
        Add2Firebase();
        j_Cycle = 0;
      }
      // Continuous Service
      LEDService();
      // Event Checkers for STATE_WAITING State
    break;

    case STATE_IN_QUEUE:
      // Event Checkers for STATE_IN_QUEUE State
      CheckDeletefromTablet();
      CheckQueuePosition();
      Check4Delete();
      CheckDebounceTimerExpired();
      if (true == Delete){
        CurrentState = STATE_WAITING;
        Serial.println("CurrentState = STATE_WAITING");
        Delete2Firebase();
        deleteService();
      }
    break;
  }
}


/*---------------------------- Event Checker ---------------------------*/
//static void Check4Add(void){
//  if ((millis() - LastAccelSampleTime) < AccelSampleTimeLength){
//    if (accel.available())
//    {
//      // First, use accel.read() to read the new variables:
//      accel.read();
//      Accelz = accel.cz;
//      if (Accelz > AccelThreshold){
//        AccelCounter++;
//      }
//    }
//  }else{
//    LastAccelSampleTime = millis();
//    //Serial.print("The total coutner is: ");
//    //Serial.println(AccelCounter);
//    if (AccelCounter > AccelCounterThreshold){
//      Add = true;
//    }else{
//      Add = false;
//    }
//    AccelCounter = 0;
//  }
//}

static void Check4Add(void)
{
  if ((millis() - LastAccelSampleTime) < AccelSampleTimeLength)
  {
    if(accel.available()){
      accel.read();
      AccelZ = accel.cz;
      //Serial.print("AccelZ value is: ");
      //Serial.println(AccelZ);
      if (AccelZ > AccelZMax)
      {
        AccelZMax = AccelZ;
      }
      if (AccelZ < AccelZMin)
      {
        AccelZMin = AccelZ;
      }
      AccelZSample++;
      if (AccelZSample > 10){
        AccelZRange = (AccelZMax - AccelZMin);
        AccelZSample = 0;
        AccelZMax = 0;
        AccelZMin = 4;
        //Serial.print("AccelZRange value is: ");
        //Serial.println(AccelZRange);
        if (AccelZRange > AccelZRangeThreshold)
        {
          AccelZCounter++;
        }
      }
    }
  }
  else
  {
      LastAccelSampleTime = millis();
      //Serial.print("The AccelZ counter is: ");
      //Serial.println(AccelZCounter);
      if (AccelZCounter > AccelZCounterThreshold)
      {
        Add = true;
        fadeblue2blue();
      }
      else
      {
        Add = false;
      }
      AccelZCounter = 0;
  }
}

static void Check4Start(void){
  if ((digitalRead(ButtonPin) == HIGH)) {
    CurrentButtonPinStatus = true;
    //Serial.println("ButtonPin is high at check4start"); 
  }else{
    CurrentButtonPinStatus = false;
    //Serial.println("ButtonPin is low at check4start"); 
  }
  if ((true == CurrentButtonPinStatus) && (false == Debouncing_Flag)) { // if legit Button Press
    Debouncing_Flag = true; // Debouncing flag set, ignore subsequent button presses for duration of debounce timer
    Serial.println("Start Button Pressed");
    LastTimeDebounce = millis();
  } else { // fake button press
    // Do nothing 
  }  
}

static void Check4Delete(void){
  if ((digitalRead(ButtonPin) == HIGH)) {
    CurrentButtonPinStatus = true;
    //Serial.println("ButtonPin is high"); 
  }else{
    CurrentButtonPinStatus = false;
    //Serial.println("ButtonPin is low"); 
  }
  //if (CurrentButtonPinStatus != LastButtonPinStatus) { 
    if ((true == CurrentButtonPinStatus) && (false == Debouncing_Flag)) { // if legit Button Press
      Delete = true;
      Debouncing_Flag = true; // Debouncing flag set, ignore subsequent button presses for duration of debounce timer
      Serial.println("Delete Button Pressed");
      LastTimeDebounce = millis();
    } else { // fake button press
      // Do nothing 
    }
  //}
  //LastButtonPinStatus = CurrentButtonPinStatus;
}

static void CheckDebounceTimerExpired() {
  if (millis() - LastTimeDebounce > TIME_DEBOUNCE) {
    Debouncing_Flag = false;
  }
}

static void CheckDeletefromTablet(void)
{
  FirebaseObject object = Firebase.get("6e14c151-0ca3-43b2-b2ab-1ec1bbcd0db0/RunningQueue/18fe34d460aa");
  String& json = (String&)object;
  if (json.equals("null")) {
    Serial.println("Deleted");
    Delete = true;
  } else {
    //Serial.println("exists");
  }
}

static void CheckQueuePosition(void)
{
  // Queue Position is 1
  FirebaseObject object = Firebase.get("6e14c151-0ca3-43b2-b2ab-1ec1bbcd0db0/RunningQueue/18fe34d460aa/QueuePosition");
  CurrentQueuePos = (int)object;
  //Serial.print("QueuePosition is: ");
  //Serial.println(QueuePos);
  if (CurrentQueuePos != LastQueuePos){
    if (1 == CurrentQueuePos){
      LEDServiceFlag = false;
      //Serial.println("Color is Pink");
      fadepink2red();
    }
    else if (2 == CurrentQueuePos){
      LEDServiceFlag = false;
      //Serial.println("Color is Pink");
      fadeblue2pink();
    }
    else{
      LEDServiceFlag = false;
      //Serial.println("Color is Blue");
      fadeblue2blue();
    }
    LastQueuePos = CurrentQueuePos;
  }
  if (1 == CurrentQueuePos)
  {
    LEDServiceFlag = false;
    FlashRedService();
    //Serial.println("Color is flashred");
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
  Firebase.set("6e14c151-0ca3-43b2-b2ab-1ec1bbcd0db0/RunningQueue/18fe34d460aa", "0");
  //Firebase.set("6e14c151-0ca3-43b2-b2ab-1ec1bbcd0db0/RunningQueue/18fe34d460aa/QueuePosition", 5);
}

static void Delete2Firebase(void){
  Firebase.remove("6e14c151-0ca3-43b2-b2ab-1ec1bbcd0db0/RunningQueue/18fe34d460aa");
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
  FlashRedCounter = 0;
  LEDServiceFlag = true;
//  fadered2blue();
//  delay(1000);
//  setRingColor(off);
}

static void LEDService(void) {
  if (LEDServiceFlag == true){
    if ((millis() - LastLEDSampleTime) > TIME_LEDUPDATE)
    {        
    for(i_LED = 0; i_LED< strip.numPixels(); i_LED++) {
      strip.setPixelColor(i_LED, Wheel(((i_LED * 256 / strip.numPixels()) + j_Cycle) & 255));
    }
    strip.show();
    j_Cycle += 75;
    //Serial.print("LEDService j_cycle is: ");
    //Serial.println(j_Cycle);
    LastLEDSampleTime = millis();
    }
  }
}

void setRingColor(uint32_t c) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
  }
  strip.show();
}

void FlashRedService(void) {
  if ((millis() - LastFlashRed) > TIME_FLASHRED)
  {
    if (FlashRedCounter % 2 == 0)
    {
      setRingColor(strip.Color(255,0,0));
    }
    else 
    {
      setRingColor(strip.Color(50,0,0));
    }
    FlashRedCounter++;
    LastFlashRed = millis();
  }
}

void fadepink2red(void) {
  uint16_t i, j;
   for (i=0; i<127; i++) {
      for(j=0; j<strip.numPixels(); j++) {
         strip.setPixelColor(j, strip.Color(127+i, 0, 127-i)); // R upto 100 is Red
      }  
   strip.show();
   delay(10);
   }  
}

void fadeblue2pink(void) {
  uint16_t i, j;
   for (i=0; i<127; i++) {
      for(j=0; j<strip.numPixels(); j++) {
         strip.setPixelColor(j, strip.Color(0+i, 0, 255-i)); // R upto 100 is Red
      }  
   strip.show();
   delay(10);
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

void fadeblue2blue(void) {
  uint16_t i, j;
   for (i=0; i<245; i++) {
      for(j=0; j<strip.numPixels(); j++) {
         strip.setPixelColor(j, strip.Color(0, 0, 10+i)); // B upto 100 is Blue
      }  
   strip.show();
   delay(10);
   }  
}

void Set2Blue(void) {
   setRingColor(strip.Color(0,0,255));
}

void Set2Pink(void) {
   setRingColor(strip.Color(127,0,127));
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}
