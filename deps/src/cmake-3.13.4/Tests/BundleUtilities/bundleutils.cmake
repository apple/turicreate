
# clean passed in arguments
get_filename_component(INPUT ${INPUT} ABSOLUTE)
get_filename_component(INPUTDIR ${INPUTDIR} ABSOLUTE)

message("INPUT = ${INPUT}")
message("MODULE = ${MODULE}")
message("INPUTDIR = ${INPUTDIR}")
message("OUTPUTDIR = ${OUTPUTDIR}")

# compute location to install/test things
file(RELATIVE_PATH relative_exe "${INPUTDIR}" "${INPUT}")
set(OUTPUT "${OUTPUTDIR}/${relative_exe}")
message("OUTPUT = ${OUTPUT}")
get_filename_component(EXE_DIR "${OUTPUT}" PATH)
get_filename_component(MODULE_NAME "${MODULE}" NAME)
set(OUTPUT_MODULE "${EXE_DIR}/${MODULE_NAME}")
message("OUTPUTMODULE = ${OUTPUT_MODULE}")

# clean output dir
file(REMOVE_RECURSE "${OUTPUTDIR}")
# copy the app and plugin to installation/testing directory
configure_file("${INPUT}" "${OUTPUT}" COPYONLY)
configure_file("${MODULE}" "${OUTPUT_MODULE}" COPYONLY)

# have BundleUtilities grab all dependencies and
# check that the app runs

# for this test we'll override location to put all dependencies
# (in the same dir as the app)
# this shouldn't be necessary except for the non-bundle case on Mac
function(gp_item_default_embedded_path_override item path)
  set(path "@executable_path" PARENT_SCOPE)
endfunction()

include(BundleUtilities)
fixup_bundle("${OUTPUT}" "${OUTPUT_MODULE}" "${INPUTDIR}")

# make sure we can run the app
message("Executing ${OUTPUT} in ${EXE_DIR}")
execute_process(COMMAND "${OUTPUT}" RESULT_VARIABLE result OUTPUT_VARIABLE out ERROR_VARIABLE out WORKING_DIRECTORY "${EXE_DIR}")

if(NOT result STREQUAL "0")
  message(FATAL_ERROR " failed to execute test program\n${out}")
endif()
