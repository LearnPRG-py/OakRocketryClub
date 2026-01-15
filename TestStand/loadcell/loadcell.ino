#include "HX711.h"
#include <EEPROM.h>

#define DOUT 3
#define SCK 2

HX711 scale;

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
}
