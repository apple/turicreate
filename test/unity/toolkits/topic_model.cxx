#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <util/test_macros.hpp>
#include <iostream>


#include <unistd.h>
#include <timer/timer.hpp>
#include <unity/lib/unity_sarray.hpp>
#include <unity/lib/flex_dict_view.hpp>
#include <unity/toolkits/text/topic_model.hpp>
#include <unity/toolkits/text/cgs.hpp>
#include <unity/toolkits/text/alias.hpp>

using namespace turi;
using namespace text;

struct topic_model_test {
  unity_sarray* sa;
  std::vector<flexible_type> vocab;
  size_t ROW_COUNT;
  size_t ELEMENT_COUNT;

 public:
  topic_model_test() {
    global_logger().set_log_level(LOG_FATAL);
    ROW_COUNT = 10;
    ELEMENT_COUNT = 5;
    setUp();
  }

  void setUp() {
    std::vector<flexible_type> vocab = {"a", "b", "c", "d"};

    // Construct synthetic daata
    std::vector<flexible_type> v(ROW_COUNT);
    for(size_t i = 0; i < ROW_COUNT; ++i) {
      std::vector<std::pair<flexible_type, flexible_type>> elem;

      for(size_t j = 0; j < vocab.size(); ++j) {
        elem.push_back(std::make_pair(vocab[j], i * j + 1));
      }
      v[i] = flexible_type(elem);
    }

    sa = new unity_sarray();
    sa->construct_from_vector(v, flex_type_enum::DICT);

    size_t i = 0;  // i is row, j is element index in each row
    sa->begin_iterator();
    while(true) {
      auto vec = sa->iterator_get_next(1);
      if (vec.size() == 0) break;

      flex_dict_view fdv = vec[0];
      size_t j = 0;
      for(auto iter = fdv.begin(); iter != fdv.end(); iter++) {
        std::pair<flexible_type, flexible_type> val = *iter;
        /* TS_ASSERT_EQUALS(val.second, i * j); */
        /* std::cout << "doc " << i << " token " << j */
        /*           << " (word,count) " << val.first << " " << val.second */
        /*           << std::endl; */
        j++;
      }
      i++;
    }
  }

  void create_large_example(size_t num_documents=30000,
                            size_t max_word_frequency=200,
                            size_t doc_length=150,
                            size_t vocab_size=30000) {

    // Construct synthetic daata
    std::vector<flexible_type> v(num_documents);
    std::vector<std::pair<flexible_type, flexible_type>> elem(doc_length);
    for(size_t i = 0; i < num_documents; ++i) {
      for(size_t j = 0; j < doc_length; ++j) {
        flexible_type word_id = random::fast_uniform<size_t>(0, vocab_size);
        size_t freq = random::fast_uniform<size_t>(1, max_word_frequency);
        elem[j] = std::make_pair((flex_string) word_id, freq);
      }
      /* std::cout << flexible_type(elem) << std::endl; */
      v[i] = flexible_type(elem);
    }

    sa = new unity_sarray();
    sa->construct_from_vector(v, flex_type_enum::DICT);
  }


  void test_example_properly_formed() {
    TS_ASSERT_EQUALS(sa->dtype(), flex_type_enum::DICT);
    TS_ASSERT_EQUALS(sa->size(), ROW_COUNT);
  }

  void test_eigen_basics() {
    Eigen::MatrixXd a = Eigen::MatrixXd::Zero(5, 5);
    a(1,1) = 5.0;
    TS_ASSERT_EQUALS(a(1,1), 5.0);

    Eigen::MatrixXd b = Eigen::MatrixXd::Zero(10, 5);
    b.topRows(5) = a;
    TS_ASSERT_EQUALS(b.rows(), 10);
    TS_ASSERT_EQUALS(b(1,1), 5.0);
  }

  void test_sparse_mat() {
    auto z = spmat(100);
    TS_ASSERT_EQUALS(z.num_rows(), 100);
    TS_ASSERT_EQUALS(z.get(0, 0), 0);
    TS_ASSERT_EQUALS(z.get_row(0).size(), 0);
    z.increment(5, 5, 20);
    TS_ASSERT_EQUALS(z.get(5, 5), 20);
    TS_ASSERT_EQUALS(z.get_row(5).size(), 1);
    z.increment(5, 5, -20);
    TS_ASSERT_EQUALS(z.get(5, 5), 0);
    z.trim(5);
    TS_ASSERT_EQUALS(z.get_row(5).size(), 0);
    z.increment(99, 0, 1);
    z.increment(99, 0, -1);
    z.increment(99, 2, 1);
    z.increment(99, 2, -1);
    z.trim(99);
    TS_ASSERT_EQUALS(z.get_row(99).size(), 0);

    Eigen::MatrixXi m;
    z.increment(2, 3, 1);
    m = z.as_matrix();
    TS_ASSERT_EQUALS(m.rows(), 100);
    TS_ASSERT_EQUALS(m.cols(), 4);
    TS_ASSERT_EQUALS(m(2, 3), 1);
    z.increment(80, 300, 1);
    m = z.as_matrix();
    TS_ASSERT_EQUALS(m.rows(), 100);
    TS_ASSERT_EQUALS(m.cols(), 301);
    TS_ASSERT_EQUALS(m(80, 300), 1);
  }

  void test_topic_model() {

     // Initialize topic model with the above SArray
    std::map<std::string, flexible_type> options;
    options["verbose"] = true;
    options["num_topics"] = 3;
    options["num_iterations"] = 500;
    options["print_interval"] = 1;
    options["num_burnin"] = 3;
    options["alpha"] = .1;
    options["beta"] = .01;
    auto dataset = sa->get_underlying_sarray();

    std::shared_ptr<cgs_topic_model> m;
    m.reset(new cgs_topic_model);
    m->init_options(options);
    m->train(dataset, true);

    // Test retrieval of most probable words per topic
    size_t num_words = 2;
    size_t topic_id = 0;
    auto top_words = m->get_topic(topic_id, num_words);
    TS_ASSERT_EQUALS(top_words.first.size(), num_words);
    TS_ASSERT_EQUALS(top_words.second.size(), num_words);
    for (auto w : top_words.first) {
      TS_ASSERT_EQUALS(w.get_type(), flex_type_enum::STRING);
    }

    // Test initialization from old topics
    m->set_topics(m->get_topics_matrix(), m->get_vocabulary(), 1000);
    m->train(dataset, true);
    TS_ASSERT(m->is_trained());

    // Make predictions
    size_t num_burnin = 10;
    auto pred = m->predict_gibbs(dataset, num_burnin);
    TS_ASSERT(m->is_trained());
    auto pred_counts = m->predict_counts(dataset, num_burnin);
    /* TS_ASSERT(pred_counts.rows() == dataset->size()); */

    // Test validation set
    m.reset(new cgs_topic_model);
    m->init_options(options);
    m->init_validation(dataset, dataset);
    m->train(dataset, true);
    TS_ASSERT(m->is_trained());

  }

  void test_alias_solver() {
    // Initialize topic model with the above SArray
    std::map<std::string, flexible_type> options;
    options["verbose"] = true;
    options["num_topics"] = 3;
    options["num_iterations"] = 5;
    options["print_interval"] = 1;
    options["num_burnin"] = 3;
    options["alpha"] = .1;
    options["beta"] = .01;
    auto dataset = sa->get_underlying_sarray();

    // Test alias method solver
    std::shared_ptr<alias_topic_model> model;
    model.reset(new alias_topic_model);
    model->init_options(options);
    model->train(dataset, true);

  }

  void test_alias_solver_on_large() {

    global_logger().set_log_level(LOG_PROGRESS);

    timer ti;
    ti.start();
    random::seed(0);
    create_large_example(5, 10, 10);
    /* std::cout << "Example created in " << ti.current_time() << std::endl; */

    // Initialize topic model with the above SArray
    std::map<std::string, flexible_type> options;
    options["verbose"] = false;
    options["num_topics"] = 100;
    options["num_iterations"] = 1;
    options["print_interval"] = 1;
    options["alpha"] = .1;
    options["beta"] = .01;
    auto dataset = sa->get_underlying_sarray();

    // Test alias method solver
    std::shared_ptr<alias_topic_model> model;
    model.reset(new alias_topic_model);
    model->init_options(options);
    model->init_validation(dataset, dataset);
    model->train(dataset, true);

  }


};

BOOST_FIXTURE_TEST_SUITE(_topic_model_test, topic_model_test)
BOOST_AUTO_TEST_CASE(test_example_properly_formed) {
  topic_model_test::test_example_properly_formed();
}
BOOST_AUTO_TEST_CASE(test_sparse_mat) {
  topic_model_test::test_sparse_mat();
}
BOOST_AUTO_TEST_CASE(test_topic_model) {
  topic_model_test::test_topic_model();
}
BOOST_AUTO_TEST_CASE(test_alias_solver) {
  topic_model_test::test_alias_solver();
}
BOOST_AUTO_TEST_CASE(test_alias_solver_on_large) {
  topic_model_test::test_alias_solver_on_large();
}
BOOST_AUTO_TEST_SUITE_END()
