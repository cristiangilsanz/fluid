#include "fluid/arguments.hpp"
#include "fluid/error.hpp"
#include "fluid/fld_file.hpp"
#include "fluid/simulation.hpp"

#include <exception>
#include <iostream>
#include <span>
#include <string_view>
#include <vector>

int main(int argc, char * argv[]) {
  std::span const raw_arguments{argv, static_cast<std::size_t>(argc)};
  std::vector<std::string_view> const arguments{raw_arguments.begin() + 1, raw_arguments.end()};
  try {
    fluid::Arguments const parsed = fluid::parse_arguments(arguments);
    fluid::FluidState const state = fluid::read_fld(parsed.input_path);
    fluid::Simulation simulation{state.particles_per_meter, state.particles, parsed.model};
    fluid::print_parameters(std::cout, simulation, state.particles.size());
    simulation.run(parsed.time_steps);
    fluid::write_fld(parsed.output_path, simulation.particles_per_meter(), simulation.particles());
  } catch (fluid::FluidError const & error) {
    std::cerr << error.what() << '\n';
    return static_cast<int>(error.code());
  } catch (std::exception const & error) {
    std::cerr << "Error: " << error.what() << '\n';
    return 1;
  }
  return 0;
}
