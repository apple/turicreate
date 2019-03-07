#ifndef _LOGGING_H_
#define _LOGGING_H_
#include <iostream>
struct null_stream {
  template<typename T>
  inline null_stream operator<<(T t) { return null_stream(); }
  inline null_stream operator<<(const char* a) { return null_stream(); }
  inline null_stream operator<<(std::ostream& (*f)(std::ostream&)) { return null_stream(); }
};
namespace google {
inline void InitGoogleLogging(...) {}

inline void InstallFailureSignalHandler(...) {}
}
#define LOG(...) std::cerr
#define LOG_EVERY_N(...) null_stream()
#define LOG_FIRST_N(...) null_stream()
#define LOG_IF(...) null_stream()
#define DLOG(...) null_stream()
#define DLOG_EVERY_N(...) null_stream()
#define DLOG_FIRST_N(...) null_stream()
#define DLOG_IF(...) null_stream()

#define CHECK(...) null_stream()
#define CHECK_NOTNULL(x) x
#define DCHECK(...) null_stream()
#define DCHECK_NOTNULL(x) x

#define INFO


#define DCHECK_EQ(val1, val2)  null_stream()
#define DCHECK_DELTA(val1, val2, delta) null_stream()
#define DCHECK_NE(val1, val2) null_stream()
#define DCHECK_LE(val1, val2) null_stream()
#define DCHECK_LT(val1, val2) null_stream()
#define DCHECK_GE(val1, val2) null_stream()
#define DCHECK_GT(val1, val2) null_stream()
#define DASSERT_TRUE(cond) null_stream()
#define DASSERT_FALSE(cond) null_stream()
#define DASSERT_EQ(val1, val2) null_stream()
#define DASSERT_DELTA(val1, val2, delta) null_stream()
#define DASSERT_NE(val1, val2) null_stream()
#define DASSERT_LE(val1, val2) null_stream()
#define DASSERT_LT(val1, val2) null_stream()
#define DASSERT_GE(val1, val2) null_stream()
#define DASSERT_GT(val1, val2) null_stream()

#define CHECK_EQ(val1, val2) null_stream()
#define CHECK_DELTA(val1, val2, delta) null_stream()
#define CHECK_NE(val1, val2) null_stream()
#define CHECK_LE(val1, val2) null_stream()
#define CHECK_LT(val1, val2) null_stream()
#define CHECK_GE(val1, val2) null_stream()
#define CHECK_GT(val1, val2) null_stream()
#define ASSERT_TRUE(cond) null_stream()
#define ASSERT_FALSE(cond) null_stream()
#define ASSERT_EQ(val1, val2) null_stream()
#define ASSERT_DELTA(val1, val2, delta) null_stream()
#define ASSERT_NE(val1, val2) null_stream()
#define ASSERT_LE(val1, val2) null_stream()
#define ASSERT_LT(val1, val2) null_stream()
#define ASSERT_GE(val1, val2) null_stream()
#define ASSERT_GT(val1, val2) null_stream()

#endif
