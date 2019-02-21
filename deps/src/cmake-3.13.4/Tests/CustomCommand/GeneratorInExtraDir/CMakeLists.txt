add_definitions(-DGENERATOR_EXTERN)

# add the executable which will be used for generating files
add_executable(generator_extern ../generator.cxx)
set_target_properties(generator_extern PROPERTIES OUTPUT_NAME the_external_generator)

# add an executable which will be called from add_custom_command( ... POST_BUILD)
add_executable(dummy_generator ../generator.cxx)
