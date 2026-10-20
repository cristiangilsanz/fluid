#include "fluid/simulation.hpp"

#include "fluid/constants.hpp"

#include <algorithm>
#include <cmath>

namespace fluid {

  using namespace constants;

  namespace {

    double square(double value) {
      return value * value;
    }

    double cube(double value) {
      return value * value * value;
    }

  }

  Simulation::Simulation(double particles_per_meter, std::vector<Particle> const & particles,
                         ViscosityModel model)
    : model_{model}, particles_per_meter_{particles_per_meter},
      smoothing_length_{smoothing_multiplier / particles_per_meter},
      particle_mass_{fluid_density / std::pow(particles_per_meter, 3)},
      smoothing_length_squared_{std::pow(smoothing_length_, 2)},
      smoothing_length_pow6_{std::pow(smoothing_length_squared_, 3)},
      smoothing_length_pow9_{std::pow(smoothing_length_, 9)},
      pressure_factor_{(15 / (pi * smoothing_length_pow6_)) *
                       ((3 * particle_mass_ * pressure_stiffness) / 2)},
      viscosity_factor_{45 / (pi * smoothing_length_pow6_)},
      viscous_coefficient_{viscosity_factor_ * viscosity * particle_mass_},
      density_factor_{(315 / (64 * pi * smoothing_length_pow9_)) * particle_mass_},
      grid_{smoothing_length_} {
    for (Particle const & particle : particles) {
      grid_.block_at(grid_.cell_of(particle.position)).particles.push_back(particle);
    }
  }

  void print_parameters(std::ostream & output, Simulation const & simulation,
                        std::size_t particle_count) {
    Vector3 const & counts = simulation.grid().cell_counts();
    Vector3 const & size   = simulation.grid().cell_size();
    output << "Number of particles: " << particle_count << '\n'
           << "Particles per meter: " << simulation.particles_per_meter() << '\n'
           << "Smoothing length: " << simulation.smoothing_length() << '\n'
           << "Particle mass: " << simulation.particle_mass() << '\n'
           << "Grid size: " << counts[0] << " x " << counts[1] << " x " << counts[2] << '\n'
           << "Number of blocks: " << simulation.grid().blocks().size() << '\n'
           << "Block size: " << size[0] << " x " << size[1] << " x " << size[2] << '\n';
  }

  void Simulation::step() {
    regroup_particles();
    compute_densities();
    compute_accelerations();
    integrate();
  }

  void Simulation::run(int steps) {
    for (int i = 0; i < steps; ++i) { step(); }
  }

  std::vector<Particle> Simulation::particles() const {
    std::vector<Particle> all;
    for (Block const & block : grid_.blocks()) {
      all.insert(all.end(), block.particles.begin(), block.particles.end());
    }
    std::ranges::sort(all, {}, &Particle::id);
    return all;
  }

  void Simulation::regroup_particles() {
    for (Block & block : grid_.blocks()) {
      for (Particle & particle : block.particles) {
        Cell const cell       = grid_.cell_of(particle.position);
        particle.density      = 0.0;
        particle.acceleration = gravity;
        Block & destination   = (cell == block.cell) ? block : grid_.block_at(cell);
        destination.incoming.push_back(particle);
      }
      block.particles.clear();
    }
  }

  void Simulation::compute_densities() {
    pairs_.clear();
    auto & blocks = grid_.blocks();
    for (Block & block : blocks) {
      auto & own = block.incoming;
      for (std::size_t i = 0; i < own.size(); ++i) {
        Particle & particle = own[i];
        for (std::size_t j = i + 1; j < own.size(); ++j) {
          add_density_contribution(particle, own[j]);
        }
        for (std::size_t const index : block.later_neighbors) {
          for (Particle & neighbor : blocks[index].incoming) {
            add_density_contribution(particle, neighbor);
          }
        }
        particle.density = (particle.density + smoothing_length_pow6_) * density_factor_;
      }
    }
  }

  void Simulation::add_density_contribution(Particle & particle, Particle & neighbor) {
    double const distance_squared = squared_distance(particle.position, neighbor.position);
    if (distance_squared < smoothing_length_squared_) {
      double const contribution  = cube(smoothing_length_squared_ - distance_squared);
      particle.density          += contribution;
      neighbor.density          += contribution;
      pairs_.push_back({&particle, &neighbor, distance_squared});
    }
  }

  void Simulation::compute_accelerations() {
    for (InteractingPair const & pair : pairs_) {
      double const distance = std::sqrt(std::max(pair.distance_squared, minimum_distance));
      transfer_acceleration(*pair.particle, *pair.neighbor, distance);
    }
  }

  void Simulation::transfer_acceleration(Particle & particle, Particle & neighbor,
                                         double distance) const {
    double const pressure = pressure_factor_ * (square(smoothing_length_ - distance) / distance) *
                            (particle.density + neighbor.density - 2 * fluid_density);
    double viscous = viscous_coefficient_;
    if (model_ == ViscosityModel::standard) { viscous *= smoothing_length_ - distance; }
    double const density_product = particle.density * neighbor.density;
    for (int axis = 0; axis < dimensions; ++axis) {
      double const position_difference = particle.position[axis] - neighbor.position[axis];
      double const velocity_difference = neighbor.velocity[axis] - particle.velocity[axis];
      double const change =
          (position_difference * pressure + velocity_difference * viscous) / density_product;
      particle.acceleration[axis] = particle.acceleration[axis] + change;
      neighbor.acceleration[axis] = neighbor.acceleration[axis] - change;
    }
  }

  void Simulation::integrate() {
    for (Block & block : grid_.blocks()) {
      for (Particle & particle : block.incoming) { move_particle(block.cell, particle); }
      block.incoming.clear();
    }
  }

  void Simulation::move_particle(Cell const & cell, Particle & particle) {
    for (int axis = 0; axis < dimensions; ++axis) {
      double const next_coordinate =
          particle.position[axis] + particle.half_velocity[axis] * time_step;
      double const penetration = wall_penetration(cell[axis], next_coordinate, axis);
      if (penetration > minimum_variation) { apply_wall_collision(particle, penetration, axis); }
      particle.position[axis] += particle.half_velocity[axis] * time_step +
                                 particle.acceleration[axis] * time_step_squared;
      particle.velocity[axis] =
          particle.half_velocity[axis] + (particle.acceleration[axis] * time_step) / 2;
      particle.half_velocity[axis] += particle.acceleration[axis] * time_step;
      bounce_off_wall(particle, cell[axis], axis);
    }
    grid_.block_at(cell).particles.push_back(particle);
  }

  double Simulation::wall_penetration(int block_cell, double coordinate, int axis) const {
    if (block_cell == 0) { return particle_size - (coordinate - lower_limit[axis]); }
    if (block_cell == grid_.cell_counts()[axis] - 1) {
      return particle_size - (upper_limit[axis] - coordinate);
    }
    return 0.0;
  }

  void Simulation::apply_wall_collision(Particle & particle, double penetration, int axis) const {
    int const cell        = grid_.axis_cell(particle.position[axis], axis);
    double const velocity = particle.velocity[axis];
    if (cell == 0) {
      particle.acceleration[axis] += (collision_stiffness * penetration - damping * velocity);
    } else if (cell == grid_.cell_counts()[axis] - 1) {
      particle.acceleration[axis] -= (collision_stiffness * penetration + damping * velocity);
    }
  }

  void Simulation::bounce_off_wall(Particle & particle, int block_cell, int axis) const {
    bool const at_lower = block_cell == 0;
    bool const at_upper = block_cell == grid_.cell_counts()[axis] - 1;
    double overshoot    = 0.0;
    if (at_lower) {
      overshoot = particle.position[axis] - lower_limit[axis];
    } else if (at_upper) {
      overshoot = upper_limit[axis] - particle.position[axis];
    }
    if (overshoot >= 0) { return; }
    if (at_lower) {
      particle.position[axis] = lower_limit[axis] - overshoot;
    } else if (at_upper) {
      particle.position[axis] = upper_limit[axis] + overshoot;
    }
    particle.velocity[axis]      = -particle.velocity[axis];
    particle.half_velocity[axis] = -particle.half_velocity[axis];
  }

}
