cmake_minimum_required(VERSION 3.7)
project(testFindGTK2 C)

# First call
find_package(GTK2 REQUIRED)

# Backup variables
set(GTK2_LIBRARIES_BAK ${GTK2_LIBRARIES})
set(GTK2_TARGETS_BAK ${GTK2_TARGETS})

# Second call
find_package(GTK2 REQUIRED)

# Check variables
if(NOT "${GTK2_LIBRARIES_BAK}" STREQUAL "${GTK2_LIBRARIES}")
  message(SEND_ERROR "GTK2_LIBRARIES is different:\nbefore: ${GTK2_LIBRARIES_BAK}\nafter:  ${GTK2_LIBRARIES}")
endif()

if(NOT "${GTK2_TARGETS_BAK}" STREQUAL "${GTK2_TARGETS}")
  message(SEND_ERROR "GTK2_TARGETS is different:\nbefore: ${GTK2_TARGETS_BAK}\nafter:  ${GTK2_TARGETS}")
endif()
