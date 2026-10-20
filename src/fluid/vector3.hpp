#pragma once

#include <array>

namespace fluid {

  using Vector3 = std::array<double, 3>;
  using Cell    = std::array<int, 3>;

  inline constexpr int dimensions = 3;

  inline double squared_distance(Vector3 const & a, Vector3 const & b) {
    double sum = 0.0;
    for (int axis = 0; axis < dimensions; ++axis) {
      sum += (a[axis] - b[axis]) * (a[axis] - b[axis]);
    }
    return sum;
  }

}
