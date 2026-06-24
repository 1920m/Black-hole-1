// Phase 1 gate — the CPU reference tracer must reproduce the analytic
// Schwarzschild shadow: a perfect circle whose edge sits at the critical
// impact parameter b_c = 3√3 M ≈ 5.196 M (Success Criterion SC1), and photons
// must be correctly classified as captured vs escaped either side of it.
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <numbers>

#include "kerr_physics/metric.hpp"
#include "kerr_physics/raytrace_cpu.hpp"
#include "test_util.hpp"

using kerr::critical_impact_parameter;
using kerr::DiskParams;
using kerr::KerrMetric;
using kerr::ObserverCamera;
using kerr::RayOutcome;
using kerr::trace_ray;
using kerr_test::rel_close;

namespace {
ObserverCamera equatorial_camera() {
  ObserverCamera cam;
  cam.r_obs = 30.0;
  cam.theta_obs = std::numbers::pi_v<double> / 2.0;  // exactly edge-on for the test
  return cam;
}
}  // namespace

TEST_CASE("Schwarzschild shadow edge sits at b_c = 3*sqrt(3) M", "[raytrace][shadow]") {
  const KerrMetric bh(1.0, 0.0);
  const ObserverCamera cam = equatorial_camera();
  const double analytic = 3.0 * std::sqrt(3.0);  // 5.196152...

  const double b_c = critical_impact_parameter(bh, cam, /*psi=*/0.0);
  INFO("measured b_c = " << b_c << ", analytic = " << analytic);
  REQUIRE(rel_close(b_c, analytic, 0.02));  // SC1: within 2%
}

TEST_CASE("Schwarzschild shadow is a perfect circle (isotropic b_c)", "[raytrace][shadow]") {
  const KerrMetric bh(1.0, 0.0);
  const ObserverCamera cam = equatorial_camera();
  const double pi = std::numbers::pi_v<double>;

  const double b0 = critical_impact_parameter(bh, cam, 0.0);        // +alpha
  const double b90 = critical_impact_parameter(bh, cam, pi / 2.0);  // +beta
  const double b45 = critical_impact_parameter(bh, cam, pi / 4.0);  // diagonal

  // Spherical symmetry: the shadow radius is direction-independent.
  REQUIRE(rel_close(b0, b90, 0.01));
  REQUIRE(rel_close(b0, b45, 0.01));
}

TEST_CASE("Photons are captured inside the shadow and escape outside", "[raytrace][shadow]") {
  const KerrMetric bh(1.0, 0.0);
  const ObserverCamera cam = equatorial_camera();
  DiskParams no_disk;
  no_disk.enabled = false;

  // Well inside the critical radius -> swallowed.
  REQUIRE(trace_ray(bh, 3.0, 0.0, cam, no_disk).outcome == RayOutcome::Horizon);
  REQUIRE(trace_ray(bh, 0.0, 4.0, cam, no_disk).outcome == RayOutcome::Horizon);
  // Well outside -> deflected but escapes to the sky.
  REQUIRE(trace_ray(bh, 7.0, 0.0, cam, no_disk).outcome == RayOutcome::Escaped);
  REQUIRE(trace_ray(bh, 0.0, 8.0, cam, no_disk).outcome == RayOutcome::Escaped);
}
