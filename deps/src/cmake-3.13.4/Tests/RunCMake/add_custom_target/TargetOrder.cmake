# Add a target that requires step1 to run first but enforces
# it only by target-level ordering dependency.
add_custom_command(
  OUTPUT step2.txt
  COMMAND ${CMAKE_COMMAND} -E copy step1.txt step2.txt
  )
add_custom_target(step2 DEPENDS step2.txt)
add_dependencies(step2 step1)

# Add a target that requires step1 and step2 to work,
# only depends on step1 transitively through step2, but
# also gets a copy of step2's custom command.
# The Ninja generator in particular must be careful with
# this case because it needs to compute the proper set of
# target ordering dependencies for the step2 custom command
# even though it appears in both the step2 and step3
# targets due to dependency propagation.
add_custom_command(
  OUTPUT step3.txt
  COMMAND ${CMAKE_COMMAND} -E copy step1.txt step3-1.txt
  COMMAND ${CMAKE_COMMAND} -E copy step2.txt step3.txt
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/step2.txt
  )
add_custom_target(step3 ALL DEPENDS step3.txt)
add_dependencies(step3 step2)

# We want this target to always run first.  Add it last so
# that serial builds require dependencies to order it first.
add_custom_target(step1
  COMMAND ${CMAKE_COMMAND} -E touch step1.txt
  )
