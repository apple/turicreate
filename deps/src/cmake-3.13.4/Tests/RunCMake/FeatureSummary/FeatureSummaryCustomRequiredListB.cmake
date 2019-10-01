include(FeatureSummary)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})

set_property(GLOBAL PROPERTY FeatureSummary_PKG_TYPES TYPE1 TYPE2 TYPE3)
set_property(GLOBAL PROPERTY FeatureSummary_REQUIRED_PKG_TYPES TYPE2 TYPE3)
set_property(GLOBAL PROPERTY FeatureSummary_DEFAULT_PKG_TYPE TYPE1)

find_package(Foo)
find_package(Bar)

set_package_properties(Foo PROPERTIES TYPE TYPE3)
feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

set_package_properties(Bar PROPERTIES TYPE TYPE3)
feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
