#include "Float16.h"
#include "Error.h"

#include <limits>
#include <cmath>

using namespace Halide;

namespace Halide {

// An optional implementation of float16_t
// Float16 conversion op that removes LLVM dep,
// so things can be invariant from LLVM until codegen
//5A
// The float16_t here is not accurate for arithmetic(uses float)
// But can be used as a good storage type.

namespace {

union Bits {
  float f;
  int32_t si;
  uint32_t ui;
};

static int const shift = 13;
static int const shiftSign = 16;

static int32_t const infN = 0x7F800000;  // flt32 infinity
static int32_t const maxN = 0x477FE000;  // max flt16 normal as a flt32
static int32_t const minN = 0x38800000;  // min flt16 normal as a flt32
static int32_t const signN = 0x80000000;  // flt32 sign bit

static int32_t const infC = infN >> shift;
static int32_t const nanN = (infC + 1) << shift;  // minimum flt16 nan as a flt32
static int32_t const maxC = maxN >> shift;
static int32_t const minC = minN >> shift;
static int32_t const signC = signN >> shiftSign;  // flt16 sign bit

static int32_t const mulN = 0x52000000;  // (1 << 23) / minN
static int32_t const mulC = 0x33800000;  // minN / (1 << (23 - shift))

static int32_t const subC = 0x003FF;  // max flt32 subnormal down shifted
static int32_t const norC = 0x00400;  // min flt32 normal down shifted

static int32_t const maxD = infC - maxC - 1;
static int32_t const minD = minC - subC - 1;

inline uint16_t float2half(const float& value)  {
  Bits v, s;
  v.f = value;
  uint32_t sign = v.si & signN;
  v.si ^= sign;
  sign >>= shiftSign;  // logical shift
  s.si = mulN;
  s.si = static_cast<int32_t>(s.f * v.f);  // correct subnormals
  v.si ^= (s.si ^ v.si) & -(minN > v.si);
  v.si ^= (infN ^ v.si) & -((infN > v.si) & (v.si > maxN));
  v.si ^= (nanN ^ v.si) & -((nanN > v.si) & (v.si > infN));
  v.ui >>= shift;  // logical shift
  v.si ^= ((v.si - maxD) ^ v.si) & -(v.si > maxC);
  v.si ^= ((v.si - minD) ^ v.si) & -(v.si > subC);
  return v.ui | sign;
}

inline float half2float(const uint16_t& value) {
  Bits v;
  v.ui = value;
  int32_t sign = v.si & signC;
  v.si ^= sign;
  sign <<= shiftSign;
  v.si ^= ((v.si + minD) ^ v.si) & -(v.si > subC);
  v.si ^= ((v.si + maxD) ^ v.si) & -(v.si > maxC);
  Bits s;
  s.si = mulC;
  s.f *= v.si;
  int32_t mask = -(norC > v.si);
  v.si <<= shift;
  v.si ^= (s.si ^ v.si) & mask;
  v.si |= sign;
  return v.f;
}
}  // namespace

// The static_asserts checking the size is to make sure
// float16_t can be used as a 16-bits wide POD type.
float16_t::float16_t(float value, RoundingMode roundingMode) {
    static_assert(sizeof(float16_t) == 2, "float16_t is wrong size");
    this->data = float2half(value);
}

float16_t::float16_t(double value, RoundingMode roundingMode) {
    static_assert(sizeof(float16_t) == 2, "float16_t is wrong size");
    this->data = float2half(static_cast<float>(value));
}

float16_t::float16_t(const char *stringRepr, RoundingMode roundingMode) {
    static_assert(sizeof(float16_t) == 2, "float16_t is wrong size");
    std::memcpy(&data, stringRepr, 2);
}

float16_t::float16_t() {
    static_assert(sizeof(float16_t) == 2, "float16_t is wrong size");
    this->data = 0;
}


float16_t::operator float() const {
    return half2float(data);
}

float16_t::operator double() const {
    return half2float(data);
}

float16_t float16_t::make_zero(bool positive) {
    return float16_t(0.0f, RoundingMode::TowardZero);
}

float16_t float16_t::make_infinity(bool positive) {
    return float16_t(std::numeric_limits<float>::infinity(), RoundingMode::TowardZero);
}

float16_t float16_t::make_nan() {
    return float16_t(std::nan(""), RoundingMode::TowardZero);
}

float16_t float16_t::add(float16_t rhs, RoundingMode roundingMode) const {
    return float16_t(half2float(data) + half2float(rhs.data), roundingMode);
}

float16_t float16_t::subtract(float16_t rhs, RoundingMode roundingMode) const {
    return float16_t(half2float(data) - half2float(rhs.data), roundingMode);
}

float16_t float16_t::multiply(float16_t rhs, RoundingMode roundingMode) const {
    return float16_t(half2float(data) * half2float(rhs.data), roundingMode);
}

float16_t float16_t::divide(float16_t rhs, RoundingMode roundingMode) const {
    return float16_t(half2float(data) / half2float(rhs.data), roundingMode);
}

float16_t float16_t::operator-() const {
    return float16_t(-half2float(data), RoundingMode::TowardZero);
}

float16_t float16_t::operator+(float16_t rhs) const {
    return this->add(rhs, RoundingMode::ToNearestTiesToEven);
}

float16_t float16_t::operator-(float16_t rhs) const {
    return this->subtract(rhs, RoundingMode::ToNearestTiesToEven);
}

float16_t float16_t::operator*(float16_t rhs) const {
    return this->multiply(rhs, RoundingMode::ToNearestTiesToEven);
}

float16_t float16_t::operator/(float16_t rhs) const {
    return this->divide(rhs, RoundingMode::ToNearestTiesToEven);
}

bool float16_t::operator==(float16_t rhs) const {
    return half2float(data) == half2float(rhs.data);
}

bool float16_t::operator>(float16_t rhs) const {
    internal_assert(!this->are_unordered(rhs)) << "Cannot compare unorderable values\n";
    return half2float(data) > half2float(rhs.data);
}

bool float16_t::operator<(float16_t rhs) const {
    internal_assert(!this->are_unordered(rhs)) << "Cannot compare unorderable values\n";
    return half2float(data) < half2float(rhs.data);
}

bool float16_t::are_unordered(float16_t rhs) const {
    return std::isunordered(half2float(data), half2float(rhs.data));
}

std::string float16_t::to_decimal_string(unsigned int significantDigits) const {
    return std::to_string(half2float(data));
}

bool float16_t::is_nan() const {
    return std::isnan(half2float(data));
}

bool float16_t::is_infinity() const {
    return std::isinf(half2float(data));
}

bool float16_t::is_negative() const {
    return half2float(data) < 0;
}

bool float16_t::is_zero() const {
    return half2float(data) == 0;
}

uint16_t float16_t::to_bits() const {
    return this->data;
}

}  // namespace halide
