/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_STANDARDIZATION_H_
#define TURI_STANDARDIZATION_H_

#include <string>
#include <flexible_type/flexible_type.hpp>

// Eigen
#include <numerics/armadillo.hpp>
#include <numerics/armadillo.hpp>

// Optimizaiton
#include <optimization/optimization_interface.hpp>

// ML-Data
#include <ml_data/ml_data.hpp>
#include <ml_data/metadata.hpp>
#include <serialization/serialization_includes.hpp>


// TODO: List of todo's for this file
//------------------------------------------------------------------------------
//

namespace turi {
namespace supervised {

/**
 *
 * Interface for affine transformation of data for machine learning and
 * optimization purposes.
 *
 *
 * Background: Feature Scaling
 * --------------------------------------------------------------------------
 *
 *  Feature scaling performs standardization of data for supervised learning
 *  methods. Since the range of values of raw data varies widely, in some
 *  machine learning algorithms, objective functions will not work properly
 *  without normalization.  Therefore, the range of all features should be
 *  normalized so that each feature contributes approximately equally.
 *
 * What we need for a standardization scheme.
 * ---------------------------------------------------------------
 *
 * The standardization interface makes sure that you can implement various
 * types of data standardization methods without effecting much of the code
 * base.
 *
 *
 * Each standardization scheme requires the following methods:
 *
 * *) Construction based on metadata: Given a complete metadata object,
 *    we can construct the standardization object.
 *
 * *) Transform: Perform a transformation from the original space to the
 *    standardized space.
 *
 * *) Inverse-Transform: Perform a transformation from the standardized space
 * to the original space.
 *
 * Comparison of various methods for standardization
 * ---------------------------------------------------------------
 *
 * 1) Norm-Rescaling: Given a column of data x, the norm re-scaling changes
 * the column to:
 *                  x' = x / ||x||
 *
 * where ||x|| can be the L1, L2, or L-Inf norm.
 *
 * PROS: Sparsity preserving.
 * CONS: May not be the right thing to do for regularized problems.
 *
 * 2) Mean-Stdev: Given a column of data x, the norm re-scaling changes
 * the column to:
 *                  x' = (x - mean) / stdev
 *
 * PROS: Statistically well .
 * CONS: Sparsity breaking
 *
 * 3) Min-Max: Given a column of data x, the norm re-scaling changes
 * the column to:
 *                  x' = (x - min(x)) / (max(x) - min(x))
 *
 * PROS: Well documented for SVM.
 * CONS: Sparsity breaking
 *
 * \note The important part is for us to get something that helps with
 * numerical issues and is sparsity preserving. The interface here allows
 * us to try many things and see what works best.
 *
*/
class standardization_interface {

  protected:

    size_t total_size;                        /**< # Total size */

  public:

  /**
   * Default destructor.
   */
  virtual ~standardization_interface() = default;

  /**
  * Default constructor.
  */
  standardization_interface() = default;

  // Dense Vectors
  // --------------------------------------------------------------------------

  /**
   * Transform a point from the original space to the standardized space.
   *
   * \param[in,out] point(DenseVector) Point to be transformed.
   *
   */
  virtual void transform(DenseVector &point) const = 0;


  /**
   * Inverse transform a point from the standardized space to the original space.
   *
   * \param[in,out] point(DenseVector) Point to be transformed.
   *
   */
  virtual void inverse_transform(DenseVector &point) const = 0;


  // Sparse Vectors
  // --------------------------------------------------------------------------

  /**
   * Inverse transform a point from the standardized space to the original space.
   *
   * \param[in,out] point(SparseVector) Point to be transformed.
   *
   */
  virtual void inverse_transform(SparseVector &point) const = 0;

  /**
   * Transform a point from the original space to the standardized space.
   *
   * \param[in,out] point(SparseVector) Point to be transformed.
   *
   */
  virtual void transform(SparseVector &point) const = 0;


  /**
   * Serialization -- Save object
   *
   * Save this class to a Turi oarc object.
   * \param[in] oarc Turi oarc object
   */
  virtual void save(turi::oarchive& oarc) const = 0;

  /**
   * Serialization -- Load object
   *
   * Load this class from a Turi iarc object.
   * \param[in] iarc Turi iarc object
   */
  virtual void load(turi::iarchive& iarc) = 0;


  /**
   * Return the total size of all the variables in the space.
   *
   * \param[out] total_size Size of all the variables in the space.
   *
   * \note This is the sum of the sizes of the individual features that created
   *       this object. They are
   *
   * Numeric           : 1
   * Categorical       : # Unique categories
   * Vector            : Size of the vector.
   * CategoricalVector : # Unique categories.
   * Dictionary        : # Keys
   *
   * For reference encoding, subtract 1 from the Categorical and
   * Categorical-Vector types.
   *
   * \return Column size.
   *
   */
  size_t get_total_size() const {
    return total_size;
  }


};


/**
 * Rescale columns by L2-norm
 *   x >= 0
 */
class l2_rescaling: public standardization_interface {

  protected:

    DenseVector scale;                               /**< Scale */
    bool use_reference;                 /**< Reference encoding */

  public:


  /**
   * Default destructor.
   */
  virtual ~l2_rescaling() {}

  /**
   * Default constructor.
   *
   * \param[in] metadata          Metadata object for the features.
   * \param[in] index_size        Sizes of each of the features.
   * \param[in] use_reference     Reference encoding of categorical?
   *
   * \note The index_size refers to the size of each of the features. The
   * sizes of each type of features are:
   *
   * Numeric            : 1
   * String             : # categories
   * List               : Size
   * Categorical Vector : Total number of categories
   * Dictionary         : # keys
   *
   * \note Although the metadata keeps a copy of these sizes, they may not be
   * consistent with what was seen during training (because of new categories).
   * Hence, you would need both the metadata for the column stats collected
   * during training and the index_size for feature sizes captured at the
   * end of training.
   *
   */
  l2_rescaling(
      const std::shared_ptr<ml_metadata> & ml_mdata,
      bool _use_reference = true) {

    // Make sure the size is set
    use_reference = _use_reference;
    total_size = 1;
    for(size_t i = 0; i < ml_mdata->num_columns(); i++){
    if (ml_mdata->is_categorical(i)) {
        total_size += ml_mdata->index_size(i) - use_reference;
      } else {
        total_size += ml_mdata->index_size(i);
      }
    }

    // Init the scale
    scale.resize(total_size);
    scale.zeros();
    size_t idx = 0;

    for(size_t i = 0; i < ml_mdata->num_columns(); i++) {

      const auto& stats = ml_mdata->statistics(i);
      bool skip_first = (use_reference && ml_mdata->is_categorical(i));

      // For each column in the metadata
      // \note: Computing the L2 norm (averaged over example)
      // Here, we compute the scale using the variance and means are follows:
      //
      //  scale = sqrt(E[X^2]) = sqrt(Var(x) + E[X]^2)
      //
      // The stdev is the L2 norm of the data shifted by the mean. This undoes
      // this shift. There could be an multiplication by an "N" to get the
      // L2 norm but that multiple doesn't quite help.

      for (size_t k = skip_first ? 1 : 0; k < ml_mdata->index_size(i); ++k) {
        double r = std::pow(stats->mean(k), 2) + std::pow(stats->stdev(k), 2);
        scale(idx) = std::sqrt(std::max(r, optimization::OPTIMIZATION_ZERO));
        ++idx;
      }
    }
    scale(total_size-1) = 1;

  } // End of constructor

  // Dense Vectors
  // --------------------------------------------------------------------------

  /**
   * Transform a point from the original space to the standardized space.
   *
   * \param[in,out] point(DenseVector) Point to be transformed.
   *
   */
  void transform(DenseVector &point) const {
    DASSERT_EQ(point.size(), total_size);
    for(size_t i = 0; i < point.n_rows; ++i) {
      point(i) /= scale(i);
    }
  }

  /**
   * Transform a row of points from the original space to the standardized space.
   *
   * \param[in,out] point(DenseVector) Point to be transformed.
   *
   */
  void transform(DenseMatrix &points) const {
    DASSERT_EQ(points.n_cols, total_size);

    points *= arma::diagmat(scale).i();
  }

  /**
   * Inverse transform a point from the standardized space to the original space.
   *
   * \param[in,out] point(DenseVector) Point to be transformed.
   *
   */
  void inverse_transform(DenseVector &point) const {
    DASSERT_EQ(point.size(), total_size);
    for(size_t i = 0; i < point.n_rows; ++i) {
      point(i) *= scale(i);
    }
  }

  // Sparse Vectors
  // --------------------------------------------------------------------------

  /**
   * Inverse transform a point from the standardized space to the original space.
   *
   * \param[in,out] point(SparseVector) Point to be transformed.
   *
   */
  void inverse_transform(SparseVector &point) const {
    for(auto& p : point) {
      p.second *= scale(p.first);
    }
  }

  /**
   * Transform a point from the original space to the standardized space.
   *
   * \param[in,out] point(SparseVector) Point to be transformed.
   *
   */
  void transform(SparseVector &point) const {
    DASSERT_EQ(point.size(), total_size);
    for(auto& p : point) {
      p.second /= scale(p.first);
    }
  }

  /**
   * Serialization -- Save object
   *
   * Save this class to a Turi oarc object.
   * \param[in] oarc Turi oarc object
   */
  void save(turi::oarchive& oarc) const{
    oarc << total_size
         << scale
         << use_reference;
  }

  /**
   * Serialization -- Load object
   *
   * Load this class from a Turi iarc object.
   * \param[in] iarc Turi iarc object
   */
  void load(turi::iarchive& iarc){
    iarc >> total_size
         >> scale
         >> use_reference;
  }


};


} // supervised 
} // turicreate
#endif
