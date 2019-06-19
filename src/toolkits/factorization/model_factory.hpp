/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_FACTORIZATION_MODEL_CREATION_FACTORY_H_
#define TURI_FACTORIZATION_MODEL_CREATION_FACTORY_H_

#include <toolkits/factorization/factorization_model_sgd_interface.hpp>
#include <toolkits/factorization/factorization_model_impl.hpp>
#include <toolkits/sgd/basic_sgd_solver.hpp>
#include <toolkits/sgd/sgd_interface.hpp>
#include <toolkits/factorization/ranking_sgd_solver_explicit.hpp>
#include <toolkits/factorization/ranking_sgd_solver_implicit.hpp>

#include <string>
#include <cstdlib>

////////////////////////////////////////////////////////////////////////////////
// All of the macros we use to create an instance of the solvers,
// instantiate the templates, and suppress the instantiations.  We
// have a set of instantiation files in factory_instantiations/*,
// which separates the different parts of the tree created by these
// macros into different compilation units.


#define _BAD(param)                                                     \
  ASSERT_MSG(false, (std::string(#param) + " \'" + param + "\' not recognized.").c_str()); \
  ASSERT_UNREACHABLE(); \

////////////////////////////////////////////////////////////////////////////////
// Step 1: Select the solver.

#define CREATE_RETURN_SOLVER(...)                                       \
  do{                                                                   \
    if(solver_class == "sgd::basic_sgd_solver")                         \
      CREATE_RETURN_LOSS_NORMAL(sgd::basic_sgd_solver, __VA_ARGS__);    \
    else if(solver_class == "factorization::explicit_ranking_sgd_solver") \
      CREATE_RETURN_LOSS_NORMAL(factorization::explicit_ranking_sgd_solver, __VA_ARGS__); \
    else                                                                \
      _BAD(solver_class);                                               \
  }while(false)

#define SUPPRESS_SOLVERS()                                       \
  _SUPPRESS_BY_LOSS(sgd::basic_sgd_solver);                       \
  _SUPPRESS_BY_LOSS(factorization::explicit_ranking_sgd_solver);


////////////////////////////////////////////////////////////////////////////////
// Step 2: Select the loss.  We have to handle two cases; the first
// one is the case with a normal loss, and the second with the
// implicit_ranking_sgd_solver.

#define CREATE_RETURN_LOSS_NORMAL(...)                                  \
  do{                                                                   \
    if(loss_type == "loss_squared_error")                               \
      CREATE_RETURN_REGULARIZER(loss_squared_error, __VA_ARGS__);       \
    else if(loss_type == "loss_logistic")                               \
      CREATE_RETURN_REGULARIZER(loss_logistic, __VA_ARGS__);            \
    else                                                                \
      _BAD(loss_type);                                                  \
  } while(false)

#define _SUPPRESS_BY_LOSS(...)                                  \
  _SUPPRESS_BY_REGULARIZER(loss_squared_error, __VA_ARGS__);    \
  _SUPPRESS_BY_REGULARIZER(loss_logistic, __VA_ARGS__);


////////////////////////////////////////////////////////////////////////////////
// Step 3: Select the regularizer.

#define CREATE_RETURN_REGULARIZER(...)                  \
  do {                                                  \
    if(regularization_type == "L2") {                   \
      CREATE_RETURN_FACTORS(L2, __VA_ARGS__);           \
    } else if(regularization_type == "ON_THE_FLY") {    \
      CREATE_RETURN_FACTORS(ON_THE_FLY, __VA_ARGS__);   \
    } else if(regularization_type == "NONE") {          \
      /* does tempering with L2; still need this. */    \
      CREATE_RETURN_FACTORS(L2, __VA_ARGS__);           \
    } else {                                            \
      _BAD(regularization_type);                        \
    }                                                   \
  } while(false)

#define _SUPPRESS_BY_REGULARIZER(...)                   \
  _SUPPRESS_BY_FACTORS(L2, __VA_ARGS__);                \
  _SUPPRESS_BY_FACTORS(ON_THE_FLY, __VA_ARGS__);

// We are now at the point where
#define _INSTANTIATE_LOSS_AND_SOLVER(...)               \
  _INSTANTIATE_BY_FACTORS(L2, __VA_ARGS__);             \
  _INSTANTIATE_BY_FACTORS(ON_THE_FLY, __VA_ARGS__);

////////////////////////////////////////////////////////////////////////////////
// Step 4: Select the factor mode and associated number of factors.

#define CREATE_RETURN_FACTORS(...)                                      \
  do {                                                                  \
    if(factor_mode == "pure_linear_model" || num_factors == 0) {        \
      _CREATE_AND_RETURN(pure_linear_model, 0, __VA_ARGS__);            \
    } else if(factor_mode == "matrix_factorization"                     \
              || ( ((factor_mode == "factorization_machine"             \
                    && (train_data.metadata()->num_columns() == 2))))) { \
      if(num_factors == 8)                                              \
        _CREATE_AND_RETURN(matrix_factorization, 8, __VA_ARGS__);       \
      else                                                              \
        _CREATE_AND_RETURN(matrix_factorization, -1, __VA_ARGS__);      \
    } else if(factor_mode == "factorization_machine") {                 \
      if(num_factors == 8)                                              \
        _CREATE_AND_RETURN(factorization_machine, 8, __VA_ARGS__);      \
      else                                                              \
        _CREATE_AND_RETURN(factorization_machine, -1, __VA_ARGS__);     \
    } else {                                                            \
      _BAD(factor_mode);                                                \
    }                                                                   \
  } while(false)

#define _SUPPRESS_BY_FACTORS(...)                               \
  _SUPPRESS_SOLVER(pure_linear_model, 0, __VA_ARGS__);          \
  _SUPPRESS_SOLVER(matrix_factorization, 8, __VA_ARGS__);       \
  _SUPPRESS_SOLVER(matrix_factorization, -1, __VA_ARGS__);      \
  _SUPPRESS_SOLVER(factorization_machine, 8, __VA_ARGS__);      \
  _SUPPRESS_SOLVER(factorization_machine, -1, __VA_ARGS__);     \

#define _INSTANTIATE_BY_FACTORS(...)                            \
  _INSTANTIATE_SOLVER(pure_linear_model, 0, __VA_ARGS__);       \
  _INSTANTIATE_SOLVER(matrix_factorization, 8, __VA_ARGS__);    \
  _INSTANTIATE_SOLVER(matrix_factorization, -1, __VA_ARGS__);   \
  _INSTANTIATE_SOLVER(factorization_machine, 8, __VA_ARGS__);   \
  _INSTANTIATE_SOLVER(factorization_machine, -1, __VA_ARGS__);  \


////////////////////////////////////////////////////////////////////////////////
// Step 5: Now we know everything we need to create, instantiate, or
// suppress the solvers

////////////////////////////////////////
// The macro to suppress instantiation of the macro

#define _SUPPRESS_SOLVER(                                               \
    factor_mode, num_factors_if_known, regularization_type,             \
    loss_type, solver_class)                                            \
                                                                        \
  namespace turi {                                                  \
  using solver_class;                                                   \
  using namespace factorization;                                        \
                                                                        \
  extern template class                                                 \
  solver_class<                                                         \
    factorization_sgd_interface<                                        \
    factorization_model_impl<model_factor_mode::factor_mode,            \
                             num_factors_if_known>,                     \
    loss_type,                                                          \
    model_regularization_type::regularization_type> >;                  \
  }

////////////////////////////////////////
// The macro to create full instantiation of the solver.

#define _INSTANTIATE_SOLVER(                                            \
    factor_mode, num_factors_if_known, regularization_type,             \
    loss_type, solver_class)                                            \
                                                                        \
  namespace turi {                                                  \
  using solver_class;                                                   \
  using namespace factorization;                                        \
                                                                        \
  template class solver_class<                                          \
    factorization_sgd_interface<                                        \
    factorization_model_impl<model_factor_mode::factor_mode,            \
                             num_factors_if_known>,                     \
    loss_type,                                                          \
    model_regularization_type::regularization_type>  >;                 \
  }

////////////////////////////////////////
// The main macro to actually create the model and the solver.

#define _CREATE_AND_RETURN(                                             \
    factor_mode, num_factors_if_known, regularization_type,             \
    loss_type, solver_class,                                            \
    train_data, options)                                                \
                                                                        \
  do {                                                                  \
                                                                        \
    /* Set up the correct model type. */                                \
    typedef factorization_model_impl                                    \
        <model_factor_mode::factor_mode,                                \
         num_factors_if_known> model_type;                              \
                                                                        \
    std::shared_ptr<model_type> model(new model_type);                  \
                                                                        \
    /* Set up the model with the correct loss. */                       \
    model->setup(loss_type::name(), train_data, options);               \
                                                                        \
    /* Set up the correct interface type. */                            \
    typedef factorization_sgd_interface                                 \
        <model_type, loss_type,                                         \
         model_regularization_type::regularization_type> interface_type; \
                                                                        \
    /* Set up the interface. */                                         \
    std::shared_ptr<sgd::sgd_interface_base> iface(                     \
        new interface_type(model));                                     \
                                                                        \
    /* Set up the solver. */                                            \
    std::shared_ptr<sgd::sgd_solver_base> solver(                       \
        new solver_class<interface_type>(                               \
            iface, train_data, options));                               \
                                                                        \
    return {model, solver};                                             \
  } while(false)



namespace turi { namespace factorization {

/** The main function to create a version of the model and the solver.
 *
 */
std::pair<std::shared_ptr<factorization_model>,
          std::shared_ptr<sgd::sgd_solver_base> >
create_model_and_solver(const v2::ml_data& train_data,
                        std::map<std::string, flexible_type> options,
                        const std::string& loss_type,
                        const std::string& solver_class,
                        const std::string& regularization_type,
                        const std::string& factor_mode,
                        flex_int num_factors);

}}

#endif /* TURI_FACTORIZATION_MODEL_CREATION_FACTORY_H_ */
