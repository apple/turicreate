#include <gtest/gtest.h>

TEST(FindCMake, LinksAndRuns)
{
  using namespace testing;
  EXPECT_FALSE(GTEST_FLAG(list_tests));
  ASSERT_TRUE(true);
}
