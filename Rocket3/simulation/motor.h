#ifndef MOTOR_H
#define MOTOR_H

#include <cassert>
#include <vector>

struct Motor {
  std::vector<double> time_steps = {0.25, 0.5, 0.75, 1.0, 1.25, 1.5,
                                    1.75, 2.0, 2.25, 2.5, 2.75, 3.0};
  std::vector<double> discrete_thrust_curve = {5,  5,  15, 24.5, 35, 49,
                                               53, 42, 17, 6,    4,  1};
  double burn_time = 3.0;     // s
  double switch_every = 0.25; // s
  double current_force = 0.0; // N

  void SetThrust(double time) {
    if (time >= burn_time) {
      current_force = 0.0;
      return;
    }
    int index = static_cast<int>(time / switch_every);
    assert(index < discrete_thrust_curve.size()); // Prevent OOB
    current_force = discrete_thrust_curve[index];
  }
};

#endif // MOTOR_H
