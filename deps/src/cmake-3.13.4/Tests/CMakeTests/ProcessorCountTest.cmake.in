include(ProcessorCount)

ProcessorCount(processor_count)

message("### 1. This line should be the first line of text in the test output.")
message("### 2. If there was output from this test before line #1, then the")
message("### 3. ProcessorCount(...) function call is emitting output that it shouldn't...")

message("processor_count='${processor_count}'")

execute_process(
  COMMAND "${KWSYS_TEST_EXE}"
  testSystemInformation
  OUTPUT_VARIABLE tsi_out
  ERROR_VARIABLE tsi_err
  RESULT_VARIABLE tsi_res
)
if (tsi_res)
  message("executing \"${KWSYS_TEST_EXE}\" failed")
  message(FATAL_ERROR "output: ${tsi_res}")
endif ()

string(REGEX REPLACE "(.*)GetNumberOfPhysicalCPU:.([0-9]*)(.*)" "\\2"
  system_info_processor_count "${tsi_out}")

message("system_info_processor_count='${system_info_processor_count}'")

if(system_info_processor_count EQUAL processor_count)
  message("processor count matches system information")
endif()

message("")
message("CTEST_FULL_OUTPUT (Avoid ctest truncation of output)")
message("")
message("tsi_out='${tsi_out}'")
message("tsi_err='${tsi_err}'")
message("")

# Evaluate possible error conditions:
#
set(err 0)
set(fatal 0)

if(processor_count EQUAL 0)
  set(err 1)
  set(fatal 1)
  message("err 1")
  message("could not determine number of processors
- Additional code for this platform needed in ProcessorCount.cmake?")
  message("")
endif()

if(NOT system_info_processor_count EQUAL processor_count)
  set(err 2)
  message("err 2")
  message("SystemInformation and ProcessorCount.cmake disagree:\n"
    "processor_count='${processor_count}'\n"
    "SystemInformation processor_count='${system_info_processor_count}'")
  message("")
endif()

if(NOT processor_count MATCHES "^[0-9]+$")
  set(err 3)
  set(fatal 1)
  message("err 3")
  message("ProcessorCount function returned a non-integer")
  message("")
endif()

if(NOT system_info_processor_count MATCHES "^[0-9]+$")
  set(err 4)
  message("err 4")
  message("SystemInformation ProcessorCount function returned a non-integer")
  message("")
endif()

if(fatal)
  message(FATAL_ERROR "processor_count='${processor_count}' - see previous test output for more details - it is likely more/different code is needed in ProcessorCount.cmake to fix this test failure - processor_count should be a non-zero positive integer (>=1) for all supported CMake platforms")
endif()
