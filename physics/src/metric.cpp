#include "kerr_physics/metric.hpp"

#include <algorithm>
#include <cmath>

namespace kerr {

KerrMetric::KerrMetric(double mass, double spin) : mass_(mass), spin_(spin) {}

KerrMetric KerrMetric::from_spin_parameter(double mass, double a_star) {
  return KerrMetric(mass, a_star * mass);
}

// --- metric building blocks --------------------------------------------------

double KerrMetric::sigma(double r, double theta) const noexcept {
  const double cos_t = std::cos(theta);
  return r * r + spin_ * spin_ * cos_t * cos_t;
}

double KerrMetric::delta(double r) const noexcept {
  return r * r - 2.0 * mass_ * r + spin_ * spin_;
}

double KerrMetric::A(double r, double theta) const noexcept {
  const double sin_t = std::sin(theta);
  const double r2_a2 = r * r + spin_ * spin_;
  return r2_a2 * r2_a2 - spin_ * spin_ * delta(r) * sin_t * sin_t;
}

// --- characteristic radii ----------------------------------------------------

double KerrMetric::r_plus() const noexcept {
  return mass_ + std::sqrt(std::max(0.0, mass_ * mass_ - spin_ * spin_));
}

double KerrMetric::r_minus() const noexcept {
  return mass_ - std::sqrt(std::max(0.0, mass_ * mass_ - spin_ * spin_));
}

double KerrMetric::r_ergosphere(double theta) const noexcept {
  const double cos_t = std::cos(theta);
  return mass_ + std::sqrt(std::max(0.0, mass_ * mass_ - spin_ * spin_ * cos_t * cos_t));
}

// Photon-sphere radii: r_ph = 2M{1 + cos[(2/3) arccos(∓a/M)]}.
// Prograde (co-rotating) takes −a/M, retrograde takes +a/M; both give 3M at a=0.
double KerrMetric::r_photon_prograde() const noexcept {
  const double a_star = spin_ / mass_;
  return 2.0 * mass_ * (1.0 + std::cos((2.0 / 3.0) * std::acos(-a_star)));
}

double KerrMetric::r_photon_retrograde() const noexcept {
  const double a_star = spin_ / mass_;
  return 2.0 * mass_ * (1.0 + std::cos((2.0 / 3.0) * std::acos(a_star)));
}

// ISCO radii (Bardeen, Press & Teukolsky 1972):
//   Z1 = 1 + (1−a*²)^(1/3) [ (1+a*)^(1/3) + (1−a*)^(1/3) ]
//   Z2 = √(3a*² + Z1²)
//   r_isco/M = 3 + Z2 ∓ √[(3−Z1)(3+Z1+2Z2)]   (− prograde, + retrograde)
namespace {
struct IscoZ { double z1, z2; };
IscoZ isco_z(double a_star) noexcept {
  const double z1 = 1.0 + std::cbrt(1.0 - a_star * a_star) *
                              (std::cbrt(1.0 + a_star) + std::cbrt(1.0 - a_star));
  const double z2 = std::sqrt(3.0 * a_star * a_star + z1 * z1);
  return {z1, z2};
}
}  // namespace

double KerrMetric::r_isco_prograde() const noexcept {
  const double a_star = spin_ / mass_;
  const IscoZ z = isco_z(a_star);
  return mass_ * (3.0 + z.z2 - std::sqrt((3.0 - z.z1) * (3.0 + z.z1 + 2.0 * z.z2)));
}

double KerrMetric::r_isco_retrograde() const noexcept {
  const double a_star = spin_ / mass_;
  const IscoZ z = isco_z(a_star);
  return mass_ * (3.0 + z.z2 + std::sqrt((3.0 - z.z1) * (3.0 + z.z1 + 2.0 * z.z2)));
}

bool KerrMetric::is_extremal() const noexcept {
  return std::abs(spin_) >= mass_;
}

}  // namespace kerr
