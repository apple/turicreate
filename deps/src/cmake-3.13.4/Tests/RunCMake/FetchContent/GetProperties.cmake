include(FetchContent)

# First confirm properties are empty even before declare
FetchContent_GetProperties(t1)
if(t1_POPULATED)
    message(FATAL_ERROR "Property says populated before doing anything")
endif()
if(t1_SOURCE_DIR)
    message(FATAL_ERROR "SOURCE_DIR property not initially empty")
endif()
if(t1_BINARY_DIR)
    message(FATAL_ERROR "BINARY_DIR property not initially empty")
endif()

# Declare, but no properties should change yet
FetchContent_Declare(
  t1
  SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/savedSrc
  BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/savedBin
  DOWNLOAD_COMMAND ${CMAKE_COMMAND} -E echo "Do nothing"
)

FetchContent_GetProperties(t1)
if(t1_POPULATED)
    message(FATAL_ERROR "Property says populated after only declaring details")
endif()
if(t1_SOURCE_DIR)
    message(FATAL_ERROR "SOURCE_DIR property not empty after declare")
endif()
if(t1_BINARY_DIR)
    message(FATAL_ERROR "BINARY_DIR property not empty after declare")
endif()

# Populate should make all properties non-empty/set
FetchContent_Populate(t1)

FetchContent_GetProperties(t1)
if(NOT t1_POPULATED)
    message(FATAL_ERROR "Population did not set POPULATED property")
endif()
if(NOT "${t1_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_BINARY_DIR}/savedSrc")
    message(FATAL_ERROR "SOURCE_DIR property not correct after population: "
            "${t1_SOURCE_DIR}\n"
            "    Expected: ${CMAKE_CURRENT_BINARY_DIR}/savedSrc")
endif()
if(NOT "${t1_BINARY_DIR}" STREQUAL "${CMAKE_CURRENT_BINARY_DIR}/savedBin")
    message(FATAL_ERROR "BINARY_DIR property not correct after population: "
            "${t1_BINARY_DIR}\n"
            "    Expected: ${CMAKE_CURRENT_BINARY_DIR}/savedBin")
endif()

# Verify we can retrieve properties individually too
FetchContent_GetProperties(t1 POPULATED  varPop)
FetchContent_GetProperties(t1 SOURCE_DIR varSrc)
FetchContent_GetProperties(t1 BINARY_DIR varBin)

if(NOT varPop)
    message(FATAL_ERROR "Failed to retrieve POPULATED property")
endif()
if(NOT "${varSrc}" STREQUAL "${CMAKE_CURRENT_BINARY_DIR}/savedSrc")
    message(FATAL_ERROR "SOURCE_DIR property not retrieved correctly: ${varSrc}\n"
            "    Expected: ${CMAKE_CURRENT_BINARY_DIR}/savedSrc")
endif()
if(NOT "${varBin}" STREQUAL "${CMAKE_CURRENT_BINARY_DIR}/savedBin")
    message(FATAL_ERROR "BINARY_DIR property not retrieved correctly: ${varBin}\n"
            "    Expected: ${CMAKE_CURRENT_BINARY_DIR}/savedBin")
endif()
