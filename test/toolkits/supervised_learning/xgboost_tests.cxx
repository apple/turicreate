#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>

#define XGBOOST_CUSTOMIZE_MSG_
#include <xgboost/src/io/simple_fmatrix-inl.hpp>
#include <xgboost/src/learner/learner-inl.hpp>
#include <xgboost/src/io/simple_dmatrix-inl.hpp>

using namespace xgboost;
using namespace xgboost::learner;
using namespace xgboost::io;

constexpr double DELTA = 1e-7;

namespace xgboost {
namespace utils {
  void HandleAssertError(const char *msg) {
    fprintf(stderr, "AssertError:%s\n", msg);
    exit(-1);
  }
  void HandleCheckError(const char *msg) {
    throw std::runtime_error(msg);
  }
  void HandlePrint(const char *msg) {
    printf("%s", msg);
  }
 }
}

struct decision_tree_test {

  void set_options(BoostLearner& model, std::string obj) {
    model.SetParam("eta", "1"); // learning_rate
    model.SetParam("max_depth", "1");
    model.SetParam("gamma", "0.0"); // min loss reduction
    model.SetParam("min_child_weight", "0.0"); // min child weight
    model.SetParam("lambda", "0.0"); // regularizer
    model.SetParam("objective", obj.c_str());
  }

 public:
  void test_regression() {
    DMatrixSimple data;
    data.info.labels = {-1, 1};
    data.AddRow({RowBatch::Entry(0, 1)});
    data.AddRow({RowBatch::Entry(0, -1)});

    BoostLearner gbm;
    set_options(gbm, "reg:linear");
    gbm.SetCacheData({&data});
    gbm.InitModel();
    gbm.CheckInit(&data);
    gbm.UpdateOneIter(0, data);

    std::vector<float> preds;
    bool output_margin = true;
    bool pred_leaf = false;
    gbm.Predict(data, output_margin, &preds, 0, pred_leaf);

    // Base scores (B): 0.5, 0.5
    // Gradients (G): 1.5, -0.5
    // Hessians (H): 1, 1
    // Leaf weights (W) = -G / H : -1.5, 0.5
    // Preds = B + learning_rate * W
    TS_ASSERT_DELTA(preds[0], -1.0, DELTA);
    TS_ASSERT_DELTA(preds[1], 1.0, DELTA);
  }

  void test_classifier() {
    DMatrixSimple data;
    data.info.labels = {0, 1};
    data.AddRow({RowBatch::Entry(0, 1)});
    data.AddRow({RowBatch::Entry(0, -1)});

    BoostLearner gbm;
    set_options(gbm, "binary:logistic");
    gbm.SetCacheData({&data});
    gbm.InitModel();
    gbm.CheckInit(&data);
    gbm.UpdateOneIter(0, data);

    std::vector<float> preds;
    bool output_margin = true;
    bool pred_leaf = false;
    gbm.Predict(data, output_margin, &preds, 0, pred_leaf);

    // Base scores (B): 0.0, 0.0
    // Gradients (G): 0.5, -0.5
    // Hessians (H): 0.25, 0.25
    // Leaf weights (W) = -G / H : -2, 2
    // Preds = B + learning_rate * W
    TS_ASSERT_DELTA(preds[0], -2.0, DELTA);
    TS_ASSERT_DELTA(preds[1], 2.0, DELTA);
  }

  void test_multiclass_classifier() {
    DMatrixSimple data;
    data.info.labels = {0, 1, 2};
    data.AddRow({RowBatch::Entry(0, 1)});
    data.AddRow({RowBatch::Entry(0, 0)});
    data.AddRow({RowBatch::Entry(0, -1)});

    BoostLearner gbm;
    set_options(gbm, "multi:softmax");
    gbm.SetParam("num_class", "3");
    gbm.SetParam("max_depth", "2");
    gbm.SetCacheData({&data});
    gbm.InitModel();
    gbm.CheckInit(&data);
    gbm.UpdateOneIter(0, data);

    std::vector<float> preds;
    bool output_margin = true;
    bool pred_leaf = false;
    gbm.Predict(data, output_margin, &preds, 0, pred_leaf);

    // Base scores (B): 0.5
    // Gradients (G): (-2/3, 1/3, 1/3), (1/3, -2/3, 1/3), (1/3, 1/3, -2/3)
    // Hessians (H): 0.444
    // Leaf weights (W) = -G / H : (1.5, -.75, -.75), (-.75, 1.5, -.75), ...
    // Preds = B + learning_rate * W
    TS_ASSERT_DELTA(preds[0], 2.0, DELTA);
    TS_ASSERT_DELTA(preds[1], -.25, DELTA);
    TS_ASSERT_DELTA(preds[2], -.25, DELTA);
    TS_ASSERT_DELTA(preds[3], -.25, DELTA);
    TS_ASSERT_DELTA(preds[4], 2.0, DELTA);
    TS_ASSERT_DELTA(preds[5], -.25, DELTA);
    TS_ASSERT_DELTA(preds[6], -.25, DELTA);
    TS_ASSERT_DELTA(preds[7], -.25, DELTA);
    TS_ASSERT_DELTA(preds[8], 2.0, DELTA);
  }
};

BOOST_FIXTURE_TEST_SUITE(_decision_tree_test, decision_tree_test)
BOOST_AUTO_TEST_CASE(test_regression) {
  decision_tree_test::test_regression();
}
BOOST_AUTO_TEST_CASE(test_classifier) {
  decision_tree_test::test_classifier();
}
BOOST_AUTO_TEST_CASE(test_multiclass_classifier) {
  decision_tree_test::test_multiclass_classifier();
}
BOOST_AUTO_TEST_SUITE_END()
