#pragma once
/// @file metric.hpp
/// Kerr metric in Boyer-Lindquist coordinates, geometric units (G = c = 1).
///
/// Spin is stored as a = J/M (geometric length); the dimensionless spin is
/// a* = a/M ∈ [0, 1). Every quantity below must reduce to its Schwarzschild
/// value at a = 0 (CLAUDE.md §3) — this is enforced by test_schwarzschild_limit.

namespace kerr {

class KerrMetric {
 public:
  /// @param mass  Black hole mass M (geometric length units, normally 1).
  /// @param spin  Spin parameter a = J/M (geometric units); require |spin| <= mass.
  explicit KerrMetric(double mass = 1.0, double spin = 0.0);

  /// Construct from the dimensionless spin a* = a/M ∈ [0, 1).
  static KerrMetric from_spin_parameter(double mass, double a_star);

  double mass() const noexcept { return mass_; }
  double spin() const noexcept { return spin_; }
  double spin_parameter() const noexcept { return spin_ / mass_; }  ///< a* = a/M

  // --- metric building blocks --------------------------------------------------
  double sigma(double r, double theta) const noexcept;  ///< Σ = r² + a²cos²θ
  double delta(double r) const noexcept;                ///< Δ = r² − 2Mr + a²
  double A(double r, double theta) const noexcept;      ///< (r²+a²)² − a²Δsin²θ

  // --- characteristic radii (CLAUDE.md §3 table) -------------------------------
  double r_plus()  const noexcept;                      ///< outer horizon  r₊
  double r_minus() const noexcept;                      ///< inner horizon  r₋
  double r_ergosphere(double theta) const noexcept;     ///< static limit r_E(θ)

  double r_photon_prograde()   const noexcept;          ///< co-rotating photon sphere
  double r_photon_retrograde() const noexcept;          ///< counter-rotating photon sphere

  double r_isco_prograde()   const noexcept;            ///< ISCO co-rotating   (BPT72)
  double r_isco_retrograde() const noexcept;            ///< ISCO counter-rotating (BPT72)

  bool is_extremal() const noexcept;                    ///< |a| >= M (no real horizon)

 private:
  double mass_;
  double spin_;
};

}  // namespace kerr
