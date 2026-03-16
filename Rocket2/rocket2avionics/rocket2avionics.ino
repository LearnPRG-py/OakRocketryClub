#include "Arduino_BMI270_BMM150.h"
#include <Arduino_LPS22HB.h>
#include <ArduinoBLE.h>

// Flash storage includes
#include <mbed.h>
#include <FlashIAPBlockDevice.h>
#include <LittleFileSystem.h>
#include <BlockDevice.h>

using namespace mbed;

// Flash storage setup
#define FLASH_SIZE (1024 * 1024)
#define STORAGE_SIZE (256 * 1024)
#define STORAGE_ADDRESS (FLASH_SIZE - STORAGE_SIZE)

FlashIAPBlockDevice bd(STORAGE_ADDRESS, STORAGE_SIZE);
LittleFileSystem fs("fs");

// Original variables
bool flag = true;
bool burnStarted = false;
bool doDataLog = false;
bool chuteDeployed = false;
unsigned long burnCompleteTime = 0;
bool burnDetected = false;

const int maxDataPoints = 5000; 
uint64_t flightData[maxDataPoints];
int dataIndex = 0;
float burnStartTime = 2147483647;
bool bleConnected = false;

// Flash storage variables
bool flashReady = false;
int flightNumber = 1;

BLEService flightDataService("12345678-1234-1234-1234-123456789abc");
BLECharacteristic dataCharacteristic("87654321-4321-4321-4321-cba987654321", BLERead | BLENotify, 512);

void setup() {
  pinMode(3, OUTPUT);
  digitalWrite(3, LOW);
  Serial.begin(115200);
  delay(2000);
  
  // Initialize flash storage
  initFlashStorage();
  
  // Auto-output stored flight data on startup
  if (flashReady && Serial) {
    // but if serial is connected!
    Serial.println("=== STORED FLIGHT DATA ===");
    getStorageInfo();
    listFlightFiles();
    
    // Output any existing flight data
    downloadFlightData();
  }
  //if not it just overwrites rip :(
  
  Serial.println("Flight computer ready!");
  delay(180000); // if sachit is 100% confident then sure
    // Initialize sensors
  if (!BARO.begin()) {
    Serial.println("Failed to initialize pressure sensor!");
    while (1);
  }
  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (1);
  }
}

void loop() {
  Serial.println("In loop");
  BLEDevice central = BLE.central();
  
  // Read sensors
  float pressure = BARO.readPressure();
  float altitude = 44330 * (1.0 - pow(pressure / 101.325, 0.1903));
  float smoothedAltitude = altitude;
  float accelX, accelY, accelZ, gyroX, gyroY, gyroZ;
  IMU.readAcceleration(accelX, accelY, accelZ);
  IMU.readGyroscope(gyroX, gyroY, gyroZ);
  float accelerationMagnitude = sqrt(accelX*accelX + accelY*accelY + accelZ*accelZ);
  Serial.println(accelerationMagnitude);
  
  // Prevent sensor faults
  if (flag) {
    if (accelerationMagnitude >= 0.5f) {
      flag = false;
    } else {
      accelerationMagnitude = 1;
    }
  }
  
  // Data logging starts at burn start
  if (millis()-burnStartTime <= 100000 && doDataLog) {
    logData(smoothedAltitude, accelerationMagnitude, gyroY, gyroZ, bleConnected);
  }
  
  // Parachute deployment logic
  if (!burnDetected && accelerationMagnitude < 0.5 && burnStarted) {
    burnDetected = true;
    burnCompleteTime = millis();
    Serial.println("Burn Ended!");
    digitalWrite(LED_BUILTIN, HIGH);
  }
  
  if (!burnDetected && accelerationMagnitude > 3) { 
    burnStarted = true;
    doDataLog = true;
    burnStartTime = millis();
    Serial.println("Burn Detected!");
    
    // Create new flight data file
    createFlightDataFile();
  }
  
  if ((burnDetected) && (!chuteDeployed) && (millis() - burnCompleteTime > 3500)) {
    Serial.println("PARACHUTE TRIGGERED!");
    chuteDeployed = true;
    // digitalWrite(3, HIGH);
    delay(1000);
    // digitalWrite(3, LOW);
    digitalWrite(LED_BUILTIN, LOW);
  }

  if (millis() - burnStartTime >= 100000 && doDataLog) {
    // Save flight data to flash and output to serial
    saveFlightData();
    outputFlightData();
    doDataLog = false; // Stop logging
  }
  
  delay(10);
}

// ===== FLASH STORAGE FUNCTIONS =====

void initFlashStorage() {
  Serial.println("Initializing flash storage...");
  
  int err = fs.mount(&bd);
  if (err) {
    Serial.println("Formatting flash storage...");
    err = fs.format(&bd);
    if (err) {
      Serial.println("Flash format failed!");
      return;
    }
    err = fs.mount(&bd);
    if (err) {
      Serial.println("Flash mount failed!");
      return;
    }
  }
  
  flashReady = true;
  Serial.println("Flash storage ready!");
}

void loadFlightConfig() {
  if (!flashReady) return;
  
  FILE* f = fopen("/fs/config.txt", "r");
  if (f == NULL) {
    // Create default config
    saveFlightConfig();
    return;
  }
  
  char buffer[64];
  while (fgets(buffer, sizeof(buffer), f)) {
    if (strstr(buffer, "flight_number=")) {
      sscanf(buffer, "flight_number=%d", &flightNumber);
    }
  }
  fclose(f);
  
  Serial.print("Loaded flight number: ");
  Serial.println(flightNumber);
}

void saveFlightConfig() {
  if (!flashReady) return;
  
  FILE* f = fopen("/fs/config.txt", "w");
  if (f == NULL) {
    Serial.println("Failed to save config");
    return;
  }
  
  fprintf(f, "device_id=nano33ble_flight\n");
  fprintf(f, "version=1.0\n");
  
  fclose(f);
}

void createFlightDataFile() {
  if (!flashReady) return;
  
  // Use fixed filename instead of flight number
  FILE* f = fopen("/fs/flight_data.csv", "w");
  if (f == NULL) {
    Serial.println("Failed to create flight data file");
    return;
  }
  
  // Write CSV header
  fprintf(f, "timestamp,altitude,accel_mag,gyro_pitch,gyro_yaw,chute_deployed,burn_detected\n");
  fclose(f);
  
  Serial.println("Created flight data file: flight_data.csv");
}

void saveFlightData() {
  if (!flashReady || dataIndex == 0) return;
  
  // Use fixed filename
  FILE* f = fopen("/fs/flight_data.csv", "a");
  if (f == NULL) {
    Serial.println("Failed to open flight data file");
    return;
  }
  
  // Save all logged data points
  for (int i = 0; i < dataIndex; i++) {
    uint64_t packedData = flightData[i];
    
    // Unpack the data (reverse of logData packing)
    float altitude = (float)(packedData & 0x3FFF) / 10.0;
    float accelZ = (float)((packedData >> 14) & 0x7FFF) / 2048.0;
    float gyroPitch = (float)((packedData >> 29) & 0x7FFF) / 262.1;
    float gyroYaw = (float)((packedData >> 44) & 0x7FFF) / 262.1;
    bool chuteState = (packedData >> 59) & 0x1;
    bool burnState = (packedData >> 60) & 0x1;
    
    // Write to CSV
    fprintf(f, "%lu,%.2f,%.3f,%.2f,%.2f,%d,%d\n", 
            (unsigned long)(burnStartTime + i * 33), // Approximate timestamp
            altitude, accelZ, gyroPitch, gyroYaw, chuteState, burnState);
  }
  
  fclose(f);
  
  Serial.print("Saved ");
  Serial.print(dataIndex);
  Serial.println(" data points to flash");
}

void outputFlightData() {
  // Original serial output for backward compatibility
  for (int i = 0; i < dataIndex; i++) {
    Serial.print(flightData[i]);
    Serial.print(", ");
  }
  Serial.println("ENDOFDATA");
}

void listFlightFiles() {
  if (!flashReady) return;
  
  DIR* dir = opendir("/fs");
  if (dir == NULL) {
    Serial.println("Failed to open directory");
    return;
  }
  
  Serial.println("Flight data files:");
  struct dirent* entry;
  while ((entry = readdir(dir)) != NULL) {
    if (strstr(entry->d_name, "flight_") && strstr(entry->d_name, ".csv")) {
      Serial.print("  ");
      Serial.println(entry->d_name);
      
      // Get file size
      char filepath[64];
      snprintf(filepath, sizeof(filepath), "/fs/%s", entry->d_name);
      
      struct stat file_stat;
      if (stat(filepath, &file_stat) == 0) {
        Serial.print("    Size: ");
        Serial.print(file_stat.st_size);
        Serial.println(" bytes");
      }
    }
  }
  closedir(dir);
}

void downloadFlightData() {
  if (!flashReady) return;
  
  FILE* f = fopen("/fs/flight_data.csv", "r");
  if (f == NULL) {
    Serial.println("No flight data found");
    return;
  }
  Serial.println("=== FLIGHT DATA ===");
  
  char buffer[128];
  while (fgets(buffer, sizeof(buffer), f)) {
    Serial.print(buffer);
  }
  
  fclose(f);
  Serial.println("=== END FLIGHT DATA ===");
  while(1){}; //wait forever
}

void deleteFlightData(int flightNum) {
  if (!flashReady) return;
  
  char filename[32];
  snprintf(filename, sizeof(filename), "/fs/flight_%03d.csv", flightNum);
  
  if (remove(filename) == 0) {
    Serial.print("Deleted flight ");
    Serial.println(flightNum);
  } else {
    Serial.print("Failed to delete flight ");
    Serial.println(flightNum);
  }
}

void getStorageInfo() {
  if (!flashReady) return;
  
  struct statvfs stats;
  if (statvfs("/fs", &stats) == 0) {
    uint64_t total = (uint64_t)stats.f_blocks * stats.f_frsize;
    uint64_t free = (uint64_t)stats.f_bavail * stats.f_frsize;
    uint64_t used = total - free;
    
    Serial.println("Storage Info:");
    Serial.print("  Total: ");
    Serial.print((uint32_t)total);
    Serial.println(" bytes");
    Serial.print("  Used: ");
    Serial.print((uint32_t)used);
    Serial.println(" bytes");
    Serial.print("  Free: ");
    Serial.print((uint32_t)free);
    Serial.println(" bytes");
    Serial.print("  Usage: ");
    Serial.print((used * 100) / total);
    Serial.println("%");
  }
}

// ===== ORIGINAL FUNCTIONS =====

void logData(float altitude, float accelZ, float gyroPitch, float gyroYaw, bool bc) {
  if (dataIndex < maxDataPoints) {
    uint64_t packedData = 0;
    packedData += (uint64_t)(altitude * 10);             
    packedData += (uint64_t)(accelZ * 2048) << 14;       
    packedData += (uint64_t)(gyroPitch * 262.1) << 29;   
    packedData += (uint64_t)(gyroYaw * 262.1) << 44;     
    packedData += (uint64_t)(chuteDeployed) << 59;
    packedData += (uint64_t)(burnDetected) << 60;
    flightData[dataIndex++] = packedData;
    Serial.println(packedData);
  }
}