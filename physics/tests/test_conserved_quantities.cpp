// Gate 0 — the integrator health-check (CLAUDE.md §5). Along a correctly
// integrated null geodesic, E, L_z and Carter's Q must stay constant and the
// super-Hamiltonian must stay ~0, all to within 1e-6 relative drift.
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <numbers>

#include "kerr_physics/geodesics.hpp"
#include "kerr_physics/metric.hpp"
#include "test_util.hpp"

using kerr::GeodesicState;
using kerr::KerrGeodesic;
using kerr::KerrMetric;
using kerr_test::rel_close;

TEST_CASE("E, L_z, Q conserved along a Kerr null geodesic", "[geodesics][conserved]") {
  const double M = 1.0;
  const auto metric = KerrMetric::from_spin_parameter(M, 0.5);
  const double E = 1.0;
  const double Lz = 2.0;
  KerrGeodesic geo(metric, E, Lz);

  // A non-equatorial photon: started on the equator with p_θ ≠ 0 so that
  // Q = p_θ² > 0 and the θ-motion is genuinely exercised. Outgoing so r grows
  // away from the horizon and the trajectory stays numerically well-behaved.
  GeodesicState s =
      geo.make_null_state(10.0, std::numbers::pi / 2.0, 1.0, /*outgoing=*/true);

  const double Q0 = geo.carter_Q(s);
  const double E0 = geo.energy();
  const double L0 = geo.angular_momentum();

  REQUIRE(Q0 > 0.0);
  REQUIRE(std::abs(geo.hamiltonian(s)) < 1e-10);  // null condition at the start

  const double dlambda = 0.05;
  for (int i = 0; i < 100; ++i) {
    geo.step_rk4(s, dlambda);
  }

  INFO("r after 100 steps = " << s.r << ", theta = " << s.theta);
  REQUIRE(rel_close(geo.energy(), E0, 1e-6));
  REQUIRE(rel_close(geo.angular_momentum(), L0, 1e-6));
  REQUIRE(rel_close(geo.carter_Q(s), Q0, 1e-6));
  REQUIRE(std::abs(geo.hamiltonian(s)) < 1e-6);  // remains a null geodesic
}
