#include <cmath>
#include <iostream>
#include <vector>

#include "motor.h"
#include "params.h"
#include "physics.h"
#include "pid.h"

constexpr double kSimulationTime = 10.0; // s
constexpr double kTimeStep = 0.033;      // Identical to arduino loop time, s

struct State {
  static double time;           // s
  static double altitude;       // m
  static Vector2D velocity;     // m/s
  static Vector2D acceleration; // m/s^2
  static Vector2D position;     // m
  static double angle;          // radians
  static double angle_velocity; // radians/s
  static Rocket rocket;
};

double State::time = 0.0;
double State::altitude = 0.0;
Vector2D State::velocity = {0.0, 0.0};
Vector2D State::acceleration = {0.0, 0.0};
Vector2D State::position = {0.0, 0.0};
double State::angle = 0.0;
double State::angle_velocity = 0.0;
Rocket State::rocket;

int main() {
  while (State::time < kSimulationTime) {
    State::time += kTimeStep;
    State::rocket.motor.SetThrust(State::time);
    State::rocket.SetMass(State::time);

    float PID_output =
        State::rocket.pid.ComputePIDOutput(State::angle, kTimeStep);

    double thrust_force_x = State::rocket.motor.current_force * sin(PID_output);
    double thrust_force_y = State::rocket.motor.current_force * cos(PID_output);
    double drag_force_x = CalculateDragForce(
        -PhysicsConstants::wind_speed + State::velocity.first,
        State::rocket.drag_coeff_body,
        State::rocket.radius * State::rocket.length * cos(State::angle));
    double drag_force_y = CalculateDragForce(
        State::velocity.second, State::rocket.drag_coeff_nose,
        M_PI * State::rocket.radius * State::rocket.radius * sin(State::angle));
    double weight_force = State::rocket.live_mass * PhysicsConstants::gravity *
                          -1; // Negative because it acts downward

    State::acceleration.first =
        (thrust_force_x + drag_force_x) / State::rocket.live_mass;
    State::acceleration.second =
        (thrust_force_y + drag_force_y + weight_force) /
        State::rocket.live_mass;
    State::position.first =
        DoubleIntegrate(State::position.first, State::velocity.first,
                        State::acceleration.first, kTimeStep);
    State::position.second =
        DoubleIntegrate(State::position.second, State::velocity.second,
                        State::acceleration.second, kTimeStep);
    State::velocity.first = SingleIntegrate(
        State::velocity.first, State::acceleration.first, kTimeStep);
    State::velocity.second = SingleIntegrate(
        State::velocity.second, State::acceleration.second, kTimeStep);

    double torque = 0;
    torque +=
        thrust_force_x * (State::rocket.length - State::rocket.cg_position);
    torque +=
        drag_force_x * (State::rocket.cp_position - State::rocket.cg_position);

    double angular_acceleration = torque / State::rocket.I_pitch;
    State::angle = DoubleIntegrate(State::angle, State::angle_velocity,
                                   angular_acceleration, kTimeStep);
    State::angle_velocity =
        SingleIntegrate(State::angle_velocity, angular_acceleration, kTimeStep);
    State::altitude = State::position.second;
  }

  return 0; // Exit status 0.
}
