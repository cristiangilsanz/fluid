#include "fluid/arguments.hpp"

#include "fluid/error.hpp"

#include <charconv>
#include <fstream>
#include <system_error>

namespace fluid {

  namespace {

    int parse_time_steps(std::string_view text) {
      int value         = 0;
      auto const result = std::from_chars(text.data(), text.data() + text.size(), value);
      if (result.ec != std::errc{} || result.ptr != text.data() + text.size()) {
        throw FluidError{ExitCode::invalid_arguments, "Error: time steps must be numeric."};
      }
      if (value < 0) {
        throw FluidError{ExitCode::invalid_time_steps, "Error: Invalid number of time steps."};
      }
      return value;
    }

    void ensure_readable(std::string const & path) {
      if (!std::ifstream{path, std::ios::binary}) {
        throw FluidError{ExitCode::unreadable_input, "Cannot open " + path + " for reading"};
      }
    }

    void ensure_writable(std::string const & path) {
      if (!std::ofstream{path, std::ios::binary | std::ios::app}) {
        throw FluidError{ExitCode::unwritable_output, "Cannot open " + path + " for writing"};
      }
    }

  }

  Arguments parse_arguments(std::span<std::string_view const> arguments) {
    ViscosityModel model = ViscosityModel::standard;
    if (!arguments.empty() && arguments.front() == course_reference_flag) {
      model     = ViscosityModel::course_reference;
      arguments = arguments.subspan(1);
    }
    if (arguments.size() != 3) {
      throw FluidError{ExitCode::invalid_arguments, "Error: Invalid number of arguments: " +
                                                        std::to_string(arguments.size()) + "."};
    }
    Arguments parsed{
      .time_steps  = parse_time_steps(arguments[0]),
      .input_path  = std::string{arguments[1]},
      .output_path = std::string{arguments[2]},
      .model       = model,
    };
    ensure_readable(parsed.input_path);
    ensure_writable(parsed.output_path);
    return parsed;
  }

}
