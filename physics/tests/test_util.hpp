#pragma once
/// Small floating-point comparison helpers shared by the physics test suites.
#include <algorithm>
#include <cmath>

namespace kerr_test {

/// Relative closeness: |a−b| / max(|a|,|b|) <= tol, with a sensible fallback
/// to an absolute test when both values are ~zero.
inline bool rel_close(double a, double b, double tol) {
  const double denom = std::max(std::abs(a), std::abs(b));
  if (denom == 0.0) return std::abs(a - b) <= tol;
  return std::abs(a - b) / denom <= tol;
}

/// Absolute closeness: |a−b| <= tol.
inline bool abs_close(double a, double b, double tol) {
  return std::abs(a - b) <= tol;
}

}  // namespace kerr_test
