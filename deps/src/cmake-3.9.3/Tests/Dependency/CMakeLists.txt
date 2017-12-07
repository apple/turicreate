cmake_minimum_required (VERSION 2.6)
project( Dependency )

# to test directories with only one character One was changed to 1
# There is one executable that depends on eight libraries. The
# system has the following dependency graph:
#
# NoDepA:
# NoDepB: NoDepA
# NoDepC: NoDepA
# 1:
# Two: Three
# Three: 1 Four
# Four: 1 Two NoDepA
# Five: Two
# SixA: Two Five
# SixB: Four Five
# Seven: Two
# Eight: Seven
#
# Exec: NoDepB NoDepC SixA SixB
# Exec2: Eight Five
# Exec3: Eight Five
# Exec4: Five Two
#
# The libraries One,...,Eight have their dependencies explicitly
# encoded. The libraries NoDepA,...,NoDepC do not.
#
# Although SixB does not depend on Two, there is a dependency listed
# in the corresponding CMakeLists.txt just because of commands used.

add_subdirectory(NoDepA)
add_subdirectory(NoDepB)
add_subdirectory(NoDepC)
add_subdirectory(1)
add_subdirectory(Two)
add_subdirectory(Three)
add_subdirectory(Four)
add_subdirectory(Five)
add_subdirectory(Six)
add_subdirectory(Seven)
add_subdirectory(Eight)
add_subdirectory(Exec)
add_subdirectory(Exec2)
add_subdirectory(Exec3)
add_subdirectory(Exec4)

# Specific cases added to test fixes to problems found in real
# projects.
add_subdirectory(Case1)
add_subdirectory(Case2)
add_subdirectory(Case3)
add_subdirectory(Case4)
add_subdirectory(Case5)
