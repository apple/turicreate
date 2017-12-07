#ifndef HALIDE_ROUNDING_MODE_H
#define HALIDE_ROUNDING_MODE_H
namespace Halide {

/** Rounding modes (IEEE754 2008 4.3 Rounding-direction attributes) */
enum class RoundingMode {
    TowardZero, ///< Round towards zero (IEEE754 2008 4.3.2)
    ToNearestTiesToEven, ///< Round to nearest, when there is a tie pick even integral significand (IEEE754 2008 4.3.1)
    ToNearestTiesToAway, ///< Round to nearest, when there is a tie pick value furthest away from zero (IEEE754 2008 4.3.1)
    TowardPositiveInfinity, ///< Round towards positive infinity (IEEE754 2008 4.3.2)
    TowardNegativeInfinity ///< Round towards negative infinity (IEEE754 2008 4.3.2)
};

}
#endif
