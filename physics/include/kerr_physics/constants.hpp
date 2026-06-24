#pragma once
/// @file constants.hpp
/// Physical constants and Sgr A* calibration targets (CLAUDE.md §4).
///
/// The physics core works in geometric units (G = c = 1); the SI constants
/// here exist only to convert simulation lengths to/from physical scales
/// (e.g. for the Phase 8 angular-size calibration).

namespace kerr {

// --- SI fundamental constants ------------------------------------------------
inline constexpr double kSpeedOfLight       = 299'792'458.0;       ///< c [m s^-1]
inline constexpr double kGravitationalConst = 6.674'30e-11;        ///< G [m^3 kg^-1 s^-2]
inline constexpr double kSolarMass          = 1.988'92e30;         ///< M_sun [kg]
inline constexpr double kParsecInMeters     = 3.085'677'581e16;    ///< pc [m]

// --- Sagittarius A* (EHT 2022) calibration targets ---------------------------
inline constexpr double kSgrAMassSolar              = 4.297e6;     ///< [M_sun]
inline constexpr double kSgrADistancePc             = 8277.0;      ///< [pc]
inline constexpr double kSgrAShadowMicroArcsec      = 48.7;        ///< ± 7   [μas]
inline constexpr double kSgrAPhotonRingMicroArcsec  = 51.8;        ///< ± 2.3 [μas]

/// Gravitational length GM/c² in metres for a mass given in kilograms.
/// This is the natural length unit of the geometric-unit system (M ≡ 1).
double gravitational_length_m(double mass_kg) noexcept;

/// Gravitational length GM/c² in metres for a mass given in solar masses.
double gravitational_length_solar_m(double mass_solar) noexcept;

}  // namespace kerr
