#
# Example CMakeLists.txt file to demonstrate how to make a designable Windows Forms project with CMake.
#
# Code modifications and example by John Farrier, john.farrier@helleboreconsulting.com
#

cmake_minimum_required(VERSION 2.8.10)

# Project Name
project(VSWindowsFormsResx CXX)

include(CheckFunctionExists)
include(CheckCXXSourceCompiles)
include(CheckIncludeFile)

# Note: The designable form is assumed to have a .h extension as is default in Visual Studio.
# Note: The designable form is assumed to have a .resx file with the same name and path (save extension) as is default in Visual Studio

set(TARGET_H
  WindowsFormsResx/MyForm.h
  WindowsFormsResx/Header.h
  )

set(TARGET_SRC
  WindowsFormsResx/MyForm.cpp
  WindowsFormsResx/Source.cpp
  )
set_source_files_properties(${TARGET_SRC} PROPERTIES COMPILE_FLAGS "/clr")

set(TARGET_RESX
  WindowsFormsResx/MyForm.resx
  )

set(TARGET_LIBRARIES ${SYSLIBS})
add_executable(${PROJECT_NAME} ${TARGET_SRC} ${TARGET_H} ${TARGET_RESX})

# Note: The property VS_GLOBAL_KEYWORD must be set.
set_property(TARGET ${PROJECT_NAME} PROPERTY VS_GLOBAL_KEYWORD "ManagedCProj")

# Note: The property VS_DOTNET_REFERENCES must be set.
set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DOTNET_REFERENCES "System" "System.Data" "System.Drawing" "System.Windows.Forms" "System.Xml")

# Note: Modification of compiler flags is required for CLR compatibility now that we are using .resx files.
string(REPLACE "/EHsc" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
string(REPLACE "/RTC1" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
