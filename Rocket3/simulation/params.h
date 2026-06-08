#ifndef PARAMS_H
#define PARAMS_H

struct Rocket {
  double radius = 0.09;                // m
  double length = 1.0;                 // m
  double mass_without_motor = 0.5;     // kg
  double mass_with_empty_motor = 0.58; // kg
  double mass_with_full_motor = 0.68;  // kg
  double live_mass = 0.68;             // kg (current mass)
  double cp_position = 0.6;            // m from top
  double cg_position = 0.4;            // m from top
  double I_pitch = 1 / 12 * live_mass *
                   (3 * radius * radius +
                    length * length); // kg*m^2, moment of inertia for pitch
  double drag_coeff_nose = 0.5;       // dimensionless
  double drag_coeff_body = 1.25;      // dimensionless
};

#endif // PARAMS_H
