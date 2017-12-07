set(mylist FILTER_THIS_BIT DO_NOT_FILTER_THIS thisisanitem FILTER_THIS_THING)
list(FILTER mylist EXCLUDE REGEX "^FILTER_THIS_.+" one_too_many)
