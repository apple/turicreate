#include <boost/filesystem.hpp>
#include <boost/thread.hpp>

namespace {

boost::mutex m1;
boost::recursive_mutex m2;

void threadmain()
{
  boost::lock_guard<boost::mutex> lock1(m1);
  boost::lock_guard<boost::recursive_mutex> lock2(m2);

  boost::filesystem::path p(boost::filesystem::current_path());
}
}

int main()
{
  boost::thread foo(threadmain);
  foo.join();

  return 0;
}
