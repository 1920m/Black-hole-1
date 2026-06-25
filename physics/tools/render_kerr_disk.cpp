// Phase 3/4 deliverable — render a rapidly spinning (a*=0.9) black hole with a
// relativistically-emitting accretion disk. The result shows BOTH the lopsided
// Kerr shadow (Phase 3) and the Doppler-beamed, asymmetric disk (Phase 4).
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <numbers>

#include "kerr_physics/metric.hpp"
#include "kerr_physics/raytrace_cpu.hpp"

int main() {
  using namespace kerr;
  constexpr double pi = std::numbers::pi_v<double>;

  const auto black_hole = KerrMetric::from_spin_parameter(1.0, 0.9);  // rapidly spinning

  ObserverCamera cam;
  cam.r_obs = 30.0;
  cam.theta_obs = 80.0 * pi / 180.0;  // inclined so the disk is seen, not edge-on
  cam.width = 360;
  cam.height = 360;
  cam.half_extent = 9.0;

  DiskParams disk;
  disk.enabled = true;
  disk.r_inner = black_hole.r_isco_prograde();  // physical inner edge = r_ISCO(a*)
  disk.r_outer = 20.0;

  std::printf("Rendering Kerr disk: a*=%.2f, i=80deg, ISCO inner edge=%.3f M ...\n",
              black_hole.spin_parameter(), disk.r_inner);
  const std::vector<std::uint8_t> img = render_image(black_hole, cam, disk, /*samples_per_axis=*/2);

  std::filesystem::create_directories("validation/reference_images");
  const char* out = "validation/reference_images/kerr_disk_cpu.png";
  if (stbi_write_png(out, cam.width, cam.height, 3, img.data(), cam.width * 3) == 0) {
    std::fprintf(stderr, "ERROR: failed to write %s\n", out);
    return 1;
  }

  // Quantify the shadow lopsidedness from an equatorial viewpoint.
  ObserverCamera eq = cam;
  eq.theta_obs = pi / 2.0;
  const double edge_plus = critical_impact_parameter(black_hole, eq, 0.0);
  const double edge_minus = critical_impact_parameter(black_hole, eq, pi);
  std::printf("Wrote %s\n", out);
  std::printf("Shadow edges (equatorial): +alpha=%.3f M  -alpha=%.3f M  asymmetry=%.3f M\n",
              edge_plus, edge_minus, std::abs(edge_plus - edge_minus));
  return 0;
}
