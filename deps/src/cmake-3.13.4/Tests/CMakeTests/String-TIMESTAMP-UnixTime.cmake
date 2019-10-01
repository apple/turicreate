string(TIMESTAMP timestamp "[%Y-%m-%d %H:%M:%S] %s" UTC)

string(TIMESTAMP unix_time "%s")

string(TIMESTAMP year "%Y" UTC)
string(TIMESTAMP days "%j" UTC)

# Doing proper date calculations here to verify unix timestamps
# could be error prone.
# At the very least use some safe lower and upper bounds to
# see if we are somewhere in the right region.

math(EXPR years_since_epoch "${year} - 1970")
math(EXPR lower_bound "((${years_since_epoch} * 365) + ${days} - 1) * 86400")
math(EXPR upper_bound "((${years_since_epoch} * 366) + ${days}) * 86400")


if(unix_time GREATER_EQUAL lower_bound AND unix_time LESS upper_bound)
  message("~${unix_time}~")
else()
  message(FATAL_ERROR "${timestamp} unix time not in expected range [${lower_bound}, ${upper_bound})")
endif()
