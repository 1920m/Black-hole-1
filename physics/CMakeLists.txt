# kerr_physics — pure-CPU physics core (GLM only) plus Catch2 test suites.
find_package(glm CONFIG REQUIRED)
find_package(Catch2 3 CONFIG REQUIRED)

add_library(kerr_physics STATIC
  src/constants.cpp
  src/metric.cpp
  src/geodesics.cpp
)
target_include_directories(kerr_physics
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(kerr_physics PUBLIC cxx_std_20)
target_link_libraries(kerr_physics PUBLIC glm::glm)

# --- unit tests (Gate 0) -----------------------------------------------------
# One executable per suite; each is registered as a CTest test.
function(add_physics_test name)
  add_executable(${name} tests/${name}.cpp)
  target_include_directories(${name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)
  target_link_libraries(${name} PRIVATE kerr_physics Catch2::Catch2WithMain)
  add_test(NAME ${name} COMMAND ${name})
endfunction()

add_physics_test(test_schwarzschild_limit)
add_physics_test(test_key_radii)
add_physics_test(test_conserved_quantities)
