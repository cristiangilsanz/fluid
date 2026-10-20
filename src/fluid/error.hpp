#pragma once

#include <stdexcept>
#include <string>

namespace fluid {

  enum class ExitCode : int {
    invalid_arguments  = -1,
    invalid_time_steps = -2,
    unreadable_input   = -3,
    unwritable_output  = -4,
    invalid_input_file = -5,
  };

  class FluidError : public std::runtime_error {
    public:
      FluidError(ExitCode code, std::string const & message)
        : std::runtime_error{message}, code_{code} { }

      [[nodiscard]] ExitCode code() const { return code_; }

    private:
      ExitCode code_;
  };

}
