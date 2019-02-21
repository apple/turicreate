add_library(empty UNKNOWN IMPORTED)
add_custom_target(custom COMMAND echo $<TARGET_BUNDLE_CONTENT_DIR:empty>)
