// Gate 0 — at a* = 0 every Kerr radius must reduce to its Schwarzschild value
// (CLAUDE.md §3 table): r₊ = 2M, r_E = 2M, r_photon = 3M, r_ISCO = 6M.
#include <catch2/catch_test_macros.hpp>

#include <numbers>

#include "kerr_physics/metric.hpp"
#include "test_util.hpp"

using kerr::KerrMetric;
using kerr_test::rel_close;

TEST_CASE("a*=0 reduces exactly to Schwarzschild", "[metric][schwarzschild]") {
  const double M = 1.0;
  const KerrMetric bh(M, 0.0);
  constexpr double tol = 1e-12;

  SECTION("outer horizon r+ = 2M") {
    REQUIRE(rel_close(bh.r_plus(), 2.0 * M, tol));
  }

  SECTION("ergosphere = 2M at every latitude (no oblateness at a=0)") {
    REQUIRE(rel_close(bh.r_ergosphere(std::numbers::pi / 2.0), 2.0 * M, tol));
    REQUIRE(rel_close(bh.r_ergosphere(0.30), 2.0 * M, tol));
    REQUIRE(rel_close(bh.r_ergosphere(1.20), 2.0 * M, tol));
  }

  SECTION("photon sphere = 3M, prograde == retrograde") {
    REQUIRE(rel_close(bh.r_photon_prograde(), 3.0 * M, tol));
    REQUIRE(rel_close(bh.r_photon_retrograde(), 3.0 * M, tol));
    REQUIRE(rel_close(bh.r_photon_prograde(), bh.r_photon_retrograde(), tol));
  }

  SECTION("ISCO = 6M, prograde == retrograde") {
    REQUIRE(rel_close(bh.r_isco_prograde(), 6.0 * M, 1e-9));
    REQUIRE(rel_close(bh.r_isco_retrograde(), 6.0 * M, 1e-9));
    REQUIRE(rel_close(bh.r_isco_prograde(), bh.r_isco_retrograde(), 1e-9));
  }
}
