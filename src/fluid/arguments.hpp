#pragma once

#include "fluid/viscosity_model.hpp"

#include <span>
#include <string>
#include <string_view>

namespace fluid {

  inline constexpr std::string_view course_reference_flag = "--course-reference";

  struct Arguments {
      int time_steps = 0;
      std::string input_path;
      std::string output_path;
      ViscosityModel model = ViscosityModel::standard;
  };

  [[nodiscard]] Arguments parse_arguments(std::span<std::string_view const> arguments);

}
