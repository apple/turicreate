#include <gtest/gtest.h>

TEST(GoogleTest, SomethingElse)
{
  ASSERT_TRUE(true);
}

TEST(GoogleTest, DISABLED_OffTest1)
{
  ASSERT_TRUE(true);
}

TEST(DISABLED_GoogleTest, OffTest2)
{
  ASSERT_TRUE(true);
}

TEST(DISABLED_GoogleTest, DISABLED_OffTest3)
{
  ASSERT_TRUE(true);
}
