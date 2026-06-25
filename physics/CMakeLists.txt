# kerr_physics — pure-CPU physics core (GLM only) plus Catch2 test suites.
find_package(glm CONFIG REQUIRED)
find_package(Catch2 3 CONFIG REQUIRED)
find_package(Stb REQUIRED)

add_library(kerr_physics STATIC
  src/constants.cpp
  src/metric.cpp
  src/geodesics.cpp
  src/raytrace_cpu.cpp
  src/disk.cpp
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
add_physics_test(test_shadow_size)
add_physics_test(test_kerr_geodesics)
add_physics_test(test_disk_emission)
add_physics_test(test_kerr_shadow)

# --- Phase 1 reference renderer (CPU ground-truth image) ---------------------
add_executable(render_reference_schwarzschild tools/render_reference_schwarzschild.cpp)
target_link_libraries(render_reference_schwarzschild PRIVATE kerr_physics)
target_include_directories(render_reference_schwarzschild PRIVATE ${Stb_INCLUDE_DIR})

add_executable(render_kerr_disk tools/render_kerr_disk.cpp)
target_link_libraries(render_kerr_disk PRIVATE kerr_physics)
target_include_directories(render_kerr_disk PRIVATE ${Stb_INCLUDE_DIR})
