#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <core/storage/fileio/sanitize_url.hpp>

using namespace turi;

struct sanitize_url_test {
 public:
  void test_sanitize_url(void) {
    TS_ASSERT_EQUALS(sanitize_url("http://www.google.com"), "http://www.google.com");
    TS_ASSERT_EQUALS(sanitize_url("file://www.google.com"), "file://www.google.com");
    TS_ASSERT_EQUALS(sanitize_url("hdfs://hello:world@www.google.com"), "hdfs://hello:world@www.google.com");
    TS_ASSERT_EQUALS(sanitize_url("s3://aa:pika/chu"), "s3://pika/chu");
    TS_ASSERT_EQUALS(sanitize_url("s3://aa:bb:pika/chu"), "s3://pika/chu");
    TS_ASSERT_EQUALS(sanitize_url("s3://aa:bb:s3.amazonaws.com/pika/chu"), "s3://s3.amazonaws.com/pika/chu");
    TS_ASSERT_EQUALS(sanitize_url("s3://a/a:bb:cc:pika/chu"), "s3://pika/chu");
    TS_ASSERT_EQUALS(sanitize_url("s3://a/a:bb:cc:s3.amazonaws.com/pika/chu"), "s3://s3.amazonaws.com/pika/chu");
    TS_ASSERT_EQUALS(sanitize_url("s3://a/a:b/b:cc:pika/chu"), "s3://pika/chu");
    TS_ASSERT_EQUALS(sanitize_url("s3://a/a:b/b:cc:s3.amazonaws.com/pika/chu"), "s3://s3.amazonaws.com/pika/chu");
    TS_ASSERT_EQUALS(sanitize_url("s3://:pika/chu"), "s3://pika/chu");
    TS_ASSERT_EQUALS(sanitize_url("s3://:s3.amazonaws.com/pika/chu"), "s3://s3.amazonaws.com/pika/chu");
    TS_ASSERT_EQUALS(sanitize_url("s3://:::pika/chu"), "s3://pika/chu");
    TS_ASSERT_EQUALS(sanitize_url("s3://:::s3.amazonaws.com/pika/chu"), "s3://s3.amazonaws.com/pika/chu");
  }
};

BOOST_FIXTURE_TEST_SUITE(_sanitize_url_test, sanitize_url_test)
BOOST_AUTO_TEST_CASE(test_sanitize_url) {
  sanitize_url_test::test_sanitize_url();
}
BOOST_AUTO_TEST_SUITE_END()
