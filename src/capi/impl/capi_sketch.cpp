#include <capi/TuriCore.h>
#include <capi/impl/capi_error_handling.hpp>
#include <capi/impl/capi_flex_dict.hpp>
#include <capi/impl/capi_flex_list.hpp>
#include <capi/impl/capi_flexible_type.hpp>
#include <capi/impl/capi_sketch.hpp>
#include <export.hpp> 

#include <flexible_type/flexible_type.hpp> 
#include <unity/lib/unity_sarray.hpp>

extern "C" {

  EXPORT tc_sketch* tc_sketch_create(const tc_sarray* sa, bool background, const tc_flex_list* keys, tc_error **error) {
    ERROR_HANDLE_START();

    tc_sketch* ret = new_tc_sketch();
    ret->value->construct_from_sarray(
        sa->value.get_proxy(),
        background,
        keys ? keys->value : turi::flex_list());
    return ret;

    ERROR_HANDLE_END(error, NULL);
  }

  EXPORT bool tc_sketch_ready(tc_sketch* sk) {
    if (!sk) {
      return false;
    }
    return sk->value->sketch_ready();
  }

  EXPORT size_t tc_sketch_num_elements_processed(tc_sketch* sk) {
    if (!sk) {
      return 0;
    }
    return sk->value->num_elements_processed();
  }

  EXPORT double tc_sketch_get_quantile(tc_sketch* sk, double quantile, tc_error **error) {
    ERROR_HANDLE_START();

    CHECK_NOT_NULL(error, sk, "Sketch", 0.0);
    return sk->value->get_quantile(quantile);

    ERROR_HANDLE_END(error, 0.0);
  }

  EXPORT double tc_sketch_frequency_count(tc_sketch* sk, const tc_flexible_type* value, tc_error **error) {
    ERROR_HANDLE_START();

    CHECK_NOT_NULL(error, sk, "Sketch", 0.0);
    CHECK_NOT_NULL(error, value, "Flexible type", 0.0);
    return sk->value->frequency_count(value->value);

    ERROR_HANDLE_END(error, 0.0);
  }

  EXPORT tc_flex_dict* tc_sketch_frequent_items(tc_sketch* sk) {
    if (!sk) {
      return NULL;
    }

    // Have to convert from
    // std::vector<std::pair<flexible_type, unsigned int>> (frequent_items() return type) to
    // std::vector<std::pair<flexible_type, flexible_type>> (flex_dict)
    // Not sure why implicit conversion from pair<flexible_type, uint> (A) to pair<flexible_type, flexible_type> (B)
    // seems allowed, but conversion from vector<A> to vector<B> does not seem allowed.
    turi::flex_dict ret;
    const auto& freq = sk->value->frequent_items();
    for (const auto& pair : freq) {
      ret.push_back(pair);
    }
    return new_tc_flex_dict(ret);
  }

  EXPORT double tc_sketch_num_unique(tc_sketch* sk) {
    if (!sk) {
      return 0.0;
    }
    return sk->value->num_unique();
  }

  EXPORT tc_sketch* tc_sketch_element_sub_sketch(const tc_sketch* sk, const tc_flexible_type* key, tc_error **error) {
    ERROR_HANDLE_START();

    CHECK_NOT_NULL(error, sk, "Sketch", NULL);
    CHECK_NOT_NULL(error, key, "Sub-sketch key", NULL);

    // This method exposes only a single-key variant, but
    // there is no C++ API for a single-key sub-sketch.
    // Instead, construct a 1-element vector to get a "many" key sub-sketch, per-key.
    auto sub_sketches = sk->value->element_sub_sketch(std::vector<turi::flexible_type>(1, key->value));
    const auto& result = sub_sketches.find(key->value);
    if (result == sub_sketches.end()) {
      set_error(error, "Unable to get sub-sketch for supplied key.");
      return NULL;
    }
    return new_tc_sketch(result->second);

    ERROR_HANDLE_END(error, NULL);
  }

  EXPORT tc_sketch* tc_sketch_element_length_summary(const tc_sketch* sk, tc_error** error) {
    ERROR_HANDLE_START();
    CHECK_NOT_NULL(error, sk, "Sketch", NULL);

    return new_tc_sketch(sk->value->element_length_summary());

    ERROR_HANDLE_END(error, NULL);
  }

  EXPORT tc_sketch* tc_sketch_element_summary(const tc_sketch* sk, tc_error** error) {
    ERROR_HANDLE_START();
    CHECK_NOT_NULL(error, sk, "Sketch", NULL);

    return new_tc_sketch(sk->value->element_summary());

    ERROR_HANDLE_END(error, NULL);
  }

  EXPORT tc_sketch* tc_sketch_dict_key_summary(const tc_sketch* sk, tc_error **error) {
    ERROR_HANDLE_START();
    CHECK_NOT_NULL(error, sk, "Sketch", NULL);

    return new_tc_sketch(sk->value->dict_key_summary());

    ERROR_HANDLE_END(error, NULL);
  }

  EXPORT tc_sketch* tc_sketch_dict_value_summary(const tc_sketch* sk, tc_error **error) {
    ERROR_HANDLE_START();
    CHECK_NOT_NULL(error, sk, "Sketch", NULL);

    return new_tc_sketch(sk->value->dict_value_summary());

    ERROR_HANDLE_END(error, NULL);
  }

  EXPORT double tc_sketch_mean(const tc_sketch* sk, tc_error** error) {
    ERROR_HANDLE_START();
    CHECK_NOT_NULL(error, sk, "Sketch", NULL);

    return sk->value->mean();

    ERROR_HANDLE_END(error, NULL);
  }

  EXPORT double tc_sketch_max(const tc_sketch* sk, tc_error** error) {
    ERROR_HANDLE_START();
    CHECK_NOT_NULL(error, sk, "Sketch", NULL);

    return sk->value->max();

    ERROR_HANDLE_END(error, NULL);
  }

  EXPORT double tc_sketch_min(const tc_sketch* sk, tc_error **error) {
    ERROR_HANDLE_START();
    CHECK_NOT_NULL(error, sk, "Sketch", NULL);

    return sk->value->min();

    ERROR_HANDLE_END(error, NULL);
  }

  EXPORT double tc_sketch_sum(const tc_sketch* sk, tc_error **error) {
    ERROR_HANDLE_START();
    CHECK_NOT_NULL(error, sk, "Sketch", NULL);

    return sk->value->sum();

    ERROR_HANDLE_END(error, NULL);
  }

  EXPORT double tc_sketch_variance(const tc_sketch* sk, tc_error **error) {
    ERROR_HANDLE_START();
    CHECK_NOT_NULL(error, sk, "Sketch", NULL);

    return sk->value->var();

    ERROR_HANDLE_END(error, NULL);
  }

  EXPORT size_t tc_sketch_size(const tc_sketch* sk) {
    if (!sk) {
      return 0;
    }
    return sk->value->size();
  }

  EXPORT size_t tc_sketch_num_undefined(const tc_sketch* sk) {
    if (!sk) {
      return 0;
    }
    return sk->value->num_undefined();
  }

  EXPORT void tc_sketch_cancel(tc_sketch* sk) {
    if (!sk) {
      return;
    }
    sk->value->cancel();
  }

  EXPORT void tc_sketch_destroy(tc_sketch *sk) {
    if (sk) {
      delete sk;
    }
  }

}
