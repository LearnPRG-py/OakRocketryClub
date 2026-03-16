#include "HX711.h"
#include <SPI.h>
#include <SD.h>

#define DOUT 2
#define SCK  3
#define SD_CS 10

HX711 scale;
File dataFile;

float calibrationFactor = 205668.14;
bool sdAvailable = false;

float alpha = 0.08;
float emaWeight = 0;
bool emaInitialized = false;

void setup() {
  Serial.begin(9600);
  delay(2000);

  scale.begin(DOUT, SCK);

  scale.set_scale(calibrationFactor);

  Serial.println("Remove all weight...");
  delay(5000);

  scale.tare();
  Serial.println("Scale ready.");

  Serial.println("Initializing SD card...");
  if (SD.begin(SD_CS)) {
    sdAvailable = true;
    Serial.println("SD card ready.");

    dataFile = SD.open("weights.csv", FILE_WRITE);
    if (dataFile) {
      dataFile.println("Time(ms),RawWeight(kg),FilteredWeight(kg)");
      dataFile.close();
    } else {
      Serial.println("Failed to create file.");
      sdAvailable = false;
    }
  } else {
    Serial.println("SD card initialization failed. Logging disabled.");
  }
}


void loop() {

  unsigned long currentTime = millis();

  if (scale.is_ready()) {

    float rawWeight = scale.get_units();

    if (!emaInitialized) {
      emaWeight = rawWeight;
      emaInitialized = true;
    } else {
      emaWeight = alpha * rawWeight + (1 - alpha) * emaWeight;
    }

    Serial.print("Time(ms): ");
    Serial.print(currentTime);
    Serial.print(" | Raw (kg): ");
    Serial.print(rawWeight, 4);
    Serial.print(" | EMA (kg): ");
    Serial.println(emaWeight, 4);

    if (sdAvailable) {
      dataFile = SD.open("weights.csv", FILE_WRITE);

      if (dataFile) {
        dataFile.print(currentTime);
        dataFile.print(",");
        dataFile.print(rawWeight, 4);
        dataFile.print(",");
        dataFile.println(emaWeight, 4);
        dataFile.close();
      } else {
        Serial.println("SD write error. Disabling logging.");
        sdAvailable = false;
      }
    }

  } else {
    Serial.println("HX711 not ready.");
  }

  delay(100);

}
