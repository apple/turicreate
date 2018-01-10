#ifndef TURI_CAPI_H
#define TURI_CAPI_H

#ifdef __cplusplus
extern "C" { 
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>


/******************************************************************************/
/*                                                                            */
/*    ERROR HANDLING                                                          */
/*                                                                            */
/******************************************************************************/


  /**********************

   // Example Error checking code 
    
   tc_error *error = NULL; 

   tc_create_flexible_type_str("hello", &error); 

   if(error) {   
      const char* msg = tc_error_message(error); 

      // ...

      tc_error_destroy(&error); 
   }

  *************************/

struct tc_error_struct; 
typedef struct tc_error_struct tc_error; 

/** Retrieves the error message on an active error.
 *
 *  Return object is a null-terminated c-style message string. 
 *
 *  The char buffer returned is invalidated by calling tc_error_destroy. 
 */
const char* tc_error_message(const tc_error* error); 


/** Destroys an error structure, deallocating error content data. 
 *
 *  Only needs to be called if an error occured.
 *
 *  Sets the pointer to the error struct to NULL. 
 */
void tc_error_destroy(tc_error** error_ptr);



/******************************************************************************/
/*                                                                            */
/*    INITIALIZATION                                                          */
/*                                                                            */
/******************************************************************************/

/** Initialize the framework. Call before calling any previous function.
 *
 */
void tc_initialize(const char* log_file, tc_error**); 

/******************************************************************************/
/*                                                                            */
/*    FLEXIBLE TYPE                                                           */
/*                                                                            */
/******************************************************************************/


struct tc_flexible_type_struct; 
typedef struct tc_flexible_type_struct tc_flexible_type;

/** Type enum.  Ones commented out are not yet implemented in the C API. */ 
typedef enum {
  FT_TYPE_INTEGER = 0,  
  FT_TYPE_FLOAT = 1,   
  FT_TYPE_STRING = 2,   
  // FT_TYPE_VECTOR = 3, 
  FT_TYPE_LIST = 4, 
  FT_TYPE_DICT = 5, 
  // FT_TYPE_DATETIME = 6, 
  FT_TYPE_UNDEFINED = 7,
  FT_TYPE_IMAGE= 8
} tc_ft_type_enum; 

/****************************************************/

tc_flexible_type* tc_ft_create_empty(tc_error** error);

tc_flexible_type* tc_ft_create_copy(const tc_flexible_type*, tc_error** error);

tc_flexible_type* tc_ft_create_from_cstring(const char* str, tc_error** error);

tc_flexible_type* tc_ft_create_from_string(const char* str, uint64_t n, tc_error** error);

tc_flexible_type* tc_ft_create_from_double(double, tc_error** error);

tc_flexible_type* tc_ft_create_from_int64(int64_t, tc_error** error);

/****************************************************/

tc_ft_type_enum tc_ft_type(const tc_flexible_type*);

int tc_ft_is_string(const tc_flexible_type*); 
int tc_ft_is_double(const tc_flexible_type*);
int tc_ft_is_int64(const tc_flexible_type*); 
int tc_ft_is_image(const tc_flexible_type*); 

/****************************************************/

double tc_ft_double(const tc_flexible_type* ft, tc_error** error); 

int64_t tc_ft_int64(const tc_flexible_type* ft, tc_error** error); 

uint64_t tc_ft_string_length(const tc_flexible_type* ft, tc_error** error); 

const char* tc_ft_string_data(const tc_flexible_type* ft, tc_error** error);


// Cast the type to string.  Can be used to print the type.
tc_flexible_type* tc_ft_as_string(const tc_flexible_type*, tc_error** error); 


/****************************************************/

void tc_ft_destroy(tc_flexible_type*); 

/******************************************************************************/
/*                                                                            */
/*    flex_list                                                               */
/*                                                                            */
/******************************************************************************/

struct tc_flex_list_struct;
typedef struct tc_flex_list_struct tc_flex_list;

tc_flex_list* tc_flex_list_create(tc_error**);
tc_flex_list* tc_flex_list_create_with_capacity(uint64_t capacity, tc_error**);

uint64_t tc_flex_list_add_element(tc_flex_list*, const tc_flexible_type*, tc_error**);

tc_flexible_type* tc_flex_list_extract_element(
    const tc_flex_list*, uint64_t index, tc_error**);

uint64_t tc_flex_list_size(const tc_flex_list*);


// Conversion to flexible type
tc_flexible_type* tc_ft_create_from_flex_list(const tc_flex_list*, tc_error** error);

tc_flex_list* tc_ft_flex_list(const tc_flexible_type*, tc_error**);

void tc_flex_list_destroy(tc_flex_list*);


/******************************************************************************/
/*                                                                            */
/*    flex_dict                                                               */
/*                                                                            */
/******************************************************************************/

// NOTE: flex_dicts are simply key-value lists; lookups by key are not efficient
// and thus not implemented.

struct tc_flex_dict_struct;
typedef struct tc_flex_dict_struct tc_flex_dict;

// Creates an empty flex_dict object.
tc_flex_dict* tc_flex_dict_create(tc_error**);

// Conversion to/from flexible type
tc_flexible_type* tc_ft_create_from_flex_dict(const tc_flex_dict*, tc_error** error);
tc_flex_dict* tc_ft_flex_dict(const tc_flexible_type*, tc_error**);

// Adds a key to the dictionary, returning the entry index.. 
uint64_t tc_flex_dict_add_element(tc_flex_dict* ft, const tc_flexible_type* first, const tc_flexible_type* second, tc_error**);

// Extract the (key, value) pair corresponding to the entry at entry_index. 
void tc_flex_dict_extract_entry(const tc_flex_dict* ft, uint64_t entry_index, tc_flexible_type* key_dest, tc_flexible_type* value_dest, tc_error**); 

// Destroy the dictionary. 
void tc_flex_dict_destroy(tc_flex_dict*);

/******************************************************************************/
/*                                                                            */
/*    flex_image                                                              */
/*                                                                            */
/******************************************************************************/

struct tc_flex_image_struct;
typedef struct tc_flex_image_struct tc_flex_image;

// Load an image into a flexible type from a path
tc_flex_image* tc_flex_image_create_from_path(
    const char* path, const char* format, tc_error** error);

// Load an image into a flexible type from raw data
tc_flex_image* tc_flex_image_create_from_data(
    const char* data, uint64_t height, uint64_t width, uint64_t channels, 
    uint64_t total_data_size, const char* format, tc_error** error); 

// Methods to query the image size and width
uint64_t tc_flex_image_width(const tc_flex_image*, tc_error**); 
uint64_t tc_flex_image_height(const tc_flex_image*, tc_error**); 
uint64_t tc_flex_image_num_channels(const tc_flex_image*, tc_error**); 
uint64_t tc_flex_image_data_size(const tc_flex_image*, tc_error**); 
const char* tc_flex_image_data(const tc_flex_image*, tc_error**); 
const char* tc_flex_image_format(const tc_flex_image*, tc_error**); 

// Flexible type interaction 
tc_flexible_type* tc_ft_create_from_image(const tc_flex_image*, tc_error** error);
tc_flex_image* tc_ft_flex_image(const tc_flexible_type*, tc_error**); 

// Destructor
void tc_flex_image_destroy(tc_flex_image*); 


/******************************************************************************/
/*                                                                            */
/*    SARRAY                                                                  */
/*                                                                            */
/******************************************************************************/


struct tc_sarray_struct; 
typedef struct tc_sarray_struct tc_sarray; 

tc_sarray* tc_sarray_create(const tc_flex_list* data, tc_error**);

tc_sarray* tc_sarray_create_from_sequence(
    uint64_t start, uint64_t end, tc_error** error);

tc_sarray* tc_sarray_create_from_const(
    const tc_flexible_type* value, uint64_t n, tc_error** error);

tc_sarray* tc_sarray_create_from_list(
    const tc_flex_list* values, tc_error** error);

tc_sarray* tc_sarray_create_copy(const tc_sarray* src, tc_error** error);


// Gets a particular element.
tc_flexible_type* tc_sarray_extract_element(const tc_sarray*, uint64_t index, tc_error**);

// Gets the sarry size. 
uint64_t tc_sarray_size(const tc_sarray*); 

// Gets the type of the sarray. 
tc_ft_type_enum tc_sarray_type(const tc_sarray*); 

// Gets the type of the sarray. 
tc_sarray* tc_op_sarray_plus_sarray(const tc_sarray*, const tc_sarray*, tc_error**); 
tc_sarray* tc_op_sarray_minus_sarray(const tc_sarray*, const tc_sarray*, tc_error**); 
tc_sarray* tc_op_sarray_div_sarray(const tc_sarray*, const tc_sarray*, tc_error**); 
tc_sarray* tc_op_sarray_mult_sarray(const tc_sarray*, const tc_sarray*, tc_error**); 
tc_sarray* tc_op_sarray_plus_ft(const tc_sarray*, const tc_flexible_type*, tc_error**); 
tc_sarray* tc_op_sarray_minus_ft(const tc_sarray*, const tc_flexible_type*, tc_error**); 
tc_sarray* tc_op_sarray_div_ft(const tc_sarray*, const tc_flexible_type*, tc_error**); 
tc_sarray* tc_op_sarray_mult_ft(const tc_sarray*, const tc_flexible_type*, tc_error**); 

// Returns 1 if all elements are equal and 0 otherwise.
int tc_sarray_equals(const tc_sarray*, const tc_sarray*, tc_error**);

// Call sum on the tc_sarray
tc_flexible_type* tc_sarray_sum(const tc_sarray*, tc_error**);

// Wrap the printing.  Returns a string flexible type.
tc_flexible_type* tc_sarray_text_summary(const tc_sarray* sf, tc_error**);

// SArray Apply. The flexible_type* returned is not automatically freed
tc_sarray* tc_sarray_apply(const tc_sarray*,
                           tc_flexible_type*(*callback)(tc_flexible_type*, void* userdata),
                           void (*userdata_release_callback)(void* userdata),
                           void* userdata,
                           tc_ft_type_enum type,
                           bool skip_undefined,
                           tc_error**);

// Destructor
void tc_sarray_destroy(tc_sarray* sa);


/******************************************************************************/
/*                                                                            */
/*    SKETCH                                                                  */
/*                                                                            */
/******************************************************************************/

struct tc_sketch_struct;
typedef struct tc_sketch_struct tc_sketch;

tc_sketch* tc_sketch_create(const tc_sarray*, bool background, const tc_flex_list* keys, tc_error **);

bool tc_sketch_ready(tc_sketch*);
size_t tc_sketch_num_elements_processed(tc_sketch*);
double tc_sketch_get_quantile(tc_sketch*, double quantile, tc_error**);
double tc_sketch_frequency_count(tc_sketch*, const tc_flexible_type* value, tc_error**);
tc_flex_dict* tc_sketch_frequent_items(tc_sketch*);
double tc_sketch_num_unique(tc_sketch*);
tc_sketch* tc_sketch_element_sub_sketch(const tc_sketch*, const tc_flexible_type* key, tc_error**);
tc_sketch* tc_sketch_element_length_summary(const tc_sketch*, tc_error**);
tc_sketch* tc_sketch_element_summary(const tc_sketch*, tc_error**);
tc_sketch* tc_sketch_dict_key_summary(const tc_sketch*, tc_error **error);
tc_sketch* tc_sketch_dict_value_summary(const tc_sketch*, tc_error **error);
double tc_sketch_mean(const tc_sketch*, tc_error**);
double tc_sketch_max(const tc_sketch*, tc_error**);
double tc_sketch_min(const tc_sketch*, tc_error**);
double tc_sketch_sum(const tc_sketch*, tc_error**);
double tc_sketch_variance(const tc_sketch*, tc_error**);
size_t tc_sketch_size(const tc_sketch*);
size_t tc_sketch_num_undefined(const tc_sketch*);
void tc_sketch_cancel(tc_sketch*);

void tc_sketch_destroy(tc_sketch *);


/******************************************************************************/
/*                                                                            */
/*   SFRAME                                                                   */
/*                                                                            */
/******************************************************************************/

struct tc_sframe_struct; 
typedef struct tc_sframe_struct tc_sframe; 

tc_sframe* tc_sframe_create_empty(tc_error**);

tc_sframe* tc_sframe_create_copy(tc_sframe*, tc_error**);

// Adds the column to the sframe.
void tc_sframe_add_column(tc_sframe* sf, const char* column_name, 
    const tc_sarray* sarray, tc_error**);

// Remove a certain column.
void tc_sframe_remove_column(tc_sframe* sf, const char* column_name, tc_error**);

tc_sarray* tc_sframe_extract_column_by_name(
    tc_sframe* sf, const char* column_name, tc_error**);

// Wrap the printing.  Returns a string flexible type. 
tc_flexible_type* tc_sframe_text_summary(const tc_sframe* sf, tc_error**);

// Number of rows 
uint64_t tc_sframe_num_rows(const tc_sframe* sf, tc_error**); 

// Number of columns.
uint64_t tc_sframe_num_columns(const tc_sframe* sf, tc_error**); 

// Return the name of a particular column.
const char* tc_sframe_column_name(const tc_sframe* sf, size_t column_index, tc_error**);

// Return the type of a particular column.
tc_ft_type_enum tc_sframe_column_type(const tc_sframe* sf, const char* column_name, tc_error**); 

// Return all column types as a list.
tc_flex_list* tc_sframe_column_names(const tc_sframe* sf, tc_error**);

// Read csv
tc_sframe* tc_sframe_read_csv(const char *url, tc_error**);

// Write csv
void tc_sframe_write_csv(const tc_sframe* sf, const char *url, tc_error **error);

// Write other format
void tc_sframe_write(const tc_sframe* sf, const char *url, const char *format, tc_error **error);

// Head
tc_sframe* tc_sframe_head(const tc_sframe* sf, size_t n, tc_error **error);

// Tail
tc_sframe* tc_sframe_tail(const tc_sframe* sf, size_t n, tc_error **error);

// Random split.
void tc_sframe_random_split(const tc_sframe* sf, double proportion, size_t seed, tc_sframe** left, tc_sframe** right, tc_error **error);

// Whizbangery
//
// Join two sframes.  
//
// column is the column name to join on. 
// how is "inner", "outer", "left", or "right" 
tc_sframe* tc_sframe_join_on_single_column(
    tc_sframe* left, tc_sframe* right, 
    const char* column, 
    const char* how, tc_error**);

// Append one sframe onto another.
tc_sframe* tc_sframe_append(tc_sframe* top, tc_sframe* bottom, tc_error **error);


// Destructor
void tc_sframe_destroy(tc_sframe* sa);






/******************************************************************************/
/*                                                                            */
/*   Parameter List                                                           */
/*                                                                            */
/******************************************************************************/


struct tc_parameters_struct; 
typedef struct tc_parameters_struct tc_parameters;

// Create a new set of parameters
tc_parameters* tc_parameters_create_empty(tc_error**); 

// Add a new SFrame to the set of parameters. 
void tc_parameters_add_sframe(tc_parameters* params, const char* name, tc_sframe* sframe, tc_error**); 

// Add a new SArray to the set of parameters.
void tc_parameters_add_sarray(tc_parameters* params, const char* name, 
                              tc_sarray* sa, tc_error** error);

// Add a new flexible type parameter to the set of parameters. 
void tc_parameters_add_flexible_type(tc_parameters* params, const char* name, tc_flexible_type* ft, tc_error**); 

// Returns true if an entry exists, false otherwise   
bool tc_parameters_entry_exists(const tc_parameters* params, const char* name, tc_error**); 

// Query the type of a return parameter
bool tc_parameters_is_sframe(const tc_parameters* params, const char* name, tc_error**);

// Query the type of a return parameter
bool tc_parameters_is_sarray(const tc_parameters* params, const char* name, tc_error**);

// Query the type of a return vector
bool tc_parameters_is_flexible_type(const tc_parameters* params, const char* name, tc_error**); 

// Retrieve the value of an sframe as returned parameter.
tc_sarray* tc_parameters_retrieve_sarray(const tc_parameters* params, const char* name, tc_error**);

// Retrieve the value of an sframe as returned parameter.
tc_sframe* tc_parameters_retrieve_sframe(const tc_parameters* params, const char* name, tc_error**);

// Retrieve the value of an sframe as returned parameter.
tc_flexible_type* tc_parameters_retrieve_flexible_type(const tc_parameters* params, const char* name, tc_error**);

// delete the parameter container. 
void tc_parameters_destroy(tc_parameters*); 


/******************************************************************************/
/*                                                                            */
/*   Models                                                                   */
/*                                                                            */
/******************************************************************************/

struct tc_model_struct; 
typedef struct tc_model_struct tc_model;

tc_model* tc_model_new(const char* model_name, tc_error**);

tc_model* tc_model_load(const char* file_name, tc_error**);

const char* tc_model_name(const tc_model*, tc_error**);

tc_parameters* tc_model_call_method(const tc_model* model, const char* method, 
                                    const tc_parameters* arguments, tc_error**);


void tc_model_destroy(tc_model*);












#ifdef __cplusplus
}
#endif



#endif


