include(FeatureSummary)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})

find_package(Foo)
find_package(Bar)
find_package(Baz)

set_package_properties(Foo PROPERTIES TYPE RUNTIME)
set_package_properties(Bar PROPERTIES TYPE OPTIONAL)
set_package_properties(Baz PROPERTIES TYPE REQUIRED)

feature_summary(WHAT ALL)

feature_summary(WHAT RUNTIME_PACKAGES_FOUND
                INCLUDE_QUIET_PACKAGES
                QUIET_ON_EMPTY)
feature_summary(WHAT RUNTIME_PACKAGES_NOT_FOUND
                INCLUDE_QUIET_PACKAGES
                QUIET_ON_EMPTY)
feature_summary(WHAT OPTIONAL_PACKAGES_FOUND
                INCLUDE_QUIET_PACKAGES
                QUIET_ON_EMPTY)
feature_summary(WHAT OPTIONAL_PACKAGES_NOT_FOUND
                INCLUDE_QUIET_PACKAGES
                QUIET_ON_EMPTY)
feature_summary(WHAT REQUIRED_PACKAGES_FOUND
                INCLUDE_QUIET_PACKAGES
                QUIET_ON_EMPTY)
feature_summary(WHAT REQUIRED_PACKAGES_NOT_FOUND
                INCLUDE_QUIET_PACKAGES
                QUIET_ON_EMPTY)

feature_summary(WHAT RUNTIME_PACKAGES_FOUND
                INCLUDE_QUIET_PACKAGES
                QUIET_ON_EMPTY
                DESCRIPTION "RUNTIME pkgs found\n")
feature_summary(WHAT RUNTIME_PACKAGES_NOT_FOUND
                INCLUDE_QUIET_PACKAGES
                QUIET_ON_EMPTY
                DESCRIPTION "RUNTIME pkgs not found\n")
feature_summary(WHAT OPTIONAL_PACKAGES_FOUND
                INCLUDE_QUIET_PACKAGES
                QUIET_ON_EMPTY
                DESCRIPTION "OPTIONAL pkgs found\n")
feature_summary(WHAT OPTIONAL_PACKAGES_NOT_FOUND
                INCLUDE_QUIET_PACKAGES
                QUIET_ON_EMPTY
                DESCRIPTION "OPTIONAL pkgs not found\n")
feature_summary(WHAT REQUIRED_PACKAGES_FOUND
                INCLUDE_QUIET_PACKAGES
                QUIET_ON_EMPTY
                DESCRIPTION "REQUIRED pkgs found\n")
feature_summary(WHAT REQUIRED_PACKAGES_NOT_FOUND
                INCLUDE_QUIET_PACKAGES
                QUIET_ON_EMPTY
                DESCRIPTION "REQUIRED pkgs not found\n")

feature_summary(WHAT RUNTIME_PACKAGES_FOUND
                INCLUDE_QUIET_PACKAGES
                QUIET_ON_EMPTY
                DEFAULT_DESCRIPTION)
feature_summary(WHAT RUNTIME_PACKAGES_NOT_FOUND
                INCLUDE_QUIET_PACKAGES
                QUIET_ON_EMPTY
                DEFAULT_DESCRIPTION)
feature_summary(WHAT OPTIONAL_PACKAGES_FOUND
                INCLUDE_QUIET_PACKAGES
                QUIET_ON_EMPTY
                DEFAULT_DESCRIPTION)
feature_summary(WHAT OPTIONAL_PACKAGES_NOT_FOUND
                INCLUDE_QUIET_PACKAGES
                QUIET_ON_EMPTY
                DEFAULT_DESCRIPTION)
feature_summary(WHAT REQUIRED_PACKAGES_FOUND
                INCLUDE_QUIET_PACKAGES
                QUIET_ON_EMPTY
                DEFAULT_DESCRIPTION)
feature_summary(WHAT REQUIRED_PACKAGES_NOT_FOUND
                INCLUDE_QUIET_PACKAGES
                QUIET_ON_EMPTY
                DEFAULT_DESCRIPTION)
