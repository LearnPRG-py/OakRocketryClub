#include <iostream>
#include <vector>

#include "motor.h"
#include "params.h"
#include "physics.h"

constexpr double kSimulationTime = 10.0; // s
constexpr double kTimeStep = 0.033;      // Identical to arduino loop time, s

struct State {
  static double time;           // s
  static double altitude;       // m
  static Vector2D velocity;     // m/s
  static Vector2D acceleration; // m/s^2
  static Vector2D position;     // m
  static double angle;          // radians
};

double State::time = 0.0;
double State::altitude = 0.0;
Vector2D State::velocity = {0.0, 0.0};
Vector2D State::acceleration = {0.0, 0.0};
Vector2D State::position = {0.0, 0.0};
double State::angle = 0.0;

int main() {
  while (State::time < kSimulationTime) {
    // Step 1: Update motor thrust by invoking motor.h
  }

  return 0; // Exit status 0.
}
