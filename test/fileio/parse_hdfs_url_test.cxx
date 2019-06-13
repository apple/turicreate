#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <core/storage/fileio/fs_utils.hpp>

using namespace turi::fileio;

struct hdfs_parse_url_test {
 public:

  void test_default() { 
    auto input = "hdfs:///foo/bar/a.txt";
    auto expected = std::make_tuple(default_host, default_port, "/foo/bar/a.txt");
    auto actual = parse_hdfs_url(input);
    TS_ASSERT_EQUALS(std::get<0>(actual), std::get<0>(expected));
    TS_ASSERT_EQUALS(std::get<1>(actual), std::get<1>(expected));
    TS_ASSERT_EQUALS(std::get<2>(actual), std::get<2>(expected));
  }

  void test_hostname() {
    auto input = "hdfs://hostname/foo/bar/a.txt";
    auto expected = std::make_tuple("hostname", default_port, "/foo/bar/a.txt");
    auto actual = parse_hdfs_url(input);
    TS_ASSERT_EQUALS(std::get<0>(actual), std::get<0>(expected));
    TS_ASSERT_EQUALS(std::get<1>(actual), std::get<1>(expected));
    TS_ASSERT_EQUALS(std::get<2>(actual), std::get<2>(expected));
  }

  void test_hostname_and_port() { 
    auto input = "hdfs://hostname:9000/foo/bar/a.txt";
    auto expected = std::make_tuple("hostname", "9000", "/foo/bar/a.txt");
    auto actual = parse_hdfs_url(input);
    TS_ASSERT_EQUALS(std::get<0>(actual), std::get<0>(expected));
    TS_ASSERT_EQUALS(std::get<1>(actual), std::get<1>(expected));
    TS_ASSERT_EQUALS(std::get<2>(actual), std::get<2>(expected));
  }

  void test_ip_hostname() {
    auto input = "hdfs://10.10.10.10/foo/bar/a.txt";
    auto expected = std::make_tuple("10.10.10.10", default_port, "/foo/bar/a.txt");
    auto actual = parse_hdfs_url(input);
    TS_ASSERT_EQUALS(std::get<0>(actual), std::get<0>(expected));
    TS_ASSERT_EQUALS(std::get<1>(actual), std::get<1>(expected));
    TS_ASSERT_EQUALS(std::get<2>(actual), std::get<2>(expected));
  }

  void test_ip_hostname_and_port() {
    auto input = "hdfs://10.10.10.10:9000/foo/bar/a.txt";
    auto expected = std::make_tuple("10.10.10.10", "9000", "/foo/bar/a.txt");
    auto actual = parse_hdfs_url(input);
    TS_ASSERT_EQUALS(std::get<0>(actual), std::get<0>(expected));
    TS_ASSERT_EQUALS(std::get<1>(actual), std::get<1>(expected));
    TS_ASSERT_EQUALS(std::get<2>(actual), std::get<2>(expected));
  }

  void test_empty_exception() {
    auto input = "hdfs://a";
    auto actual = parse_hdfs_url(input);
    auto expected = default_expected;
    TS_ASSERT_EQUALS(std::get<0>(actual), std::get<0>(expected));
    TS_ASSERT_EQUALS(std::get<1>(actual), std::get<1>(expected));
    TS_ASSERT_EQUALS(std::get<2>(actual), std::get<2>(expected));
  }

  void test_bad_path_exception() {
    auto input = "hdfs://hostname:10000/foo:bar";
    auto expected = default_expected;
    auto actual = parse_hdfs_url(input);
    TS_ASSERT_EQUALS(std::get<0>(actual), std::get<0>(expected));
    TS_ASSERT_EQUALS(std::get<1>(actual), std::get<1>(expected));
    TS_ASSERT_EQUALS(std::get<2>(actual), std::get<2>(expected));
  }

  void test_bad_port_exception() {
    auto input = "hdfs://hostname:badport/foo/bar";
    auto expected = default_expected;
    auto actual = parse_hdfs_url(input);
    TS_ASSERT_EQUALS(std::get<0>(actual), std::get<0>(expected));
    TS_ASSERT_EQUALS(std::get<1>(actual), std::get<1>(expected));
    TS_ASSERT_EQUALS(std::get<2>(actual), std::get<2>(expected));
  }


  const std::string default_port = "0";
  const std::string default_host = "default";
  const std::tuple<std::string, std::string, std::string> default_expected = std::make_tuple(default_host, default_port, "");
};

BOOST_FIXTURE_TEST_SUITE(_hdfs_parse_url_test, hdfs_parse_url_test)
BOOST_AUTO_TEST_CASE(test_default) {
  hdfs_parse_url_test::test_default();
}
BOOST_AUTO_TEST_CASE(test_hostname) {
  hdfs_parse_url_test::test_hostname();
}
BOOST_AUTO_TEST_CASE(test_hostname_and_port) {
  hdfs_parse_url_test::test_hostname_and_port();
}
BOOST_AUTO_TEST_CASE(test_ip_hostname) {
  hdfs_parse_url_test::test_ip_hostname();
}
BOOST_AUTO_TEST_CASE(test_ip_hostname_and_port) {
  hdfs_parse_url_test::test_ip_hostname_and_port();
}
BOOST_AUTO_TEST_CASE(test_empty_exception) {
  hdfs_parse_url_test::test_empty_exception();
}
BOOST_AUTO_TEST_CASE(test_bad_path_exception) {
  hdfs_parse_url_test::test_bad_path_exception();
}
BOOST_AUTO_TEST_CASE(test_bad_port_exception) {
  hdfs_parse_url_test::test_bad_port_exception();
}
BOOST_AUTO_TEST_SUITE_END()
