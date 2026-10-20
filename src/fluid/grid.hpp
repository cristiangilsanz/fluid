#pragma once

#include "fluid/particle.hpp"
#include "fluid/vector3.hpp"

#include <cstddef>
#include <vector>

namespace fluid {

  struct Block {
      Cell cell{};
      std::vector<Particle> particles;
      std::vector<Particle> incoming;
      std::vector<std::size_t> neighbors;
      std::vector<std::size_t> later_neighbors;
  };

  class Grid {
    public:
      explicit Grid(double smoothing_length);

      [[nodiscard]] Vector3 const & cell_counts() const { return cell_counts_; }

      [[nodiscard]] Vector3 const & cell_size() const { return cell_size_; }

      [[nodiscard]] int axis_cell(double coordinate, int axis) const;

      [[nodiscard]] Cell cell_of(Vector3 const & position) const;

      [[nodiscard]] bool contains(Cell const & cell) const;

      [[nodiscard]] std::size_t index_of(Cell const & cell) const;

      [[nodiscard]] std::vector<Block> & blocks() { return blocks_; }

      [[nodiscard]] std::vector<Block> const & blocks() const { return blocks_; }

      [[nodiscard]] Block & block_at(Cell const & cell) { return blocks_[index_of(cell)]; }

    private:
      void create_blocks();

      Vector3 cell_counts_{};
      Vector3 cell_size_{};
      std::vector<Block> blocks_;
  };

}
