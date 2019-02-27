include(FeatureSummary)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})

find_package(Foo)

# Type not set => OPTIONAL
feature_summary(WHAT ALL)

# RUNTIME > not set => RUNTIME
set_package_properties(Foo PROPERTIES TYPE RUNTIME)
feature_summary(WHAT ALL)

# OPTIONAL > RUNTIME => OPTIONAL
set_package_properties(Foo PROPERTIES TYPE OPTIONAL)
feature_summary(WHAT ALL)

# RUNTIME < OPTIONAL => OPTIONAL
set_package_properties(Foo PROPERTIES TYPE OPTIONAL)
feature_summary(WHAT ALL)

# RECOMMENDED > OPTIONAL => RECOMMENDED
set_package_properties(Foo PROPERTIES TYPE RECOMMENDED)
feature_summary(WHAT ALL)

# OPTIONAL < RECOMMENDED => RECOMMENDED
set_package_properties(Foo PROPERTIES TYPE OPTIONAL)
feature_summary(WHAT ALL)

# RUNTIME < RECOMMENDED => RECOMMENDED
set_package_properties(Foo PROPERTIES TYPE RUNTIME)
feature_summary(WHAT ALL)

# REQUIRED > RECOMMENDED => REQUIRED
set_package_properties(Foo PROPERTIES TYPE REQUIRED)
feature_summary(WHAT ALL)

# RECOMMENDED < REQUIRED => REQUIRED
set_package_properties(Foo PROPERTIES TYPE RECOMMENDED)
feature_summary(WHAT ALL)

# OPTIONAL < REQUIRED => REQUIRED
set_package_properties(Foo PROPERTIES TYPE OPTIONAL)
feature_summary(WHAT ALL)

# RUNTIME < REQUIRED => REQUIRED
set_package_properties(Foo PROPERTIES TYPE RUNTIME)
feature_summary(WHAT ALL)
