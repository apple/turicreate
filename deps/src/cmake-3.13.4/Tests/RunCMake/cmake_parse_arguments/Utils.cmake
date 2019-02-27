include(${CMAKE_CURRENT_LIST_DIR}/test_utils.cmake)

# test the TEST macro itself

TEST(asdf UNDEFINED)

SET (asdf FALSE)
TEST(asdf FALSE)

SET (asdf TRUE)
TEST(asdf TRUE)

SET (asdf TRUE)
TEST(asdf TRUE)

SET (asdf "some value")
TEST(asdf "some value")

SET (asdf some list)
TEST(asdf some list)
TEST(asdf "some;list")
