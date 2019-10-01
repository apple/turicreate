# Set Qt test version and include the Autogen test macros
set(QT_TEST_VERSION 4)
include("../QtAutogen/TestMacros.cmake")

# Qt4 only tests
ADD_AUTOGEN_TEST(DefinesTest)

# Common tests
include("../QtAutogen/CommonTests.cmake")
