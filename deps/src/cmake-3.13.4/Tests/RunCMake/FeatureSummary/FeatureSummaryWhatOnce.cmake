include(FeatureSummary)

set(WITH_FOO 1)

add_feature_info(Foo WITH_FOO "Foo description.")
add_feature_info(Foo WITH_FOO "Foo description.")

feature_summary(WHAT ENABLED_FEATURES)
