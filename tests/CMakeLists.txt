include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.tar.gz
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(fluid_tests
  arguments_test.cpp
  fld_file_test.cpp
  grid_test.cpp
  simulation_test.cpp
)
target_link_libraries(fluid_tests PRIVATE fluid_sim fluid_warnings GTest::gtest_main)
target_compile_definitions(fluid_tests PRIVATE FLUID_DATA_DIR="${PROJECT_SOURCE_DIR}/data")

include(GoogleTest)
gtest_discover_tests(fluid_tests WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

foreach(input small large)
  foreach(steps RANGE 1 5)
    add_test(
      NAME reference.${input}.${steps}
      COMMAND ${CMAKE_COMMAND}
        -DFLUID=$<TARGET_FILE:fluid>
        -DSTEPS=${steps}
        -DINPUT=${PROJECT_SOURCE_DIR}/data/${input}.fld
        -DEXPECTED=${PROJECT_SOURCE_DIR}/data/reference/${input}-${steps}.fld
        -DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/${input}-${steps}.fld
        -P ${CMAKE_CURRENT_SOURCE_DIR}/reference_test.cmake
    )
  endforeach()
endforeach()
