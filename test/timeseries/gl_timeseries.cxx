#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <core/data/sframe/gl_sarray.hpp>
#include <core/data/sframe/gl_sframe.hpp>
#include <unity/extensions/timeseries/timeseries.hpp>
#include <unity/extensions/timeseries/registration.hpp>

using namespace turi;
using namespace turi::timeseries;

const size_t MINUTE = 60;
const size_t HOUR   = 3600;
const size_t DAY    = 86400;

struct gl_timeseries_test  {

  public:

    void test_basic_resample_all_aggregates() {

      gl_sframe sf;
      sf["index"] = gl_sarray({
          "20-Oct-2011 00:00:00",
          "20-Oct-2011 06:00:00",
          "21-Oct-2011 00:00:00",
          "21-Oct-2011 06:00:00",
          "22-Oct-2011 00:00:00",
          "22-Oct-2011 06:00:00",
          "23-Oct-2011 00:00:00",
          "23-Oct-2011 06:00:00",
          "24-Oct-2011 00:00:00",
          "24-Oct-2011 06:00:00"
          }).str_to_datetime("%d-%b-%Y %H:%M:%S");
      sf["a"] = gl_sarray({20, 21, 22, 23, 24, 25, 26, 27, 28, 29});
      sf["b"] = gl_sarray({0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
      sf["c"] = gl_sarray({0.1, 1.2, 2.3, 3.4, 4.5, 5.6, 6.7, 7.8, 8.9, 9.0});


      gl_timeseries ts;
      float period =  1 * DAY;
      ts.init(sf, "index"); 
      gl_timeseries ts_out = ts.resample(period,    
                     {{"a", aggregate::SUM("a")}, 
                      {"b", aggregate::SUM("b")}, 
                      {"c", aggregate::SUM("c")}}); 
      std::cout << ts_out.get_sframe() << std::endl;
      
      ts_out = ts.resample(period,   
                        {{"a", aggregate::AVG("a")}, 
                         {"b", aggregate::AVG("b")}, 
                         {"c", aggregate::AVG("c")}}); 
      std::cout << ts_out.get_sframe() << std::endl;
      
      ts_out = ts.resample(period,   
                        {{"a", aggregate::STD("a")}, 
                         {"b", aggregate::STD("b")}, 
                         {"c", aggregate::STD("c")}}); 
      std::cout << ts_out.get_sframe() << std::endl;
      
      ts_out = ts.resample(period,   
                        {{"a", aggregate::MIN("a")}, 
                         {"b", aggregate::MIN("b")}, 
                         {"c", aggregate::MIN("c")}}); 
      std::cout << ts_out.get_sframe() << std::endl;
      
      ts_out = ts.resample(period,   
                        {{"a", aggregate::MAX("a")}, 
                         {"b", aggregate::MAX("b")}, 
                         {"c", aggregate::MAX("c")}}); 
      std::cout << ts_out.get_sframe() << std::endl;
      
      ts_out = ts.resample(period,   
                        {{"a", aggregate::SELECT_ONE("a")}, 
                         {"b", aggregate::SELECT_ONE("b")}, 
                         {"c", aggregate::SELECT_ONE("c")}});
      std::cout << ts_out.get_sframe() << std::endl;
      
      ts_out = ts.resample(period,   
                        {{"a", aggregate::CONCAT("a")}, 
                         {"b", aggregate::CONCAT("b")}, 
                         {"c", aggregate::CONCAT("c")}}); 
      std::cout << ts_out.get_sframe() << std::endl;
      
      ts_out = ts.resample(period,   
                        {{"a", aggregate::COUNT_DISTINCT("a")}, 
                         {"b", aggregate::COUNT_DISTINCT("b")}, 
                         {"c", aggregate::COUNT_DISTINCT("c")}}); 
      std::cout << ts_out.get_sframe() << std::endl;

    }
    
    void test_resample_corner_cases() {

      gl_sframe sf;
      sf["index"] = gl_sarray({
          "20-Oct-2011 00:00:00",
          "20-Oct-2011 06:00:00",
          "21-Oct-2011 00:00:00",
          "21-Oct-2011 06:00:00",
          "22-Oct-2011 00:00:00",
          "22-Oct-2011 06:00:00",
          "23-Oct-2011 00:00:00",
          "23-Oct-2011 06:00:00",
          "24-Oct-2011 00:00:00",
          "24-Oct-2011 06:00:00"
          }).str_to_datetime("%d-%b-%Y %H:%M:%S");
      sf["a"] = gl_sarray({20, 21, 22, 23, 24, 25, 26, 27, 28, 29});
      sf["b"] = gl_sarray({0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
      sf["c"] = gl_sarray({0.1, 1.2, 2.3, 3.4, 4.5, 5.6, 6.7, 7.8, 8.9, 9.0});


      gl_timeseries ts;
      float period =  1 * DAY;

      // Only use the index.
      ts.init(sf[{"index"}], "index"); 
      gl_timeseries ts_out = ts.resample(period,    
              {{"", aggregate::COUNT()}});                     
      std::cout << ts_out.get_sframe() << std::endl;
    }

    void test_basic_resample_all_interpolation() {

      gl_sframe sf;
      sf["index"] = gl_sarray({
          "20-Oct-2011 00:00:00",
          "20-Oct-2011 06:00:00",
          "20-Oct-2011 12:00:00",
          "20-Oct-2011 18:00:00",
          "22-Oct-2011 00:00:00",
          "22-Oct-2011 06:00:00",
          "22-Oct-2011 12:00:00",
          "22-Oct-2011 18:00:00",
          "24-Oct-2011 00:00:00",
          "24-Oct-2011 06:00:00"
          }).str_to_datetime("%d-%b-%Y %H:%M:%S");
      sf["a"] = gl_sarray({20, 21, 22, 23, 24, 25, 26, 27, 28, 29});
      sf["b"] = gl_sarray({0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
      sf["c"] = gl_sarray({0.1, 1.2, 2.3, 3.4, 4.5, 5.6, 6.7, 7.8, 8.9, 9.0});


      gl_timeseries ts;
      float period =  1 * DAY;
      ts.init(sf, "index"); 
      gl_timeseries ts_out = ts.resample(period,    
                     {{"a", aggregate::SUM("a")}, 
                      {"b", aggregate::SUM("b")}, 
                      {"c", aggregate::SUM("c")}},
                     get_builtin_interpolator("__builtin__none__")); 
      std::cout << ts_out.get_sframe() << std::endl;
      
     ts_out = ts.resample(period,    
                     {{"a", aggregate::SUM("a")}, 
                      {"b", aggregate::SUM("b")}, 
                      {"c", aggregate::SUM("c")}},
                     get_builtin_interpolator("__builtin__zero__")); 
      std::cout << ts_out.get_sframe() << std::endl;
     
      ts_out = ts.resample(period,    
                     {{"a", aggregate::SUM("a")}, 
                      {"b", aggregate::SUM("b")}, 
                      {"c", aggregate::SUM("c")}},
                     get_builtin_interpolator("__builtin__ffill__")); 
      std::cout << ts_out.get_sframe() << std::endl;
                
      ts_out = ts.resample(period,    
                     {{"a", aggregate::SUM("a")}, 
                      {"b", aggregate::SUM("b")}, 
                      {"c", aggregate::SUM("c")}},
                     get_builtin_interpolator("__builtin__bfill__")); 
      std::cout << ts_out.get_sframe() << std::endl;
     
      ts_out = ts.resample(period,    
                     {{"a", aggregate::SUM("a")}, 
                      {"b", aggregate::SUM("b")}, 
                      {"c", aggregate::SUM("c")}},
                     get_builtin_interpolator("__builtin__nearest__")); 
      std::cout << ts_out.get_sframe() << std::endl;
      
      ts_out = ts.resample(period,    
                     {{"a", aggregate::SUM("a")}, 
                      {"b", aggregate::SUM("b")}, 
                      {"c", aggregate::SUM("c")}},
                     get_builtin_interpolator("__builtin__linear__")); 
      std::cout << ts_out.get_sframe() << std::endl;
    }


    std::vector<flexible_type> _to_vec(gl_sarray sa) {
      std::vector<flexible_type> ret;
      for (auto& v: sa.range_iterator()) { ret.push_back(v); }
      return ret;
    }

    void _assert_sarray_equals(gl_sarray sa, const std::vector<flexible_type>& vec) {
      TS_ASSERT_EQUALS(sa.size(), vec.size());
      for (size_t i = 0; i < vec.size(); ++i) {
        TS_ASSERT_EQUALS(sa[i], vec[i]);
      }
    }
    
    void _assert_flexvec_equals(const std::vector<flexible_type>& sa, 
                                const std::vector<flexible_type>& sb) {
      TS_ASSERT_EQUALS(sa.size(), sb.size());
      for (size_t i = 0;i < sa.size() ;++i) {
        TS_ASSERT_EQUALS(sa[i], sb[i]);
      }
    }

    void _assert_sframe_equals(gl_sframe sa, gl_sframe sb) {
      TS_ASSERT_EQUALS(sa.size(), sb.size());
      TS_ASSERT_EQUALS(sa.num_columns(), sb.num_columns());
      auto a_cols = sa.column_names();
      auto b_cols = sb.column_names();
      std::sort(a_cols.begin(), a_cols.end());
      std::sort(b_cols.begin(), b_cols.end());
      for (size_t i = 0;i < a_cols.size(); ++i) {
        TS_ASSERT_EQUALS(a_cols[i], b_cols[i]);
      }
      sb = sb[sa.column_names()];
      for (size_t i = 0; i < sa.size(); ++i) {
        _assert_flexvec_equals(sa[i], sb[i]);
      }
    }
    
};


BOOST_FIXTURE_TEST_SUITE(_gl_timeseries_test, gl_timeseries_test)
BOOST_AUTO_TEST_CASE(test_basic_resample_all_aggregates) {
  gl_timeseries_test::test_basic_resample_all_aggregates();
}
BOOST_AUTO_TEST_CASE(test_resample_corner_cases) {
  gl_timeseries_test::test_resample_corner_cases();
}
BOOST_AUTO_TEST_CASE(test_basic_resample_all_interpolation) {
  gl_timeseries_test::test_basic_resample_all_interpolation();
}
BOOST_AUTO_TEST_SUITE_END()
