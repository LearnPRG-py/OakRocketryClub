#include "HX711.h"
#include <stdio.h>

#define DOUT 3
#define SCK  2

// So that the array can grow on it's own

HX711 scale;

constexpr int endCheckFrames = 10; // Will check 330ms of low reading to detect end
constexpr int bufferSize = 200;
struct ReadingSpan {
  float readingArray[bufferSize];
  int currentIndex;
};

void AppendToSpan(float value, ReadingSpan& readingSpan){
  if (readingSpan.currentIndex < bufferSize - 1) {
    readingSpan.readingArray[readingSpan.currentIndex] = value;
    readingSpan.currentIndex++;
  }
  else {
    // Do not mutate the list and ignore the call.
  }
}

struct ReadingSpan readingSpan = { {}, 0 };
float calibrationFactor = 1.0;
bool fillToBuffer = false;
bool filledToBuffer = false;
int motorZeroFrames = 0;

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
  //TODO: Find this in the test room and then keep these as constexprs

  Serial.println("Calibration complete");
}

void loop() {
  Serial.print("Weight (kg): ");
  float reading = scale.get_units(10);
  if (reading > 0.05) {
    fillToBuffer = true;
  }
  if (fillToBuffer) {
    AppendToSpan(reading, readingSpan);
    if (reading < 0.01){
      ++motorZeroFrames;
      if (motorZeroFrames > endCheckFrames){
        fillToBuffer = false;
        filledToBuffer = true;
      }
    }
    else {
      motorZeroFrames = 0;
    }
  }
  if (filledToBuffer) {
    Serial.println("=== Starting Data Stream ===");
    Serial.print("[");
    for (int index = 0; index < readingSpan.currentIndex; index++) {
      Serial.print(readingSpan.readingArray[index]);
      Serial.print(", ");
    }
    Serial.println("]");
    Serial.println("=== Ended Data Stream ===");
  }
}
