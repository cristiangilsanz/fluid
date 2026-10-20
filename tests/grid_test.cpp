#include "fluid/constants.hpp"
#include "fluid/grid.hpp"

#include <algorithm>
#include <cstdlib>
#include <gtest/gtest.h>

using fluid::Cell;
using fluid::Grid;
using fluid::Vector3;
using namespace fluid::constants;

namespace {

  constexpr double reference_smoothing_length = 0.00830882;

}

TEST(GridTest, SplitsTheBoxIntoCellsOfAtLeastTheSmoothingLength) {
  Grid const grid{reference_smoothing_length};
  EXPECT_DOUBLE_EQ(grid.cell_counts()[0], 15.0);
  EXPECT_DOUBLE_EQ(grid.cell_counts()[1], 21.0);
  EXPECT_DOUBLE_EQ(grid.cell_counts()[2], 15.0);
  for (int axis = 0; axis < fluid::dimensions; ++axis) {
    EXPECT_DOUBLE_EQ(grid.cell_size()[axis],
                     (upper_limit[axis] - lower_limit[axis]) / grid.cell_counts()[axis]);
    EXPECT_GE(grid.cell_size()[axis], reference_smoothing_length);
  }
  EXPECT_EQ(grid.blocks().size(), 15U * 21U * 15U);
}

TEST(GridTest, HasNoCellsForANonPositiveSmoothingLength) {
  for (double const length : {0.0, -reference_smoothing_length}) {
    Grid const grid{length};
    EXPECT_EQ(grid.cell_counts(), (Vector3{0.0, 0.0, 0.0}));
    EXPECT_EQ(grid.cell_size(), (Vector3{0.0, 0.0, 0.0}));
    EXPECT_TRUE(grid.blocks().empty());
  }
}

TEST(GridTest, FindsTheCellOfAParticle) {
  Grid const grid{reference_smoothing_length};
  EXPECT_EQ(grid.cell_of({-0.04, -0.05, -0.04}), (Cell{2, 3, 2}));
}

TEST(GridTest, ClampsPositionsOutsideTheBoxToTheBorderCells) {
  Grid const grid{reference_smoothing_length};
  EXPECT_EQ(grid.cell_of({-0.07, -0.07, -0.05}), (Cell{0, 1, 1}));
  EXPECT_EQ(grid.cell_of({-0.05, -0.09, -0.05}), (Cell{1, 0, 1}));
  EXPECT_EQ(grid.cell_of({-0.05, -0.07, -0.09}), (Cell{1, 1, 0}));
  EXPECT_EQ(grid.cell_of({20.0, 40.0, 50.0}), (Cell{14, 20, 14}));
}

TEST(GridTest, ContainsOnlyCellsInsideEveryAxis) {
  Grid const grid{reference_smoothing_length};
  EXPECT_TRUE(grid.contains({0, 0, 0}));
  EXPECT_TRUE(grid.contains({14, 20, 14}));
  EXPECT_FALSE(grid.contains({-1, 0, 0}));
  EXPECT_FALSE(grid.contains({0, -1, 0}));
  EXPECT_FALSE(grid.contains({0, 0, -1}));
  EXPECT_FALSE(grid.contains({15, 0, 0}));
  EXPECT_FALSE(grid.contains({0, 0, 15}));
}

TEST(GridTest, RejectsCellsAboveTheTopOfTheBox) {
  Grid const grid{reference_smoothing_length};
  EXPECT_FALSE(grid.contains({0, 21, 0}));
  EXPECT_FALSE(grid.contains({3, 21, 3}));
}

TEST(GridTest, LinksEveryBlockOnlyToItsExistingNeighbors) {
  Grid const grid{reference_smoothing_length};
  auto const & blocks = grid.blocks();
  EXPECT_EQ(blocks[grid.index_of({0, 0, 0})].neighbors.size(), 8U);
  EXPECT_EQ(blocks[grid.index_of({5, 5, 5})].neighbors.size(), 27U);
  EXPECT_EQ(blocks[grid.index_of({5, 20, 5})].neighbors.size(), 18U);
  for (auto const & block : blocks) {
    for (std::size_t const neighbor : block.neighbors) {
      Cell const & other = blocks[neighbor].cell;
      for (int axis = 0; axis < fluid::dimensions; ++axis) {
        EXPECT_LE(std::abs(other[axis] - block.cell[axis]), 1);
      }
    }
  }
}

TEST(GridTest, IndexesBlocksInRowMajorOrder) {
  Grid const grid{reference_smoothing_length};
  EXPECT_EQ(grid.index_of({0, 0, 1}), 1U);
  EXPECT_EQ(grid.index_of({0, 1, 0}), 15U);
  EXPECT_EQ(grid.index_of({1, 0, 0}), 21U * 15U);
  EXPECT_EQ(grid.blocks()[grid.index_of({3, 7, 11})].cell, (Cell{3, 7, 11}));
}

TEST(GridTest, ListsLaterNeighborsInAscendingOrderAfterTheBlockItself) {
  Grid const grid{reference_smoothing_length};
  auto const & blocks = grid.blocks();
  for (std::size_t self = 0; self < blocks.size(); ++self) {
    auto const & later = blocks[self].later_neighbors;
    EXPECT_TRUE(std::ranges::is_sorted(later));
    for (std::size_t const index : later) { EXPECT_GT(index, self); }
    auto const expected = std::ranges::count_if(blocks[self].neighbors, [self](std::size_t index) {
      return index > self;
    });
    EXPECT_EQ(std::ssize(later), expected);
  }
}
