/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#define BOOST_TEST_MODULE test_parameter_sampler

#include <toolkits/object_detection/one_shot_object_detection/parameter_sampler.hpp>

#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <experimental/random>

static constexpr int max_dimension = 6000;
static constexpr int num_rows = 1000;

namespace turi {
namespace one_shot_object_detection {
namespace {

ParameterSampler create_parameter_sampler(int seed) {

  std::uniform_int_distribution<int> dist(0, max_dimension);
  std::mt19337 engine(seed);

  size_t starter_width = dist(engine);
  size_t starter_height = dist(engine);
  size_t dx = 0;
  size_t dy = 0;
  ParameterSampler sampler(starter_width, starter_height, dx, dy);
  return sampler;
}

std::unordered_map<std::string, std::vector<double> > get_all_angles(
    ParameterSampler &sampler, int seed) {
  std::unordered_map<std::string, std::vector<double> > all_angles;
  std::vector<double> thetas;
  std::vector<double> phis;
  std::vector<double> gammas;
  for (int i=0; i<num_rows; i++) {
    sampler.sample(seed+i);
    double theta = sampler.get_theta();
    double phi = sampler.get_phi();
    double gamma = sampler.get_gamma();
    thetas.push_back(theta);
    phis.push_back(phi);
    gammas.push_back(gamma);
  }
  all_angles["theta"] = thetas;
  all_angles["phi"] = phis;
  all_angles["gamma"] = gammas;
  return all_angles;
}

bool angles_match_distribution(std::vector<double> angles,
    std::vector<double> angle_means, double angle_stdev) {
  // for every mean, assert that the count of angles in [mean-stdev, mean+stdev] is
  // greater than the count of angles in [mean-2*stdev, mean-stdev] U [mean+stdev, mean+2*stdev]
  for (double mean : angle_means) {
    range_count()
  }
}

BOOST_AUTO_TEST_CASE(test_parameter_distributions) {
  int seed = std::experimental::randint(0, 1000);
  ParameterSampler sampler = create_parameter_sampler(seed);
  std::unordered_map<std::string, std::vector<double> > all_angles = get_all_angles(
    sampler, seed);
  std::vector<double> thetas = all_angles["theta"];
  std::vector<double> phis = all_angles["phi"];
  std::vector<double> gammas = all_angles["gamma"];
  TS_ASSERT(angles_match_distribution(thetas, sampler.get_theta_means(), sampler.get_theta_stdev()));
  TS_ASSERT(angles_match_distribution(phis, sampler.get_phi_means(), sampler.get_phi_stdev()));
  TS_ASSERT(angles_match_distribution(gammas, sampler.get_gamma_means(), sampler.get_gamma_stdev()));
}


}  // namespace
}  // namespace one_shot_object_detection
}  // namespace turi

