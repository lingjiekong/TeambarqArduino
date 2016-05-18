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
/*----------------------------- Module Defines ----------------------------*/
#define AccelSampleTimeLength 3000 // 2000 millisecond
#define AccelCounterThreshold 1000
#define AccelThreshold 2.5
#define ButtonPin 14

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

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well.
// type of state variable should match that of enum in header file
typedef enum {STATEWAIT4SERVICE, STATEONQUEUE} BarqState_t;
BarqState_t CurrentState;
MMA8452Q accel;
unsigned long LastAccelSampleTime;
float Accelz = 0;
int AccelCounter = 0;
static bool Add = false;
static bool Delete = false;
static bool CurrentButtonPinStatus = false; // put pull down resistor on the circuit
static bool LastButtonPinStatus = false; // put pull down resistor on the cirucit
static uint8_t MAC_array[6];
static char MAC_char[18];
static String MAC_string;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************/


void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(ButtonPin, INPUT); 
  WifiInit();
  AccelInit();
  ReadMACID();
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
        CurrentState = STATEONQUEUE;
        Serial.println("CurrentState = STATEONQUEUE");
      }
    break;

    case STATEONQUEUE:
      if (true == Delete){
        CurrentState = STATEWAIT4SERVICE;
        Serial.println("CurrentState = STATEWAIT4SERVICE");
      }
    break;
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
      Add = true;
      Add2Firebase();
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
    Delete2Firebase();
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
}

static void Add2Firebase(void){
  Firebase.set("Bar1/RunningQueue/5ccf7f006c6c/MACid", "5ccf7f006c6c");
}

static void Delete2Firebase(void){
  Firebase.remove("Bar1/RunningQueue/5ccf7f006c6c");
}

static void ReadMACID(void){
  WiFi.macAddress(MAC_array);
  for (int i = 0; i < sizeof(MAC_array); ++i){
    sprintf(MAC_char,"%s%02x:",MAC_char,MAC_array[i]);
  }
  MAC_string = String(MAC_char);
  Serial.println("End of ReadMACID"); 
}



//
//// put your main code here, to run repeatedly:
//// Use the accel.available() function to wait for new data
////  from the accelerometer.
//if (accel.available())
//{
//  // First, use accel.read() to read the new variables:
//  accel.read();
//  
//  // accel.read() will update two sets of variables. 
//  // * int's x, y, and z will store the signed 12-bit values 
//  //   read out of the accelerometer.
//  // * floats cx, cy, and cz will store the calculated 
//  //   acceleration from those 12-bit values. These variables 
//  //   are in units of g's.
//  // Check the two function declarations below for an example
//  // of how to use these variables.
//  printCalculatedAccels();
//  //printAccels(); // Uncomment to print digital readings
//  
//  // The library also supports the portrait/landscape detection
//  //  of the MMA8452Q. Check out this function declaration for
//  //  an example of how to use that.
//  printOrientation();
//  
//  Serial.println(); // Print new line every time.
//}
//// The function demonstrates how to use the accel.x, accel.y and
////  accel.z variables.
//// Before using these variables you must call the accel.read()
////  function!
//void printAccels()
//{
//  Serial.print(accel.x, 3);
//  Serial.print("\t");
//  Serial.print(accel.y, 3);
//  Serial.print("\t");
//  Serial.print(accel.z, 3);
//  Serial.print("\t");
//}
//
//// This function demonstrates how to use the accel.cx, accel.cy,
////  and accel.cz variables.
//// Before using these variables you must call the accel.read()
////  function!
//void printCalculatedAccels()
//{ 
//  Serial.print(accel.cx, 3);
//  Serial.print("\t");
//  Serial.print(accel.cy, 3);
//  Serial.print("\t");
//  Serial.print(accel.cz, 3);
//  Serial.print("\t");
//}
//
//// This function demonstrates how to use the accel.readPL()
//// function, which reads the portrait/landscape status of the
//// sensor.
//void printOrientation()
//{
//  // accel.readPL() will return a byte containing information
//  // about the orientation of the sensor. It will be either
//  // PORTRAIT_U, PORTRAIT_D, LANDSCAPE_R, LANDSCAPE_L, or
//  // LOCKOUT.
//  byte pl = accel.readPL();
//  switch (pl)
//  {
//  case PORTRAIT_U:
//    Serial.print("Portrait Up");
//    break;
//  case PORTRAIT_D:
//    Serial.print("Portrait Down");
//    break;
//  case LANDSCAPE_R:
//    Serial.print("Landscape Right");
//    break;
//  case LANDSCAPE_L:
//    Serial.print("Landscape Left");
//    break;
//  case LOCKOUT:
//    Serial.print("Flat");
//    break;
//  }
//}
