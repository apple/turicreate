#include <xercesc/util/PlatformUtils.hpp>

int main()
{
  xercesc::XMLPlatformUtils::Initialize();
  xercesc::XMLPlatformUtils::Terminate();
}
