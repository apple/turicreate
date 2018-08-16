#include "tests/MLModelTests.hpp"
#include <iostream>

// TODO -- Make these macros
int main(int, char *[]) {
    int result = 0;
#define MLMODEL_RUN_TEST(t) result += t();
#include "tests/MLModelTests.hpp"
    if (result == 0) {
        std::cerr << "Passed all tests!" << std::endl;
    } else {
        std::cerr << "Failed " << result << " tests." << std::endl;
    }
    return result;
}
