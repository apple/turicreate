project(ProjectA LANGUAGES)
get_property(langs GLOBAL PROPERTY ENABLED_LANGUAGES)
message(STATUS "ENABLED_LANGUAGES='${langs}'")
