#include "fluid/error.hpp"
#include "fluid/fld_file.hpp"

#include <bit>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <string>
#include <vector>

using fluid::ExitCode;
using fluid::FluidError;
using fluid::Particle;

namespace {

  std::string temp_file(std::string const & name) {
    return (std::filesystem::temp_directory_path() / ("fluid-fld-" + name)).string();
  }

  void write_header(std::string const & path, float particles_per_meter, std::int32_t count,
                    int extra_floats) {
    std::ofstream output{path, std::ios::binary};
    output.write(reinterpret_cast<char const *>(&particles_per_meter), sizeof(float));
    output.write(reinterpret_cast<char const *>(&count), sizeof(std::int32_t));
    float const value = 0.5F;
    for (int i = 0; i < extra_floats; ++i) {
      output.write(reinterpret_cast<char const *>(&value), sizeof(float));
    }
  }

  ExitCode code_reading(std::string const & path) {
    try {
      static_cast<void>(fluid::read_fld(path));
    } catch (FluidError const & error) { return error.code(); }
    ADD_FAILURE() << "expected a FluidError";
    return ExitCode::invalid_arguments;
  }

}

TEST(FldFileTest, ReadsTheSmallInput) {
  fluid::FluidState const state = fluid::read_fld(FLUID_DATA_DIR "/small.fld");
  EXPECT_DOUBLE_EQ(state.particles_per_meter, 204.0);
  ASSERT_EQ(state.particles.size(), 4800U);
  EXPECT_EQ(state.particles.front().id, 0);
  EXPECT_EQ(state.particles.back().id, 4799);
}

TEST(FldFileTest, RoundTripsParticlesAsFloats) {
  Particle particle;
  particle.position      = {0.25, -0.5, 0.125};
  particle.half_velocity = {1.0, 2.0, 3.0};
  particle.velocity      = {-1.0, -2.0, -3.0};
  auto const path        = temp_file("round-trip.fld");
  fluid::write_fld(path, 204.0, {particle, particle});

  fluid::FluidState const state = fluid::read_fld(path);
  std::filesystem::remove(path);
  EXPECT_DOUBLE_EQ(state.particles_per_meter, 204.0);
  ASSERT_EQ(state.particles.size(), 2U);
  EXPECT_EQ(state.particles[1].id, 1);
  EXPECT_EQ(state.particles[1].position, particle.position);
  EXPECT_EQ(state.particles[1].half_velocity, particle.half_velocity);
  EXPECT_EQ(state.particles[1].velocity, particle.velocity);
}

TEST(FldFileTest, IsBinarySafeOnEveryPlatform) {
  Particle particle;
  particle.position                 = {0.0, 0.0, 0.0};
  float const control_z_and_newline = std::bit_cast<float>(std::uint32_t{0x0A1A0D1A});
  particle.velocity = {control_z_and_newline, control_z_and_newline, control_z_and_newline};
  auto const path   = temp_file("binary.fld");
  fluid::write_fld(path, 100.0, std::vector<Particle>(3, particle));
  fluid::FluidState const state = fluid::read_fld(path);
  std::filesystem::remove(path);
  EXPECT_EQ(state.particles.size(), 3U);
}

TEST(FldFileTest, RejectsAMissingFile) {
  EXPECT_EQ(code_reading(temp_file("does-not-exist.fld")), ExitCode::unreadable_input);
}

TEST(FldFileTest, RejectsAParticleCountMismatch) {
  auto const path = temp_file("mismatch.fld");
  write_header(path, 204.0F, 3, 9 * 2);
  EXPECT_EQ(code_reading(path), ExitCode::invalid_input_file);
  std::filesystem::remove(path);
}

TEST(FldFileTest, RejectsANegativeParticleCount) {
  auto const path = temp_file("negative.fld");
  write_header(path, 204.0F, -1, 0);
  EXPECT_EQ(code_reading(path), ExitCode::invalid_input_file);
  std::filesystem::remove(path);
}

TEST(FldFileTest, RejectsANonPositiveParticlesPerMeter) {
  auto const path = temp_file("zero-ppm.fld");
  write_header(path, 0.0F, 0, 0);
  EXPECT_EQ(code_reading(path), ExitCode::invalid_input_file);
  std::filesystem::remove(path);
}

TEST(FldFileTest, RejectsAnEmptyParticleCount) {
  auto const path = temp_file("zero-count.fld");
  write_header(path, 204.0F, 0, 0);
  try {
    static_cast<void>(fluid::read_fld(path));
    ADD_FAILURE() << "expected a FluidError";
  } catch (FluidError const & error) {
    EXPECT_EQ(error.code(), ExitCode::invalid_input_file);
    EXPECT_STREQ(error.what(), "Error: Invalid number of particles: 0.");
  }
  std::filesystem::remove(path);
}

TEST(FldFileTest, ReportsAMismatchLikeTheSpecification) {
  auto const path = temp_file("mismatch-message.fld");
  write_header(path, 204.0F, 3, 9 * 2);
  try {
    static_cast<void>(fluid::read_fld(path));
    ADD_FAILURE() << "expected a FluidError";
  } catch (FluidError const & error) {
    EXPECT_STREQ(error.what(), "Error: Number of particles mismatch. Header: 3, Found: 2.");
  }
  std::filesystem::remove(path);
}
