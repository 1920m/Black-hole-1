// Gate 0 — for a* > 0 the spin must break the prograde/retrograde symmetry
// (CLAUDE.md §3): co-rotating photon sphere and ISCO sit closer to the hole
// than the counter-rotating ones, and the prograde ISCO moves toward r₊ as
// a* → 1. A perfectly symmetric result here means the spin terms are inert.
#include <catch2/catch_test_macros.hpp>

#include <cmath>

#include "kerr_physics/metric.hpp"
#include "test_util.hpp"

using kerr::KerrMetric;
using kerr_test::rel_close;

TEST_CASE("Kerr prograde/retrograde asymmetry", "[metric][kerr]") {
  const double M = 1.0;

  SECTION("a* = 0.5") {
    const auto bh = KerrMetric::from_spin_parameter(M, 0.5);
    // r+ = M + sqrt(M^2 - a^2), a = 0.5M
    REQUIRE(rel_close(bh.r_plus(), M + std::sqrt(M * M - 0.25 * M * M), 1e-12));

    REQUIRE(bh.r_photon_prograde() < bh.r_photon_retrograde());
    REQUIRE(bh.r_photon_prograde() < 3.0 * M);
    REQUIRE(bh.r_photon_retrograde() > 3.0 * M);

    REQUIRE(bh.r_isco_prograde() < bh.r_isco_retrograde());
    REQUIRE(bh.r_isco_prograde() < 6.0 * M);
    REQUIRE(bh.r_isco_retrograde() > 6.0 * M);
  }

  SECTION("a* = 0.94 (Sgr A* best-bet spin)") {
    const auto bh = KerrMetric::from_spin_parameter(M, 0.94);
    REQUIRE(bh.r_photon_prograde() < bh.r_photon_retrograde());
    REQUIRE(bh.r_isco_prograde() < bh.r_isco_retrograde());
    // Prograde ISCO has dropped well inside 6M but stays outside the horizon.
    REQUIRE(bh.r_isco_prograde() < 3.0 * M);
    REQUIRE(bh.r_isco_prograde() > bh.r_plus());
    REQUIRE(bh.r_photon_prograde() > bh.r_plus());
  }

  SECTION("prograde ISCO and photon sphere shrink monotonically with spin") {
    const auto a0 = KerrMetric::from_spin_parameter(M, 0.0);
    const auto a50 = KerrMetric::from_spin_parameter(M, 0.5);
    const auto a94 = KerrMetric::from_spin_parameter(M, 0.94);
    REQUIRE(a94.r_isco_prograde() < a50.r_isco_prograde());
    REQUIRE(a50.r_isco_prograde() < a0.r_isco_prograde());
    REQUIRE(a94.r_photon_prograde() < a50.r_photon_prograde());
    REQUIRE(a50.r_photon_prograde() < a0.r_photon_prograde());
  }
}
