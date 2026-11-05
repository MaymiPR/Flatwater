#pragma once
// exp, log and what follows from them, computed the same to the last bit on every platform. The C library's own
// differ by a unit in the last place between MSVC, glibc and Apple's libm, and a DSP that decides on near-equal
// values (Flatwater Comp's release law) turns that into an audible divergence. These use only +, -, *, / and the
// exact frexp / ldexp / floor, so they give identical results wherever IEEE-754 doubles are rounded to nearest and
// the compiler does not fuse a multiply and an add (CMakeLists.txt turns that off for GCC and Clang).
// Accuracy: within about 2 units in the last place of the true value.
#include <cmath>
#include <functional>
#include <limits>

namespace fw::math
{
// a == b, meant exactly: through std::equal_to, so -Wfloat-equal stays quiet (juce::exactlyEqual where JUCE is in)
template <typename T> constexpr bool exactlyEqual (T a, T b) { return std::equal_to<T>{} (a, b); }

inline constexpr double ln2Hi = 6.93147180369123816490e-01;   // ln 2 split so that k * ln2Hi is exact for |k| < 2^20
inline constexpr double ln2Lo = 1.90821492927058770002e-10;
inline constexpr double invLn2 = 1.44269504088896338700e+00;
inline constexpr double ln10 = 2.30258509299404568402e+00;
inline constexpr double invLn10 = 4.34294481903251827651e-01;

// e^x: x = k ln 2 + r with |r| <= ln 2 / 2, e^r by its Taylor series to r^13 (the rest is below 1e-17), then 2^k
inline double exp (double x)
{
    if (std::isnan (x)) return x;
    if (x > 709.78) return std::numeric_limits<double>::infinity();
    if (x < -745.2) return 0.0;
    const double k = std::floor (x * invLn2 + 0.5);
    const double r = (x - k * ln2Hi) - k * ln2Lo;
    double p = 1.0 / 6227020800.0;                     // 1 / 13!
    p = 1.0 / 479001600.0 + r * p;
    p = 1.0 / 39916800.0 + r * p;
    p = 1.0 / 3628800.0 + r * p;
    p = 1.0 / 362880.0 + r * p;
    p = 1.0 / 40320.0 + r * p;
    p = 1.0 / 5040.0 + r * p;
    p = 1.0 / 720.0 + r * p;
    p = 1.0 / 120.0 + r * p;
    p = 1.0 / 24.0 + r * p;
    p = 1.0 / 6.0 + r * p;
    p = 0.5 + r * p;
    p = 1.0 + r * p;
    p = 1.0 + r * p;
    return std::ldexp (p, (int) k);
}

// ln x: x = m 2^e with m in [sqrt(1/2), sqrt(2)), ln m = 2 atanh (s), s = (m - 1) / (m + 1), by its series to s^23
inline double log (double x)
{
    if (std::isnan (x) || x < 0.0) return std::numeric_limits<double>::quiet_NaN();
    if (exactlyEqual (x, 0.0)) return -std::numeric_limits<double>::infinity();
    if (std::isinf (x)) return x;
    int e = 0;
    double m = std::frexp (x, &e);
    if (m < 0.70710678118654752440) { m *= 2.0; --e; }
    const double f = m - 1.0;
    const double s = f / (2.0 + f), z = s * s;
    double p = 1.0 / 23.0;
    p = 1.0 / 21.0 + z * p;
    p = 1.0 / 19.0 + z * p;
    p = 1.0 / 17.0 + z * p;
    p = 1.0 / 15.0 + z * p;
    p = 1.0 / 13.0 + z * p;
    p = 1.0 / 11.0 + z * p;
    p = 1.0 / 9.0 + z * p;
    p = 1.0 / 7.0 + z * p;
    p = 1.0 / 5.0 + z * p;
    p = 1.0 / 3.0 + z * p;
    const double lm = 2.0 * s + 2.0 * s * (z * p);
    return (double) e * ln2Hi + ((double) e * ln2Lo + lm);
}

inline double log10 (double x) { return log (x) * invLn10; }
inline double pow10 (double y) { return exp (y * ln10); }                 // 10^y
inline double pow (double a, double b) { return exactlyEqual (a, 0.0) ? (b > 0.0 ? 0.0 : std::pow (a, b)) : exp (b * log (a)); }   // a > 0
inline double tanh (double x)
{
    if (x > 20.0) return 1.0;
    if (x < -20.0) return -1.0;
    const double e = exp (2.0 * x);
    return (e - 1.0) / (e + 1.0);
}
inline float expf (float x) { return (float) exp ((double) x); }
inline float tanhf (float x) { return (float) tanh ((double) x); }
}
