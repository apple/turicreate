#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <vector>
#include <set>
#include <string>
#include <random>

#include <core/storage/sframe_data/testing_utils.hpp>
#include <core/storage/sframe_interface/unity_sframe.hpp>
#include <toolkits/search/search_indexer.hpp>
#include <toolkits/search/sframe_to_sarray.hpp>
#include <toolkits/search/testing_utils.hpp>
#include <core/util/testing_utils.hpp>


using namespace turi::sdk_model::search;
using turi::search::query_rows;
using turi::search::write_rows_to_sarray;

void run_search_test(size_t num_trials = 3,
                     size_t num_docs = 100000,
                     size_t vocab_size = 1000,
                     size_t sentence_size = 50,
                     size_t word_size = 5) {
  logprogress_stream << std::endl
                     << "num_trials\t" << num_trials << std::endl
                     << "num docs\t"   << num_docs   << std::endl
                     << "vocab size\t" << vocab_size << std::endl
                     << "sentence size\t" << sentence_size << std::endl
                     << "word size\t"  << word_size << std::endl;

  auto vocab = random_vocab(vocab_size, word_size);
  gl_sframe data = create_synthetic(num_docs, sentence_size, vocab);

  // Define options
  std::map<std::string, flexible_type> options = {
    {"bm25_k1", 1.95},
    {"tfidf_threshold", 0.1}
  };

  // Train the model
  std::shared_ptr<search_model> model;
  model.reset(new search_model);
  model->init_options(options);
  model->index(data);

  std::vector<std::string> tokens = {};
  for (size_t i = 0; i < num_trials; ++i) {
    auto word_id = random::fast_uniform<size_t>(0, vocab.size()-1);
    tokens.push_back(vocab[word_id]);
  }

  std::cout << std::setw(20) << "Method"
            << std::setw(15) << "# results"
            << std::setw(10) << "seconds"
            << std::endl;
  timer ti;
  for (auto& method : {"sarray", "sarray_string", "join"}) {
    for (size_t i = 0; i < num_trials; ++i) {
      std::vector<std::string> query{{tokens[i]}};
      ti.start();
      auto query_result = model->query_index(query, 0.0, 0, 0.0);
			auto result = model->join_query_result(query_result, method, 10000000);
      double elapsed = ti.current_time();
      std::cout << std::setw(20) << method
                << std::setw(15) << result.size()
                << std::setw(10) << elapsed
                << std::endl;
    }
  }

  // Test save and load
  // Record thing to test
  std::map<std::string, flexible_type> _options;
  _options = model->get_current_options();

  // Save it
  dir_archive archive_write;
  archive_write.open_directory_for_write("search_cxx_test");
  turi::oarchive oarc(archive_write);
  oarc << *model;
  archive_write.close();

  // Load it
  std::shared_ptr<search_model> loaded_model;
  loaded_model.reset(new search_model);

  dir_archive archive_read;
  archive_read.open_directory_for_read("search_cxx_test");
  turi::iarchive iarc(archive_read);
  iarc >> *loaded_model;

  // Check that stuff in the loaded model is correct
  size_t observed_doc_count = variant_get_value<flexible_type>(
      model->get_value_from_state("num_documents"));
  TS_ASSERT(observed_doc_count == num_docs);
  for (auto& kvp: options){
    TS_ASSERT(_options[kvp.first] == kvp.second);
  }

  // Check that we can make queries with the loaded model.
  auto query = std::vector<std::string>({vocab[0]});  // just use the first word
	auto query_result = model->query_index(query, 0.0, 0, 0.0);
	auto result = model->join_query_result(query_result, "default", 10);
  // Need to make the synthetic data less random before we can assert this.
  // TS_ASSERT(result.size() > 0);
  //

}



/**
 *  Check search model
*/
struct search_test  {

 public:

   void test_search_basic_string() {
     //run_search_test(3, 100100, 100);
   }

   void debug_print(std::shared_ptr<search_model> model) {
     logprogress_stream << "Model: " << std::endl;
     model->print_index();
   }

   void debug_print(gl_sframe result) {
     auto res_sf = *(result.get_proxy()->get_underlying_sframe());
     res_sf.debug_print();
   }

   void test_small_data() {
     gl_sframe data{{"a", {"A a b c", "a b b c c", "a b"}},
       {"b", {"e f", "e e f g", "e f g h"}}};
     flex_list f{"a", "b"};
     std::map<std::string, flexible_type> options = {
       {"tfidf_threshold", 0.0}
     };
     std::shared_ptr<search_model> model;
     model.reset(new search_model);
     model->init_options(options);
     model->index(data);

     gl_sframe query_result;
     gl_sframe result;
     {
       std::vector<std::string> query = {"a"};
       query_result = model->query_index(query, 0.0, 0, 0.0);
       debug_print(query_result);
       result = model->join_query_result(query_result, "default", 10);
       debug_print(result);
       TS_ASSERT(result.size() == 3);
     }
   }

   void test_missing_data() {
     gl_sframe data{{"a", {"11", "12", "13", "14", "15"}},
       {"b", {"1", "2", flex_undefined(), "4", "5"}}};
     flex_list f{"b"};
     std::map<std::string, flexible_type> options = {
       {"tfidf_threshold", 0.0}
     };
     std::shared_ptr<search_model> model;
     model.reset(new search_model);
     model->init_options(options);
     model->index(data);
     std::vector<std::string> query = {"2"};
     gl_sframe query_result;
     gl_sframe result;
     {
       query_result = model->query_index(query, 0.0, 0, 0.0);
       debug_print(query_result);
       result = model->join_query_result(query_result, "default", 10);
       TS_ASSERT(result.size() == 1);
     }

     debug_print(query_result);
     debug_print(result);
   }

   void test_small_indexing() {
     size_t vocab_size = 20;
     size_t word_size = 5;
     std::vector<std::string> vocab = random_vocab(vocab_size, word_size);
     size_t num_docs = 10;
     size_t sentence_size = 5;
     gl_sframe data = create_synthetic(num_docs, sentence_size, vocab);

     // Define options so that we index all words.
     flex_list f = {"text"};
     std::map<std::string, flexible_type> options = {
       {"tfidf_threshold", 0.0},
       {"features", f}
     };

     std::shared_ptr<search_model> model;
     model.reset(new search_model);
     model->init_options(options);
     model->index(data);

     auto example_word = vocab[0];

     // Get an SFrame of row ids and the corresponding BM25 score
     std::vector<std::string> qtokens{example_word};
     gl_sframe query_result = model->query_index(qtokens, 0.0, 0, 0.0);
     auto res = model->join_query_result(query_result, "sarray_string", 10);
     debug_print(res);

   }

   void test_query_expansion() {

     // Set up vocabulary
     size_t vocab_size = 100;
     size_t word_size = 5;
     std::vector<std::string> vocab = random_vocab(vocab_size, word_size);
     std::set<flexible_type> expected_expanded_queries = {"helllo", "ello", "shello"};
     for (auto w : expected_expanded_queries) {
       vocab.push_back(w);
     }

     // Set up synthetic text dataset
     size_t num_docs = 1000;
     size_t sentence_size = 10;
     gl_sframe data = create_synthetic(num_docs, sentence_size, vocab);

     // Define options so that we index all words.
     flex_list f = {"text"};
     std::map<std::string, flexible_type> options = {
       {"tfidf_threshold", 0.0},
       {"features", f}
     };

     // ACT
     // Train the model
     std::shared_ptr<search_model> model;
     model.reset(new search_model);
     model->init_options(options);
     model->index(data);

     // Run query expansion
     auto query_token = "hello";
     size_t k = 5;
     double epsilon = .99;
     auto expanded = model->expand_query_token(query_token, k, epsilon);

     // ASSERT
     std::set<flexible_type> observed_expanded_queries;
     std::cout << "Expanding " << query_token << ": ";
     for (const auto& w : expanded) {
       std::cout << w << " ";
       observed_expanded_queries.insert(w);
     }
     std::cout << std::endl;
     TS_ASSERT(observed_expanded_queries == expected_expanded_queries);

     // Debug print
     std::vector<std::string> q{"hello"};

     auto query_result = model->query_index(q, 0.0, 0, 0.0);
     auto result = model->join_query_result(query_result, "sarray_string", 10);
     debug_print(result);

   }

   void test_bench_tradeoff() {
     size_t num_docs = 100000;
     size_t sentence_size = 30;
     auto vocab = random_vocab(1000, 5);

     gl_sframe data = create_synthetic(num_docs, sentence_size, vocab);
     gl_sarray packed = write_rows_to_sarray(data);

     std::vector<size_t> query_sizes = {{10, 50, 100, 500, 750, 1000, 2000, 3000, 5000}};
     std::cout << "Query size" << "\t"
               << "Join time (s)" << "\t"
               << "Random access time (s)" << std::endl;

     for (size_t query_size : query_sizes) {

       // Get row ids in various formats
       std::vector<size_t> ixs = sample_row_ids(data.size() - 1, query_size);
       std::vector<flexible_type> ixs_flex(ixs.begin(), ixs.end());
       gl_sarray ixs_sarray(ixs_flex);

       gl_sframe result;

       timer ti;
       ti.start();
       result = query_rows(data, packed, ixs_sarray);
       auto sarray_time = ti.current_time();

       gl_sframe query;
       query["_id"] = ixs_sarray;
       ti.start();
       std::vector<std::string> joinkeys = {"_id"};
       result = data.join(query,  joinkeys);
       auto join_time = ti.current_time();

       std::cout << query_size << "\t"
                 << join_time << "\t"
                 << sarray_time << std::endl;
     }
   }
};

BOOST_FIXTURE_TEST_SUITE(_search_test, search_test)
BOOST_AUTO_TEST_CASE(test_search_basic_string) {
  search_test::test_search_basic_string();
}
BOOST_AUTO_TEST_CASE(test_small_data) {
  search_test::test_small_data();
}
BOOST_AUTO_TEST_CASE(test_missing_data) {
  search_test::test_missing_data();
}
BOOST_AUTO_TEST_CASE(test_small_indexing) {
  search_test::test_small_indexing();
}
BOOST_AUTO_TEST_CASE(test_query_expansion) {
  search_test::test_query_expansion();
}
BOOST_AUTO_TEST_CASE(test_bench_tradeoff) {
  search_test::test_bench_tradeoff();
}
BOOST_AUTO_TEST_SUITE_END()
