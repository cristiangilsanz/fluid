#include "fluid/constants.hpp"
#include "fluid/fld_file.hpp"
#include "fluid/simulation.hpp"

#include <algorithm>
#include <cmath>
#include <gtest/gtest.h>
#include <sstream>
#include <vector>

using fluid::Particle;
using fluid::Simulation;
using namespace fluid::constants;

namespace {

  constexpr double particles_per_meter = 204.0;

  Particle particle_at(int id, fluid::Vector3 position) {
    Particle particle;
    particle.id       = id;
    particle.position = position;
    return particle;
  }

}

TEST(SimulationTest, DerivesItsParametersFromTheParticleDensity) {
  Simulation const simulation{particles_per_meter, {}};
  EXPECT_DOUBLE_EQ(simulation.smoothing_length(), smoothing_multiplier / particles_per_meter);
  EXPECT_DOUBLE_EQ(simulation.particle_mass(), fluid_density / std::pow(particles_per_meter, 3));
}

TEST(SimulationTest, ALonelyParticleFallsFreely) {
  Simulation simulation{particles_per_meter, {particle_at(0, {0.0, 0.0, 0.0})}};
  simulation.step();
  Particle const particle = simulation.particles().front();
  EXPECT_DOUBLE_EQ(particle.position[1], gravity[1] * time_step_squared);
  EXPECT_DOUBLE_EQ(particle.velocity[1], gravity[1] * time_step / 2);
  EXPECT_DOUBLE_EQ(particle.half_velocity[1], gravity[1] * time_step);
  EXPECT_DOUBLE_EQ(particle.position[0], 0.0);
  EXPECT_DOUBLE_EQ(particle.position[2], 0.0);
}

TEST(SimulationTest, NeighborsFeelEqualAndOppositeForces) {
  double const gap = 0.002;
  Simulation simulation{
    particles_per_meter,
    {particle_at(0, {-gap / 2, 0.0, 0.0}), particle_at(1, {gap / 2, 0.0, 0.0})}
  };
  simulation.step();
  auto const particles = simulation.particles();
  EXPECT_NE(particles[0].velocity[0], 0.0);
  EXPECT_DOUBLE_EQ(particles[0].velocity[0], -particles[1].velocity[0]);
  EXPECT_DOUBLE_EQ(particles[0].velocity[1], particles[1].velocity[1]);
}

TEST(SimulationTest, ParticlesOutOfRangeDoNotInteract) {
  double const gap = 0.05;
  Simulation simulation{
    particles_per_meter,
    {particle_at(0, {-gap / 2, 0.0, 0.0}), particle_at(1, {gap / 2, 0.0, 0.0})}
  };
  simulation.step();
  for (Particle const & particle : simulation.particles()) {
    EXPECT_DOUBLE_EQ(particle.velocity[0], 0.0);
  }
}

TEST(SimulationTest, KeepsEveryParticleAndItsOrder) {
  fluid::FluidState const state = fluid::read_fld(FLUID_DATA_DIR "/small.fld");
  Simulation simulation{state.particles_per_meter, state.particles};
  simulation.run(3);
  auto const particles = simulation.particles();
  ASSERT_EQ(particles.size(), state.particles.size());
  for (std::size_t i = 0; i < particles.size(); ++i) {
    EXPECT_EQ(particles[i].id, static_cast<int>(i));
  }
}

namespace {

  double max_speed(std::vector<Particle> const & particles) {
    double fastest = 0.0;
    for (Particle const & particle : particles) {
      fastest = std::max(
          fastest, std::hypot(particle.velocity[0], particle.velocity[1], particle.velocity[2]));
    }
    return fastest;
  }

}

TEST(SimulationTest, StandardViscosityKeepsTheFluidCalm) {
  fluid::FluidState const state = fluid::read_fld(FLUID_DATA_DIR "/small.fld");
  Simulation simulation{state.particles_per_meter, state.particles};
  simulation.run(100);
  EXPECT_LT(max_speed(simulation.particles()), 2.0);
}

TEST(SimulationTest, CourseReferenceViscosityBlowsUp) {
  fluid::FluidState const state = fluid::read_fld(FLUID_DATA_DIR "/small.fld");
  Simulation simulation{state.particles_per_meter, state.particles,
                        fluid::ViscosityModel::course_reference};
  simulation.run(5);
  EXPECT_GT(max_speed(simulation.particles()), 1000.0);
}

TEST(SimulationTest, KeepsTheFluidInsideTheBox) {
  fluid::FluidState const state = fluid::read_fld(FLUID_DATA_DIR "/small.fld");
  Simulation simulation{state.particles_per_meter, state.particles};
  simulation.run(200);
  double const margin = 10 * particle_size;
  for (Particle const & particle : simulation.particles()) {
    for (int axis = 0; axis < fluid::dimensions; ++axis) {
      EXPECT_TRUE(std::isfinite(particle.position[axis]));
      EXPECT_GE(particle.position[axis], lower_limit[axis] - margin);
      EXPECT_LE(particle.position[axis], upper_limit[axis] + margin);
    }
  }
}

TEST(SimulationTest, PrintsTheParametersFromTheSpecification) {
  fluid::FluidState const state = fluid::read_fld(FLUID_DATA_DIR "/small.fld");
  Simulation const simulation{state.particles_per_meter, state.particles};
  std::ostringstream output;
  fluid::print_parameters(output, simulation, state.particles.size());
  EXPECT_EQ(output.str(), "Number of particles: 4800\n"
                          "Particles per meter: 204\n"
                          "Smoothing length: 0.00830882\n"
                          "Particle mass: 0.00011779\n"
                          "Grid size: 15 x 21 x 15\n"
                          "Number of blocks: 4725\n"
                          "Block size: 0.00866667 x 0.00857143 x 0.00866667\n");
}
