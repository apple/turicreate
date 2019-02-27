get_property(val DIRECTORY PROPERTY USER_PROP2)
message(STATUS "Subdir-to-parent chaining: '${val}'")

get_property(val DIRECTORY PROPERTY USER_PROP1)
message(STATUS "Subdir-to-global chaining: '${val}'")

set_property(DIRECTORY PROPERTY USER_PROP1 vSubdir)
get_property(val DIRECTORY PROPERTY USER_PROP1)
message(STATUS "Subdir no chaining required: '${val}'")

set_property(DIRECTORY APPEND PROPERTY USER_PROP1 aSubdir)
get_property(val DIRECTORY PROPERTY USER_PROP1)
message(STATUS "Subdir preset append chaining: '${val}'")

set_property(DIRECTORY APPEND PROPERTY USER_PROP2 aSubdir)
get_property(val DIRECTORY PROPERTY USER_PROP2)
message(STATUS "Subdir unset append chaining: '${val}'")

set_property(DIRECTORY APPEND PROPERTY USER_PROP5 aSubdir)
get_property(val DIRECTORY PROPERTY USER_PROP5)
message(STATUS "Subdir undefined append chaining: '${val}'")
