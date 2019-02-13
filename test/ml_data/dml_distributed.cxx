#include<distributed/distributed_context.hpp>
#include<distributed/cluster_interface.hpp>
#include<parallel/lambda_omp.hpp>
#include<dlfcn.h>

// Unit test.
#include <cxxtest/TestSuite.h>

using namespace turi;

/**
 *  Check linear regression opt interface
*/
class distributed_ml_data_test : public CxxTest::TestSuite {
 public:
  std::shared_ptr<cluster_interface> cluster;
  void* lib_handle;

  static distributed_ml_data_test *createSuite() {

    auto t = new distributed_ml_data_test;

    // Standlone.
    // t->cluster = make_standalone_cluster("turi-cluster.conf");
    // t->cluster->set_option("startup_timeout", "150");
    // t->cluster->set_option("passive_mode", "1");

    // Inproc.
    t->cluster = make_local_inproc_cluster(4);
    t->cluster->start();

    create_distributed_context(t->cluster);

    // Get distributed context.
    auto& ctx = get_distributed_context();

    const char* lib = "./libmldatatest.so";

    ctx.register_shared_library(lib);
    t->lib_handle = dlopen(lib, RTLD_LOCAL | RTLD_NOW);

    return t;
  }

  static void destroySuite(distributed_ml_data_test *ts) {
    destroy_distributed_context();
    delete ts;
  }

  void run_test(size_t n, const std::string& run_string, const std::string& target_type, bool cat_sorted) {

    std::ostringstream ss;
    ss << "test_distributed_ml_data_" << n << "_" << run_string << "_" << target_type
       << "_withsort" << (cat_sorted ? "true" : "false");

    void (*test) (void) =
        (void (*)(void )) dlsym(lib_handle, ss.str().c_str());

    DASSERT_TRUE(test != nullptr);
    test();
  }

  void test_5_n_NONE_withsort_false() {
    run_test(5, "n", "NONE", false);
  }

  void test_5_b_NONE_withsort_false() {
    run_test(5, "b", "NONE", false);
  }

  void test_5_c_NONE_withsort_false() {
    run_test(5, "c", "NONE", false);
  }

  void test_5_C_NONE_withsort_false() {
    run_test(5, "C", "NONE", false);
  }

  void test_13_b_NONE_withsort_false() {
    run_test(13, "b", "NONE", false);
  }

  void test_13_bc_NONE_withsort_false() {
    run_test(13, "bc", "NONE", false);
  }

  void test_13_zc_NONE_withsort_false() {
    run_test(13, "zc", "NONE", false);
  }

  void test_30_C_NONE_withsort_false() {
    run_test(30, "C", "NONE", false);
  }

  void test_3000_C_NONE_withsort_false() {
    run_test(3000, "C", "NONE", false);
  }

  void test_100_Zc_NONE_withsort_false() {
    run_test(100, "Zc", "NONE", false);
  }

  void test_100_Cc_NONE_withsort_false() {
    run_test(100, "Cc", "NONE", false);
  }

  void test_1000_Zc_NONE_withsort_false() {
    run_test(1000, "Zc", "NONE", false);
  }

  void test_1000_bc_NONE_withsort_false() {
    run_test(1000, "bc", "NONE", false);
  }

  void test_1_bc_NONE_withsort_false() {
    run_test(1, "bc", "NONE", false);
  }

  void test_200_u_NONE_withsort_false() {
    run_test(200, "u", "NONE", false);
  }

  void test_200_d_NONE_withsort_false() {
    run_test(200, "d", "NONE", false);
  }

  void test_1000_cnv_NONE_withsort_false() {
    run_test(1000, "cnv", "NONE", false);
  }

  void test_1000_du_NONE_withsort_false() {
    run_test(1000, "du", "NONE", false);
  }

  void test_3_UDccccV_NONE_withsort_false() {
    run_test(3, "UDccccV", "NONE", false);
  }

  void test_10_Zcuvd_NONE_withsort_false() {
    run_test(10, "Zcuvd", "NONE", false);
  }

  void test_0_n_NUMERICAL_withsort_false() {
    run_test(0, "n", "NUMERICAL", false);
  }

  void test_5_n_NUMERICAL_withsort_false() {
    run_test(5, "n", "NUMERICAL", false);
  }

  void test_5_c_NUMERICAL_withsort_false() {
    run_test(5, "c", "NUMERICAL", false);
  }

  void test_5_b_NUMERICAL_withsort_false() {
    run_test(5, "b", "NUMERICAL", false);
  }

  void test_13_C_NUMERICAL_withsort_false() {
    run_test(13, "C", "NUMERICAL", false);
  }

  void test_13_b_NUMERICAL_withsort_false() {
    run_test(13, "b", "NUMERICAL", false);
  }

  void test_13_bc_NUMERICAL_withsort_false() {
    run_test(13, "bc", "NUMERICAL", false);
  }

  void test_13_zc_NUMERICAL_withsort_false() {
    run_test(13, "zc", "NUMERICAL", false);
  }

  void test_100_Zc_NUMERICAL_withsort_false() {
    run_test(100, "Zc", "NUMERICAL", false);
  }

  void test_100_Cc_NUMERICAL_withsort_false() {
    run_test(100, "Cc", "NUMERICAL", false);
  }

  void test_1000_Zc_NUMERICAL_withsort_false() {
    run_test(1000, "Zc", "NUMERICAL", false);
  }

  void test_1000_bc_NUMERICAL_withsort_false() {
    run_test(1000, "bc", "NUMERICAL", false);
  }

  void test_1_bc_NUMERICAL_withsort_false() {
    run_test(1, "bc", "NUMERICAL", false);
  }

  void test_200_u_NUMERICAL_withsort_false() {
    run_test(200, "u", "NUMERICAL", false);
  }

  void test_200_d_NUMERICAL_withsort_false() {
    run_test(200, "d", "NUMERICAL", false);
  }

  void test_1000_cnv_NUMERICAL_withsort_false() {
    run_test(1000, "cnv", "NUMERICAL", false);
  }

  void test_1000_du_NUMERICAL_withsort_false() {
    run_test(1000, "du", "NUMERICAL", false);
  }

  void test_3_UDccccV_NUMERICAL_withsort_false() {
    run_test(3, "UDccccV", "NUMERICAL", false);
  }

  void test_10_Zcuvd_NUMERICAL_withsort_false() {
    run_test(10, "Zcuvd", "NUMERICAL", false);
  }

  void test_1000_n_NUMERICAL_withsort_false() {
    run_test(1000, "n", "NUMERICAL", false);
  }

  void test_0_n_CATEGORICAL_withsort_false() {
    run_test(0, "n", "CATEGORICAL", false);
  }

  void test_5_n_CATEGORICAL_withsort_false() {
    run_test(5, "n", "CATEGORICAL", false);
  }

  void test_5_c_CATEGORICAL_withsort_false() {
    run_test(5, "c", "CATEGORICAL", false);
  }

  void test_5_b_CATEGORICAL_withsort_false() {
    run_test(5, "b", "CATEGORICAL", false);
  }

  void test_13_C_CATEGORICAL_withsort_false() {
    run_test(13, "C", "CATEGORICAL", false);
  }

  void test_13_b_CATEGORICAL_withsort_false() {
    run_test(13, "b", "CATEGORICAL", false);
  }

  void test_13_bc_CATEGORICAL_withsort_false() {
    run_test(13, "bc", "CATEGORICAL", false);
  }

  void test_13_zc_CATEGORICAL_withsort_false() {
    run_test(13, "zc", "CATEGORICAL", false);
  }

  void test_100_Zc_CATEGORICAL_withsort_false() {
    run_test(100, "Zc", "CATEGORICAL", false);
  }

  void test_100_Cc_CATEGORICAL_withsort_false() {
    run_test(100, "Cc", "CATEGORICAL", false);
  }

  void test_1000_Zc_CATEGORICAL_withsort_false() {
    run_test(1000, "Zc", "CATEGORICAL", false);
  }

  void test_1000_bc_CATEGORICAL_withsort_false() {
    run_test(1000, "bc", "CATEGORICAL", false);
  }

  void test_1_bc_CATEGORICAL_withsort_false() {
    run_test(1, "bc", "CATEGORICAL", false);
  }

  void test_200_u_CATEGORICAL_withsort_false() {
    run_test(200, "u", "CATEGORICAL", false);
  }

  void test_200_d_CATEGORICAL_withsort_false() {
    run_test(200, "d", "CATEGORICAL", false);
  }

  void test_1000_cnv_CATEGORICAL_withsort_false() {
    run_test(1000, "cnv", "CATEGORICAL", false);
  }

  void test_1000_du_CATEGORICAL_withsort_false() {
    run_test(1000, "du", "CATEGORICAL", false);
  }

  void test_3_UDccccV_CATEGORICAL_withsort_false() {
    run_test(3, "UDccccV", "CATEGORICAL", false);
  }

  void test_10_Zcuvd_CATEGORICAL_withsort_false() {
    run_test(10, "Zcuvd", "CATEGORICAL", false);
  }

  void test_1000_n_CATEGORICAL_withsort_false() {
    run_test(1000, "n", "CATEGORICAL", false);
  }

  void test_5_n_NONE_withsort_true() {
    run_test(5, "n", "NONE", true);
  }

  void test_5_b_NONE_withsort_true() {
    run_test(5, "b", "NONE", true);
  }

  void test_5_c_NONE_withsort_true() {
    run_test(5, "c", "NONE", true);
  }

  void test_5_C_NONE_withsort_true() {
    run_test(5, "C", "NONE", true);
  }

  void test_13_b_NONE_withsort_true() {
    run_test(13, "b", "NONE", true);
  }

  void test_13_bc_NONE_withsort_true() {
    run_test(13, "bc", "NONE", true);
  }

  void test_13_zc_NONE_withsort_true() {
    run_test(13, "zc", "NONE", true);
  }

  void test_30_C_NONE_withsort_true() {
    run_test(30, "C", "NONE", true);
  }

  void test_3000_C_NONE_withsort_true() {
    run_test(3000, "C", "NONE", true);
  }

  void test_100_Zc_NONE_withsort_true() {
    run_test(100, "Zc", "NONE", true);
  }

  void test_100_Cc_NONE_withsort_true() {
    run_test(100, "Cc", "NONE", true);
  }

  void test_1000_Zc_NONE_withsort_true() {
    run_test(1000, "Zc", "NONE", true);
  }

  void test_1000_bc_NONE_withsort_true() {
    run_test(1000, "bc", "NONE", true);
  }

  void test_1_bc_NONE_withsort_true() {
    run_test(1, "bc", "NONE", true);
  }

  void test_200_u_NONE_withsort_true() {
    run_test(200, "u", "NONE", true);
  }

  void test_200_d_NONE_withsort_true() {
    run_test(200, "d", "NONE", true);
  }

  void test_1000_cnv_NONE_withsort_true() {
    run_test(1000, "cnv", "NONE", true);
  }

  void test_1000_du_NONE_withsort_true() {
    run_test(1000, "du", "NONE", true);
  }

  void test_3_UDccccV_NONE_withsort_true() {
    run_test(3, "UDccccV", "NONE", true);
  }

  void test_10_Zcuvd_NONE_withsort_true() {
    run_test(10, "Zcuvd", "NONE", true);
  }

  void test_0_n_NUMERICAL_withsort_true() {
    run_test(0, "n", "NUMERICAL", true);
  }

  void test_5_n_NUMERICAL_withsort_true() {
    run_test(5, "n", "NUMERICAL", true);
  }

  void test_5_c_NUMERICAL_withsort_true() {
    run_test(5, "c", "NUMERICAL", true);
  }

  void test_5_b_NUMERICAL_withsort_true() {
    run_test(5, "b", "NUMERICAL", true);
  }

  void test_13_C_NUMERICAL_withsort_true() {
    run_test(13, "C", "NUMERICAL", true);
  }

  void test_13_b_NUMERICAL_withsort_true() {
    run_test(13, "b", "NUMERICAL", true);
  }

  void test_13_bc_NUMERICAL_withsort_true() {
    run_test(13, "bc", "NUMERICAL", true);
  }

  void test_13_zc_NUMERICAL_withsort_true() {
    run_test(13, "zc", "NUMERICAL", true);
  }

  void test_100_Zc_NUMERICAL_withsort_true() {
    run_test(100, "Zc", "NUMERICAL", true);
  }

  void test_100_Cc_NUMERICAL_withsort_true() {
    run_test(100, "Cc", "NUMERICAL", true);
  }

  void test_1000_Zc_NUMERICAL_withsort_true() {
    run_test(1000, "Zc", "NUMERICAL", true);
  }

  void test_1000_bc_NUMERICAL_withsort_true() {
    run_test(1000, "bc", "NUMERICAL", true);
  }

  void test_1_bc_NUMERICAL_withsort_true() {
    run_test(1, "bc", "NUMERICAL", true);
  }

  void test_200_u_NUMERICAL_withsort_true() {
    run_test(200, "u", "NUMERICAL", true);
  }

  void test_200_d_NUMERICAL_withsort_true() {
    run_test(200, "d", "NUMERICAL", true);
  }

  void test_1000_cnv_NUMERICAL_withsort_true() {
    run_test(1000, "cnv", "NUMERICAL", true);
  }

  void test_1000_du_NUMERICAL_withsort_true() {
    run_test(1000, "du", "NUMERICAL", true);
  }

  void test_3_UDccccV_NUMERICAL_withsort_true() {
    run_test(3, "UDccccV", "NUMERICAL", true);
  }

  void test_10_Zcuvd_NUMERICAL_withsort_true() {
    run_test(10, "Zcuvd", "NUMERICAL", true);
  }

  void test_1000_n_NUMERICAL_withsort_true() {
    run_test(1000, "n", "NUMERICAL", true);
  }

  void test_0_n_CATEGORICAL_withsort_true() {
    run_test(0, "n", "CATEGORICAL", true);
  }

  void test_5_n_CATEGORICAL_withsort_true() {
    run_test(5, "n", "CATEGORICAL", true);
  }

  void test_5_c_CATEGORICAL_withsort_true() {
    run_test(5, "c", "CATEGORICAL", true);
  }

  void test_5_b_CATEGORICAL_withsort_true() {
    run_test(5, "b", "CATEGORICAL", true);
  }

  void test_13_C_CATEGORICAL_withsort_true() {
    run_test(13, "C", "CATEGORICAL", true);
  }

  void test_13_b_CATEGORICAL_withsort_true() {
    run_test(13, "b", "CATEGORICAL", true);
  }

  void test_13_bc_CATEGORICAL_withsort_true() {
    run_test(13, "bc", "CATEGORICAL", true);
  }

  void test_13_zc_CATEGORICAL_withsort_true() {
    run_test(13, "zc", "CATEGORICAL", true);
  }

  void test_100_Zc_CATEGORICAL_withsort_true() {
    run_test(100, "Zc", "CATEGORICAL", true);
  }

  void test_100_Cc_CATEGORICAL_withsort_true() {
    run_test(100, "Cc", "CATEGORICAL", true);
  }

  void test_1000_Zc_CATEGORICAL_withsort_true() {
    run_test(1000, "Zc", "CATEGORICAL", true);
  }

  void test_1000_bc_CATEGORICAL_withsort_true() {
    run_test(1000, "bc", "CATEGORICAL", true);
  }

  void test_1_bc_CATEGORICAL_withsort_true() {
    run_test(1, "bc", "CATEGORICAL", true);
  }

  void test_200_u_CATEGORICAL_withsort_true() {
    run_test(200, "u", "CATEGORICAL", true);
  }

  void test_200_d_CATEGORICAL_withsort_true() {
    run_test(200, "d", "CATEGORICAL", true);
  }

  void test_1000_cnv_CATEGORICAL_withsort_true() {
    run_test(1000, "cnv", "CATEGORICAL", true);
  }

  void test_1000_du_CATEGORICAL_withsort_true() {
    run_test(1000, "du", "CATEGORICAL", true);
  }

  void test_3_UDccccV_CATEGORICAL_withsort_true() {
    run_test(3, "UDccccV", "CATEGORICAL", true);
  }

  void test_10_Zcuvd_CATEGORICAL_withsort_true() {
    run_test(10, "Zcuvd", "CATEGORICAL", true);
  }

  void test_1000_n_CATEGORICAL_withsort_true() {
    run_test(1000, "n", "CATEGORICAL", true);
  }
};
