include(FeatureSummary)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})

find_package(Foo)

# Purpose not set
feature_summary(WHAT ALL)

# Purpose set once
set_package_properties(Foo PROPERTIES PURPOSE "Because everyone needs some Foo.")
feature_summary(WHAT ALL)

# Purpose set twice
set_package_properties(Foo PROPERTIES PURPOSE "Because Foo is better than Bar.")
feature_summary(WHAT ALL)
