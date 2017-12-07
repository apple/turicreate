include(FeatureSummary)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})

find_package(Foo QUIET)
find_package(Bar QUIET)
find_package(Baz)

feature_summary(WHAT ALL)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES)
