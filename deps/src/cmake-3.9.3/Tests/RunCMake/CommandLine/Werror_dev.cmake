# with -Werror=dev this will also cause an (upgraded) AUTHOR_ERROR message,
# checks that messages issued outside of the message command, by other CMake
# commands, also are affected by -Werror=dev
include("")

# message command sets fatal occurred flag, so run it last
message(AUTHOR_WARNING "Some author warning")
