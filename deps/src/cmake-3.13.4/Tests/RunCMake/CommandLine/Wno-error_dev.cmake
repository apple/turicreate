# This should still produce a warning when -Wno-error=dev is specified
message(AUTHOR_WARNING "Some author warning")

# with -Wno-error=dev this will also cause an AUTHOR_WARNING message, checks
# that messages issued outside of the message command, by other CMake commands,
# also are affected by -Wno-error=dev
include("")
