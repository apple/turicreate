#include <gtest/gtest.h>

#include <string>

namespace {
bool shouldFail = false;
}

TEST(GoogleTest, LinksAndRuns)
{
  ASSERT_TRUE(true);
}

TEST(GoogleTest, ConditionalFail)
{
  ASSERT_FALSE(shouldFail);
}

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);

  if (argc > 1) {
    if (argv[1] != std::string("--forceFail")) {
      throw "Unexpected argument";
    }
    shouldFail = true;
  }
  return RUN_ALL_TESTS();
}
