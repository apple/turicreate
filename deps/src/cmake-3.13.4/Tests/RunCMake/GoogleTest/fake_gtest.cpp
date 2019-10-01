#include <iostream>
#include <string>

int main(int argc, char** argv)
{
  // Note: GoogleTest.cmake doesn't actually depend on Google Test as such;
  // it only requires that we produces output in the expected format when
  // invoked with --gtest_list_tests. Thus, we fake that here. This allows us
  // to test the module without actually needing Google Test.
  if (argc > 1 && std::string(argv[1]) == "--gtest_list_tests") {
    std::cout << "basic." << std::endl;
    std::cout << "  case_foo" << std::endl;
    std::cout << "  case_bar" << std::endl;
    std::cout << "  DISABLED_disabled_case" << std::endl;
    std::cout << "DISABLED_disabled." << std::endl;
    std::cout << "  case" << std::endl;
    std::cout << "typed/0.  # TypeParam = short" << std::endl;
    std::cout << "  case" << std::endl;
    std::cout << "typed/1.  # TypeParam = float" << std::endl;
    std::cout << "  case" << std::endl;
    std::cout << "value/test." << std::endl;
    std::cout << "  case/0  # GetParam() = 1" << std::endl;
    std::cout << "  case/1  # GetParam() = \"foo\"" << std::endl;
    return 0;
  }

  if (argc > 5) {
    // Simple test of EXTRA_ARGS
    if (std::string(argv[3]) == "how" && std::string(argv[4]) == "now" &&
        std::string(argv[5]) == "\"brown\" cow") {
      return 0;
    }
  }

  // Print arguments for debugging, if we didn't get the expected arguments
  for (int i = 1; i < argc; ++i) {
    std::cerr << "arg[" << i << "]: '" << argv[i] << "'\n";
  }

  return 1;
}
