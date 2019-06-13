/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <toolkits/sparse_similarity/sparse_similarity_lookup.hpp>
#include <toolkits/sparse_similarity/sparse_similarity_lookup_impl.hpp>
#include <toolkits/sparse_similarity/similarities.hpp>

namespace turi {

void sparse_similarity_lookup::add_options(option_manager& options) {

    option_handling::option_info opt;

    opt.name = "max_item_neighborhood_size";
    opt.description = ("Maxmimum number of similar items to save for predictions."
                       "Increasing this increases both memory and computation "
                       "requirements, but may give more accurate results.");
    opt.default_value = 64;
    opt.lower_bound = 1;
    opt.upper_bound = std::numeric_limits<flex_int>::max();
    opt.parameter_type = option_handling::option_info::INTEGER;
    options.create_option(opt);

    opt.name = "degree_approximation_threshold";
    opt.description = ("The maximum number of items for a given entry before "
                       "which we approximate the interaction through sampling.");
    opt.default_value = 4096;
    opt.lower_bound = 1;
    opt.upper_bound = std::numeric_limits<flex_int>::max();
    opt.parameter_type = option_handling::option_info::INTEGER;
    options.create_option(opt);

    opt.name = "target_memory_usage";
    opt.description = ("Target memory usage for processing");
    opt.default_value = 8LL*1024L*1024L*1024L;
    opt.lower_bound = 1024*1024;
    opt.upper_bound = std::numeric_limits<flex_int>::max();
    opt.parameter_type = option_handling::option_info::INTEGER;
    options.create_option(opt);

    opt.name = "threshold";
    opt.description = ("All items with similarity score below this "
                       "threshold are ignored at predict time.");
    opt.default_value = .0001;
    opt.parameter_type = option_handling::option_info::REAL;
    opt.lower_bound = 0.0;
    opt.upper_bound = std::numeric_limits<double>::max();
    options.create_option(opt);

    opt.name = "sparse_density_estimation_sample_size";
    opt.description = ("The number of samples to use for estimating how dense the "
                       "item-item connection matrix is. This data is used to "
                       "determine how many passes to take through the data.");
    opt.default_value = 4*1024;
    opt.lower_bound = 32;
    opt.upper_bound = std::numeric_limits<flex_int>::max();
    opt.parameter_type = option_handling::option_info::INTEGER;
    options.create_option(opt);

    opt.name = "max_data_passes";
    opt.description = ("The maximum number of passes allowed.  Increasing this "
                       "can allow the algorithms to run with less memory, but they will take longer.");
    opt.default_value = 4096;
    opt.lower_bound = 1;
    opt.upper_bound = std::numeric_limits<flex_int>::max();
    opt.parameter_type = option_handling::option_info::INTEGER;
    options.create_option(opt);

    opt.name = "nearest_neighbors_interaction_proportion_threshold";
    opt.description = ("Any item that was rated by more than this proportion of users is "
                       "treated by doing a nearest neighbors search.  For frequent items, this "
                       "is always faster but is slower for infrequent items.  Furthermore, "
                       "decreasing this causes more items to use the nearest neighbor path, "
                       "and may decrease memory requirements.");
    opt.default_value = 0.05;
    opt.lower_bound = 0;
    opt.upper_bound = 1;
    opt.parameter_type = option_handling::option_info::REAL;
    options.create_option(opt);

    opt.name = "training_method";
    opt.description = ("The method used for training.");
    opt.default_value = "auto";
    opt.parameter_type = option_handling::option_info::CATEGORICAL;
    opt.allowed_values = {"auto", "dense", "sparse",
                          "nn", "nn:dense", "nn:sparse"};
    options.create_option(opt);
}



std::shared_ptr<sparse_similarity_lookup> sparse_similarity_lookup::create(
    const std::string& similarity_name, const std::map<std::string, flexible_type>& options) {

#define create_and_return(SimilarityType)                               \
  do {                                                                  \
    typedef sparse_sim::sparse_similarity_lookup_impl<SimilarityType> alloc_class; \
    return std::make_shared<alloc_class>(SimilarityType(), options); \
  } while(false)

  if(similarity_name == "jaccard") {
    create_and_return(sparse_sim::jaccard);
  } else if (similarity_name == "cosine") {
    create_and_return(sparse_sim::cosine);
  } else if (similarity_name == "pearson") {
    create_and_return(sparse_sim::pearson);
  } else {
    log_and_throw( ( std::string("Item search method ") + similarity_name + " not available.").c_str());
  }
}

}
