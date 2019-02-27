project(ProjectA NONE)
get_property(langs GLOBAL PROPERTY ENABLED_LANGUAGES)
message(STATUS "ENABLED_LANGUAGES='${langs}'")
