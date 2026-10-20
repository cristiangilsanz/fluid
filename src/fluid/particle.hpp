#pragma once

#include "fluid/constants.hpp"
#include "fluid/vector3.hpp"

namespace fluid {

  struct Particle {
      int id = 0;
      Vector3 position{};
      Vector3 half_velocity{};
      Vector3 velocity{};
      Vector3 acceleration = constants::gravity;
      double density       = 0.0;
  };

}
