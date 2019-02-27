include(FeatureSummary)
set_property(GLOBAL PROPERTY FeatureSummary_PKG_TYPES TYPE1 TYPE2 TYPE3)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})

find_package(Foo)
find_package(Bar)
find_package(Baz)

set_package_properties(Foo PROPERTIES TYPE TYPE1)
set_package_properties(Bar PROPERTIES TYPE TYPE2)
set_package_properties(Baz PROPERTIES TYPE TYPE3)

feature_summary(WHAT ALL)

feature_summary(WHAT TYPE1_PACKAGES_FOUND
                INCLUDE_QUIET_PACKAGES
                QUIET_ON_EMPTY)
feature_summary(WHAT TYPE1_PACKAGES_NOT_FOUND
                INCLUDE_QUIET_PACKAGES
                QUIET_ON_EMPTY)
feature_summary(WHAT TYPE2_PACKAGES_FOUND
                INCLUDE_QUIET_PACKAGES
                QUIET_ON_EMPTY)
feature_summary(WHAT TYPE2_PACKAGES_NOT_FOUND
                INCLUDE_QUIET_PACKAGES
                QUIET_ON_EMPTY)
feature_summary(WHAT TYPE3_PACKAGES_FOUND
                INCLUDE_QUIET_PACKAGES
                QUIET_ON_EMPTY)
feature_summary(WHAT TYPE3_PACKAGES_NOT_FOUND
                INCLUDE_QUIET_PACKAGES
                QUIET_ON_EMPTY)

feature_summary(WHAT TYPE1_PACKAGES_FOUND
                INCLUDE_QUIET_PACKAGES
                QUIET_ON_EMPTY
                DESCRIPTION "TYPE1 pkgs found\n")
feature_summary(WHAT TYPE1_PACKAGES_NOT_FOUND
                INCLUDE_QUIET_PACKAGES
                QUIET_ON_EMPTY
                DESCRIPTION "TYPE1 pkgs not found\n")
feature_summary(WHAT TYPE2_PACKAGES_FOUND
                INCLUDE_QUIET_PACKAGES
                QUIET_ON_EMPTY
                DESCRIPTION "TYPE2 pkgs found\n")
feature_summary(WHAT TYPE2_PACKAGES_NOT_FOUND
                INCLUDE_QUIET_PACKAGES
                QUIET_ON_EMPTY
                DESCRIPTION "TYPE2 pkgs not found\n")
feature_summary(WHAT TYPE3_PACKAGES_FOUND
                INCLUDE_QUIET_PACKAGES
                QUIET_ON_EMPTY
                DESCRIPTION "TYPE3 pkgs found\n")
feature_summary(WHAT TYPE3_PACKAGES_NOT_FOUND
                INCLUDE_QUIET_PACKAGES
                QUIET_ON_EMPTY
                DESCRIPTION "TYPE3 pkgs not found\n")

feature_summary(WHAT TYPE1_PACKAGES_FOUND
                INCLUDE_QUIET_PACKAGES
                QUIET_ON_EMPTY
                DEFAULT_DESCRIPTION)
feature_summary(WHAT TYPE1_PACKAGES_NOT_FOUND
                INCLUDE_QUIET_PACKAGES
                QUIET_ON_EMPTY
                DEFAULT_DESCRIPTION)
feature_summary(WHAT TYPE2_PACKAGES_FOUND
                INCLUDE_QUIET_PACKAGES
                QUIET_ON_EMPTY
                DEFAULT_DESCRIPTION)
feature_summary(WHAT TYPE2_PACKAGES_NOT_FOUND
                INCLUDE_QUIET_PACKAGES
                QUIET_ON_EMPTY
                DEFAULT_DESCRIPTION)
feature_summary(WHAT TYPE3_PACKAGES_FOUND
                INCLUDE_QUIET_PACKAGES
                QUIET_ON_EMPTY
                DEFAULT_DESCRIPTION)
feature_summary(WHAT TYPE3_PACKAGES_NOT_FOUND
                INCLUDE_QUIET_PACKAGES
                QUIET_ON_EMPTY
                DEFAULT_DESCRIPTION)

set_property(GLOBAL PROPERTY FeatureSummary_TYPE1_DESCRIPTION "first type packages")
set_property(GLOBAL PROPERTY FeatureSummary_TYPE2_DESCRIPTION "second type packages")
set_property(GLOBAL PROPERTY FeatureSummary_TYPE3_DESCRIPTION "third type packages")

feature_summary(WHAT ALL)

feature_summary(WHAT TYPE1_PACKAGES_FOUND
                INCLUDE_QUIET_PACKAGES
                QUIET_ON_EMPTY)
feature_summary(WHAT TYPE1_PACKAGES_NOT_FOUND
                INCLUDE_QUIET_PACKAGES
                QUIET_ON_EMPTY)
feature_summary(WHAT TYPE2_PACKAGES_FOUND
                INCLUDE_QUIET_PACKAGES
                QUIET_ON_EMPTY)
feature_summary(WHAT TYPE2_PACKAGES_NOT_FOUND
                INCLUDE_QUIET_PACKAGES
                QUIET_ON_EMPTY)
feature_summary(WHAT TYPE3_PACKAGES_FOUND
                INCLUDE_QUIET_PACKAGES
                QUIET_ON_EMPTY)
feature_summary(WHAT TYPE3_PACKAGES_NOT_FOUND
                INCLUDE_QUIET_PACKAGES
                QUIET_ON_EMPTY)

feature_summary(WHAT TYPE1_PACKAGES_FOUND
                INCLUDE_QUIET_PACKAGES
                QUIET_ON_EMPTY
                DESCRIPTION "TYPE1 pkgs found\n")
feature_summary(WHAT TYPE1_PACKAGES_NOT_FOUND
                INCLUDE_QUIET_PACKAGES
                QUIET_ON_EMPTY
                DESCRIPTION "TYPE1 pkgs not found\n")
feature_summary(WHAT TYPE2_PACKAGES_FOUND
                INCLUDE_QUIET_PACKAGES
                QUIET_ON_EMPTY
                DESCRIPTION "TYPE2 pkgs found\n")
feature_summary(WHAT TYPE2_PACKAGES_NOT_FOUND
                INCLUDE_QUIET_PACKAGES
                QUIET_ON_EMPTY
                DESCRIPTION "TYPE2 pkgs not found\n")
feature_summary(WHAT TYPE3_PACKAGES_FOUND
                INCLUDE_QUIET_PACKAGES
                QUIET_ON_EMPTY
                DESCRIPTION "TYPE3 pkgs found\n")
feature_summary(WHAT TYPE3_PACKAGES_NOT_FOUND
                INCLUDE_QUIET_PACKAGES
                QUIET_ON_EMPTY
                DESCRIPTION "TYPE3 pkgs not found\n")

feature_summary(WHAT TYPE1_PACKAGES_FOUND
                INCLUDE_QUIET_PACKAGES
                QUIET_ON_EMPTY
                DEFAULT_DESCRIPTION)
feature_summary(WHAT TYPE1_PACKAGES_NOT_FOUND
                INCLUDE_QUIET_PACKAGES
                QUIET_ON_EMPTY
                DEFAULT_DESCRIPTION)
feature_summary(WHAT TYPE2_PACKAGES_FOUND
                INCLUDE_QUIET_PACKAGES
                QUIET_ON_EMPTY
                DEFAULT_DESCRIPTION)
feature_summary(WHAT TYPE2_PACKAGES_NOT_FOUND
                INCLUDE_QUIET_PACKAGES
                QUIET_ON_EMPTY
                DEFAULT_DESCRIPTION)
feature_summary(WHAT TYPE3_PACKAGES_FOUND
                INCLUDE_QUIET_PACKAGES
                QUIET_ON_EMPTY
                DEFAULT_DESCRIPTION)
feature_summary(WHAT TYPE3_PACKAGES_NOT_FOUND
                INCLUDE_QUIET_PACKAGES
                QUIET_ON_EMPTY
                DEFAULT_DESCRIPTION)
