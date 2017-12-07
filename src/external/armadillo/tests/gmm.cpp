#include <vector>
#include <numerics/armadillo.hpp>
#include "catch.hpp"

using namespace std;
using namespace arma;

/**
 * Make sure that gmm_full can fit manually constructed Gaussians.
 */
TEST_CASE("gmm_full_1")
  {
  // Higher dimensionality gives us a greater chance of having separated Gaussians.
  const uword dims      = 8;
  const uword gaussians = 3;
  const uword maxTrials = 3;

  // Generate dataset.
  mat data(dims, 500, fill::zeros);

  vector<vec>  means(gaussians);
  vector<mat> covars(gaussians);

  vec weights(gaussians);
  uvec counts(gaussians);

  bool success = true;

  for(uword trial = 0; trial < maxTrials; ++trial)
    {
    // Preset weights.
    weights[0] = 0.25;
    weights[1] = 0.325;
    weights[2] = 0.425;

    for(size_t i = 0; i < gaussians; i++)
      {
      counts[i] = data.n_cols * weights[i];
      }

    // Account for rounding errors (possibly necessary).
    counts[gaussians - 1] += (data.n_cols - accu(counts));

    // Build each Gaussian individually.
    size_t point = 0;
    for(size_t i = 0; i < gaussians; i++)
      {
      mat gaussian;
      gaussian.randn(dims, counts[i]);

      // Randomly generate mean and covariance.
      means[i].randu(dims);
      means[i] -= 0.5;
      means[i] *= (5 * i);

      // We need to make sure the covariance is positive definite.
      // We will take a random matrix C and then set our covariance to C * C',
      // which will be positive semidefinite.
      covars[i].randu(dims, dims);
      covars[i] += 0.5 * eye<mat>(dims, dims);
      covars[i] *= trans(covars[i]);

      data.cols(point, point + counts[i] - 1) = (covars[i] * gaussian + means[i] * ones<rowvec>(counts[i]));

      // Calculate the actual means and covariances
      // because they will probably be different.
      means[i] = mean(data.cols(point, point + counts[i] - 1), 1);
      covars[i] = cov(data.cols(point, point + counts[i] - 1).t(), 1 /* biased */);

      point += counts[i];
      }

    // Calculate actual weights.
    for(size_t i = 0; i < gaussians; i++)
      {
      weights[i] = (double) counts[i] / data.n_cols;
      }

    gmm_full gmm;
    gmm.learn(data, gaussians, eucl_dist, random_subset, 10, 500, 1e-10, false);

    uvec sortRef = sort_index(weights);
    uvec sortTry = sort_index(gmm.hefts);

    // Check the model to see that it is correct.
    success = ( gmm.hefts.n_elem == gaussians );
    for(size_t i = 0; i < gaussians; i++)
      {
      // Check weight.
      success &= ( weights[sortRef[i]] == Approx(gmm.hefts[sortTry[i]]).epsilon(0.1) );

      for(uword j = 0; j < gmm.means.n_rows; ++j)
        {
        success &= ( means[sortRef[i]][j] == Approx(gmm.means(j, sortTry[i])).epsilon(0.1) );
        }

      for(uword j = 0; j < gmm.fcovs.n_rows * gmm.fcovs.n_cols; ++j)
        {
        success &= ( covars[sortRef[i]][j] == Approx(gmm.fcovs.slice(sortTry[i])[j]).epsilon(0.1) );
        }

      if(success == false)  { continue; }
      }

    if(success)  { break; }
    }

  REQUIRE( success == true );
  }



TEST_CASE("gmm_diag_1")
  {
  // Higher dimensionality gives us a greater chance of having separated Gaussians.
  const uword dims      = 4;
  const uword gaussians = 3;
  const uword maxTrials = 8; // Needs more trials...

  // Generate dataset.
  mat data(dims, 500, fill::zeros);

  vector<vec>  means(gaussians);
  vector<mat> covars(gaussians);

  vec weights(gaussians);
  uvec counts(gaussians);

  bool success = true;
  for(uword trial = 0; trial < maxTrials; ++trial)
    {
    // Preset weights.
    weights[0] = 0.25;
    weights[1] = 0.325;
    weights[2] = 0.425;

    for(size_t i = 0; i < gaussians; i++)
      {
      counts[i] = data.n_cols * weights[i];
      }

    // Account for rounding errors (possibly necessary).
    counts[gaussians - 1] += (data.n_cols - accu(counts));

    // Build each Gaussian individually.
    size_t point = 0;
    for(size_t i = 0; i < gaussians; i++)
      {
      mat gaussian;
      gaussian.randn(dims, counts[i]);

      // Randomly generate mean and covariance.
      means[i].randu(dims);
      means[i] -= 0.5;
      means[i] *= (3 * (i + 1));

      // Use a diagonal covariance matrix.
      covars[i].zeros(dims, dims);
      covars[i].diag() = 0.5 * randu<vec>(dims) + 0.5;

      data.cols(point, point + counts[i] - 1) = (covars[i] * gaussian + means[i] * ones<rowvec>(counts[i]));

      // Calculate the actual means and covariances
      // because they will probably be different.
      means[i] = mean(data.cols(point, point + counts[i] - 1), 1);
      covars[i] = cov(data.cols(point, point + counts[i] - 1).t(), 1 /* biased */);

      point += counts[i];
      }

    // Calculate actual weights.
    for(size_t i = 0; i < gaussians; i++)
      {
      weights[i] = (double) counts[i] / data.n_cols;
      }

    gmm_diag gmm;
    gmm.learn(data, gaussians, eucl_dist, random_subset, 50, 500, 1e-10, false);

    uvec sortRef = sort_index(weights);
    uvec sortTry = sort_index(gmm.hefts);

    // Check the model to see that it is correct.
    success = ( gmm.hefts.n_elem == gaussians );
    for(size_t i = 0; i < gaussians; i++)
      {
      // Check weight.
      success &= ( weights[sortRef[i]] == Approx(gmm.hefts[sortTry[i]]).epsilon(0.1) );

      for(uword j = 0; j < gmm.means.n_rows; ++j)
        {
        success &= ( means[sortRef[i]][j] == Approx(gmm.means(j, sortTry[i])).epsilon(0.1) );
        }

      for(uword j = 0; j < gmm.dcovs.n_rows; ++j)
        {
        success &= ( covars[sortRef[i]](j, j) == Approx(gmm.dcovs.col(sortTry[i])[j]).epsilon(0.1) );
        }

      if(success == false)  { continue; }
      }

    if(success)  { break; }
    }

  REQUIRE( success == true );
  }
