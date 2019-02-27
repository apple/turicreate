set(mylist alpha bravo charlie delta)
list(SUBLIST mylist 1 2 result)

if (NOT result STREQUAL "bravo;charlie")
  message (FATAL_ERROR "SUBLIST is \"${result}\", expected is \"bravo;charlie\"")
endif()


unset(result)
list(SUBLIST mylist 0 2 result)

if (NOT result STREQUAL "alpha;bravo")
  message (FATAL_ERROR "SUBLIST is \"${result}\", expected is \"alpha;bravo\"")
endif()


unset(result)
list(SUBLIST mylist 3 2 result)

if (NOT result STREQUAL "delta")
  message (FATAL_ERROR "SUBLIST is \"${result}\", expected is \"delta\"")
endif()


unset(result)
list(SUBLIST mylist 2 0 result)
list(LENGTH result length)
if (NOT length EQUAL 0)
  message (FATAL_ERROR "SUBLIST is \"${result}\", expected is an empty list")
endif()


unset(result)
list(SUBLIST mylist 1 5 result)

if (NOT result STREQUAL "bravo;charlie;delta")
  message (FATAL_ERROR "SUBLIST is \"${result}\", expected is \"bravo;charlie;delta\"")
endif()


unset(result)
list(SUBLIST mylist 1 -1 result)

if (NOT result STREQUAL "bravo;charlie;delta")
  message (FATAL_ERROR "SUBLIST is \"${result}\", expected is \"bravo;charlie;delta\"")
endif()
