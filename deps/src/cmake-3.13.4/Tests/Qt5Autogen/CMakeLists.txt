# Set Qt test version and include the Autogen test macros
set(QT_TEST_VERSION 5)
include("../QtAutogen/TestMacros.cmake")

# Common tests
include("../QtAutogen/CommonTests.cmake")
