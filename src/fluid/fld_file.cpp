#include "fluid/fld_file.hpp"

#include "fluid/error.hpp"

#include <array>
#include <bit>
#include <cstdint>
#include <fstream>
#include <ios>
#include <string>
#include <utility>

namespace fluid {

  namespace {

    template <typename T>
    bool read_value(std::ifstream & input, T & value) {
      std::array<char, sizeof(T)> bytes{};
      if (!input.read(bytes.data(), sizeof(T))) { return false; }
      value = std::bit_cast<T>(bytes);
      return true;
    }

    template <typename T>
    void write_value(std::ofstream & output, T value) {
      auto const bytes = std::bit_cast<std::array<char, sizeof(T)>>(value);
      output.write(bytes.data(), sizeof(T));
    }

    bool read_vector(std::ifstream & input, Vector3 & vector) {
      for (double & component : vector) {
        float value = 0.0F;
        if (!read_value(input, value)) { return false; }
        component = static_cast<double>(value);
      }
      return true;
    }

    void write_vector(std::ofstream & output, Vector3 const & vector) {
      for (double const component : vector) { write_value(output, static_cast<float>(component)); }
    }

    bool read_particle(std::ifstream & input, Particle & particle) {
      return read_vector(input, particle.position) && read_vector(input, particle.half_velocity) &&
             read_vector(input, particle.velocity);
    }

  }

  FluidState read_fld(std::string const & path) {
    std::ifstream input{path, std::ios::binary};
    if (!input) {
      throw FluidError{ExitCode::unreadable_input, "Cannot open " + path + " for reading"};
    }

    float particles_per_meter = 0.0F;
    std::int32_t header_count = 0;
    if (!read_value(input, particles_per_meter) || !read_value(input, header_count)) {
      throw FluidError{ExitCode::invalid_input_file, "Error: Invalid input file header."};
    }
    if (header_count <= 0) {
      throw FluidError{ExitCode::invalid_input_file,
                       "Error: Invalid number of particles: " + std::to_string(header_count) + "."};
    }
    if (particles_per_meter <= 0.0F) {
      throw FluidError{ExitCode::invalid_input_file, "Error: Invalid particles per meter."};
    }

    FluidState state;
    state.particles_per_meter = static_cast<double>(particles_per_meter);
    Particle particle;
    while (read_particle(input, particle)) {
      particle.id = static_cast<int>(state.particles.size());
      state.particles.push_back(particle);
      particle = Particle{};
    }
    if (std::cmp_not_equal(state.particles.size(), header_count)) {
      throw FluidError{
        ExitCode::invalid_input_file,
        "Error: Number of particles mismatch. Header: " + std::to_string(header_count) +
            ", Found: " + std::to_string(state.particles.size()) + "."};
    }
    return state;
  }

  void write_fld(std::string const & path, double particles_per_meter,
                 std::vector<Particle> const & particles) {
    std::ofstream output{path, std::ios::binary};
    if (!output) {
      throw FluidError{ExitCode::unwritable_output, "Cannot open " + path + " for writing"};
    }
    write_value(output, static_cast<float>(particles_per_meter));
    write_value(output, static_cast<std::int32_t>(particles.size()));
    for (Particle const & particle : particles) {
      write_vector(output, particle.position);
      write_vector(output, particle.half_velocity);
      write_vector(output, particle.velocity);
    }
  }

}
