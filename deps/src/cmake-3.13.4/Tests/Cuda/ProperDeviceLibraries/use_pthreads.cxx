
#if defined(USE_THREADS_POSIX) && defined(HAVE_PTHREAD_H)

#  include <pthread.h>
static int verify_linking_to_pthread_cxx()
{
  return static_cast<int>(pthread_self());
}
#endif
