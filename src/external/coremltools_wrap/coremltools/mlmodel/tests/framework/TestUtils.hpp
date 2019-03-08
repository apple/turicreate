#define STR(s) #s
#define ML_ASSERT(x) { if (!(x)) { \
    std::cout << __FILE__ << ":" << __LINE__ << ": error: " << STR(x) << " was false, expected true." << std::endl; \
    return 1; \
} } \

#define ML_ASSERT_GOOD(x) ML_ASSERT((x).good())
#define ML_ASSERT_BAD(x) ML_ASSERT(!((x).good()))
#define ML_ASSERT_EQ(x, y) ML_ASSERT((x) == (y))
#define ML_ASSERT_LT(x, y) ML_ASSERT((x) < (y))
#define ML_ASSERT_GT(x, y) ML_ASSERT((x) > (y))
