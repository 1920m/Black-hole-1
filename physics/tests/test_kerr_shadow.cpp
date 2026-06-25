// Phase 3/4 gate — the spinning shadow must be lopsided (Success Criterion 2).
// For a* = 0 the shadow edge is the same distance from the centre in every
// direction (a circle); for a* = 0.9 the prograde and retrograde edges differ,
// because frame dragging shifts and flattens the shadow. A symmetric result at
// a* > 0 would mean the spin terms were inert in the ray tracer.
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <numbers>

#include "kerr_physics/metric.hpp"
#include "kerr_physics/raytrace_cpu.hpp"
#include "test_util.hpp"

using kerr::critical_impact_parameter;
using kerr::KerrMetric;
using kerr::ObserverCamera;
using kerr_test::rel_close;

namespace {
constexpr double kPi = std::numbers::pi_v<double>;
ObserverCamera equatorial_camera() {
  ObserverCamera cam;
  cam.r_obs = 30.0;
  cam.theta_obs = kPi / 2.0;
  return cam;
}
}  // namespace

TEST_CASE("Schwarzschild shadow is circular; Kerr shadow is lopsided (SC2)",
          "[raytrace][kerr][shadow]") {
  const ObserverCamera cam = equatorial_camera();

  SECTION("a*=0 -> equal edges (circle)") {
    const KerrMetric schwarzschild(1.0, 0.0);
    const double edge_plus = critical_impact_parameter(schwarzschild, cam, 0.0);
    const double edge_minus = critical_impact_parameter(schwarzschild, cam, kPi);
    REQUIRE(rel_close(edge_plus, edge_minus, 0.02));
  }

  SECTION("a*=0.9 -> the two edges differ (shadow shifted/flattened)") {
    const auto kerr = KerrMetric::from_spin_parameter(1.0, 0.9);
    const double edge_plus = critical_impact_parameter(kerr, cam, 0.0);   // +alpha
    const double edge_minus = critical_impact_parameter(kerr, cam, kPi);  // -alpha
    INFO("Kerr shadow edges: +alpha=" << edge_plus << "  -alpha=" << edge_minus
         << "  diff=" << std::abs(edge_plus - edge_minus));
    REQUIRE(std::abs(edge_plus - edge_minus) > 0.5);
  }
}
