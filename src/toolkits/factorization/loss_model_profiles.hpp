/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_FACTORIZATION_LOSS_MODEL_PROFILES_H_
#define TURI_FACTORIZATION_LOSS_MODEL_PROFILES_H_

#include <core/logging/assertions.hpp>
#include <core/util/logit_math.hpp>
#include <core/util/code_optimization.hpp>
#include <cmath>
#include <memory>
#include <string>
#include <utility>

namespace turi { namespace factorization {


/** The base class for the generative models.  These models
 *  encapsulate the part of the problem surrounding the translation of
 *  the underlying linear model to the target/response variable.  Thus
 *  it encapsulates the (1) translation function from linear model to
 *  response and (2) the loss function used to fit the coefficients of
 *  the linear model to predict the response.
 *
 *  To make reporting easier, report_loss translates a cumulative loss
 *  value -- the sum of loss(...) over all data points -- to a
 *  standard error measure.  It's name is returned along with the
 *  reportable value.
 */
class loss_model_profile {
 public:
  virtual double loss(double fx, double y) const = 0;
  virtual double loss_grad(double fx, double y) const = 0;
  virtual double translate_fx_to_prediction(double f_x) const = 0;
  virtual bool prediction_is_translated() const = 0;
  virtual std::string reported_loss_name() const = 0;
  virtual double reported_loss_value(double cumulative_loss_value) const = 0;
};

////////////////////////////////////////////////////////////////////////////////

/** Implements squared error loss for the linear models.
 */
class loss_squared_error final : public loss_model_profile {
 public:

  static std::string name() { return "squared_error"; }

  double loss(double fx, double y) const GL_HOT_INLINE_FLATTEN {
    return sq(fx - y);
  }

  double loss_grad(double fx, double y) const GL_HOT_INLINE_FLATTEN {
    return 2*(fx - y);
  }

  double translate_fx_to_prediction(double f_x) const GL_HOT_INLINE_FLATTEN {
    return f_x;
  }

  bool prediction_is_translated() const { return false; }

  std::string reported_loss_name() const { return "RMSE"; }

  double reported_loss_value(double cumulative_loss_value) const {
    return std::sqrt(cumulative_loss_value);
  }
};

////////////////////////////////////////////////////////////////////////////////

void _logistic_loss_value_is_bad(double) __attribute__((noinline, cold));

/** Implements logistic loss for the linear models.
 */
class loss_logistic final : public loss_model_profile {
 public:

  static std::string name() { return "logistic"; }

  double loss(double fx, double y) const GL_HOT_INLINE_FLATTEN {
    if(y < 0 || y > 1.0)
      _logistic_loss_value_is_bad(y);

    return (1 - y) * fx + log1pen(fx);
  }

  double loss_grad(double fx, double y) const GL_HOT_INLINE_FLATTEN {
    return (1 - y) + log1pen_deriviative(fx);
  }

  double translate_fx_to_prediction(double fx) const GL_HOT_INLINE_FLATTEN {
    return sigmoid(fx);
  }

  bool prediction_is_translated() const { return true; }

  std::string reported_loss_name() const { return "Predictive Error"; }

  double reported_loss_value(double cumulative_loss_value) const {
    return cumulative_loss_value;
  }
};

////////////////////////////////////////////////////////////////////////////////

/** Implements ranking loss for the model
 */
class loss_ranking_hinge final : public loss_model_profile {
 public:

  static std::string name() { return "hinge_ranking"; }

  double loss(double fx_diff, double) const GL_HOT_INLINE_FLATTEN {
    return std::max(0.0, 1 - fx_diff);
  }

  double loss_grad(double fx_diff, double) const GL_HOT_INLINE_FLATTEN {
    return (fx_diff < 1) ? -1 : 0;
  }

  double translate_fx_to_prediction(double fx) const GL_HOT_INLINE_FLATTEN {
    return fx;
  }

  bool prediction_is_translated() const { return false; }

  std::string reported_loss_name() const { return "Hinge Loss"; }

  double reported_loss_value(double cumulative_loss_value) const {
    return cumulative_loss_value;
  }
};

/** Implements ranking loss for the model
 */
class loss_ranking_logit final : public loss_model_profile {
 public:

  static std::string name() { return "logit rank"; }

  double loss(double fx_diff, double) const GL_HOT_INLINE_FLATTEN {
    return log1pen(fx_diff);
  }

  double loss_grad(double fx_diff, double) const GL_HOT_INLINE_FLATTEN {
    return log1pen_deriviative(fx_diff);
  }

  double translate_fx_to_prediction(double fx) const GL_HOT_INLINE_FLATTEN {
    return sigmoid(fx);
  }

  bool prediction_is_translated() const { return true; }

  std::string reported_loss_name() const { return "Logistic Rank Loss"; }

  double reported_loss_value(double cumulative_loss_value) const {
    return cumulative_loss_value;
  }
};


/// A quick helper function to retrieve the correct profile by name.
std::shared_ptr<loss_model_profile> get_loss_model_profile(
    const std::string& name);

}}

#endif
