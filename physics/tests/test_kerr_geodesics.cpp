// Phase 2 gate — prove the spin (Kerr) physics is genuinely present, not the
// faked Lense-Thirring term of the old project. Three checks: Carter's Q stays
// conserved at high spin; an L_z=0 photon is dragged in azimuth by spin (frame
// dragging) but not without it; and a non-equatorial photon oscillates in θ
// between Carter turning points.
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <numbers>

#include "kerr_physics/geodesics.hpp"
#include "kerr_physics/metric.hpp"
#include "test_util.hpp"

using kerr::GeodesicState;
using kerr::KerrGeodesic;
using kerr::KerrMetric;
using kerr_test::rel_close;

namespace {
constexpr double kPi = std::numbers::pi_v<double>;
}  // namespace

TEST_CASE("Carter's Q is conserved for a rapidly spinning hole (a*=0.9)",
          "[geodesics][kerr][conserved]") {
  const auto metric = KerrMetric::from_spin_parameter(1.0, 0.9);
  KerrGeodesic geo(metric, /*E=*/1.0, /*L_z=*/2.0);
  GeodesicState s = geo.make_null_state(12.0, kPi / 2.0, /*p_theta=*/1.0, /*outgoing=*/true);

  const double q0 = geo.carter_Q(s);
  REQUIRE(q0 > 0.0);
  for (int i = 0; i < 100; ++i) geo.step_rk4(s, 0.05);

  REQUIRE(rel_close(geo.carter_Q(s), q0, 1e-6));
  REQUIRE(std::abs(geo.hamiltonian(s)) < 1e-6);
}

TEST_CASE("Frame dragging sweeps a zero-angular-momentum photon in phi",
          "[geodesics][kerr][frame-dragging]") {
  // Drop a purely radial (L_z = 0, Q = 0) photon from rest-in-phi and measure
  // how much azimuth it accumulates falling toward the hole.
  auto accumulated_dphi = [](double a_star) {
    const auto metric = KerrMetric::from_spin_parameter(1.0, a_star);
    KerrGeodesic geo(metric, /*E=*/1.0, /*L_z=*/0.0);
    GeodesicState s = geo.make_null_state(15.0, kPi / 2.0, /*p_theta=*/0.0, /*outgoing=*/false);
    const double phi0 = s.phi;
    const double r_stop = metric.r_plus() + 0.05;
    for (int i = 0; i < 6000 && s.r > r_stop; ++i) geo.step_rk4(s, 0.01);
    return std::abs(s.phi - phi0);
  };

  const double dphi_schwarzschild = accumulated_dphi(0.0);
  const double dphi_kerr = accumulated_dphi(0.9);
  INFO("dphi Schwarzschild = " << dphi_schwarzschild << ", dphi Kerr(a*=0.9) = " << dphi_kerr);

  REQUIRE(dphi_schwarzschild < 1e-6);  // no spin -> no dragging
  REQUIRE(dphi_kerr > 0.3);            // spin drags it substantially around the hole
}

TEST_CASE("A non-equatorial Kerr photon oscillates in theta", "[geodesics][kerr][theta]") {
  const double M = 1.0, a = 0.9;
  const auto metric = KerrMetric::from_spin_parameter(M, a);

  // Spherical photon orbit (Teo 2003) at r_s between the prograde/retrograde
  // photon spheres: constant r, sustained latitude oscillation.
  const double rs = 2.5;
  const double L = -(rs * rs * rs - 3.0 * M * rs * rs + a * a * rs + M * a * a) /
                   (a * (rs - M));
  const double Q = -rs * rs * rs *
                   (rs * rs * rs - 6.0 * M * rs * rs + 9.0 * M * M * rs - 4.0 * M * a * a) /
                   (a * a * (rs - M) * (rs - M));
  REQUIRE(Q > 0.0);

  KerrGeodesic geo(metric, /*E=*/1.0, /*L_z=*/L);
  GeodesicState s = geo.make_null_state(rs, kPi / 2.0, /*p_theta=*/std::sqrt(Q), /*outgoing=*/false);
  const double q0 = geo.carter_Q(s);

  double prev_p_theta = s.p_theta;
  int turning_points = 0;
  double theta_min = s.theta, theta_max = s.theta;
  double r_min = s.r, r_max = s.r;
  for (int i = 0; i < 6000 && turning_points < 2; ++i) {
    geo.step_rk4(s, 0.01);
    if (s.p_theta * prev_p_theta < 0.0) ++turning_points;  // p_theta crossed zero
    prev_p_theta = s.p_theta;
    theta_min = std::min(theta_min, s.theta);
    theta_max = std::max(theta_max, s.theta);
    r_min = std::min(r_min, s.r);
    r_max = std::max(r_max, s.r);
  }
  INFO("turning_points=" << turning_points << "  theta in [" << theta_min << ","
       << theta_max << "]  r in [" << r_min << "," << r_max << "]");

  REQUIRE(turning_points >= 2);              // up-swing then down-swing => oscillation
  REQUIRE((theta_max - theta_min) > 0.2);    // a real excursion in latitude
  REQUIRE(rel_close(geo.carter_Q(s), q0, 1e-4));  // Q held through the oscillation
}
