
cmake_policy(SET CMP0059 NEW)

add_definitions(-DSOME_DEF)

get_property(defs DIRECTORY .
  PROPERTY DEFINITIONS
)
message("DEFS:${defs}")

set_property(DIRECTORY .
  PROPERTY DEFINITIONS CUSTOM_CONTENT
)
get_property(content DIRECTORY .
  PROPERTY DEFINITIONS
)
message("CUSTOM CONTENT:${content}")
