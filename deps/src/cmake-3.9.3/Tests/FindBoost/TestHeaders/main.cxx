#include <boost/any.hpp>

int main()
{
  boost::any a;
  a = 5;
  a = std::string("A string");

  return 0;
}
