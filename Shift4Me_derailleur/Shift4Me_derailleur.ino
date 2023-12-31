// DO NOT CHANGE THE FOLLOWING CODE (unless you know what you're doing)

int Frequency;
int ToleranceHigherGear;
int ToleranceLowerGear;
int ChangeDelayHeavy;
int ChangeDelayLight;
int HiLoAmount;
int MaxTurns;
String UseBuzzer;
String UseLed;

#include <Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

Servo myservo;
LiquidCrystal_I2C lcd(0x27,16,2);

int RPM;
int FrequencyCount;
int ButtonDebounce = 250;
int ButtonDebounceShort = 50;
//int ReleaseTime;
unsigned long previousMillis = 0;
int pos;    // variable to store the servo position - taken from array of gears
int GearAmount;
int CorrectAmount;
int prev_pos;
int ActiveGear;
int buttonRed = 9;  // to force to a smaller sprocket
int buttonGreen = 8; // to force to a larger sprocket
int Sensor = 2;     // reads the hall sensor on/off value
int buttonBypass = 7; // reads the Bypass button
int buzzer = 12; //buzzer to arduino pin 12
String action = "none";
unsigned long currentMillis;
unsigned long refTime;
boolean SensorOverride = false;
int correctFrequency = 0;
int ledPin = 4; // led-connection (on/off and battery status)
int  sensorValue; // analog voltage sensing at pin Vin
float  Voltage; // convert sensorvalue voltage to 5V max
int Cycles = 0;
unsigned long pressTime;
String mode;
String Act = "no";

void setup() {
myservo.attach(10);  // attaches the servo 
pinMode(buttonRed, INPUT_PULLUP); // connects the manual override button (shift to smaller sprocket)
pinMode(buttonGreen, INPUT_PULLUP); // connects the manual override button (shift to larger sprocket)
pinMode(buttonBypass, INPUT_PULLUP); // connects the bypass button
pinMode(Sensor, INPUT_PULLUP);    // connection of the hall sensor
pinMode(ledPin, OUTPUT); // led-connection (on/off and battery status)
pinMode(buzzer, OUTPUT); // Set buzzer output
lcd.init();
lcd.backlight();

Serial.begin(9600);
// load derailleur data
  CorrectAmount = LoadSprocketWidth("CorrectAmount",0);
  GearAmount = LoadSprocketWidth("GearAmount",0);
  pos = LoadSprocketWidth("GetGear",99);
  ActiveGear = LoadSprocketWidth("StartingGear",0);
prev_pos = pos;
// load preferences
  Frequency = LoadRidePrefs("Frequency");
  ToleranceHigherGear = LoadRidePrefs("ToleranceHigherGear");
  ToleranceLowerGear = LoadRidePrefs("ToleranceLowerGear");
  ChangeDelayHeavy = LoadRidePrefs("ChangeDelayHeavy");
  ChangeDelayLight = LoadRidePrefs("ChangeDelayLight");
  HiLoAmount = LoadRidePrefs("HiLoAmount");
  MaxTurns = LoadRidePrefs("MaxTurns");
  UseBuzzer = LoadSignalPrefs("UseBuzzer");
  UseLed = LoadSignalPrefs("UseLed");
// change 24 feb, was before: myservo.writeMicroseconds(pos);
PerformReset();
Serial.print("Servo Pos: ");
lcd.print("Servo Pos: ");
lcd.print(pos);
Serial.print(pos);
Serial.print(", gear number ");
Serial.println(ActiveGear+1);

unsigned long currentMillis = millis();
Serial.println(currentMillis);
previousMillis = currentMillis;
refTime = currentMillis;
digitalWrite(ledPin, HIGH); 

}

void loop() 
{

// correct if button actions (MANUAL OVERRIDE)
   if(digitalRead(buttonRed)==LOW)
    {
    action = "less";
    SensorOverride = true;
    delay(ButtonDebounce);
    }
   if(digitalRead(buttonGreen)==LOW)
    {
    action = "more";  
    SensorOverride = true;
    delay(ButtonDebounce);
    }
    if(digitalRead(buttonBypass)==LOW) {
      pressTime = millis();
     // delay(ButtonDebounce);
      BypassButton(pressTime);
Serial.print("mode: ");
Serial.println(mode);
    if(mode == "bypass") {
    action = "none";  
    SensorOverride = true;
    }
    else if(mode == "reset") {
      SensorOverride = true;
      action = "reset";  
    }
    
    }
    else {
      mode = "normal";
    }
// if max Cycles is exceeded
   if(Cycles > MaxTurns) {
  //  action = "less";
    SensorOverride = true;
    Serial.println("MAX TURNS EXCEEDED");
    lcd.print("MAX TURNS EXCEEDED!");
    //  Serial.println(RPM);
   }

// count rotation
currentMillis = millis();

if(digitalRead(Sensor)==LOW || SensorOverride == true)
{
    if(digitalRead(Sensor)==LOW) {
Serial.println("hall triggered");
      Cycles = Cycles + 1;
     //currentMillis = millis();
     FrequencyCount = currentMillis - previousMillis;
      delay(ButtonDebounce);
      previousMillis = currentMillis;    
      // 1000 milliseconds, 60 seconds
      RPM = 60000/FrequencyCount;
    //    Serial.print("actual RPM: ");
    //  Serial.println(RPM);

// if RPM exceeds limits

  // turning too fast, so move to smaller sprocket
if((RPM - ToleranceHigherGear)>(Frequency)) {
  action = "less";
  // Serial.println("HALL TRIGGERED less");
  }
  // turning too slow, so move to larger sprocket
  else if((RPM + ToleranceLowerGear)<(Frequency)) {
      action = "more";
    //  Serial.println("HALL TRIGGERED more");   
 }
 else {
  action = "none";
 }
//} // THIS WAS HERE BEFORE v1.1.7
// else if((refTime + ChangeDelay)<currentMillis){
//  action = "none";
// }

// if RPM is too low (stalling): take no action
      if(RPM < 15) {
    //  if(FrequencyCount>3000) {
    //  if(FrequencyCount>6000) {
         action = "none";
         Serial.println("STALLING");
         lcd.print("Stalling");
         
         // RESET
         // prev_pos = pos;
          RPM = 0;
          refTime = currentMillis;
          Cycles = 0;

      }   
// if RPM is too high (pedal stalls at hall sensor): take no action
       if(RPM > 200) {
         action = "none";
      }   

      
    }
//Serial.println(Cycles);
   
// process action if ChangeDelay time has passed since last change
// if buttonBypass is not pushed
if(  ( (refTime + ChangeDelayHeavy)<currentMillis || SensorOverride == true) && digitalRead(buttonBypass)==HIGH ) {


if(action == "less") {
  Act = "yes";
  // move to smaller sprocket
  ActiveGear = ActiveGear-1;
  pos = LoadSprocketWidth("GetGear",ActiveGear);
  

  // correct if below zero gear
  if (ActiveGear < 0) { ActiveGear = 0; action = "none"; pos = LoadSprocketWidth("GetGear",ActiveGear); }
  else{
    pos = pos - (CorrectAmount/4);
    
   if(UseLed == "yes") {
    digitalWrite(ledPin, HIGH);
   }
   if(UseBuzzer == "yes"){
BuzzerHeavyGear ();
        }
     }
}
}
if(  ( (refTime + ChangeDelayLight)<currentMillis || SensorOverride == true) && digitalRead(buttonBypass)==HIGH ) {

if(action == "more") {
  Act = "yes";
  // move to larger sprocket
   ActiveGear = ActiveGear+1;
   pos = LoadSprocketWidth("GetGear",ActiveGear);
  pos = pos + CorrectAmount;
     // correct if above derailleur maximum
  if (ActiveGear > (GearAmount-1)) { ActiveGear = GearAmount-1; action = "none";   pos = LoadSprocketWidth("GetGear",ActiveGear);
 // pos = pos + CorrectAmount;
  }
  else {
    if(UseLed == "yes") {
    digitalWrite(ledPin, HIGH);
   }
     if(UseBuzzer == "yes"){
    // send audio signal for light gear
BuzzerLightGear ();
     }
  }
  

}
if(mode == "reset") {
PerformReset();
   Act = "yes";
}
//  new  
if(mode == "bypass") {
delay(5000);
   Act = "no";
}
} //end of ChangeDelay cycles

if(Act == "yes"){ 
    Act = "no";
     myservo.writeMicroseconds(pos);  

     // moved to larger sprocket - delay a bit longer
      if(action == "more")  {
         pos = pos - CorrectAmount;
         delay(800);
         }

      // moved to smaller sprocket lesser CorrectAmount
      if(action == "less")  {
        pos = pos + (CorrectAmount/4);
         delay(400);
         }
      myservo.writeMicroseconds(pos);

         // display action every ChangeDelay and reset all
  // check battery voltage
  // read the input on analog pin 0 for the batteries (2.4V and 6V in series):
sensorValue = analogRead(A0);
  // Convert the analog reading (which goes fr om 0 - 1023) to a voltage (0 - 9V):
Voltage = sensorValue * (8.4 / 1023.0);
// flash LED if Voltage is too low
// at aprox 7V total, the 6V battery will be about 5.5V, it is too dropped to pull the servo
  if(Voltage<7){
  LowBatteryWarning();
  // correct time because of delaying of flashing LED
 // currentMillis = currentMillis + 240; 
  }
  else {
    digitalWrite(ledPin, HIGH); 
  }
 Serial.println();
  Serial.print("battery voltage: ");
      Serial.println(Voltage);
      Serial.println();
      Serial.print("measured RPM: ");
      lcd.print("RPM: ");
      lcd.print(RPM);
      
      Serial.println(RPM);
      Serial.print("target RPM: ");
      Serial.println(Frequency+correctFrequency);
      Serial.print("action: ");
      Serial.println(action);
      if(mode != "normal"){
        Serial.print("button mode: ");
      Serial.println(mode);
      }
      Serial.print("now in gear number: ");
      //lcd.print("Gear: ");
      Serial.print(ActiveGear+1);
      //lcd.print(ActiveGear+1);
      Serial.print(", pos is ");
      Serial.println(pos);

    prev_pos = pos;
    action = "none";
    RPM = 0;
    refTime = currentMillis;
    Cycles = 0;
    if(SensorOverride == true){
      refTime = refTime + 5000;
      Cycles = Cycles-5;
    SensorOverride = false;
    Serial.print(" - battery voltage: ");
      Serial.println(Voltage);
    }
 //   if(digitalRead(buttonBypass)==LOW){
 //     Serial.println("servo action BYPASSED");
 //   }
    
 } // end of processing
} // end of Sensor Loop 
} // end of loop
void BuzzerHeavyGear (){
  tone(buzzer, 2000); 
  delay(100);     
  noTone(buzzer);    
  delay(80);      
   tone(buzzer, 1500);
  delay(100);       
  noTone(buzzer);   
  delay(80);      
   tone(buzzer, 1000); 
  delay(100);       
 noTone(buzzer);    
  delay(300);        // delay to allow pedal force disengage
}
void BuzzerLightGear (){
  tone(buzzer, 2000); 
  delay(150);     
  noTone(buzzer);   
  delay(50);      
   tone(buzzer, 2220); 
   delay(50);       
  noTone(buzzer);   
  delay(20);     
   tone(buzzer, 2300); 
  delay(50);   
  noTone(buzzer);     
  delay(50);       
   tone(buzzer, 2220); 
   delay(50);     
   noTone(buzzer); 
  delay(400);        // delay to allow pedal force disengage
}
void LowBatteryWarning(){
   digitalWrite(ledPin, HIGH); 
  delay(100);      
  digitalWrite(ledPin, LOW); 
 // tone(buzzer, 2000); 
  delay(100); 
  digitalWrite(ledPin, HIGH); 
 // noTone(buzzer); 
  delay(100);      
  digitalWrite(ledPin, LOW); 
 // tone(buzzer, 2000); 
  delay(100); 
  digitalWrite(ledPin, HIGH); 
 // noTone(buzzer); 
  delay(100); 
  digitalWrite(ledPin, LOW); 
 // tone(buzzer, 2000); 
  delay(100); 
  digitalWrite(ledPin, HIGH); 
 // noTone(buzzer); 
  // correct time because of delaying of flashing LED
 // currentMillis = currentMillis + 240;
}
void PerformReset(){
  ActiveGear = LoadSprocketWidth("StartingGear",0);
   pos = LoadSprocketWidth("GetGear",99);
   pos = pos + CorrectAmount;
   myservo.writeMicroseconds(pos);
         delay(200);
   pos = pos - CorrectAmount;
   myservo.writeMicroseconds(pos);  
}

String BypassButton(unsigned long var){
//int pressTime2 = pressTime;
int StopSound = 0;
unsigned long ReleaseTime;
delay(ButtonDebounceShort);
 while (digitalRead(buttonBypass) == LOW)
 {
// buzz shortly for bypass
if(StopSound == 0) 
  { 
     
    if (millis()>(var+500) ) {
      noTone(buzzer); 
   //   StopSound = 1;
    }
    else {
      tone(buzzer, 500);
    }
  } 
// if button is released buzz for reset
if (millis()>(var+2000) && StopSound == 0){
  noTone(buzzer); 
  delay(100); 
  tone(buzzer, 2000); 
  delay(50); 
  noTone(buzzer); 
  delay(50); 
  tone(buzzer, 1000); 
  delay(100); 
  noTone(buzzer); 
  StopSound = 1;
  }
 }
  ReleaseTime = millis();
  noTone(buzzer); 

delay(ButtonDebounceShort);

if (( ReleaseTime-2000 )<var){
  mode = "bypass";
  return mode;
}
else
{
  mode = "reset";
  return mode;
}
}
