#include <capi/TuriCore.h>
#include <capi/impl/capi_error_handling.hpp>
#include <capi/impl/capi_flexible_type.hpp>
#include <capi/impl/capi_flex_dict.hpp>
#include <capi/impl/capi_flex_list.hpp>
#include <capi/impl/capi_flex_enum_list.hpp>
#include <capi/impl/capi_sarray.hpp>
#include <capi/impl/capi_sframe.hpp>

#include <sstream>
#include <string>

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

EXPORT tc_sframe* tc_sframe_append(tc_sframe* top, tc_sframe* bottom, tc_error** error) {

  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, top, "top sframe", bottom);
  CHECK_NOT_NULL(error, bottom, "bottom sframe", top);

  return new_tc_sframe(top->value.append(bottom->value));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT int tc_sframe_is_materialized(
    const tc_sframe* src, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, src, "sframe", NULL);

  return src->value.is_materialized();

  ERROR_HANDLE_END(error, NULL);
}

EXPORT int tc_sframe_size_is_known(
    const tc_sframe* src, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, src, "sframe", NULL);

  return src->value.has_size();

  ERROR_HANDLE_END(error, NULL);
}

EXPORT void tc_sframe_save_reference(
    const tc_sframe* src, const char* path, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, src, "sframe");

  src->value.save(path);

  ERROR_HANDLE_END(error);
}

EXPORT void tc_sframe_materialize(tc_sframe* src, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, src, "sframe");

  src->value.materialize();

  ERROR_HANDLE_END(error);
}

EXPORT bool tc_sframe_contains_column(
    const tc_sframe* src, const char* col_name, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, src, "sframe", NULL);

  return src->value.contains_column(col_name);

  ERROR_HANDLE_END(error, NULL);
}


EXPORT tc_sframe* tc_sframe_sample(
    const tc_sframe* src, double fraction, uint64_t seed, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, src, "sframe", NULL);

  return new_tc_sframe(src->value.sample(fraction, seed));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT void tc_sframe_replace_add_column(
    tc_sframe* sf, const char* name, const tc_sarray* new_column, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, sf, "sframe");
  CHECK_NOT_NULL(error, new_column, "sarray");

  sf->value.replace_add_column(new_column->value, name);

  ERROR_HANDLE_END(error);
}

EXPORT void tc_sframe_add_constant_column(
    tc_sframe* sf, const char* column_name, const tc_flexible_type* value, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, sf, "sframe");
  CHECK_NOT_NULL(error, value, "tc_flexible_type");

  sf->value.add_column(value->value, column_name);

  ERROR_HANDLE_END(error);
}

EXPORT void tc_sframe_add_columns(
    tc_sframe* sf, const tc_sframe* other, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, sf, "sframe");
  CHECK_NOT_NULL(error, other, "sframe");

  sf->value.add_columns(other->value);

  ERROR_HANDLE_END(error);
}

EXPORT tc_sframe* tc_sframe_topk(
    const tc_sframe* src, const char* column_name, uint64_t k, bool reverse, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, src, "sframe", NULL);

  return new_tc_sframe(src->value.topk(column_name, k, reverse));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT void tc_sframe_swap_columns(tc_sframe* sf, const char* column_1, const char* column_2, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, sf, "sframe");

  sf->value.swap_columns(column_1, column_2);

  ERROR_HANDLE_END(error);
}

EXPORT void tc_sframe_rename_column(
    tc_sframe* sf, const char* old_name, const char* new_name, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, sf, "sframe");

  sf->value.rename({ {std::string(old_name), std::string(new_name)} });

  ERROR_HANDLE_END(error);
}

EXPORT void tc_sframe_rename_columns(
    tc_sframe* sf, const tc_flex_dict* name_mapping, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, sf, "sframe");
  CHECK_NOT_NULL(error, name_mapping, "tc_flex_dict");

  std::map<std::string, std::string> m;

  for(auto p : name_mapping->value) {

    if(p.first.get_type() != turi::flex_type_enum::STRING){
      set_error(error, "entries are not of type str");
    }

    if(p.second.get_type() != turi::flex_type_enum::STRING){
      set_error(error, "entries are not of type str");
    }

    m.insert(std::pair<std::string, std::string>(p.first.get<turi::flex_string>(), p.second.get<turi::flex_string>()));
  }

  sf->value.rename(m);

  ERROR_HANDLE_END(error);
}

EXPORT tc_sframe* tc_sframe_filter_by(const tc_sframe* sf, const tc_sarray* values, const char* column_name, bool exclude, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, sf, "sframe", NULL);
  CHECK_NOT_NULL(error, values, "sarray", NULL);

  return new_tc_sframe(sf->value.filter_by(values->value, column_name, exclude));

  ERROR_HANDLE_END(error, NULL);
}


EXPORT tc_sframe* tc_sframe_pack_columns_vector(
    const tc_sframe* sf, const tc_flex_list* columns, const char* column_name, tc_ft_type_enum type, tc_flexible_type* value, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, sf, "sframe", NULL);
  CHECK_NOT_NULL(error, columns, "flex_list", NULL);

  for (const turi::flexible_type& i : columns->value){
    if(i.get_type() != turi::flex_type_enum::STRING){
      set_error(error, "Contains a non-string column.");
      return NULL;
    }
  }

  std::vector<std::string> column_transform(columns->value.begin(), columns->value.end());

  return new_tc_sframe(sf->value.pack_columns(column_transform, column_name, static_cast<turi::flex_type_enum>(type), value->value));

  ERROR_HANDLE_END(error, NULL);
}


EXPORT tc_sframe* tc_sframe_pack_columns_string(const tc_sframe* sf, const char* column_prefix, const char* column_name, tc_ft_type_enum type, tc_flexible_type* value, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, sf, "sframe", NULL);

  return new_tc_sframe(sf->value.pack_columns(column_prefix, column_name, static_cast<turi::flex_type_enum>(type), value->value));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sframe* tc_sframe_split_datetime(const tc_sframe* sf, const char* expand_column, const char* column_prefix, const tc_flex_list* limit, bool tzone, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, sf, "sframe", NULL);

  for (const turi::flexible_type& i : limit->value){
    if(i.get_type() != turi::flex_type_enum::STRING){
      set_error(error, "Element of limit is not of type str");
      return NULL;
    }
  }

  std::vector<std::string> limit_transform(limit->value.begin(), limit->value.end());

  return new_tc_sframe(sf->value.split_datetime(expand_column, column_prefix, limit_transform, tzone));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sframe* tc_sframe_unpack(const tc_sframe* sf, const char* unpack_column, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, sf, "sframe", NULL);

  return new_tc_sframe(sf->value.unpack(unpack_column));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sframe* tc_sframe_unpack_detailed(const tc_sframe* sf, const char* unpack_column, const char* column_prefix, const tc_flex_enum_list* type, tc_flexible_type* value, const tc_flex_list* limit, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, sf, "sframe", NULL);

  std::vector<turi::flex_type_enum> type_transform;

  for (const turi::flex_type_enum& i : type->value) {
    type_transform.push_back(i);
  }

  return new_tc_sframe(sf->value.unpack(unpack_column, column_prefix, type_transform, value->value, limit->value));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sframe* tc_sframe_stack(const tc_sframe* sf, const char* column_name, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, sf, "sframe", NULL);

  return new_tc_sframe(sf->value.stack(column_name, column_name));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sframe* tc_sframe_stack_and_rename(const tc_sframe* sf, const char* column_name, const char* new_column_name, bool drop_na, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, sf, "sframe", NULL);

  return new_tc_sframe(sf->value.stack(column_name, new_column_name, drop_na));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sframe* tc_sframe_unstack(const tc_sframe* sf, const char* column, const char* new_column_name, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, sf, "sframe", NULL);

  return new_tc_sframe(sf->value.unstack(column, new_column_name));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sframe* tc_sframe_unstack_vector(
    const tc_sframe* sf, const tc_flex_list* columns, const char* new_column_name, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, sf, "sframe", NULL);
  CHECK_NOT_NULL(error, columns, "flex_list", NULL);

  for (const turi::flexible_type& i : columns->value){
    if(i.get_type() != turi::flex_type_enum::STRING){
      set_error(error, "Contains a non-string column.");
      return NULL;
    }
  }

  std::vector<std::string> columns_transform(columns->value.begin(), columns->value.end());

  return new_tc_sframe(sf->value.unstack(columns_transform, new_column_name));

  ERROR_HANDLE_END(error, NULL);
}


EXPORT tc_sframe* tc_sframe_unique(const tc_sframe* sf, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, sf, "sframe", NULL);

  return new_tc_sframe(sf->value.unique());

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sframe* tc_sframe_sort_single_column(const tc_sframe* sf, const char* column, bool ascending, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, sf, "sframe", NULL);

  return new_tc_sframe(sf->value.sort(column, ascending));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sframe* tc_sframe_dropna(const tc_sframe* sf, const tc_flex_list* columns, const char* how, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, sf, "sframe", NULL);
  CHECK_NOT_NULL(error, columns, "flex_list", NULL);

  std::vector<std::string> columns_transform;

  for (const turi::flexible_type& i : columns->value){
    if(i.get_type() != turi::flex_type_enum::STRING){
      set_error(error, "Contains a non-string column.");
      return NULL;
    }

    columns_transform.push_back(i.get<turi::flex_string>());
  }

  return new_tc_sframe(sf->value.dropna(columns_transform, how));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sframe* tc_sframe_sort_multiple_columns(const tc_sframe* sf, const tc_flex_list* columns, bool ascending, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, sf, "sframe", NULL);
  CHECK_NOT_NULL(error, columns, "flex_list", NULL);

  std::vector<std::string> columns_transform;

  for (const turi::flexible_type& i : columns->value){
    if(i.get_type() != turi::flex_type_enum::STRING){
      set_error(error, "Contains a non-string column.");
      return NULL;
    }

    columns_transform.push_back(i.get<turi::flex_string>());
  }

  return new_tc_sframe(sf->value.sort(columns_transform, ascending));

  ERROR_HANDLE_END(error, NULL);
}


EXPORT tc_sframe* tc_sframe_slice(const tc_sframe* sf, const uint64_t start, const uint64_t end, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, sf, "sframe", NULL);

  return new_tc_sframe(sf->value[{static_cast<long long>(start), static_cast<long long>(end)}]);

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sframe* tc_sframe_slice_stride(const tc_sframe* sf, const uint64_t start, const uint64_t end, const uint64_t stride, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, sf, "sframe", NULL);

  return new_tc_sframe(sf->value[{static_cast<long long>(start), static_cast<long long>(end), static_cast<long long>(stride)}]);

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sframe* tc_sframe_fillna(const tc_sframe* data,const char* column,const tc_flexible_type* value, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, data, "sframe", NULL);
  CHECK_NOT_NULL(error, value, "flexible_type", NULL);

  return new_tc_sframe(data->value.fillna(column, value->value));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT void tc_sframe_destroy(tc_sframe* sf) {
  delete sf;
}

}
