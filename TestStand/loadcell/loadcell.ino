#include "HX711.h"

#define DOUT 3
#define SCK  2

HX711 scale;

float calibrationFactor = 1.0;

void setup() {
  Serial.begin(9600);
  scale.begin(DOUT, SCK);

  Serial.println("Remove all weight");
  delay(3000);
  scale.tare();

  Serial.println("Place known weight (kg)");
  delay(5000);

  float knownWeight = 1.0;
  long reading = scale.read_average(20);

  calibrationFactor = reading / knownWeight;
  scale.set_scale(calibrationFactor);

  Serial.println("Calibration complete");
}

void loop() {
  Serial.print("Weight (kg): ");
  Serial.println(scale.get_units(10));
  delay(500);
}
