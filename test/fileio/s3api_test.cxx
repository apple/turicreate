#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <core/storage/fileio/s3_api.hpp>
BOOST_TEST_DONT_PRINT_LOG_VALUE(turi::s3url)
using namespace turi;
struct s3api_test {
 public:


  void test_parse_s3url(void) {

    s3url out; // {access_key_id, secret_key, bucket, object_name, endpoint}
    s3url expected;

    out = {};
    std::string err_msg;
    TS_ASSERT(parse_s3url("s3://::foo/bar", out, err_msg));
    expected = {"", "", "foo", "bar", ""};
    BOOST_TEST(out == expected);

    out = {};
    TS_ASSERT(parse_s3url("s3://id:key:foo/bar", out, err_msg));
    expected = {"id", "key", "foo", "bar", ""};
    BOOST_TEST(out == expected);

    out = {};
    TS_ASSERT(parse_s3url("s3://id:key:s3.amazonaws.com/foo/bar", out, err_msg));
    expected = {"id", "key", "foo", "bar", "s3.amazonaws.com"};
    BOOST_TEST(out == expected);

    out = {};
    TS_ASSERT(parse_s3url("s3://id:key:s3.amazonaws.com/foo.123.xyz-pikachu/1:::,/2'/3\\/4", out, err_msg));
    expected = {"id", "key", "foo.123.xyz-pikachu", "1:::,/2'/3\\/4", "s3.amazonaws.com"};
    BOOST_TEST(out == expected);

    out = {};
    TS_ASSERT(
        parse_s3url("s3://id:key:gl-rv-test/psone_logs/"
                    "2014-12-11T18:40:40.Roberts-MacBook-Pro.local_server.log",
                    out, err_msg));
    expected = {
        "id", "key", "gl-rv-test",
        "psone_logs/2014-12-11T18:40:40.Roberts-MacBook-Pro.local_server.log",
        ""};
    BOOST_TEST(out == expected);

    // missing both id and key
    TS_ASSERT(!parse_s3url("s3://foo/bar", out, err_msg));

    // missing one of the id and key
    TS_ASSERT(!parse_s3url("s3://key:foo/bar", out, err_msg));

    // bad bucket name
    TS_ASSERT(parse_s3url("s3://::AAA/bar", out, err_msg)); // capital letter
    TS_ASSERT(!parse_s3url("s3://::abc-/bar", out, err_msg)); // hyphen end
    TS_ASSERT(!parse_s3url("s3://::-abc/bar", out, err_msg)); // hyphen begin
    TS_ASSERT(!parse_s3url("s3://::a./bar", out, err_msg)); // end dot
    TS_ASSERT(!parse_s3url("s3://::.a/bar", out, err_msg)); // begin dot
    TS_ASSERT(!parse_s3url("s3://::a/bar", out, err_msg)); // too short
    TS_ASSERT(!parse_s3url("s3://::10.10.10.10/bar", out, err_msg)); // ip address
    TS_ASSERT(parse_s3url("s3://::Turi-Dataset/bar", out, err_msg)); // hyphen end
  }
};

BOOST_FIXTURE_TEST_SUITE(_s3api_test, s3api_test)
BOOST_AUTO_TEST_CASE(test_parse_s3url) {
  s3api_test::test_parse_s3url();
}
BOOST_AUTO_TEST_SUITE_END()
