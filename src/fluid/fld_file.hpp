#pragma once

#include "fluid/particle.hpp"

#include <string>
#include <vector>

namespace fluid {

  struct FluidState {
      double particles_per_meter = 0.0;
      std::vector<Particle> particles;
  };

  [[nodiscard]] FluidState read_fld(std::string const & path);

  void write_fld(std::string const & path, double particles_per_meter,
                 std::vector<Particle> const & particles);

}
