set(mylist FILTER_THIS_BIT DO_NOT_FILTER_THIS thisisanitem FILTER_THIS_THING)
message("mylist was: ${mylist}")
list(FILTER mylist INCLUDE REGEX "^FILTER_THIS_.+")
message("mylist is: ${mylist}")
