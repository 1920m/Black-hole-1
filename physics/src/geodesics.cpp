#include "kerr_physics/geodesics.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace kerr {

namespace {
constexpr double kPi = std::numbers::pi_v<double>;
constexpr double kThetaEpsilon = 1e-8;  // keep θ off the BL spin axis (CLAUDE.md §9)

double clamp_theta(double theta) noexcept {
  return std::clamp(theta, kThetaEpsilon, kPi - kThetaEpsilon);
}
}  // namespace

KerrGeodesic::KerrGeodesic(const KerrMetric& metric, double energy,
                           double angular_momentum)
    : metric_(metric), energy_(energy), l_z_(angular_momentum) {}

// Inverse Kerr metric components in Boyer-Lindquist coordinates.
// The (t,φ) 2×2 block determinant is −Δ sin²θ, giving the closed forms below.
KerrGeodesic::InverseMetric KerrGeodesic::inverse_metric(double r,
                                                         double theta) const noexcept {
  const double th = clamp_theta(theta);
  const double sin_t = std::sin(th);
  const double sin2 = sin_t * sin_t;
  const double a = metric_.spin();
  const double m = metric_.mass();
  const double sigma = metric_.sigma(r, th);
  const double delta = metric_.delta(r);
  const double a_coef = metric_.A(r, th);

  InverseMetric g;
  g.g_tt     = -a_coef / (sigma * delta);
  g.g_rr     = delta / sigma;
  g.g_thth   = 1.0 / sigma;
  g.g_tphi   = -2.0 * m * a * r / (sigma * delta);
  g.g_phiphi = (sigma - 2.0 * m * r) / (sigma * delta * sin2);
  return g;
}

double KerrGeodesic::hamiltonian_rt(double r, double theta, double p_r,
                                    double p_theta) const noexcept {
  const InverseMetric g = inverse_metric(r, theta);
  const double p_t = -energy_;
  const double p_phi = l_z_;
  return 0.5 * (g.g_tt * p_t * p_t + 2.0 * g.g_tphi * p_t * p_phi +
                g.g_rr * p_r * p_r + g.g_thth * p_theta * p_theta +
                g.g_phiphi * p_phi * p_phi);
}

double KerrGeodesic::hamiltonian(const GeodesicState& s) const noexcept {
  return hamiltonian_rt(s.r, s.theta, s.p_r, s.p_theta);
}

double KerrGeodesic::carter_Q(const GeodesicState& s) const noexcept {
  const double th = clamp_theta(s.theta);
  const double cos_t = std::cos(th);
  const double sin_t = std::sin(th);
  const double a = metric_.spin();
  // Null (μ = 0) Carter constant: Q = p_θ² + cos²θ[ L_z²/sin²θ − a²E² ].
  return s.p_theta * s.p_theta +
         cos_t * cos_t * (l_z_ * l_z_ / (sin_t * sin_t) - a * a * energy_ * energy_);
}

// Hamilton's equations. Only p_r and p_θ have non-zero derivatives because the
// metric is cyclic in t and φ; their conjugate momenta (−E, L_z) are constant.
// ∂H/∂r and ∂H/∂θ are evaluated by central finite differences (the analytic
// forms are error-prone; the FD step is tuned so the truncation/round-off error
// is far below the 1e-6 conservation tolerance).
void KerrGeodesic::derivatives(const GeodesicState& s, GeodesicState& d) const noexcept {
  const InverseMetric g = inverse_metric(s.r, s.theta);
  const double p_t = -energy_;
  const double p_phi = l_z_;

  d.t = g.g_tt * p_t + g.g_tphi * p_phi;
  d.r = g.g_rr * s.p_r;
  d.theta = g.g_thth * s.p_theta;
  d.phi = g.g_tphi * p_t + g.g_phiphi * p_phi;

  const double hr = 1e-5 * std::max(1.0, std::abs(s.r));
  const double ht = 1e-5;
  d.p_r = -(hamiltonian_rt(s.r + hr, s.theta, s.p_r, s.p_theta) -
            hamiltonian_rt(s.r - hr, s.theta, s.p_r, s.p_theta)) /
          (2.0 * hr);
  d.p_theta = -(hamiltonian_rt(s.r, s.theta + ht, s.p_r, s.p_theta) -
                hamiltonian_rt(s.r, s.theta - ht, s.p_r, s.p_theta)) /
              (2.0 * ht);
}

GeodesicState KerrGeodesic::make_null_state(double r, double theta,
                                            double p_theta, bool outgoing) const {
  const double th = clamp_theta(theta);
  const InverseMetric g = inverse_metric(r, th);
  const double p_t = -energy_;
  const double p_phi = l_z_;
  // Solve H = 0 for p_r²:  g_rr p_r² = −(rest), rest = all non-radial terms.
  const double rest = g.g_tt * p_t * p_t + 2.0 * g.g_tphi * p_t * p_phi +
                      g.g_thth * p_theta * p_theta + g.g_phiphi * p_phi * p_phi;
  double p_r_sq = -rest / g.g_rr;
  if (p_r_sq < 0.0) p_r_sq = 0.0;  // at/inside a radial turning point

  GeodesicState s;
  s.r = r;
  s.theta = th;
  s.p_theta = p_theta;
  s.p_r = (outgoing ? 1.0 : -1.0) * std::sqrt(p_r_sq);
  return s;
}

void KerrGeodesic::step_rk4(GeodesicState& s, double h) const {
  auto axpy = [](const GeodesicState& base, const GeodesicState& k, double f) {
    GeodesicState out;
    out.t = base.t + f * k.t;
    out.r = base.r + f * k.r;
    out.theta = base.theta + f * k.theta;
    out.phi = base.phi + f * k.phi;
    out.p_r = base.p_r + f * k.p_r;
    out.p_theta = base.p_theta + f * k.p_theta;
    return out;
  };

  GeodesicState k1, k2, k3, k4, tmp;
  derivatives(s, k1);
  tmp = axpy(s, k1, 0.5 * h);
  derivatives(tmp, k2);
  tmp = axpy(s, k2, 0.5 * h);
  derivatives(tmp, k3);
  tmp = axpy(s, k3, h);
  derivatives(tmp, k4);

  const double sixth = h / 6.0;
  s.t += sixth * (k1.t + 2.0 * k2.t + 2.0 * k3.t + k4.t);
  s.r += sixth * (k1.r + 2.0 * k2.r + 2.0 * k3.r + k4.r);
  s.theta += sixth * (k1.theta + 2.0 * k2.theta + 2.0 * k3.theta + k4.theta);
  s.phi += sixth * (k1.phi + 2.0 * k2.phi + 2.0 * k3.phi + k4.phi);
  s.p_r += sixth * (k1.p_r + 2.0 * k2.p_r + 2.0 * k3.p_r + k4.p_r);
  s.p_theta += sixth * (k1.p_theta + 2.0 * k2.p_theta + 2.0 * k3.p_theta + k4.p_theta);
  s.theta = clamp_theta(s.theta);
}

}  // namespace kerr
