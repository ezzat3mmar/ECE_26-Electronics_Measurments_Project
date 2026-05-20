#include <Wire.h>
#include <Encoder.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

Encoder myEnc(3, 2); 
const int pwmPin = 9; 
const int feedbackPin = A3; 

int currentPWM = 0;
long oldPosition = -999;


float targetVoltage = 3.0;        
float actualVoltage = 0.0;        
const float sourceVoltage = 11.8; 
const float refVoltage = 1.107;    
const float dividerRatio = 0.083454; 


const float calFactor = .935; 


float errorIntegral = 0.0;

unsigned long lastDisplayUpdate = 0;
unsigned long lastControlUpdate = 0;

void setup() {

  analogReference(INTERNAL); 
  
  pinMode(pwmPin, OUTPUT);

  TCCR1B = (TCCR1B & 0b11111000) | 0x01; 

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    for(;;); 
  }
  display.clearDisplay();
}

void loop() {

  long newPosition = myEnc.read() / 4;
  if (newPosition != oldPosition) {
    targetVoltage += (newPosition - oldPosition) * 0.1; 
    targetVoltage = constrain(targetVoltage, 0.0, sourceVoltage - 1.0);
    oldPosition = newPosition;
  }


  if (millis() - lastControlUpdate > 60) {
    readAndAdjust();
    lastControlUpdate = millis();
  }

  // 3. تحديث الشاشة كل 300 مللي ثانية
  if (millis() - lastDisplayUpdate > 300) {
    updateDisplay();
    lastDisplayUpdate = millis();
  }
}


void readAndAdjust() {

  long sum = 0;
  for(int i = 0; i < 150; i++) {
    sum += analogRead(feedbackPin);
  }
  float avgRaw = sum / 150.0;


  float vDrain = (avgRaw * refVoltage / 1023.0) / dividerRatio;
  

  actualVoltage = (sourceVoltage - vDrain) * calFactor;
  
  if (actualVoltage < 0) actualVoltage = 0;


  float error = targetVoltage - actualVoltage;


  float Kp = 5.0; 
  float Ki = 0.1;  


  if (abs(error) > 0.05) { 

    errorIntegral += error;
    errorIntegral = constrain(errorIntegral, -50.0, 50.0);


    float pidOutput = (Kp * error) + (Ki * errorIntegral);

    int step = round(pidOutput);
    if (step == 0 && error > 0) step = 1;
    if (step == 0 && error < 0) step = -1;

    currentPWM += step;
  } else {
    errorIntegral = 0.0; 
  }


  currentPWM = constrain(currentPWM, 0, 255);
  

  if (targetVoltage < 0.1) currentPWM = 0;
  
  analogWrite(pwmPin, currentPWM);
}


void updateDisplay() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("N-MOS CALIBRATED");
  
  display.setTextSize(2);
  display.setCursor(0, 20);
  display.print("Set:"); 
  display.print(targetVoltage, 1); 
  display.print("V");
  
  display.setTextSize(1);
  display.setCursor(0, 50);
  display.print("Out:"); 
  display.print(actualVoltage, 1); 
  display.print("V PWM:"); 
  display.print(currentPWM);
  
  display.display();
}