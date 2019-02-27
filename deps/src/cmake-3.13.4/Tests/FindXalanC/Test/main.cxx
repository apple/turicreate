#include <xalanc/XalanTransformer/XalanTransformer.hpp>
#include <xercesc/util/PlatformUtils.hpp>

int main()
{
  xercesc::XMLPlatformUtils::Initialize();
  xalanc::XalanTransformer::initialize();
  xalanc::XalanTransformer::terminate();
  xercesc::XMLPlatformUtils::Terminate();
}
