#pragma once
/// @file disk.hpp
/// Phase 4 — relativistic emission from a geometrically thin, optically thick
/// equatorial accretion disk (the project specification §10). All functions use
/// geometric units (G = c = M = 1) and assume a prograde, circular-geodesic disk.
///
/// The observed image of such a disk is shaped by two relativistic effects:
///   * a redshift factor g = ν_obs/ν_emit combining gravitational time dilation
///     and the orbital Doppler shift, and
///   * relativistic beaming, which boosts observed intensity as I_obs ∝ g⁴.
/// Together these make the side of the disk rotating *towards* the observer
/// markedly brighter and bluer — the asymmetry tested for in Success Criterion 6.

#include <glm/vec3.hpp>

namespace kerr {

/// Angular velocity dφ/dt of a prograde circular equatorial geodesic, Ω = 1/(r^{3/2}+a).
double disk_orbital_angular_velocity(double r, double a) noexcept;

/// Redshift factor g = ν_obs/ν_emit for a photon of axial impact parameter
/// b = L_z/E emitted by the disk at Boyer-Lindquist radius r:
///     g = √(r³ − 3r² + 2a·r^{3/2}) / ((r^{3/2}+a) − b).
/// g > 1 is a blueshift (disk approaching), g < 1 a redshift (receding).
/// Returns 0 where no circular-orbit emitter can exist (r inside the photon orbit).
double disk_redshift_factor(double r, double a, double b) noexcept;

/// Relative effective surface temperature, 1 at the inner edge and falling
/// outwards as the thin-disk profile T ∝ (r_inner/r)^{3/4}.
double disk_temperature(double r, double r_inner) noexcept;

/// Observed bolometric brightness ∝ g⁴ · T⁴ (relativistic beaming × Stefan–Boltzmann).
double disk_observed_brightness(double g, double temperature) noexcept;

/// Approximate blackbody colour (linear RGB, components in [0,1]) for a relative
/// colour temperature: deep red when cool, through orange/white, to blue when hot.
glm::dvec3 blackbody_color(double color_temperature) noexcept;

}  // namespace kerr
