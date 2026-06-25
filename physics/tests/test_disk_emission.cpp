// Phase 4 gate — relativistic disk emission. Verifies the redshift factor,
// the >=4x Doppler-beaming asymmetry (Success Criterion 6), the temperature
// profile, and that the disk inner edge tracks r_ISCO (Success Criterion 7).
#include <catch2/catch_test_macros.hpp>

#include <cmath>

#include "kerr_physics/disk.hpp"
#include "kerr_physics/metric.hpp"
#include "test_util.hpp"

using kerr::disk_observed_brightness;
using kerr::disk_redshift_factor;
using kerr::disk_temperature;
using kerr::KerrMetric;
using kerr_test::rel_close;

TEST_CASE("Disk redshift: gravitational baseline, blueshift approaching, redshift receding",
          "[disk][redshift]") {
  // Schwarzschild, r = 6M. b = L_z/E is the photon's axial impact parameter.
  const double g_static = disk_redshift_factor(6.0, 0.0, 0.0);   // pure gravitational
  const double g_approach = disk_redshift_factor(6.0, 0.0, 5.0); // moving toward us
  const double g_recede = disk_redshift_factor(6.0, 0.0, -5.0);  // moving away

  REQUIRE(rel_close(g_static, std::sqrt(0.5), 1e-6));  // √(1 − 3M/r) at r=6
  REQUIRE(g_approach > 1.0);
  REQUIRE(g_recede < g_static);
  REQUIRE(g_approach > g_recede);
}

TEST_CASE("Relativistic beaming makes the approaching side >= 4x brighter (SC6)",
          "[disk][beaming]") {
  const double g_approach = disk_redshift_factor(6.0, 0.0, 5.0);
  const double g_recede = disk_redshift_factor(6.0, 0.0, -5.0);
  const double bright_approach = disk_observed_brightness(g_approach, 1.0);
  const double bright_recede = disk_observed_brightness(g_recede, 1.0);

  INFO("brightness ratio = " << bright_approach / bright_recede);
  REQUIRE(bright_approach / bright_recede >= 4.0);
}

TEST_CASE("Temperature falls outward; inner edge sits at r_ISCO (SC7)", "[disk][temperature]") {
  REQUIRE(disk_temperature(6.0, 6.0) > disk_temperature(12.0, 6.0));
  REQUIRE(rel_close(disk_temperature(6.0, 6.0), 1.0, 1e-12));  // normalised at inner edge

  // The inner edge is r_ISCO(a*), not a constant: 6M for a*=0, well inside for high spin.
  const KerrMetric schwarzschild(1.0, 0.0);
  const auto fast = KerrMetric::from_spin_parameter(1.0, 0.94);
  REQUIRE(rel_close(schwarzschild.r_isco_prograde(), 6.0, 1e-9));
  REQUIRE(fast.r_isco_prograde() < 6.0);
}
