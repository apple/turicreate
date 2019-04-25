/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <Eigen/Core>
#include <Eigen/Dense>
#include <boost/gil/gil_all.hpp>
#include <boost/gil/extension/io/jpeg.hpp>
#include <boost/gil/extension/numeric/sampler.hpp>
#include <boost/gil/extension/numeric/resample.hpp>
#include <boost/gil/utilities.hpp>

using namespace Eigen;
using namespace boost::gil;

MatrixXf get_2D_to_3D(int width, int height) {
  MatrixXf A1(4,3);
  A1 << 1, 0,  -1*(float)width/2, 
        0, 1,  -1*(float)height/2, 
        0, 0,         1, 
        0, 0,         1;
  return A1;
}

MatrixXf get_rotation(float theta, float phi, float gamma) {
  Matrix4f Rx, Ry, Rz, R;
  Rx << 1,               0,                0, 0,
        0, std::cos(theta), -1*std::sin(theta), 0,
        0, std::sin(theta),  std::cos(theta), 0,
        0,               0,                0, 1;
  Ry << std::cos(phi), 0, -1*std::sin(phi), 0,
                    0, 1,              0, 0,
        std::sin(phi), 0,  std::cos(phi), 0,
                    0, 0,              0, 1;
  Rz << std::cos(gamma), -1*std::sin(gamma), 0, 0,
        std::sin(gamma),  std::cos(gamma), 0, 0,
                      0,                0, 1, 0,
                      0,                0, 0, 1;
  R = (Rx * Ry) * Rz;
  return R;
}

MatrixXf get_translation(int dx, int dy, int dz) {
  Matrix4f T;
  T << 1, 0, 0, dx,
       0, 1, 0, dy,
       0, 0, 1, dz,
       0, 0, 0, 1;
  return T;
}

MatrixXf get_3D_to_2D(float focal, int width, int height) {
  MatrixXf A2(3,4);
  A2 << focal,     0,  (float)width/2, 0,
            0, focal, (float)height/2, 0,
            0,     0,               1, 0;
  return A2;
}

Matrix3f get_transformation_matrix(
  int width, int height, 
  float theta, float phi, float gamma, 
  int dx, int dy, int dz, 
  float focal) {
  MatrixXf A1  = get_2D_to_3D(width, height);
  Matrix4f R   = get_rotation(theta, phi, gamma);
  Matrix4f T   = get_translation(dx, dy, dz);
  MatrixXf A2  = get_3D_to_2D(focal, width, height);
  return (A2 * (T * (R * A1)));
}

template <typename T>
class matrix3x3 {
/*
 * a d g
 * b e h
 * c f i
 */
public:
    matrix3x3() : a(1), b(0), c(0), d(0), e(1), f(0), g(0), h(0), i(1) {}
    matrix3x3(T A, T B, T C, T D, T E, T F, T G, T H, T I) : a(A),b(B),c(C),d(D),e(E),f(F),g(G),h(H),i(I) {}
    matrix3x3(const matrix3x3& mat) : a(mat.a), b(mat.b), c(mat.c), d(mat.d), e(mat.e), f(mat.f), g(mat.g), h(mat.h), i(mat.i) {}
    matrix3x3(const Matrix3f M) : a(M(0,0)), b(M(1,0)), c(M(2,0)), d(M(0,1)), e(M(1,1)), f(M(2,1)), g(M(0,2)), h(M(1,2)), i(M(2,2)) {}
    matrix3x3& operator=(const matrix3x3& m) { a=m.a; b=m.b; c=m.c; d=m.d; e=m.e; f=m.f; g=m.g; h=m.h; i=m.i; return *this; }

    matrix3x3& operator*=(const matrix3x3& m) { (*this) = (*this)*m; return *this; }

    T a,b,c,d,e,f,g,h,i;
};

template <typename T> 
matrix3x3<T> operator*(const matrix3x3<T>& m1, const matrix3x3<T>& m2) {
    return matrix3x3<T>(
                m1.a * m2.a + m1.d * m2.b + m1.g * m2.c,
                m1.b * m2.a + m1.e * m2.b + m1.h * m2.c,
                m1.c * m2.a + m1.f * m2.b + m1.i * m2.c,
                m1.a * m2.d + m1.d * m2.e + m1.g * m2.f,
                m1.b * m2.d + m1.e * m2.e + m1.h * m2.f,
                m1.c * m2.d + m1.f * m2.e + m1.i * m2.f,
                m1.a * m2.g + m1.d * m2.h + m1.g * m2.i,
                m1.b * m2.g + m1.e * m2.h + m1.h * m2.i,
                m1.c * m2.g + m1.f * m2.h + m1.i * m2.i);
}

template <typename T, typename F> 
point2<F> operator*(const point2<T>& p, const matrix3x3<F>& m) {
  if (m.c*p.x + m.f*p.y + m.i == 0) {
    // TODO: Figure out the right failure behavior for when denominator is 0.
    return point2<F>(0,0);
  }
  return point2<F>((m.a*p.x + m.d*p.y + m.g)/(m.c*p.x + m.f*p.y + m.i), 
                   (m.b*p.x + m.e*p.y + m.h)/(m.c*p.x + m.f*p.y + m.i));
}


namespace boost {
namespace gil {
template <typename T> struct mapping_traits;

template <typename F>
struct mapping_traits<matrix3x3<F> > {
    typedef point2<F> result_type;
};

template <typename F, typename F2> 
point2<F> transform(const matrix3x3<F>& mat, const point2<F2>& src) { return src * mat; }

} // gil
} // boost
