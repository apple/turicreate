include(FeatureSummary)

set(WITH_FOO 1)
set(WITH_BAR 1)

add_feature_info(Foo WITH_FOO "Foo.")
add_feature_info(Bar WITH_BAR "Bar.")

feature_summary(WHAT ENABLED_FEATURES
                DESCRIPTION "Enabled features:"
                QUIET_ON_EMPTY)
feature_summary(WHAT DISABLED_FEATURES
                DESCRIPTION "Disabled features:"
                QUIET_ON_EMPTY)
