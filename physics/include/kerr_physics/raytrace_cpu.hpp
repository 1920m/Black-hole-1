#pragma once
/// @file raytrace_cpu.hpp
/// Phase 1 — slow CPU *reference* ray tracer. This is the ground-truth renderer
/// the later GPU shader is validated against (CLAUDE.md §7, Phase 1 / Phase 6).
///
/// It performs backward ray tracing: a photon is launched from the observer
/// "into the scene" along each image-plane direction, integrated through the
/// Kerr (here Schwarzschild, a=0) spacetime with the KerrGeodesic integrator,
/// and classified as captured by the hole, escaped to the sky, or striking the
/// equatorial disk.
///
/// Image-plane coordinates (alpha, beta) are celestial impact parameters
/// (Bardeen 1973): alpha = -L_z / sinθ_obs (horizontal), beta = ±√Q-term
/// (vertical). For Schwarzschild the total impact parameter b = √(α²+β²) fully
/// determines capture: the photon is swallowed iff b < b_c = 3√3 M.

#include <cstdint>
#include <vector>

#include "kerr_physics/metric.hpp"

namespace kerr {

enum class RayOutcome { Horizon, Escaped, Disk };

struct RayResult {
  RayOutcome outcome = RayOutcome::Escaped;
  double disk_radius = 0.0;  ///< BL radius of the equatorial crossing (if Disk)
  int steps = 0;             ///< integration steps taken
};

struct ObserverCamera {
  double r_obs = 30.0;        ///< observer Boyer-Lindquist radius (units of M)
  double theta_obs = 1.4835298641951802;  ///< polar angle (default 85°, near edge-on)
  int width = 300;
  int height = 300;
  double half_extent = 8.0;   ///< image spans ±half_extent in impact-parameter units
};

struct DiskParams {
  bool enabled = true;
  double r_inner = 6.0;       ///< inner edge (r_ISCO for Schwarzschild = 6M)
  double r_outer = 18.0;      ///< outer edge
};

/// Trace one backward ray for image-plane impact parameters (alpha, beta).
RayResult trace_ray(const KerrMetric& metric, double alpha, double beta,
                    const ObserverCamera& cam, const DiskParams& disk);

/// Critical impact parameter b_c (shadow edge) found by bisection along the
/// image-plane direction @p psi (radians; 0 = +alpha axis). For Schwarzschild
/// every direction must give b_c = 3√3 M ≈ 5.196 M (a perfect circle).
double critical_impact_parameter(const KerrMetric& metric,
                                 const ObserverCamera& cam, double psi = 0.0,
                                 double b_lo = 1.0, double b_hi = 9.0,
                                 int iterations = 34);

/// Render an RGB8 image (row-major, 3 bytes/pixel): black shadow, warm disk,
/// dark-sky background. Suitable for stb_image_write.
/// @param samples_per_axis  supersampling factor (n) — n² rays/pixel, averaged,
///        to suppress aliasing of the fine higher-order lensing structure.
std::vector<std::uint8_t> render_image(const KerrMetric& metric,
                                       const ObserverCamera& cam,
                                       const DiskParams& disk,
                                       int samples_per_axis = 1);

}  // namespace kerr
