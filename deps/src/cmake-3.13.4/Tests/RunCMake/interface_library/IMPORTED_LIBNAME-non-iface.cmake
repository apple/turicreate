add_custom_target(MyCustom)
set_property(TARGET MyCustom PROPERTY IMPORTED_LIBNAME item1)
set_property(TARGET MyCustom APPEND PROPERTY IMPORTED_LIBNAME item2)
set_property(TARGET MyCustom PROPERTY IMPORTED_LIBNAME_DEBUG item1)
set_property(TARGET MyCustom APPEND PROPERTY IMPORTED_LIBNAME_DEBUG item2)

add_library(MyStatic STATIC IMPORTED)
set_property(TARGET MyStatic PROPERTY IMPORTED_LIBNAME item1)

add_library(MyShared SHARED IMPORTED)
set_property(TARGET MyShared PROPERTY IMPORTED_LIBNAME item1)

add_library(MyModule MODULE IMPORTED)
set_property(TARGET MyModule PROPERTY IMPORTED_LIBNAME item1)

add_executable(MyExe IMPORTED)
set_property(TARGET MyExe PROPERTY IMPORTED_LIBNAME item1)
