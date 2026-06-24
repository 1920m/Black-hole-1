// Phase 1 deliverable — render the CPU reference Schwarzschild image and write
// it to validation/reference_images/. This image is the ground truth that the
// Phase 6 GPU shader is later diffed against.
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

  const KerrMetric black_hole(/*mass=*/1.0, /*spin=*/0.0);  // Schwarzschild

  ObserverCamera cam;
  cam.r_obs = 30.0;
  cam.theta_obs = 85.0 * std::numbers::pi_v<double> / 180.0;  // near edge-on
  cam.width = 300;
  cam.height = 300;
  cam.half_extent = 8.0;

  DiskParams disk;
  disk.enabled = true;
  disk.r_inner = 6.0;   // r_ISCO (Schwarzschild)
  disk.r_outer = 18.0;

  std::printf("Rendering %dx%d Schwarzschild reference (r_obs=%.1fM, i=85deg)...\n",
              cam.width, cam.height, cam.r_obs);
  const std::vector<std::uint8_t> img =
      render_image(black_hole, cam, disk, /*samples_per_axis=*/2);

  std::filesystem::create_directories("validation/reference_images");
  const char* out = "validation/reference_images/schwarzschild_shadow_cpu.png";
  if (stbi_write_png(out, cam.width, cam.height, 3, img.data(), cam.width * 3) == 0) {
    std::fprintf(stderr, "ERROR: failed to write %s\n", out);
    return 1;
  }

  const double b_c = critical_impact_parameter(black_hole, cam);
  const double analytic = 3.0 * std::sqrt(3.0);
  std::printf("Wrote %s\n", out);
  std::printf("Measured shadow b_c = %.4f M   (analytic 3*sqrt(3) = %.4f M, err %.3f%%)\n",
              b_c, analytic, 100.0 * std::abs(b_c - analytic) / analytic);
  return 0;
}
