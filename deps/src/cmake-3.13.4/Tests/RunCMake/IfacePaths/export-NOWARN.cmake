enable_language(CXX)
add_library(foo empty.cpp)

set_property(TARGET foo APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES $<0:>/include/subdir)
set_property(TARGET foo APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES $<INSTALL_PREFIX>/include/subdir)
set_property(TARGET foo APPEND PROPERTY INTERFACE_SOURCES $<0:>/include/subdir/empty.cpp)
set_property(TARGET foo APPEND PROPERTY INTERFACE_SOURCES $<INSTALL_PREFIX>/include/subdir/empty.cpp)

set_property(TARGET foo APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include/subdir>)
set_property(TARGET foo APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES $<INSTALL_INTERFACE:include/subdir>)
set_property(TARGET foo APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES $<INSTALL_INTERFACE:include/$<0:>>)
set_property(TARGET foo APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES $<INSTALL_INTERFACE:$<0:>/include>)
set_property(TARGET foo APPEND PROPERTY INTERFACE_SOURCES $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include/subdir/empty.cpp>)
set_property(TARGET foo APPEND PROPERTY INTERFACE_SOURCES $<INSTALL_INTERFACE:include/subdir/empty.cpp>)
set_property(TARGET foo APPEND PROPERTY INTERFACE_SOURCES $<INSTALL_INTERFACE:include/subdir/empty.cpp$<0:>>)
set_property(TARGET foo APPEND PROPERTY INTERFACE_SOURCES $<INSTALL_INTERFACE:$<0:>/include/subdir/empty.cpp>)

# target_include_directories(foo INTERFACE include/subdir) # Does and should warn. INSTALL_INTERFACE must not list src dir paths.
target_include_directories(foo INTERFACE $<0:>/include/subdir) # Does not and should not should warn, because it starts with a genex.
target_include_directories(foo INTERFACE $<INSTALL_PREFIX>/include/subdir)
target_sources(foo INTERFACE $<0:>/include/subdir/empty.cpp)
target_sources(foo INTERFACE $<INSTALL_PREFIX>/include/subdir/empty.cpp)

target_include_directories(foo INTERFACE $<INSTALL_INTERFACE:include/subdir>)
target_include_directories(foo INTERFACE $<INSTALL_INTERFACE:include/$<0:>>)
target_sources(foo INTERFACE $<INSTALL_INTERFACE:include/subdir/empty.cpp>)
target_sources(foo INTERFACE $<INSTALL_INTERFACE:include/subdir/empty.cpp$<0:>>)

install(FILES include/subdir/empty.cpp
  DESTINATION include/subdir
)

install(TARGETS foo EXPORT FooTargets DESTINATION lib)
install(EXPORT FooTargets DESTINATION lib/cmake)

install(TARGETS foo EXPORT FooTargets2
  DESTINATION lib
  INCLUDES DESTINATION include # No warning. Implicit install prefix.
)
install(EXPORT FooTargets2 DESTINATION lib/cmake)

install(TARGETS foo EXPORT FooTargets3
  DESTINATION lib
  INCLUDES DESTINATION $<INSTALL_PREFIX>include
)
install(EXPORT FooTargets3 DESTINATION lib/cmake)

install(TARGETS foo EXPORT FooTargets4
  DESTINATION lib
  INCLUDES DESTINATION $<INSTALL_INTERFACE:include>
)
install(EXPORT FooTargets4 DESTINATION lib/cmake)

install(TARGETS foo EXPORT FooTargets5
  DESTINATION lib
    # The $<0:> is evaluated at export time, leaving 'include' behind, which should be treated as above.
  INCLUDES DESTINATION $<INSTALL_INTERFACE:$<0:>include>
)
install(EXPORT FooTargets5 DESTINATION lib/cmake)

install(TARGETS foo EXPORT FooTargets6
  DESTINATION lib
  INCLUDES DESTINATION $<INSTALL_INTERFACE:include$<0:>>
)
install(EXPORT FooTargets6 DESTINATION lib/cmake)

install(TARGETS foo EXPORT FooTargets7
  DESTINATION lib
  INCLUDES DESTINATION include$<0:>
)
install(EXPORT FooTargets7 DESTINATION lib/cmake)

install(TARGETS foo EXPORT FooTargets8
  DESTINATION lib
  INCLUDES DESTINATION $<0:>include
)
install(EXPORT FooTargets8 DESTINATION lib/cmake)
