// Copyright 2024 Bedodev. Subject to GNU3 License
#include "DHT.h"
#include <Encoder.h>
#include <SevSegShift.h>
#include <PushButtonTaps.h>
//Temperature sensor defines
#define DHTPIN 2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
float maxTemp = 35;
float minTemp = 25;
//Quad 7 segment display defines
#define SHIFT_PIN_SHCP 6
#define SHIFT_PIN_STCP 5
#define SHIFT_PIN_DS   4
SevSegShift sevseg(SHIFT_PIN_DS, SHIFT_PIN_SHCP, SHIFT_PIN_STCP, 1, true);
//Motor defines
#define MOTOR 3
int motorSpeed = 100;
//Encoder defines
Encoder enc(7, 8);
//Button defines
#define BUTTON 13
PushButtonTaps btn;
//Auxiliars
bool modeAuto = true;
bool celsius = true;
bool displayStandard = true;
bool setLimitLH = false; // LOWER = FALSE // HIGHER = TRUE


void setup() {
  Serial.begin(9600);
  dht.begin();
  pinMode(MOTOR, OUTPUT);
  btn.setButtonPin(BUTTON);

  byte numDigits = 4;
  byte digitPins[] = {12, 11, 10, 9}; // These are the PINS of the ** Arduino **
  byte segmentPins[] = {0, 1, 2, 3, 4, 5, 6, 7}; // these are the PINs of the ** Shift register **
  bool resistorsOnSegments = false; // 'false' means resistors are on digit pins
  byte hardwareConfig = COMMON_CATHODE; // See README.md for options
  bool updateWithDelays = false; // Default 'false' is Recommended
  bool leadingZeros = false; // Use 'true' if you'd like to keep the leading zeros
  bool disableDecPoint = false; // Use 'true' if your decimal point doesn't exist or isn't connected
  
  sevseg.begin(hardwareConfig, numDigits, digitPins, segmentPins, resistorsOnSegments, updateWithDelays, leadingZeros, disableDecPoint);
  sevseg.setBrightness(100);
}

void loop() {
  float h = dht.readHumidity(); //Humidity
  float t = dht.readTemperature(); //Celsius
  float f = dht.readTemperature(true); //Farenheit
  if (isnan(h) || isnan(t) || isnan(f)) {
    failDisplay();
    return;
  }
  
  updateButton();
  if (displayStandard == true) {
    updateSpeed(t);
    updateMotor();
  } else {
    changeLimits();
  }
  refreshDisplay(h, t, f);
  //testDisplay();
}

void updateSpeed(float t)
{
  int newPos = enc.readAndReset();
  if (modeAuto && newPos!=0) {
    modeAuto = !modeAuto;
  }
  bool aux = false;
  if (modeAuto) {
    int bkp = motorSpeed;
    updateSpeedAuto(t);
    if (bkp == 0 && motorSpeed > 0) {
      aux = true;
    }
  } else {
    if (newPos != 0 && motorSpeed+newPos >=35 && motorSpeed+newPos <=255) {
      motorSpeed += newPos;
    }
    if (newPos > 0 && motorSpeed==0) aux = true;
  }
  startStopMotor(aux);
}

void updateSpeedAuto(float t)
{
  float currentTemp = t;
  int diffLimit = maxTemp - minTemp;
  float diffCurrent = currentTemp - minTemp;
  if (diffCurrent <= 0) {
    motorSpeed = 0;
    return;
  }
  int autoSpeed = diffCurrent * 255 / diffLimit;
  if (autoSpeed > 255) {
    autoSpeed = 255;
  }
  motorSpeed = autoSpeed;
}

void updateMotor() {
  analogWrite(MOTOR, motorSpeed);
}

void startStopMotor(bool isModif) {
  if (motorSpeed >= 35 && motorSpeed <=45) {
    motorSpeed = 0;
    analogWrite(MOTOR, 0);
  } else if (isModif == true) {
    analogWrite(MOTOR, 200);
    analogWrite(MOTOR, 150);
    motorSpeed = 120;
  }
}

void refreshDisplay(float h, float t, float f)
{
  if (displayStandard) {
    if (modeAuto) {
      displayAuto(h, t, f);
    } else {
      displayManual();
    }
  } else {
      displayLimits();
  }
}

void displayAuto(float h, float t, float f)
{
  short fn, fd, sd, aux, aux2;
  char c[3];
  if (celsius) {
    fn = 0;
    fd = (short)t / 10;
    sd = (short)t % 10;
    aux = (t - (short)t )*10;
    aux2 = (int)aux % 10;
    fn = fd*10 + sd;
    sprintf(c, "%d.%dC", fn, aux2);
  } else {
    fn = 0;
    fd = (short)f / 10;
    sd = (short)f % 10;
    aux = (f - (short)f )*10;
    aux2 = (int)aux % 10;
    fn = fd*10 + sd;
    sprintf(c, "%d.%dF", fn, aux2);
  }
  sevseg.setChars(c);
  sevseg.refreshDisplay();
}

void displayManual()
{
  sevseg.setNumber(motorSpeed);
  sevseg.refreshDisplay();
}

void testDisplay()
{
  sevseg.setChars("abcd");
  sevseg.refreshDisplay();
}

void failDisplay()
{
  sevseg.setChars("FAIL");
  sevseg.refreshDisplay();
}

void updateButton()
{
  byte tap = btn.checkButtonStatus();
  if(tap != 0) {
    switch(tap) {
      case 1: //single
        if (displayStandard == true) {
          if (modeAuto == false) {
            modeAuto = !modeAuto;
          } else {
          celsius = !celsius;
          }
        } else {
          setLimitLH = !setLimitLH;
        }
        break;
      case 2: //double
        break;
      case 3: //long
        displayStandard = !displayStandard;
        break;
    }
  }
}

void displayLimits()
{
  short fn, fd, sd, aux, aux2;
  char c[3];
  if (setLimitLH) {
    fn = 0;
    fd = (short)minTemp / 10;
    sd = (short)minTemp % 10;
    aux = (minTemp - (short)minTemp )*10;
    aux2 = (int)aux % 10;
    fn = fd*10 + sd;
    sprintf(c, "%d.%dL", fn, aux2);
  } else {
    fn = 0;
    fd = (short)maxTemp / 10;
    sd = (short)maxTemp % 10;
    aux = (maxTemp - (short)maxTemp )*10;
    aux2 = (int)aux % 10;
    fn = fd*10 + sd;
    sprintf(c, "%d.%dH", fn, aux2);
  }
  sevseg.setChars(c);
  sevseg.refreshDisplay();
}

void changeLimits()
{
  int newPos = enc.readAndReset();
  float value = 0;
  if (newPos!=0) {
    if (newPos < 0) {
      value = -0.1;
    } else {
      value = 0.1;
    }
  }
  if (setLimitLH) {
    minTemp += value;
    if (maxTemp - minTemp < 0.2) {
      maxTemp = minTemp + 0.2;
    }
  } else {
    maxTemp += value;
    if (maxTemp - minTemp < 0.2) {
      minTemp = maxTemp - 0.2;
    }
  }
}

void serialPrint(float h, float t, float f)
{
  //Serial.print(F("Humidity: "));
  //Serial.print(h);
  //Serial.print(F("%"));
  //Serial.print(F(" | "));
  //Serial.print(F("Temperature: "));
  //Serial.print(t);
  //Serial.print(F("°C "));
  //Serial.print(f);
  //Serial.print(F("°F | Rpm: "));
  //Serial.println(motorSpeed);

  // // Serial reset || DEPRECATED ||
  // Serial.end();
  // Serial.begin(9600);
}
