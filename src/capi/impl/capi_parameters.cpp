#include <capi/TuriCore.h>
#include <capi/impl/capi_error_handling.hpp>
#include <capi/impl/capi_flexible_type.hpp>
#include <capi/impl/capi_sframe.hpp>
#include <capi/impl/capi_sarray.hpp>
#include <sframe/sframe.hpp>
#include <capi/impl/capi_parameters.hpp>
#include <flexible_type/flexible_type.hpp>
#include <unity/lib/unity_sframe.hpp>
#include <export.hpp> 
#include <unity/lib/toolkit_util.hpp>


/******************************************************************************/
/*                                                                            */
/*   Parameter List                                                           */
/*                                                                            */
/******************************************************************************/


// Create a new set of parameters
EXPORT tc_parameters* tc_parameters_create_empty(tc_error** error) {
  ERROR_HANDLE_START();

  return new_tc_parameters();

  ERROR_HANDLE_END(error, NULL); 
} 

// Add a new SFrame to the set of parameters. 
EXPORT void tc_parameters_add_sframe(tc_parameters* params, const char* name, 
                              tc_sframe* sf, tc_error** error) {
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, params, "tc_parameters");
  CHECK_NOT_NULL(error, sf, "tc_sframe");

  // std::shared_ptr<turi::unity_sframe> ptr(new turi::unity_sframe); 

  // ptr->construct_from_sframe( *(sf->value.get_proxy()->get_underlying_sframe())); 
  
  params->value[name] = turi::to_variant(sf->value.get_proxy());

  // params->value[name] = turi::to_variant(ptr);

  ERROR_HANDLE_END(error); 
} 

// Add a new SFrame to the set of parameters. 
EXPORT void tc_parameters_add_sarray(tc_parameters* params, const char* name, 
                              tc_sarray* sa, tc_error** error) {
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, params, "tc_parameters");
  CHECK_NOT_NULL(error, sa, "tc_sarray");

  params->value[name] = turi::to_variant(sa->value.get_proxy());

  ERROR_HANDLE_END(error); 
} 

// Add a new flexible type parameter to the set of parameters. 
EXPORT void tc_parameters_add_flexible_type(tc_parameters* params, const char* name, tc_flexible_type* ft, tc_error** error) {

  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, params, "tc_parameters");
  CHECK_NOT_NULL(error, ft, "tc_flexible_type");

  params->value[name] = ft->value;

  ERROR_HANDLE_END(error); 
}

// Returns true if an entry exists, false otherwise   
EXPORT bool tc_parameters_entry_exists(const tc_parameters* params, const char* name, tc_error** error) {
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, params, "tc_parameters", false);

  return (params->value.find(name) != params->value.end());

  ERROR_HANDLE_END(error, false); 
} 

// Query the type of the parameter
EXPORT bool tc_parameters_is_sframe(const tc_parameters* params, const char* name, tc_error** error) { 
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, params, "tc_parameters", false);

  // TODO: Make less ugly!
  return params->value.at(name).which() == 4; 

  ERROR_HANDLE_END(error, false); 
} 


// Query the type of the parameter 
EXPORT bool tc_parameters_is_sarray(const tc_parameters* params, 
      const char* name, tc_error** error) {

  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, params, "tc_parameters", false);

  // TODO: Make less ugly!
  return params->value.at(name).which() == 5; 

  ERROR_HANDLE_END(error, false); 
}


// Query the type of the parameter 
EXPORT bool tc_parameters_is_flexible_type(const tc_parameters* params, 
    const char* name, tc_error** error) {
  
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, params, "tc_parameters", false);

  // TODO: Make less ugly!
  return params->value.at(name).which() == 0; 

  ERROR_HANDLE_END(error, false); 
} 

// Retrieve the value of an sframe as returned parameter.
EXPORT tc_sarray* tc_parameters_retrieve_sarray(const tc_parameters* params, 
    const char* name, tc_error** error) {

  ERROR_HANDLE_START();
    
  CHECK_NOT_NULL(error, params, "tc_parameters", NULL);

  const turi::variant_type& v = params->value.at(name);
  return new_tc_sarray(turi::variant_get_ref<std::shared_ptr<turi::unity_sarray_base> >(v));

  ERROR_HANDLE_END(error, NULL); 
}

// Retrieve the value of an sframe as returned parameter.
EXPORT tc_sframe* tc_parameters_retrieve_sframe(
    const tc_parameters* params, 
    const char* name, tc_error** error) {

  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, params, "tc_parameters", NULL);

  const turi::variant_type& v = params->value.at(name);
  return new_tc_sframe(turi::variant_get_ref<std::shared_ptr<turi::unity_sframe_base> >(v));

  ERROR_HANDLE_END(error, NULL); 
}

// Retrieve the value of an sframe as returned parameter.
EXPORT tc_flexible_type* tc_parameters_retrieve_flexible_type(
    const tc_parameters* params, const char* name, tc_error** error) {

ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, params, "tc_parameters", NULL);

  const turi::variant_type& v = params->value.at(name);
  return new_tc_flexible_type(turi::variant_get_ref<turi::flexible_type>(v));

  ERROR_HANDLE_END(error, NULL); 
}

// delete the parameter container. 
EXPORT void tc_parameters_destroy(tc_parameters* params) {
  delete params; 
}


