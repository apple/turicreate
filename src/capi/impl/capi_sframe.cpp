#include <capi/TuriCore.h>
#include <capi/impl/capi_error_handling.hpp>
#include <capi/impl/capi_flexible_type.hpp>
#include <capi/impl/capi_flex_list.hpp>
#include <capi/impl/capi_sarray.hpp>
#include <capi/impl/capi_sframe.hpp>

#include <sstream>

#include <flexible_type/flexible_type.hpp> 
#include <export.hpp>
#include <unity/lib/gl_sarray.hpp>
#include <unity/lib/gl_sframe.hpp>

extern "C" { 


EXPORT tc_sframe* tc_sframe_create_empty(tc_error** error) { 
  ERROR_HANDLE_START();
  
  return new_tc_sframe();

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sframe* tc_sframe_create_copy(tc_sframe* sf, tc_error** error) { 
  ERROR_HANDLE_START();
  
  tc_sframe* ret = new_tc_sframe();
  ret->value = turi::gl_sframe(sf->value); 
  return ret;

  ERROR_HANDLE_END(error, NULL);
}


// Adds the column to the sframe
EXPORT void tc_sframe_add_column(
    tc_sframe* sf, const char* column_name, const tc_sarray* sa, tc_error** error) {

  ERROR_HANDLE_START();
  
  CHECK_NOT_NULL(error, sf, "tc_sframe");

  sf->value.add_column(sa->value, column_name);

  ERROR_HANDLE_END(error);
}

// Removes the column from the sframe
EXPORT void tc_sframe_remove_column(
    tc_sframe* sf, const char* column_name, tc_error** error) {

  ERROR_HANDLE_START();
  
  CHECK_NOT_NULL(error, sf, "tc_sframe");

  sf->value.remove_column(column_name);

  ERROR_HANDLE_END(error);
}
 
EXPORT tc_sarray* tc_sframe_extract_column_by_name(
    tc_sframe* sf, const char* column_name, tc_error** error) { 

  ERROR_HANDLE_START();
  
  return new_tc_sarray(sf->value.select_column(column_name));

  ERROR_HANDLE_END(error, NULL);
}

// Wrap the printing.  Returns a string flexible type.
EXPORT tc_flexible_type* tc_sframe_text_summary(const tc_sframe* sf, tc_error** error) {
  ERROR_HANDLE_START();

  if(sf == NULL) { 
    set_error(error, "SFrame passed in to summarize is null.");
    return NULL; 
  }

  std::ostringstream ss; 
  ss << sf->value;

  return new_tc_flexible_type(ss.str());

  ERROR_HANDLE_END(error, NULL);
}

EXPORT uint64_t tc_sframe_num_rows(const tc_sframe* sf, tc_error** error) {
  ERROR_HANDLE_START();

  if(sf == NULL) { 
    set_error(error, "SFrame passed in to num_rows is null.");
    return NULL; 
  }

  return sf->value.size();

  ERROR_HANDLE_END(error, NULL);
}

EXPORT uint64_t tc_sframe_num_columns(const tc_sframe* sf, tc_error** error) {
  ERROR_HANDLE_START();

  if(sf == NULL) { 
    set_error(error, "SFrame passed in to num_columns is null.");
    return NULL; 
  }

  return sf->value.num_columns();

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_flex_list* tc_sframe_column_names(const tc_sframe* sf, tc_error** error) {
  ERROR_HANDLE_START();

  if(sf == NULL) { 
    set_error(error, "SFrame passed in to summarize is null.");
    return NULL; 
  }

  std::vector<std::string> column_names = sf->value.column_names(); 

  return new_tc_flex_list(turi::flex_list(column_names.begin(), column_names.end()));

  ERROR_HANDLE_END(error, NULL);
}

 
EXPORT tc_sframe* tc_sframe_join_on_single_column(
    tc_sframe* left, tc_sframe* right, 
    const char* column, 
    const char* how, tc_error** error) {

  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, left, "left tc_sframe", NULL); 
  CHECK_NOT_NULL(error, right, "right tc_sframe", NULL); 

  return new_tc_sframe(left->value.join(right->value, {std::string(column)}, how));

  ERROR_HANDLE_END(error, NULL);
}


EXPORT tc_sframe* tc_sframe_read_csv(const char *url, tc_error **error) {
  ERROR_HANDLE_START();

  tc_sframe* ret = new_tc_sframe();
  turi::csv_parsing_config_map config;
  turi::str_flex_type_map column_type_hints;
  ret->value.construct_from_csvs(url, config, column_type_hints);
  return ret;

  ERROR_HANDLE_END(error, NULL);
}

EXPORT void tc_sframe_write(const tc_sframe* sf, const char *url, 
                            const char *format, tc_error **error) {
  ERROR_HANDLE_START();

  sf->value.save(url, format);

  ERROR_HANDLE_END(error);
}

EXPORT void tc_sframe_write_csv(const tc_sframe* sf, const char *url, tc_error **error) {
  tc_sframe_write(sf, url, "csv", error); 
}

EXPORT tc_sframe* tc_sframe_head(const tc_sframe* sf, size_t n, tc_error **error) {
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, sf, "sframe", NULL);

	return new_tc_sframe(sf->value.head(n));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sframe* tc_sframe_tail(const tc_sframe* sf, size_t n, tc_error **error) {
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, sf, "sframe", NULL);

	return new_tc_sframe(sf->value.tail(n));

  ERROR_HANDLE_END(error, NULL);
}

// Return the name of a particular column.
EXPORT const char* tc_sframe_column_name(const tc_sframe* sf, size_t column_index, tc_error** error) {
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, sf, "sframe", NULL);

  return sf->value.column_name(column_index).c_str();
  
  ERROR_HANDLE_END(error, NULL);
}

// Return the type of a particular column.
EXPORT tc_ft_type_enum tc_sframe_column_type(
    const tc_sframe* sf, const char* column_name, tc_error** error) {

  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, sf, "sframe", FT_TYPE_UNDEFINED);

  return static_cast<tc_ft_type_enum>(sf->value[column_name].dtype());

  ERROR_HANDLE_END(error, FT_TYPE_UNDEFINED);
}

EXPORT void tc_sframe_random_split(const tc_sframe* sf, double fraction, 
    size_t seed, tc_sframe** left, tc_sframe** right, tc_error** error) {
  
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, sf, "sframe");

  auto frames = sf->value.random_split(fraction, seed);

	*left = new_tc_sframe(frames.first);
	*right = new_tc_sframe(frames.second);

  ERROR_HANDLE_END(error);
}

EXPORT tc_sframe* tc_sframe_append(tc_sframe* top, tc_sframe* bottom, tc_error **error) {

  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, top, "top sframe", bottom);
  CHECK_NOT_NULL(error, bottom, "bottom sframe", top);
  
  return new_tc_sframe(top->value.append(bottom->value));

  ERROR_HANDLE_END(error, NULL);
}


EXPORT void tc_sframe_destroy(tc_sframe* sf) { 
  delete sf;
}

}
