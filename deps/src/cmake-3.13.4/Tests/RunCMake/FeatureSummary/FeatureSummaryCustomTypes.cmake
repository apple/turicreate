include(FeatureSummary)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})

set_property(GLOBAL PROPERTY FeatureSummary_PKG_TYPES TYPE1 TYPE2 TYPE3)
set_property(GLOBAL PROPERTY FeatureSummary_REQUIRED_PKG_TYPES TYPE3)
set_property(GLOBAL PROPERTY FeatureSummary_DEFAULT_PKG_TYPE TYPE2)

find_package(Foo)

# Type not set => TYPE2
feature_summary(WHAT ALL)

# TYPE1 > not set => TYPE1
set_package_properties(Foo PROPERTIES TYPE TYPE1)
feature_summary(WHAT ALL)

# TYPE2 > TYPE1 => TYPE2
set_package_properties(Foo PROPERTIES TYPE TYPE2)
feature_summary(WHAT ALL)

# TYPE1 < TYPE2 => TYPE2
set_package_properties(Foo PROPERTIES TYPE TYPE2)
feature_summary(WHAT ALL)

# TYPE3 > TYPE2 => TYPE3
set_package_properties(Foo PROPERTIES TYPE TYPE3)
feature_summary(WHAT ALL)

# TYPE2 < TYPE3 => TYPE3
set_package_properties(Foo PROPERTIES TYPE TYPE2)
feature_summary(WHAT ALL)

# TYPE1 < TYPE3 => TYPE3
set_package_properties(Foo PROPERTIES TYPE TYPE1)
feature_summary(WHAT ALL)
