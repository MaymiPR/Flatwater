#pragma once
// The shifter's and the modulators' transcendental functions, the same to the last bit on every platform (as
// Source/common/FwMath.h, which this builds on): the shifter decides on near-equal values (voicing, its marks, which
// period a grain reads), and a unit in the last place from the C library's exp, log or sin can move a grain. sin and
// cos are here (FwMath.h has none): a reduction by 2 pi split in two parts, then the Taylor series to x^21 on
// [-pi/2, pi/2] (the rest below 2e-17). Uses only +, -, *, / and floor.
#include <cmath>
#include "FwMath.h"

namespace fwpitchbend::pbmath
{
inline constexpr double ln2 = 6.93147180559945309417e-01;
inline constexpr double piHi = 3.14159265358979311600e+00;     // pi split so that k * twoPiHi is exact for |k| < 2^20
inline constexpr double piLo = 1.22464679914735317723e-16;

inline double log2 (double x) { return fw::math::log (x) * fw::math::invLn2; }
inline double exp2 (double x) { return fw::math::exp (x * ln2); }
inline double exp (double x) { return fw::math::exp (x); }
inline double log (double x) { return fw::math::log (x); }
inline double pow10 (double y) { return fw::math::pow10 (y); }
inline double pow (double a, double b) { return fw::math::pow (a, b); }      // a > 0
inline double tanh (double x) { return fw::math::tanh (x); }

inline double sin (double x)
{
    if (std::isnan (x)) return x;
    const double k = std::floor (x * (0.5 / piHi) + 0.5);
    double r = (x - k * (2.0 * piHi)) - k * (2.0 * piLo);      // [-pi, pi]
    if (r > 0.5 * piHi) r = (piHi - r) + piLo;                // sin (pi - r)
    else if (r < -0.5 * piHi) r = (-piHi - r) - piLo;
    const double z = r * r;
    double p = -1.0 / 51090942171709440000.0;                 // -1 / 21!
    p = 1.0 / 121645100408832000.0 + z * p;
    p = -1.0 / 355687428096000.0 + z * p;
    p = 1.0 / 1307674368000.0 + z * p;
    p = -1.0 / 6227020800.0 + z * p;
    p = 1.0 / 39916800.0 + z * p;
    p = -1.0 / 362880.0 + z * p;
    p = 1.0 / 5040.0 + z * p;
    p = -1.0 / 120.0 + z * p;
    p = 1.0 / 6.0 + z * p;
    return r - r * z * p;
}

inline double cos (double x) { return sin (x + 0.5 * piHi); }
}
