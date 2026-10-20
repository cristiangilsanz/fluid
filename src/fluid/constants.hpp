#pragma once

#include "fluid/vector3.hpp"

#include <cmath>
#include <numbers>

namespace fluid::constants {

  inline constexpr double smoothing_multiplier = 1.695;
  inline constexpr double fluid_density        = 1000.0;
  inline constexpr double pressure_stiffness   = 3.0;
  inline constexpr double collision_stiffness  = 30000.0;
  inline constexpr double damping              = 128.0;
  inline constexpr double viscosity            = 0.4;
  inline constexpr double particle_size        = 0.0002;
  inline constexpr double time_step            = 0.001;
  inline constexpr double pi                   = std::numbers::pi;

  inline double const time_step_squared = std::pow(time_step, 2);
  inline double const minimum_variation = std::pow(10, -10);
  inline double const minimum_distance  = std::pow(10, -12);

  inline constexpr Vector3 gravity{0.0, -9.8, 0.0};
  inline constexpr Vector3 upper_limit{0.065, 0.1, 0.065};
  inline constexpr Vector3 lower_limit{-0.065, -0.08, -0.065};

}
