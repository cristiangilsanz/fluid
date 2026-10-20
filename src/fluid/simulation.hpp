#pragma once

#include "fluid/grid.hpp"
#include "fluid/particle.hpp"
#include "fluid/viscosity_model.hpp"

#include <ostream>
#include <vector>

namespace fluid {

  class Simulation {
    public:
      Simulation(double particles_per_meter, std::vector<Particle> const & particles,
                 ViscosityModel model = ViscosityModel::standard);

      void step();

      void run(int steps);

      [[nodiscard]] std::vector<Particle> particles() const;

      [[nodiscard]] double particles_per_meter() const { return particles_per_meter_; }

      [[nodiscard]] double smoothing_length() const { return smoothing_length_; }

      [[nodiscard]] double particle_mass() const { return particle_mass_; }

      [[nodiscard]] Grid const & grid() const { return grid_; }

    private:
      struct InteractingPair {
          Particle * particle;
          Particle * neighbor;
          double distance_squared;
      };

      void regroup_particles();
      void compute_densities();
      void compute_accelerations();
      void integrate();
      void add_density_contribution(Particle & particle, Particle & neighbor);
      void transfer_acceleration(Particle & particle, Particle & neighbor, double distance) const;
      void move_particle(Cell const & cell, Particle & particle);
      [[nodiscard]] double wall_penetration(int block_cell, double coordinate, int axis) const;
      void apply_wall_collision(Particle & particle, double penetration, int axis) const;
      void bounce_off_wall(Particle & particle, int block_cell, int axis) const;

      ViscosityModel model_;
      double particles_per_meter_;
      double smoothing_length_;
      double particle_mass_;
      double smoothing_length_squared_;
      double smoothing_length_pow6_;
      double smoothing_length_pow9_;
      double pressure_factor_;
      double viscosity_factor_;
      double viscous_coefficient_;
      double density_factor_;
      Grid grid_;
      std::vector<InteractingPair> pairs_;
  };

  void print_parameters(std::ostream & output, Simulation const & simulation,
                        std::size_t particle_count);

}
