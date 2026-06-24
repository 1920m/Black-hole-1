#include "kerr_physics/raytrace_cpu.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

#include "kerr_physics/geodesics.hpp"

namespace kerr {
namespace {

constexpr double kPi = std::numbers::pi_v<double>;
constexpr int kMaxSteps = 12000;

// Affine step that is small near the hole (where the path bends sharply) and
// large in the weak-field far region (where the photon travels almost straight).
double adaptive_step(double r, double r_h) {
  return std::clamp(0.04 * (r - r_h), 0.003, 1.5);
}

std::uint8_t to_u8(double v) {
  return static_cast<std::uint8_t>(std::clamp(v, 0.0, 255.0) + 0.5);
}

double lerp(double a, double b, double t) { return a + (b - a) * t; }

}  // namespace

RayResult trace_ray(const KerrMetric& metric, double alpha, double beta,
                    const ObserverCamera& cam, const DiskParams& disk) {
  // Image plane (alpha, beta) -> conserved quantities at the observer.
  const double sin_o = std::sin(cam.theta_obs);
  const double l_z = -alpha * sin_o;
  KerrGeodesic geo(metric, /*energy=*/1.0, /*angular_momentum=*/l_z);
  GeodesicState s =
      geo.make_null_state(cam.r_obs, cam.theta_obs, /*p_theta=*/beta, /*outgoing=*/false);

  const double r_h = metric.r_plus();
  const double r_capture = r_h + 0.01;
  const double r_escape = cam.r_obs + 20.0;
  const double half_pi = kPi / 2.0;

  RayResult res;
  for (int i = 0; i < kMaxSteps; ++i) {
    const double prev_theta = s.theta;
    const double prev_r = s.r;
    geo.step_rk4(s, adaptive_step(s.r, r_h));
    res.steps = i + 1;

    if (disk.enabled) {
      const double d0 = prev_theta - half_pi;
      const double d1 = s.theta - half_pi;
      if (d0 == 0.0 || d0 * d1 < 0.0) {  // crossed (or sits on) the equatorial plane
        const double frac = (d0 == d1) ? 0.0 : d0 / (d0 - d1);
        const double r_cross = prev_r + frac * (s.r - prev_r);
        if (r_cross >= disk.r_inner && r_cross <= disk.r_outer) {
          res.outcome = RayOutcome::Disk;
          res.disk_radius = r_cross;
          return res;
        }
      }
    }

    if (s.r <= r_capture) {
      res.outcome = RayOutcome::Horizon;
      return res;
    }
    if (s.r >= r_escape && s.r > prev_r) {
      res.outcome = RayOutcome::Escaped;
      return res;
    }
  }
  // Step budget exhausted (only near-critical rays orbiting the photon sphere):
  // classify by whether it is still deep in the strong-field region.
  res.outcome = (s.r <= cam.r_obs) ? RayOutcome::Horizon : RayOutcome::Escaped;
  return res;
}

double critical_impact_parameter(const KerrMetric& metric,
                                 const ObserverCamera& cam, double psi,
                                 double b_lo, double b_hi, int iterations) {
  DiskParams no_disk;
  no_disk.enabled = false;
  const double cos_psi = std::cos(psi);
  const double sin_psi = std::sin(psi);
  auto captured = [&](double b) {
    return trace_ray(metric, b * cos_psi, b * sin_psi, cam, no_disk).outcome ==
           RayOutcome::Horizon;
  };
  // Invariant: small b captured, large b escapes; bisect to the transition.
  double lo = b_lo, hi = b_hi;
  for (int i = 0; i < iterations; ++i) {
    const double mid = 0.5 * (lo + hi);
    if (captured(mid)) {
      lo = mid;
    } else {
      hi = mid;
    }
  }
  return 0.5 * (lo + hi);
}

std::vector<std::uint8_t> render_image(const KerrMetric& metric,
                                       const ObserverCamera& cam,
                                       const DiskParams& disk,
                                       int samples_per_axis) {
  const int ss = std::max(1, samples_per_axis);
  const double inv_ss = 1.0 / ss;
  const double inv_n = 1.0 / static_cast<double>(ss * ss);
  std::vector<std::uint8_t> img(
      static_cast<std::size_t>(cam.width) * cam.height * 3, 0);

  for (int py = 0; py < cam.height; ++py) {
    for (int px = 0; px < cam.width; ++px) {
      double acc_r = 0.0, acc_g = 0.0, acc_b = 0.0;
      for (int sy = 0; sy < ss; ++sy) {
        // fy runs 0 (top) -> 1 (bottom); beta runs +half_extent -> -half_extent
        const double fy = (py + (sy + 0.5) * inv_ss) / cam.height;
        const double beta = cam.half_extent * (1.0 - 2.0 * fy);
        for (int sx = 0; sx < ss; ++sx) {
          const double fx = (px + (sx + 0.5) * inv_ss) / cam.width;
          const double alpha = cam.half_extent * (2.0 * fx - 1.0);
          const RayResult r = trace_ray(metric, alpha, beta, cam, disk);

          switch (r.outcome) {
            case RayOutcome::Horizon:
              break;  // the shadow contributes (0,0,0)
            case RayOutcome::Disk: {
              const double t = std::clamp(
                  (r.disk_radius - disk.r_inner) / (disk.r_outer - disk.r_inner),
                  0.0, 1.0);
              // inner: hot near-white; outer: deep orange (placeholder for the
              // physical Novikov-Thorne profile added in Phase 4).
              acc_r += lerp(255.0, 190.0, t);
              acc_g += lerp(248.0, 70.0, t);
              acc_b += lerp(230.0, 25.0, t);
              break;
            }
            case RayOutcome::Escaped:
              acc_r += lerp(10.0, 2.0, fy);
              acc_g += lerp(12.0, 2.0, fy);
              acc_b += lerp(28.0, 6.0, fy);
              break;
          }
        }
      }
      const std::size_t idx =
          (static_cast<std::size_t>(py) * cam.width + px) * 3;
      img[idx + 0] = to_u8(acc_r * inv_n);
      img[idx + 1] = to_u8(acc_g * inv_n);
      img[idx + 2] = to_u8(acc_b * inv_n);
    }
  }
  return img;
}

}  // namespace kerr
