#include "Arduino_BMI270_BMM150.h"
#include <Arduino_LPS22HB.h>
// #include <Servo.h>
// Servo servoX;
// Servo servoY;

float Kp = 0.6;
float Ki = 0.06;
float Kd = 0.12;
float gyroXRaw= 0;
float gyroYRaw= 0;
float angleX = 0;
float angleY = 0;
float errorX = 0;
float errorY = 0;
float previousErrorX = 0;
float previousErrorY = 0;
float integralX = 0;
float integralY = 0;
float derivativeX = 0;
float derivativeY = 0;
float outputX = 0;
float outputY = 0;
unsigned long lastTime = 0;
bool flag = true;

bool chuteDeployed = false;
unsigned long burnCompleteTime = 0;
bool burnDetected = false;


const int maxDataPoints = 1000; 
uint64_t flightData[maxDataPoints];
int dataIndex = 0;



int pidSystemCalcs(){
  float gyroX, gyroY, gyroZ;
  unsigned long currentTime = millis();
  float dt = (currentTime - lastTime) / 1000.0f;
  lastTime = currentTime;
  IMU.readGyroscope(gyroX, gyroY, gyroZ);
  gyroXRaw = gyroX/262.1f;
  gyroYRaw = gyroY/262.1f; 
  angleX = 0.8f * angleX + 0.2f * (angleX + gyroXRaw * dt);
  angleY = 0.8f * angleY + 0.2f * (angleY + gyroYRaw * dt);
  errorX = 0 - angleX;
  errorY = 0 - angleY;
  integralX += errorX * dt;
  integralY += errorY * dt;
  derivativeX = (errorX - previousErrorX) / dt;
  derivativeY = (errorY - previousErrorY) / dt;
  previousErrorX = errorX;
  previousErrorY = errorY;
  outputX = (Kp * errorX) + (Ki * integralX) + (Kd * derivativeX);
  outputY = (Kp * errorY) + (Ki * integralY) + (Kd * derivativeY);
  outputX = constrain(outputX, -0.12, 0.12);
  outputY = constrain(outputY, -0.12, 0.12);
  // servoX.write(servoAngleX);
  // servoY.write(servoAngleY); 
}




void setup() {
  // put your setup code here, to run once:
  pinMode(2, OUTPUT);
  // servoX.attach(7); 
  // servoY.attach(8);
      Serial.begin(115200);
      while (!Serial);

      if (!BARO.begin()) {
          Serial.println("Failed to initialize pressure sensor!");
          while (1);
      }
      if (!IMU.begin()) {
          Serial.println("Failed to initialize IMU!");
          while (1);
      }
      lastTime = millis();
}

void logData(float altitude, float accelZ, float gyroPitch, float gyroYaw) {
    if (dataIndex < maxDataPoints) {
        uint64_t packedData = 0;
        packedData += (uint64_t)(altitude * 10);             
        packedData += (uint64_t)(accelZ * 2048) << 14;       
        packedData += (uint64_t)(gyroPitch * 262.1) << 29;   
        packedData += (uint64_t)(gyroYaw * 262.1) << 44;     
        packedData += (uint64_t)(chuteDeployed) << 59;
        packedData += (uint64_t)(burnDetected) << 60;
        flightData[dataIndex++] = packedData;
    }
}


void loop() {
  pidSystemCalcs();
  float pressure = BARO.readPressure();
  float altitude = 44330 * (1.0 - pow(pressure / 101.325, 0.1903));
  float smoothedAltitude = altitude;
  float accelX, accelY, accelZ, gyroX, gyroY, gyroZ;
  IMU.readAcceleration(accelX, accelY, accelZ);
  IMU.readGyroscope(gyroX, gyroY, gyroZ);
  float accelerationMagnitude = sqrt(accelX * accelX + accelY * accelY + accelZ * accelZ);
  if (flag){
    if(accelerationMagnitude >= 0.5f){
      flag = false;
    }
    else{
      accelerationMagnitude = 1;
    }
  }
  Serial.println(accelerationMagnitude);
  if (millis()<=10000){
    logData(smoothedAltitude, accelerationMagnitude, gyroY, gyroZ);
  }
  else {
    for (int i = 0; i < dataIndex; i++) {
      Serial.print(flightData[i]);
      Serial.print(", ");
    }
    Serial.println("ENDOFDATA");
    delay(10000);
  }
  if (!burnDetected && accelerationMagnitude < 0.5) {
      burnDetected = true;
      burnCompleteTime = millis();
  }

  if ((burnDetected) && (!chuteDeployed) && (millis() - burnCompleteTime > 4000)) {
      Serial.println("PARACHUTE TRIGGERED!");
      chuteDeployed = true;
      digitalWrite(2, HIGH);
  }

  delay(10);
}
