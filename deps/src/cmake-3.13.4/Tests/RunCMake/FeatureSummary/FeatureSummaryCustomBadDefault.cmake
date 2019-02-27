include(FeatureSummary)
set_property(GLOBAL PROPERTY FeatureSummary_PKG_TYPES TYPE1 TYPE2 TYPE3)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})

find_package(Foo)

feature_summary(WHAT ALL)
