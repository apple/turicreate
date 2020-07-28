#define STR(s) #s
#define ML_ASSERT(x) { if (!(x)) { \
    std::cout << __FILE__ << ":" << __LINE__ << ": error: " << STR(x) << " was false, expected true." << std::endl; \
    return 1; \
} } \

#define ML_ASSERT_NOT(x) ML_ASSERT(!(x))

#define ML_ASSERT_GOOD(x) ML_ASSERT((x).good())
#define ML_ASSERT_BAD(x) ML_ASSERT(!((x).good()))
#define ML_ASSERT_BAD_WITH_REASON(x, r) ML_ASSERT_BAD((x)); ML_ASSERT_EQ((r), (x).reason())
#define ML_ASSERT_BAD_WITH_TYPE(x, t) ML_ASSERT_BAD((x)); ML_ASSERT_EQ((t), (x).type())
#define ML_ASSERT_EQ(x, y) ML_ASSERT((x) == (y))
#define ML_ASSERT_NE(x, y) ML_ASSERT((x) != (y))
#define ML_ASSERT_LT(x, y) ML_ASSERT((x) < (y))
#define ML_ASSERT_GT(x, y) ML_ASSERT((x) > (y))
#define ML_ASSERT_NULL(x) ML_ASSERT((x) == nullptr)
#define ML_ASSERT_NOT_NULL(x) ML_ASSERT((x) != nullptr)

#define ML_ASSERT_THROWS(expr, exType) { \
    bool caughtCorrectException = false; \
    try { expr; } \
    catch (const exType&) { caughtCorrectException = true; } \
    catch (...) { std::clog << __FILE__ << ":" << __LINE__ << ": error: caught unexpected exeception type.\n"; return 1;} \
    if (!caughtCorrectException) { std::clog << __FILE__ << ":" << __LINE__ << ": expected exception, but none thrown.\n"; return 1; } }
