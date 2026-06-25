#include "kerr_physics/disk.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace kerr {

double disk_orbital_angular_velocity(double r, double a) noexcept {
  return 1.0 / (std::pow(r, 1.5) + a);  // prograde, M = 1
}

double disk_redshift_factor(double r, double a, double b) noexcept {
  const double r15 = std::pow(r, 1.5);
  const double radicand = r * r * r - 3.0 * r * r + 2.0 * a * r15;  // (1/u^t)²·(r^{3/2}+a)²
  const double denom = (r15 + a) - b;                              // (r^{3/2}+a)(1 − bΩ)
  if (radicand <= 0.0 || denom <= 0.0) return 0.0;  // no static circular emitter here
  return std::sqrt(radicand) / denom;
}

double disk_temperature(double r, double r_inner) noexcept {
  if (r <= 0.0 || r_inner <= 0.0) return 0.0;
  return std::pow(r_inner / r, 0.75);
}

double disk_observed_brightness(double g, double temperature) noexcept {
  const double g2 = g * g;
  const double t2 = temperature * temperature;
  return g2 * g2 * t2 * t2;  // g⁴ · T⁴
}

glm::dvec3 blackbody_color(double t) noexcept {
  // Piecewise colour ramp: cool → deep red, warm → orange/white, hot → blue.
  static constexpr std::array<double, 6> stops = {0.0, 0.6, 1.0, 1.6, 2.2, 3.0};
  static const std::array<glm::dvec3, 6> cols = {
      glm::dvec3(0.60, 0.06, 0.02),  // deep red
      glm::dvec3(1.00, 0.30, 0.08),  // red-orange
      glm::dvec3(1.00, 0.62, 0.28),  // orange (≈ rest-frame inner disk)
      glm::dvec3(1.00, 0.92, 0.78),  // warm white
      glm::dvec3(0.85, 0.92, 1.00),  // blue-white
      glm::dvec3(0.55, 0.72, 1.00),  // blue
  };
  t = std::clamp(t, stops.front(), stops.back());
  for (std::size_t i = 0; i + 1 < stops.size(); ++i) {
    if (t <= stops[i + 1]) {
      const double f = (t - stops[i]) / (stops[i + 1] - stops[i]);
      return cols[i] + (cols[i + 1] - cols[i]) * f;
    }
  }
  return cols.back();
}

}  // namespace kerr
