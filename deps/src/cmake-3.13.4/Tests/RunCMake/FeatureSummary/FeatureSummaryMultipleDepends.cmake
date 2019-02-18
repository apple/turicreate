include(FeatureSummary)

set(WITH_FOO 1)
set(WITH_BAR 0)

add_feature_info(Foo "WITH_FOO;WITH_BAR" "Foo.")
add_feature_info(Bar "WITH_FOO;NOT WITH_BAR" "Bar.")
add_feature_info(Baz "WITH_FOO AND NOT WITH_BAR" "Baz.")
add_feature_info(Goo "WITH_FOO OR WITH_BAR" "Goo.")
add_feature_info(Fez "NOT WITH_FOO OR WITH_BAR" "Fez.")

feature_summary(WHAT ALL)
