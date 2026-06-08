#ifndef PHYSICS_LIBS_PHYSICS_H
#define PHYSICS_LIBS_PHYSICS_H

#include <utility>

struct PhysicsConstants {
  static constexpr double gravity = 9.81; // m/s^2
  static constexpr double rho = 1.225;    // kg/m^3 (air density at sea level)
};

static double DoubleIntegrate(double position, double velocity,
                              double acceleration, double dt) {
  return position + velocity * dt + acceleration * dt * dt / 2;
}

static double SingleIntegrate(double velocity, double acceleration, double dt) {
  return velocity + acceleration * dt;
}

static double CalculateDragForce(double velocity, double drag_coefficient,
                                 double area) {
  return 0.5 * PhysicsConstants::rho * velocity * velocity * drag_coefficient *
         area;
}

using Vector2D = std::pair<double, double>;

#endif // PHYSICS_LIBS_PHYSICS_H
