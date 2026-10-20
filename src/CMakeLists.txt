add_library(fluid_sim
  fluid/arguments.cpp
  fluid/fld_file.cpp
  fluid/grid.cpp
  fluid/simulation.cpp
)
target_include_directories(fluid_sim PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(fluid_sim PRIVATE fluid_warnings)

add_executable(fluid main.cpp)
target_link_libraries(fluid PRIVATE fluid_sim fluid_warnings)
