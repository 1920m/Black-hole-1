#include "kerr_physics/constants.hpp"

namespace kerr {

double gravitational_length_m(double mass_kg) noexcept {
  return kGravitationalConst * mass_kg / (kSpeedOfLight * kSpeedOfLight);
}

double gravitational_length_solar_m(double mass_solar) noexcept {
  return gravitational_length_m(mass_solar * kSolarMass);
}

}  // namespace kerr
