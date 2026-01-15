#include "HX711.h"
#include <EEPROM.h>

#define DOUT 3
#define SCK  2

HX711 scale;

// ===== CONFIG =====
constexpr int endCheckFrames = 10;   // frames below threshold to stop capture
constexpr int bufferSize     = 200;  // max samples

// ===== DATA STRUCT =====
struct ReadingSpan {
  float readingArray[bufferSize];
  int currentIndex;
};

ReadingSpan readingSpan = { {}, 0 };

// ===== STATE FLAGS =====
float calibrationFactor = 1.0;

bool captureStarted   = false;
bool fillToBuffer     = false;
bool filledToBuffer   = false;
bool writtenToEEPROM  = false;

int motorZeroFrames = 0;

// ===== EEPROM LAYOUT =====
// Address 0–1   : uint16_t sample count
// Address 2–... : float samples (4 bytes each)

constexpr int EEPROM_COUNT_ADDR = 0;
constexpr int EEPROM_DATA_ADDR  = 2;

// ===== HELPERS =====
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

  Serial.println("Data written to EEPROM.");
}

void ReadSpanFromEEPROM() {
  uint16_t count;
  EEPROM.get(EEPROM_COUNT_ADDR, count);

  Serial.println("=== EEPROM DATA DUMP ===");
  Serial.print("Stored samples: ");
  Serial.println(count);

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
  Serial.println("=== END DUMP ===");
}

// ===== SETUP =====
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
  Serial.println("Type 'DUMP' to retrieve EEPROM data.");
}

// ===== LOOP =====
void loop() {

  // ----- SERIAL COMMAND -----
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd == "DUMP") {
      ReadSpanFromEEPROM();
    }
  }

  // ----- READ SENSOR -----
  float reading = scale.get_units(10);

  // ----- START CONDITION -----
  if (!captureStarted && reading > 0.05) {
    captureStarted = true;
    fillToBuffer = true;
    Serial.println("Capture started");
  }

  // ----- DATA CAPTURE -----
  if (captureStarted && fillToBuffer && !filledToBuffer) {
    AppendToSpan(reading, readingSpan);

    if (reading < 0.01) {
      motorZeroFrames++;
      if (motorZeroFrames > endCheckFrames) {
        fillToBuffer = false;
        filledToBuffer = true;
        Serial.println("Capture ended");
      }
    } else {
      motorZeroFrames = 0;
    }
  }

  // ----- EEPROM WRITE (ONCE) -----
  if (filledToBuffer && !writtenToEEPROM) {
    WriteSpanToEEPROM(readingSpan);
    writtenToEEPROM = true;
  }
}
