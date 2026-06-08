#ifndef PID_H
#define PID_H

#include "physics.h"

struct PID {
public:
  float kP = 1.0;
  float kD = 0.2;
  float kI = 0.1;
  float setpoint = 0.0;
  float MaxDeviation = 0.12; // Radians

  float ComputePIDOutput(float current_value, float dt) {
    float error = setpoint - current_value;
    integral += Clamp(integral + error * dt, -MaxDeviation, MaxDeviation);
    float derivative = (error - previous_error) / dt;
    previous_error = error;
    return Clamp(kP * error + kI * integral + kD * derivative, -MaxDeviation,
                 MaxDeviation);
  }

private:
  float integral = 0.0;
  float previous_error = 0.0;
};

#endif // PID_H
