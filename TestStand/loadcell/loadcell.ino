#include "HX711.h"

int pinDOut = 2;
int pinSCK = 3;
int knownWeight = 0.5;
HX711 scale;

void calibration(knownWeight) {
  if (scale.is_ready()){
      Serial.println("Begin tare");
  }
  else {
    calibration(knownWeight);
  }
  delay(5000);
  Serial.println("place weight");
  for (int i = 5; i>-1; i-- ){
    delay(1000);
    Serial.println("reading weight in " + i);
  }
  int calibrationFactor = scale.read_average(10) / knownWeight;
  scale.set_scale(calibrationFactor);
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  scale.begin(pinDOut, pinSCK);
}
void loop() {
  // put your main code here, to run repeatedly:
  calibration(knownWeight);
  scale.tare;
  Serial.println(scale.read());
}
