#include "HX711.h"
<<<<<<< Updated upstream
#include <EEPROM.h>

#define DOUT 3
#define SCK 2
=======
#include <SPI.h>
#include <SD.h>

#define DOUT 2
#define SCK  3
#define SD_CS 10
>>>>>>> Stashed changes

HX711 scale;
File dataFile;

<<<<<<< Updated upstream
constexpr int endCheckFrames = 10;
constexpr int bufferSize = 200;

struct ReadingSpan {
  float readingArray[bufferSize];
  int currentIndex;
};

ReadingSpan readingSpan = { {}, 0 };

float calibrationFactor = 1.0;
bool fillToBuffer = false;
bool filledToBuffer = false;
bool writtenToEEPROM = false;
int motorZeroFrames = 0;

constexpr int EEPROM_COUNT_ADDR = 0;
constexpr int EEPROM_DATA_ADDR = 2;

void AppendToSpan(float value, ReadingSpan& span) {
  if (span.currentIndex < bufferSize) {
    span.readingArray[span.currentIndex++] = value;
  }
}

void WriteSpanToEEPROM(const ReadingSpan& span) {
  uint16_t count = span.currentIndex;
  EEPROM.put(EEPROM_COUNT_ADDR, count);
  int addr = EEPROM_DATA_ADDR;
  for (uint16_t i = 0; i < count; i++) {
    EEPROM.put(addr, span.readingArray[i]);
    addr += sizeof(float);
  }
}

void ReadSpanFromEEPROM() {
  uint16_t count;
  EEPROM.get(EEPROM_COUNT_ADDR, count);
  int addr = EEPROM_DATA_ADDR;

  Serial.print("[");
  for (uint16_t i = 0; i < count; i++) {
    float value;
    EEPROM.get(addr, value);
    addr += sizeof(float);
    Serial.print(value);
    if (i < count - 1) Serial.print(", ");
  }
  Serial.println("]");
}

void ClearEEPROM() {
  uint16_t zero = 0;
  EEPROM.put(EEPROM_COUNT_ADDR, zero);
}
=======
float calibrationFactor = 205668.14;
bool sdAvailable = false;

float alpha = 0.08;
float emaWeight = 0;
bool emaInitialized = false;
>>>>>>> Stashed changes

void setup() {
  Serial.begin(9600);
  delay(2000);

  scale.begin(DOUT, SCK);
<<<<<<< Updated upstream

  Serial.println("Remove all weight");
  delay(3000);
  scale.tare();

  Serial.println("Place known weight (kg)");
  delay(5000);

  float knownWeight = 1.0;
  long reading = scale.read_average(20);
  calibrationFactor = reading / knownWeight;
=======
>>>>>>> Stashed changes
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
<<<<<<< Updated upstream
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd == "DUMP") ReadSpanFromEEPROM();
    if (cmd == "CLEAR") ClearEEPROM();
  }

  float reading = scale.get_units(10);

  if (reading > 0.05) fillToBuffer = true;

  if (fillToBuffer) {
    AppendToSpan(reading, readingSpan);

    if (reading < 0.01) {
      motorZeroFrames++;
      if (motorZeroFrames > endCheckFrames) {
        fillToBuffer = false;
        filledToBuffer = true;
      }
    } else {
      motorZeroFrames = 0;
    }
  }

  if (filledToBuffer && !writtenToEEPROM) {
    WriteSpanToEEPROM(readingSpan);
    writtenToEEPROM = true;
  }
=======

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
>>>>>>> Stashed changes
}
