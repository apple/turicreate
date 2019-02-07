#include <vector>
#include <string>
#include <random/random.hpp>
#include <unity/toolkits/ml_data_1/ml_data.hpp>
#include <unity/toolkits/ml_data_1/metadata.hpp>
#include <unity/toolkits/ml_data_1/sframe_index_mapping.hpp>
#include <unity/toolkits/ml_data_1/ml_data_iterator.hpp>
#include <timer/timer.hpp>
#include <sframe/sframe.hpp>

// The number of categories and the sizes to use for each of the
// modes.
const size_t n_categorical_few = 100;       // 'c'
const size_t n_categorical_many = 500000;   // 'C'
const size_t vector_size_small = 10;        // 'v'
const size_t vector_size_large = 200;       // 'V'
const size_t dict_size_small = 10;          // 'd'
const size_t dict_size_large = 200;         // 'D'

using namespace turi;
using namespace turi::v1;

/**  Runs a timing run on the data size; run the program to get the
 *  help messages on how to run it to report timings.
 *
 *  \param[in] n_obs The number of observations to run the timing on.
 *  \param[in] column_type_info A string with each character denoting
 *  one type of column.  The legend is as follows:
 *
 *     n:  numeric column.
 *     c:  categorical column with 100 categories.
 *     C:  categorical column with 1000000 categories.
 *     s:  categorical column with short string keys and 1000 categories.
 *     S:  categorical column with short string keys and 100000 categories.
 *     v:  numeric vector with 10 elements.
 *     V:  numeric vector with 1000 elements.
 *     u:  categorical set with up to 10 elements.
 *     U:  categorical set with up to 1000 elements.
 *     d:  dictionary with 10 entries.
 *     D:  dictionary with 100 entries.
 *
 */
using namespace turi;

void run_benchmark(size_t n_obs, std::string column_type_info) {

  sframe data;

  size_t num_columns = column_type_info.size();
  size_t n_threads = thread::cpu_count();

  std::vector<std::string> names;
  std::vector<flex_type_enum> types;
  std::vector<bool> is_categorical;

  names.resize(column_type_info.size());
  types.resize(column_type_info.size());
  is_categorical.resize(column_type_info.size());
  std::string full_string;

  ////////////////////////////////////////////////////////////////////////////////
  //  Set up the information lookups for each of the columns: type,
  //  whether it's categorical, and the description to print.
  //
  for(size_t cid = 0; cid < num_columns; cid++){

    names[cid] = std::string("C-") + std::to_string(cid + 1) +  column_type_info[cid];

    switch(column_type_info[cid]) {
      case 'n':
        types[cid] = flex_type_enum::FLOAT;
        is_categorical[cid] = false;
        full_string += "[numeric]";
        break;

      case 'c':
        types[cid] = flex_type_enum::INTEGER;
        is_categorical[cid] = true;
        full_string += std::string("[int-cat-") + std::to_string(n_categorical_few) + "]";
        break;
      case 'C':
        types[cid] = flex_type_enum::INTEGER;
        is_categorical[cid] = true;
        full_string += std::string("[int-cat-") + std::to_string(n_categorical_many) + "]";
        break;

      case 's':
        types[cid] = flex_type_enum::STRING;
        is_categorical[cid] = true;
        full_string += "[short-str-cat]";
        break;
      case 'S':
        types[cid] = flex_type_enum::STRING;
        is_categorical[cid] = true;
        full_string += "[long-str-cat]";
        break;

      case 'v':
        types[cid] = flex_type_enum::VECTOR;
        is_categorical[cid] = false;
        full_string += std::string("[vector-") + std::to_string(vector_size_small) + "]";
        break;
      case 'V':
        types[cid] = flex_type_enum::VECTOR;
        is_categorical[cid] = false;
        full_string += std::string("[vector-") + std::to_string(vector_size_large) + "]";
        break;

      case 'u':
        types[cid] = flex_type_enum::LIST;
        is_categorical[cid] = true;
        full_string += "[cat-set-<10]";
      case 'U':
        types[cid] = flex_type_enum::LIST;
        is_categorical[cid] = true;
        full_string += "[cat-set-<1000]";

      case 'd':
        types[cid] = flex_type_enum::DICT;
        is_categorical[cid] = true;
        full_string += std::string("[dict-") + std::to_string(dict_size_small) + "]";
        break;
      case 'D':
        types[cid] = flex_type_enum::DICT;
        is_categorical[cid] = true;
        full_string += std::string("[dict-") + std::to_string(dict_size_large) + "]";
        break;

    }
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Create the sframe with each of the columns as determined above.

  data.open_for_write(names, types, "", n_threads);

  random::seed(0);

  std::string base_string = "hdouaeacrgsidduhaaabtnuoe";

  std::cout << "Building SFrame." << std::endl;

  in_parallel([&](size_t thread_idx, size_t num_segments) {

    auto it_out = data.get_output_iterator(thread_idx);

    std::vector<flexible_type> row(column_type_info.size());

    for(size_t i = 0; i < n_obs / num_segments; ++i, ++it_out) {

      for(size_t c_idx = 0; c_idx < column_type_info.size(); ++c_idx) {
        switch(column_type_info[c_idx]){

          case 'n':
            row[c_idx] = random::fast_uniform<double>(0,1);
            break;
          case 'c':
            row[c_idx] = random::fast_uniform<size_t>(0, n_categorical_few);
            break;

          case 'C':
            row[c_idx] = random::fast_uniform<size_t>(0, n_categorical_many);
            break;

          case 's':
            row[c_idx] = std::to_string(random::fast_uniform<size_t>(0, n_categorical_few));
            break;

          case 'S':
            row[c_idx] = base_string + std::to_string(random::fast_uniform<size_t>(0, n_categorical_many));
            break;

          case 'v': {
            flex_vec v(vector_size_small);
            for(double& f : v)
              f = random::fast_uniform<double>(0,1);
            row[c_idx] = v;
            break;
          }

          case 'V': {
            flex_vec v(vector_size_large);
            for(double& f : v)
              f = random::fast_uniform<double>(0,1);
            row[c_idx] = v;
            break;
          }

          case 'u': {
            size_t s = random::fast_uniform<size_t>(0,10);
            flex_list v(s);
            for(flexible_type& f : v)
              f = random::fast_uniform<size_t>(0,n_categorical_few);
            row[c_idx] = v;
            break;
          }

          case 'U': {
            size_t s = random::fast_uniform<size_t>(0,1000);
            flex_list v(s);
            for(flexible_type& f : v)
              f = random::fast_uniform<size_t>(0,n_categorical_many);
            row[c_idx] = v;
            break;
          }

          case 'd': {
            flex_dict d(dict_size_small);

            for(std::pair<flexible_type,flexible_type>& p : d) {
              p.first  = random::fast_uniform<size_t>(0, dict_size_small*n_categorical_few);
              p.second = random::fast_uniform<double>(0,1);
            }

            row[c_idx] = d;
            break;
          }

          case 'D': {
            flex_dict d(dict_size_large);

            for(std::pair<flexible_type,flexible_type>& p : d) {
              p.first  = random::fast_uniform<size_t>(0, n_categorical_many);
              p.second = random::fast_uniform<double>(0,1);
            }

            row[c_idx] = d;
            break;
          }

          default:
            std::cerr << "Column type " << column_type_info[c_idx] << " not recognized; choose in ncCsSvVdD."
                      << std::endl;
            exit(1);
            break;
        }
      }

      *it_out = row;
    }
    });

  data.close();

  std::cout << "SFrame Built, beginning timings." << std::endl;
  std::cout << "Columns: " << full_string << std::endl;
  std::cout << "------------------------------------------------------------" << std::endl;

  ////////////////////////////////////////////////////////////////////////////////
  // Step 1:  Time the data indexing.

  timer tt1;

  tt1.start();

  std::vector<std::shared_ptr<v1::column_metadata> > metadata(column_type_info.size());

  for(size_t cid = 0; cid < num_columns; ++cid)
    metadata[cid].reset(new v1::column_metadata(names[cid], is_categorical[cid], types[cid]));

  ml_data mdata;
  mdata.metadata = metadata;
  mdata.fill(data);

  ml_data_iterator_initializer it_init(mdata);

  std::cerr << "Loading and indexing (" << column_type_info
            << "):                "
            << tt1.current_time_millis()
            << "ms." << std::endl;

  ////////////////////////////////////////////////////////////////////////////////
  // Step 2:  Time sequential iteration

  for(size_t attempt : {1, 2})
  {
    size_t common_value = 0;
    timer tt2;
    tt2.start();
    for(ml_data_iterator it(it_init); !it.done(); ++it) {
      std::vector<ml_data_entry> x;
      it.fill_observation(x, false);
      for(const ml_data_entry& v : x)
        common_value += (v.column_index + v.index + (v.value != 0));
    }

    std::cerr << "Non-parallel Iteration, try " << attempt
              << ":            "
              << tt2.current_time_millis()
              << "ms." << std::endl;

  }

  ////////////////////////////////////////////////////////////////////////////////
  // Step 3:  Time parallel iterations with the ml_data_entry vector.

  for(size_t attempt : {1, 2})
  {
    size_t common_value = 0;
    timer tt2;
    tt2.start();

    in_parallel([&](size_t thread_idx, size_t num_threads) {

        std::vector<ml_data_entry> x;

        for(ml_data_iterator it(it_init, thread_idx, num_threads); !it.done(); ++it) {
          it.fill_observation(x, false);
          for(const ml_data_entry& v : x)
            common_value += (v.column_index + v.index + (v.value != 0));
        }

      });

    std::cerr << "Parallel Iteration, try " << attempt
              << ", n_cpu = " << thread::cpu_count()
              << ":     "
              << tt2.current_time_millis()
              << "ms." << std::endl;
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Step 4:  Time parallel iterations with the Eigen sparse vector.

  for(size_t attempt : {1, 2})
  {
    timer tt2;
    tt2.start();
    std::vector<ml_data_entry> x;


    in_parallel([&](size_t thread_idx, size_t num_threads) {

        double cv = 0;
        Eigen::SparseVector<double> x;

        std::vector<size_t> indexes(metadata.size());

        size_t total_size = 0;
        for(size_t i = 0; i < indexes.size(); ++i) {
          indexes[i] = metadata[i]->column_size();
          total_size += indexes[i];
        }
        x.resize(total_size);

        for(ml_data_iterator it(it_init, thread_idx, num_threads); !it.done(); ++it) {
          it.fill_observation_global_index(x, indexes);
          cv += x.sum();
        }

      });

    std::cerr << "Parallel, SparseVector, try " << attempt
              << ", n_cpu = " << thread::cpu_count()  << ": "
              << tt2.current_time_millis()
              << "ms." << std::endl;
  }

}

int main(int argc, char **argv) {

  if(argc == 1) {
    std::cerr << "Call format: " << argv[0] << " <n_observations> [type_string: [ncCsSvVuUdD]+] \n"
              << "n:  numeric column.\n"
              << "c:  categorical column with 100 categories.\n"
              << "C:  categorical column with 1000000 categories.\n"
              << "s:  categorical column with short string keys and 1000 categories.\n"
              << "S:  categorical column with short string keys and 100000 categories.\n"
              << "v:  numeric vector with 10 elements.\n"
              << "V:  numeric vector with 1000 elements.\n"
              << "u:  categorical set with 10 elements.\n"
              << "U:  categorical set with 1000 elements.\n"
              << "d:  dictionary with 10 entries.\n"
              << "D:  dictionary with 100 entries.\n"
              << "\n Example: " << argv[0] << " 100000 ccn -- benchmarks 100000 row sframe with 3 columns, 2 categorical and 1 numeric."
              << std::endl;

    exit(1);
  } else if (argc == 2) {
    size_t n_obs   = std::atoi(argv[1]);

    run_benchmark(n_obs, "cc");
    run_benchmark(n_obs, "ncsvd");
  } else if (argc == 3) {
    size_t n_obs   = std::atoi(argv[1]);

    std::string column_type_info(argv[2]);
    run_benchmark(n_obs, column_type_info);
  }

  return 0;
}
