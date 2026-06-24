#pragma once
/// @file geodesics.hpp
/// Null-geodesic integration for the Kerr metric using the Carter-separated
/// super-Hamiltonian H = ½ gᵘᵛ pᵤ pᵥ (= 0 for photons), in Boyer-Lindquist
/// coordinates and geometric units (G = c = 1).
///
/// Energy E = −p_t and axial angular momentum L_z = p_φ are constants of motion
/// (the metric is independent of t and φ) and are supplied to the integrator.
/// Carter's constant Q and the Hamiltonian are *recomputed from the live state*
/// after each step so they can serve as the integrator health-check required by
/// CLAUDE.md §5 (they must stay constant / zero to within tolerance).

#include "kerr_physics/metric.hpp"

namespace kerr {

/// Phase-space state along a geodesic. The cyclic momenta p_t, p_φ are held by
/// KerrGeodesic; here we evolve the four non-trivial variables plus t and φ.
struct GeodesicState {
  double t = 0.0;        ///< coordinate time
  double r = 0.0;        ///< Boyer-Lindquist radius
  double theta = 0.0;    ///< polar angle θ
  double phi = 0.0;      ///< azimuth φ
  double p_r = 0.0;      ///< radial momentum p_r
  double p_theta = 0.0;  ///< polar momentum p_θ
};

class KerrGeodesic {
 public:
  KerrGeodesic(const KerrMetric& metric, double energy, double angular_momentum);

  /// Build a photon state at (r, θ) with the stored (E, L_z) and a chosen polar
  /// momentum p_θ. p_r is solved from the null condition H = 0.
  /// @param outgoing  true → dr/dλ > 0 (p_r > 0); false → ingoing.
  GeodesicState make_null_state(double r, double theta, double p_theta,
                                bool outgoing) const;

  /// Advance @p state by one classic 4th-order Runge-Kutta step of affine
  /// length @p dlambda.
  void step_rk4(GeodesicState& state, double dlambda) const;

  // --- conserved quantities (recomputed from a state) --------------------------
  double energy() const noexcept { return energy_; }             ///< E   = −p_t (const)
  double angular_momentum() const noexcept { return l_z_; }      ///< L_z = p_φ  (const)
  double carter_Q(const GeodesicState& s) const noexcept;        ///< Carter's constant Q
  double hamiltonian(const GeodesicState& s) const noexcept;     ///< ½ gᵘᵛ pᵤ pᵥ (≈ 0)

 private:
  struct InverseMetric { double g_tt, g_rr, g_thth, g_tphi, g_phiphi; };

  InverseMetric inverse_metric(double r, double theta) const noexcept;
  double hamiltonian_rt(double r, double theta, double p_r,
                        double p_theta) const noexcept;
  void derivatives(const GeodesicState& s, GeodesicState& d) const noexcept;

  KerrMetric metric_;
  double energy_;  ///< E   = −p_t
  double l_z_;     ///< L_z = p_φ
};

}  // namespace kerr
