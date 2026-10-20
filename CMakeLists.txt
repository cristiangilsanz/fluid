cmake_minimum_required(VERSION 3.22)
project(fluid VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(FLUID_BUILD_TESTS "Build the unit and reference tests" ON)
option(FLUID_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

add_library(fluid_warnings INTERFACE)
if(MSVC)
  target_compile_options(fluid_warnings INTERFACE /W4 /permissive- /fp:precise)
  if(FLUID_WARNINGS_AS_ERRORS)
    target_compile_options(fluid_warnings INTERFACE /WX)
  endif()
else()
  target_compile_options(fluid_warnings INTERFACE -Wall -Wextra -Wpedantic -ffp-contract=off)
  if(FLUID_WARNINGS_AS_ERRORS)
    target_compile_options(fluid_warnings INTERFACE -Werror)
  endif()
endif()

add_subdirectory(src)

if(FLUID_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
