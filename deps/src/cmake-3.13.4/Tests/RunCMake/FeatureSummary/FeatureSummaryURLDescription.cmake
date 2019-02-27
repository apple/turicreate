include(FeatureSummary)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})

find_package(Foo) # URL and DESCRIPTION are set in the FindFoo.cmake file
find_package(Bar) # URL and DESCRIPTION are not set
find_package(Baz) # URL and DESCRIPTION are not set

feature_summary(WHAT ALL)

set_package_properties(Bar PROPERTIES URL "https://bar.net/") # URL and no DESCRIPTION
set_package_properties(Baz PROPERTIES DESCRIPTION "A Baz package") # DESCRIPTION and no URL
feature_summary(WHAT ALL)

# Overwrite with the same value (no warning)
set_package_properties(Foo PROPERTIES URL "https://foo.example/"
                                      DESCRIPTION "The Foo package")
set_package_properties(Bar PROPERTIES URL "https://bar.net/")
set_package_properties(Baz PROPERTIES DESCRIPTION "A Baz package")

# Overwrite with different values (warnings)
set_package_properties(Foo PROPERTIES URL "https://foo.net/"
                                      DESCRIPTION "A Foo package") # Overwrite URL and DESCRIPTION
set_package_properties(Bar PROPERTIES URL "https://bar.example/"
                                      DESCRIPTION "The Bar package") # Overwrite URL and add DESCRIPTION
set_package_properties(Baz PROPERTIES URL "https://baz.example/"
                                      DESCRIPTION "The Baz package") # Overwrite URL and add DESCRIPTION
feature_summary(WHAT ALL)
