/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#define BOOST_TEST_MODULE test_parameter_sampler

#include <toolkits/object_detection/one_shot_object_detection/util/parameter_sampler.hpp>

#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>

#include <unordered_map>

static constexpr int kMaxDimension = 6000;
static constexpr int kNumRows = 1000;
static constexpr int kBaseDimensionSize = 2500;

namespace turi {
namespace one_shot_object_detection {
namespace {

ParameterSampler create_parameter_sampler(std::default_random_engine &engine) {
  std::uniform_int_distribution<int> dist(0, kMaxDimension);

  size_t starter_width = dist(engine);
  size_t starter_height = dist(engine);
  size_t dx = 0;
  size_t dy = 0;
  ParameterSampler sampler(starter_width, starter_height, dx, dy);
  return sampler;
}

std::unordered_map<std::string, std::vector<double> >
constuct_angle_samples_map(ParameterSampler &sampler, int seed) {
  std::unordered_map<std::string, std::vector<double> > angle_samples_map;
  std::vector<double> thetas;
  std::vector<double> phis;
  std::vector<double> gammas;
  for (int i = 0; i < kNumRows; i++) {
    size_t background_width = kBaseDimensionSize + i;
    size_t background_height = kBaseDimensionSize - i;
    sampler.sample(background_width, background_height, seed, i);
    double theta = sampler.get_theta();
    double phi = sampler.get_phi();
    double gamma = sampler.get_gamma();
    thetas.push_back(theta);
    phis.push_back(phi);
    gammas.push_back(gamma);
  }
  angle_samples_map["theta"] = thetas;
  angle_samples_map["phi"] = phis;
  angle_samples_map["gamma"] = gammas;
  return angle_samples_map;
}

int count_in_range(std::vector<double> samples, double lower, double upper) {
  int count = 0;
  for (double sample : samples) {
    if (sample >= lower && sample <= upper) {
      count += 1;
    }
  }
  return count;
}

bool angles_match_distribution(const std::vector<double> &angles,
                               const std::vector<double> &angle_means,
                               double angle_stdev) {
  // for every mean, assert that the count of angles in
  // [mean-stdev, mean+stdev] is greater than the count of angles in
  // [mean-2*stdev, mean-stdev] U [mean+stdev, mean+2*stdev]
  int first_stdev_count;
  int second_stdev_count;
  for (double mean : angle_means) {
    first_stdev_count =
        count_in_range(angles, mean - angle_stdev, mean + angle_stdev);
    second_stdev_count =
        (count_in_range(angles, mean - 2 * angle_stdev, mean - angle_stdev) +
         count_in_range(angles, mean + angle_stdev, mean + 2 * angle_stdev));
    TS_ASSERT_LESS_THAN_EQUALS(second_stdev_count, first_stdev_count);
  }
  return true;
}

BOOST_AUTO_TEST_CASE(test_parameter_distributions) {
  std::default_random_engine engine;
  int seed = 500;
  engine.seed(seed);
  ParameterSampler sampler = create_parameter_sampler(engine);
  std::unordered_map<std::string, std::vector<double> > all_angles =
      constuct_angle_samples_map(sampler, seed);
  std::vector<double> thetas = all_angles["theta"];
  std::vector<double> phis = all_angles["phi"];
  std::vector<double> gammas = all_angles["gamma"];
  TS_ASSERT(angles_match_distribution(thetas, sampler.get_theta_means(),
                                      sampler.get_theta_stdev()));
  TS_ASSERT(angles_match_distribution(phis, sampler.get_phi_means(),
                                      sampler.get_phi_stdev()));
  TS_ASSERT(angles_match_distribution(gammas, sampler.get_gamma_means(),
                                      sampler.get_gamma_stdev()));
}

}  // namespace
}  // namespace one_shot_object_detection
}  // namespace turi
