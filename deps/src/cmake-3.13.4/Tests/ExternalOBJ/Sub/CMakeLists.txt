set_property(SOURCE ${CUSTOM_OBJECT} PROPERTY GENERATED 1)
add_executable(ExternalOBJSub ../executable.cxx ${CUSTOM_OBJECT})
add_dependencies(ExternalOBJSub ExternalOBJ) # depend on generating target
