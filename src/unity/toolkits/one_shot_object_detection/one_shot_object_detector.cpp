#include <unity/toolkits/one_shot_object_detection/one_shot_object_detector.hpp>

#include <unity/toolkits/object_detection/object_detector.hpp>

// @TODO: Clean up imports.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <limits>
#include <numeric>
#include <queue>
#include <utility>
#include <vector>
#include <random>

#include <logger/assertions.hpp>
#include <logger/logger.hpp>
#include <random/random.hpp>

#include <boost/gil/gil_all.hpp>
#include <boost/gil/extension/io/jpeg.hpp>
#include <boost/gil/extension/numeric/sampler.hpp>
#include <boost/gil/extension/numeric/resample.hpp>
#include <boost/gil/utilities.hpp>

#include <image/numeric_extension/perspective_projection.hpp>

#include <Eigen/Core>
#include <Eigen/Dense>

using turi::coreml::MLModelWrapper;
using namespace Eigen;
using namespace boost::gil;

template <typename T>
class matrix3x3 {
public:
    matrix3x3() : a(1), b(0), c(0), d(0), e(1), f(0), g(0), h(0), i(1) {}
    matrix3x3(T A, T B, T C, T D, T E, T F, T G, T H, T I) : a(A),b(B),c(C),d(D),e(E),f(F),g(G),h(H),i(I) {}
    matrix3x3(const matrix3x3& mat) : a(mat.a), b(mat.b), c(mat.c), d(mat.d), e(mat.e), f(mat.f), g(mat.g), h(mat.h), i(mat.i) {}
    matrix3x3(const Matrix3f M) : a(M(0,0)), b(M(1,0)), c(M(2,0)), d(M(0,1)), e(M(1,1)), f(M(2,1)), g(M(0,2)), h(M(1,2)), i(M(2,2)) {}
    matrix3x3& operator=(const matrix3x3& m)           { a=m.a; b=m.b; c=m.c; d=m.d; e=m.e; f=m.f; g=m.g; h=m.h; i=m.i; return *this; }

    matrix3x3& operator*=(const matrix3x3& m)          { (*this) = (*this)*m; return *this; }

    T a,b,c,d,e,f,g,h,i;
};

template <typename T> 
matrix3x3<T> operator*(const matrix3x3<T>& m1, const matrix3x3<T>& m2) {
    return matrix3x3<T>(
                m1.a * m2.a + m1.b * m2.d + m1.c * m2.g,
                m1.a * m2.b + m1.b * m2.e + m1.c * m2.h,
                m1.a * m2.c + m1.b * m2.f + m1.c * m2.i,
                m1.d * m2.a + m1.e * m2.d + m1.f * m2.g,
                m1.d * m2.b + m1.e * m2.e + m1.f * m2.h,
                m1.d * m2.c + m1.e * m2.f + m1.f * m2.i,
                m1.g * m2.a + m1.h * m2.d + m1.i * m2.g,
                m1.g * m2.b + m1.h * m2.e + m1.i * m2.h,
                m1.g * m2.c + m1.h * m2.f + m1.i * m2.i);
}

template <typename T, typename F> 
point2<F> operator*(const point2<T>& p, const matrix3x3<F>& m) {
  if (m.g + m.h + m.i == 0) {
    printf("Oops..... all zeroes!\n");
    return point2<F>(0,0);
  }
  // printf("############\n");
  // printf("(%f,%f) -> (%f,%f)\n", (float)p.x, (float)p.y, (float)(m.a*p.x + m.d*p.y + m.g)/(m.c*p.x + m.f*p.y + m.i), (float)(m.b*p.x + m.e*p.y + m.h)/(m.c*p.x + m.f*p.y + m.i));
  // printf("3. %f\n", m.c*p.x + m.f*p.y + m.i);
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

namespace turi {
namespace one_shot_object_detection {

class ParameterSweep {
public:
  ParameterSweep(int width, int height) {
    width_ = width;
    height_ = height;
  }

  double deg_to_rad(double angle) {
    return angle * M_PI / 180.0;
  }

  double get_theta() {
    return deg_to_rad(theta_);
  }

  double get_phi() {
    return deg_to_rad(phi_);
  }

  double get_gamma() {
    return deg_to_rad(gamma_);
  }

  int get_dz() {
    return dz_;
  }

  double get_focal() {
    return focal_;
  }

  void sample(long seed) {
    double theta_mean, phi_mean, gamma_mean;
    std::srand(seed);
    theta_mean = theta_means_[std::rand() % theta_means_.size()];
    std::srand(seed+1);
    phi_mean = phi_means_[std::rand() % phi_means_.size()];
    std::srand(seed+2);
    gamma_mean = gamma_means_[std::rand() % gamma_means_.size()];
    // C++17:
    // std::sample(theta_means_.begin(), theta_means_.end(), means.end(), 1, std::mt19937{std::random_device{}()});
    // std::sample(phi_means_.begin(),   phi_means_.end(),   &phi_mean,   1, std::mt19937{std::random_device{}()});
    // std::sample(gamma_means_.begin(), gamma_means_.end(), &gamma_mean, 1, std::mt19937{std::random_device{}()});
    std::normal_distribution<double> theta_distribution(theta_mean, angle_stdev_);
    std::normal_distribution<double> phi_distribution(phi_mean, angle_stdev_);
    std::normal_distribution<double> gamma_distribution(gamma_mean, angle_stdev_);
    std::normal_distribution<double> focal_distribution((double)width_, focal_stdev_);
    theta_generator_.seed(seed+3);
    theta_ = theta_distribution(theta_generator_);
    printf("theta = %f\n", theta_);
    phi_generator_.seed(seed+4);
    phi_ = phi_distribution(phi_generator_);
    printf("phi = %f\n", phi_);
    gamma_generator_.seed(seed+5);
    gamma_ = gamma_distribution(gamma_generator_);
    printf("gamma = %f\n", gamma_);
    focal_generator_.seed(seed+6);
    focal_ = focal_distribution(focal_generator_);
    printf("focal = %f\n", focal_);
    std::uniform_int_distribution<int> dz_distribution(focal_/8, focal_);
    dz_generator_.seed(seed+7);
    dz_ = focal_; // dz_distribution(dz_generator_);
    printf("dz = %d\n", dz_);
  }

private:
  int width_;
  int height_;
  // int max_depth_ = 13000;
  double angle_stdev_ = 20.0;
  double focal_stdev_ = 40.0;
  std::vector<double> theta_means_ = {-180.0, 0.0, 180.0};
  std::vector<double> phi_means_   = {-180.0, 0.0, 180.0};
  std::vector<double> gamma_means_ = {-180.0, -90.0, 0.0, 90.0, 180.0};
  std::default_random_engine theta_generator_;
  std::default_random_engine phi_generator_;
  std::default_random_engine gamma_generator_;
  std::default_random_engine dz_generator_;
  std::default_random_engine focal_generator_;
  double theta_;
  double phi_;
  double gamma_;
  int dz_;
  double focal_;

};

gl_sframe _augment_data(gl_sframe data, gl_sframe backgrounds, long seed) {
  ParameterSweep parameter_sampler = ParameterSweep(2*1024, 2*676);
  int n = 1;
  for (int i = 0; i < n; i++) {
    parameter_sampler.sample(seed+i);

    //                ^
    //                |
    //                |
    //    Image       y
    //                |
    //                |
    // <------ x -----     

    int original_top_left_x = 2*1024 - 1024/2; // - 1024;
    int original_top_left_y = 676/2; // - 676;
    int original_top_right_x = 1024 - 1024/2; // - 1024;
    int original_top_right_y = 676/2; // - 676;
    int original_bottom_left_x = 2*1024 - 1024/2; // - 1024;
    int original_bottom_left_y = 2*676 - 676/2; // - 676;
    int original_bottom_right_x = 1024 - 1024/2; // - 1024;
    int original_bottom_right_y = 2*676 - 676/2; // - 676;

    Matrix3f mat = get_transformation_matrix(2*1024, 2*676,
      parameter_sampler.get_theta(),
      parameter_sampler.get_phi(),
      parameter_sampler.get_gamma(),
      -1024/2,
      -676/2,
      parameter_sampler.get_dz(),
      parameter_sampler.get_focal());
    Matrix3f mat2 = get_transformation_matrix(2*1024, 2*676,
      parameter_sampler.get_theta(),
      parameter_sampler.get_phi(),
      parameter_sampler.get_gamma(),
      0,
      0,
      parameter_sampler.get_dz(),
      parameter_sampler.get_focal());
    Vector3f top_left_corner(3);
    top_left_corner     << original_top_left_x, original_top_left_y, 1;
    Vector3f top_right_corner(3);
    top_right_corner    << original_top_right_x, original_top_right_y, 1;
    Vector3f bottom_left_corner(3);
    bottom_left_corner  << original_bottom_left_x, original_bottom_left_y, 1;
    Vector3f bottom_right_corner(3);
    bottom_right_corner << original_bottom_right_x, original_bottom_right_y, 1;
    // rgb8_image_t mask(rgb8_image_t::point_t(view(starter_image).dimensions()*2));
    // fill_pixels(view(mask),rgb8_pixel_t(255, 255, 255));

    Vector3f new_top_left_corner = mat2 * top_left_corner;// * mat;
    Vector3f new_top_right_corner = mat2 * top_right_corner;// * mat;
    Vector3f new_bottom_left_corner = mat2 * bottom_left_corner;// * mat;
    Vector3f new_bottom_right_corner = mat2 * bottom_right_corner;// * mat;
    new_top_left_corner[0] /= new_top_left_corner[2];
    new_top_left_corner[1] /= new_top_left_corner[2];
    // new_top_left_corner[0] += 1024/2;
    // new_top_left_corner[1] += 676/2;
    new_top_right_corner[0] /= new_top_right_corner[2];
    new_top_right_corner[1] /= new_top_right_corner[2];
    // new_top_right_corner[0] += 1024/2;
    // new_top_right_corner[1] += 676/2;
    new_bottom_left_corner[0] /= new_bottom_left_corner[2];
    new_bottom_left_corner[1] /= new_bottom_left_corner[2];
    // new_bottom_left_corner[0] += 1024/2;
    // new_bottom_left_corner[1] += 676/2;
    new_bottom_right_corner[0] /= new_bottom_right_corner[2];
    new_bottom_right_corner[1] /= new_bottom_right_corner[2];
    // new_bottom_right_corner[0] += 1024/2;
    // new_bottom_right_corner[1] += 676/2;
    std::cout << "new_top_left_corner" << std::endl;
    std::cout << new_top_left_corner << std::endl;
    std::cout << "new_top_right_corner" << std::endl;
    std::cout << new_top_right_corner << std::endl;
    std::cout << "new_bottom_left_corner" << std::endl;
    std::cout << new_bottom_left_corner << std::endl;
    std::cout << "new_bottom_right_corner" << std::endl;
    std::cout << new_bottom_right_corner << std::endl;

    rgb8_image_t starter_image, background;
    read_image("in-affine.jpg", starter_image, jpeg_tag());
    read_image("background.jpg", background, jpeg_tag());

    matrix3x3<double> M(mat);
    rgb8_image_t mask(rgb8_image_t::point_t(view(background).dimensions()));
    fill_pixels(view(mask),rgb8_pixel_t(0, 0, 0));
    rgb8_image_t transformed(rgb8_image_t::point_t(view(starter_image).dimensions()*2));
    fill_pixels(view(transformed),rgb8_pixel_t(255, 255, 255));
    resample_pixels(const_view(starter_image), view(transformed), M, bilinear_sampler());

    rgb8_image_t transformed_with_background_dimensions(rgb8_image_t::point_t(view(background).dimensions()));
    fill_pixels(view(transformed_with_background_dimensions),rgb8_pixel_t(255, 255, 255));

    resample_pixels(const_view(starter_image), view(transformed_with_background_dimensions), M, bilinear_sampler());
    std::string output_filename = "out-affine-" + std::to_string(i) + ".jpg";
    write_view(output_filename, view(transformed), jpeg_tag());
    // std::cout << "Eigen:" << std::endl;
    // std::cout << mat << std::endl;
    // std::cout << "Boost:" << std::endl;
    // printf("%f %f %f\n", M.a, M.b, M.c);
    // printf("%f %f %f\n", M.d, M.e, M.f);
    // printf("%f %f %f\n", M.g, M.h, M.i);
    // std::cout << M << std::endl;
  
  }
  return data;
}

one_shot_object_detector::one_shot_object_detector() {
  model_.reset(new turi::object_detection::object_detector());
}

// size_t one_shot_object_detector::get_version() const {
//   return ONE_SHOT_OBJECT_DETECTOR_VERSION;
// }

// void one_shot_object_detector::save_impl(oarchive& oarc) const {
//   model_->save_impl(oarc);
// }

// void one_shot_object_detector::load_version(iarchive& iarc, size_t version) {
//   model_->load_version(iarc, version);
// }

void one_shot_object_detector::train(gl_sframe data,
                                     std::string target_column_name,
                                     gl_sframe backgrounds,
                                     long seed,
                                     std::map<std::string, flexible_type> options){
  gl_sframe augmented_data = _augment_data(data, backgrounds, seed);
  // model_->train(augmented_data, "annotation", target_column_name, options);
}

variant_map_type one_shot_object_detector::evaluate(gl_sframe data, 
  std::string metric, std::map<std::string, flexible_type> options) {
  return model_->evaluate(data, metric, options);
}

std::shared_ptr<MLModelWrapper> one_shot_object_detector::export_to_coreml(
  std::string filename, std::map<std::string, flexible_type> options) {
  return model_->export_to_coreml(filename, options);
}

} // one_shot_object_detection
} // turi