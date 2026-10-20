#include "fluid/grid.hpp"

#include "fluid/constants.hpp"

#include <cmath>
#include <utility>

namespace fluid {

  Grid::Grid(double smoothing_length) {
    for (int axis = 0; axis < dimensions; ++axis) {
      double const extent = constants::upper_limit[axis] - constants::lower_limit[axis];
      if (smoothing_length > 0) { cell_counts_[axis] = std::floor(extent / smoothing_length); }
      if (cell_counts_[axis] > 0) { cell_size_[axis] = extent / cell_counts_[axis]; }
    }
    create_blocks();
  }

  int Grid::axis_cell(double coordinate, int axis) const {
    double index = 0;
    if (cell_counts_[axis] > 0) {
      index = std::floor((coordinate - constants::lower_limit[axis]) / cell_size_[axis]);
    }
    index = (index < 0) ? 0 : index;
    index = (index > cell_counts_[axis] - 1) ? cell_counts_[axis] - 1 : index;
    return static_cast<int>(index);
  }

  Cell Grid::cell_of(Vector3 const & position) const {
    return {axis_cell(position[0], 0), axis_cell(position[1], 1), axis_cell(position[2], 2)};
  }

  bool Grid::contains(Cell const & cell) const {
    for (int axis = 0; axis < dimensions; ++axis) {
      if (cell[axis] < 0 || cell[axis] > cell_counts_[axis] - 1) { return false; }
    }
    return true;
  }

  std::size_t Grid::index_of(Cell const & cell) const {
    auto const counts_y = static_cast<std::size_t>(cell_counts_[1]);
    auto const counts_z = static_cast<std::size_t>(cell_counts_[2]);
    return (static_cast<std::size_t>(cell[0]) * counts_y + static_cast<std::size_t>(cell[1])) *
               counts_z +
           static_cast<std::size_t>(cell[2]);
  }

  void Grid::create_blocks() {
    for (int i = 0; i < cell_counts_[0]; ++i) {
      for (int j = 0; j < cell_counts_[1]; ++j) {
        for (int k = 0; k < cell_counts_[2]; ++k) {
          Block block;
          block.cell             = {i, j, k};
          std::size_t const self = index_of(block.cell);
          for (int di = -1; di <= 1; ++di) {
            for (int dj = -1; dj <= 1; ++dj) {
              for (int dk = -1; dk <= 1; ++dk) {
                Cell const neighbor{i + di, j + dj, k + dk};
                if (!contains(neighbor)) { continue; }
                std::size_t const index = index_of(neighbor);
                block.neighbors.push_back(index);
                if (index > self) { block.later_neighbors.push_back(index); }
              }
            }
          }
          blocks_.push_back(std::move(block));
        }
      }
    }
  }

}
