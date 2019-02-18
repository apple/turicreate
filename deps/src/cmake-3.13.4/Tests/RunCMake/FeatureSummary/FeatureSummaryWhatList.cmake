include(FeatureSummary)

set(WITH_FOO 1)
set(WITH_BAR 0)

add_feature_info(Foo WITH_FOO "Foo.")
add_feature_info(Bar WITH_BAR "Bar.")

feature_summary(WHAT DISABLED_FEATURES ENABLED_FEATURES)
