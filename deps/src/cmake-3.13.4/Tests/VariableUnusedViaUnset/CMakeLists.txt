# NOTE: Changing lines in here changes the test results since the first
# instance shouldn't warn, but the second should and they have the same message

# A warning should NOT be issued for this line:
set(UNUSED_VARIABLE)
# Warning should occur here:
set(UNUSED_VARIABLE)
message(STATUS "${UNUSED_VARIABLE}")
