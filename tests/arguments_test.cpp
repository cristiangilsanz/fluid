#include "fluid/arguments.hpp"
#include "fluid/error.hpp"

#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <string>
#include <string_view>
#include <vector>

using fluid::ExitCode;
using fluid::FluidError;

namespace {

  class ArgumentsTest : public ::testing::Test {
    protected:
      void SetUp() override {
        directory_ = std::filesystem::temp_directory_path() /
                     ("fluid-arguments-" +
                      std::to_string(::testing::UnitTest::GetInstance()->random_seed()) + "-" +
                      ::testing::UnitTest::GetInstance()->current_test_info()->name());
        std::filesystem::create_directories(directory_);
        input_  = (directory_ / "input.fld").string();
        output_ = (directory_ / "output.fld").string();
        std::ofstream{input_} << "data";
      }

      void TearDown() override { std::filesystem::remove_all(directory_); }

      static ExitCode code_for(std::vector<std::string> const & values) {
        std::vector<std::string_view> const views{values.begin(), values.end()};
        try {
          static_cast<void>(fluid::parse_arguments(views));
        } catch (FluidError const & error) { return error.code(); }
        ADD_FAILURE() << "expected a FluidError";
        return ExitCode::invalid_arguments;
      }

      std::filesystem::path directory_;
      std::string input_;
      std::string output_;
  };

}

TEST_F(ArgumentsTest, AcceptsValidArguments) {
  std::vector<std::string> const values{"2000", input_, output_};
  std::vector<std::string_view> const views{values.begin(), values.end()};
  fluid::Arguments const parsed = fluid::parse_arguments(views);
  EXPECT_EQ(parsed.time_steps, 2000);
  EXPECT_EQ(parsed.input_path, input_);
  EXPECT_EQ(parsed.output_path, output_);
  EXPECT_EQ(parsed.model, fluid::ViscosityModel::standard);
}

TEST_F(ArgumentsTest, SelectsTheCourseReferenceModelWithAFlag) {
  std::vector<std::string> const values{"--course-reference", "5", input_, output_};
  std::vector<std::string_view> const views{values.begin(), values.end()};
  fluid::Arguments const parsed = fluid::parse_arguments(views);
  EXPECT_EQ(parsed.model, fluid::ViscosityModel::course_reference);
  EXPECT_EQ(parsed.time_steps, 5);
}

TEST_F(ArgumentsTest, CountsOnlyPositionalArgumentsAfterTheFlag) {
  EXPECT_EQ(code_for({"--course-reference", "5", input_}), ExitCode::invalid_arguments);
}

TEST_F(ArgumentsTest, RejectsTooFewArguments) {
  EXPECT_EQ(code_for({}), ExitCode::invalid_arguments);
  EXPECT_EQ(code_for({"10", input_}), ExitCode::invalid_arguments);
}

TEST_F(ArgumentsTest, RejectsTooManyArguments) {
  EXPECT_EQ(code_for({"10", input_, output_, "45"}), ExitCode::invalid_arguments);
}

TEST_F(ArgumentsTest, ReportsTheNumberOfArgumentsInTheMessage) {
  try {
    static_cast<void>(fluid::parse_arguments({}));
    FAIL() << "expected a FluidError";
  } catch (FluidError const & error) {
    EXPECT_STREQ(error.what(), "Error: Invalid number of arguments: 0.");
  }
}

TEST_F(ArgumentsTest, RejectsNonNumericTimeSteps) {
  EXPECT_EQ(code_for({"ten", input_, output_}), ExitCode::invalid_arguments);
  EXPECT_EQ(code_for({"12abc", input_, output_}), ExitCode::invalid_arguments);
  EXPECT_EQ(code_for({"", input_, output_}), ExitCode::invalid_arguments);
}

TEST_F(ArgumentsTest, RejectsNegativeTimeSteps) {
  EXPECT_EQ(code_for({"-1", input_, output_}), ExitCode::invalid_time_steps);
}

TEST_F(ArgumentsTest, RejectsAMissingInputFile) {
  auto const missing = (directory_ / "missing.fld").string();
  EXPECT_EQ(code_for({"10", missing, output_}), ExitCode::unreadable_input);
}

TEST_F(ArgumentsTest, RejectsAnUnwritableOutputPath) {
  auto const unwritable = (directory_ / "no-such-folder" / "output.fld").string();
  EXPECT_EQ(code_for({"10", input_, unwritable}), ExitCode::unwritable_output);
}

TEST_F(ArgumentsTest, DoesNotEraseTheInputWhenItIsAlsoTheOutput) {
  std::vector<std::string> const values{"1", input_, input_};
  std::vector<std::string_view> const views{values.begin(), values.end()};
  static_cast<void>(fluid::parse_arguments(views));
  EXPECT_EQ(std::filesystem::file_size(input_), 4U);
}
