find_package(GTK2 COMPONENTS gtk glade gtkmm glademm QUIET)


# Test GTK2 components
if(GTK2_GTK_FOUND)
  add_test(GTK2Components.gtk ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/FindGTK2/gtk"
    "${CMake_BINARY_DIR}/Tests/FindGTK2/GTK2Components/gtk"
     ${build_generator_args}
    --build-target gtk-all-libs
    --build-project gtk
    --build-exe-dir "${CMake_BINARY_DIR}/Tests/FindGTK2/GTK2Components/gtk"
    --force-new-ctest-process
    --test-command ${CMAKE_CTEST_COMMAND} -V
    )
endif()

if(GTK2_GTKMM_FOUND)
  add_test(GTK2Components.gtkmm ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/FindGTK2/gtkmm"
    "${CMake_BINARY_DIR}/Tests/FindGTK2/GTK2Components/gtkmm"
     ${build_generator_args}
    --build-target gtkmm-all-libs
    --build-project gtkmm
    --build-exe-dir "${CMake_BINARY_DIR}/Tests/FindGTK2/GTK2Components/gtkmm"
    --force-new-ctest-process
    --test-command ${CMAKE_CTEST_COMMAND} -V
    )
endif()


# Test GTK2 targets
if(TARGET GTK2::glib)
  add_test(GTK2Targets.glib ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/FindGTK2/glib"
    "${CMake_BINARY_DIR}/Tests/FindGTK2/GTK2Targets/glib"
    ${build_generator_args}
    --build-project glib
    --build-exe-dir "${CMake_BINARY_DIR}/Tests/FindGTK2/GTK2Targets/glib"
    --force-new-ctest-process
    --test-command ${CMAKE_CTEST_COMMAND} -V
    )
endif()

if(TARGET GTK2::gobject)
  add_test(GTK2Targets.gobject ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/FindGTK2/gobject"
    "${CMake_BINARY_DIR}/Tests/FindGTK2/GTK2Targets/gobject"
    ${build_generator_args}
    --build-project gobject
    --build-exe-dir "${CMake_BINARY_DIR}/Tests/FindGTK2/GTK2Targets/gobject"
    --force-new-ctest-process
    --test-command ${CMAKE_CTEST_COMMAND} -V
    )
endif()

if(TARGET GTK2::gio)
  add_test(GTK2Targets.gio ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/FindGTK2/gio"
    "${CMake_BINARY_DIR}/Tests/FindGTK2/GTK2Targets/gio"
    ${build_generator_args}
    --build-project gio
    --build-exe-dir "${CMake_BINARY_DIR}/Tests/FindGTK2/GTK2Targets/gio"
    --force-new-ctest-process
    --test-command ${CMAKE_CTEST_COMMAND} -V
    )
endif()

if(TARGET GTK2::gmodule)
  add_test(GTK2Targets.gmodule ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/FindGTK2/gmodule"
    "${CMake_BINARY_DIR}/Tests/FindGTK2/GTK2Targets/gmodule"
    ${build_generator_args}
    --build-project gmodule
    --build-exe-dir "${CMake_BINARY_DIR}/Tests/FindGTK2/GTK2Targets/gmodule"
    --force-new-ctest-process
    --test-command ${CMAKE_CTEST_COMMAND} -V
    )
endif()

if(TARGET GTK2::gthread)
  add_test(GTK2Targets.gthread ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/FindGTK2/gthread"
    "${CMake_BINARY_DIR}/Tests/FindGTK2/GTK2Targets/gthread"
    ${build_generator_args}
    --build-project gthread
    --build-exe-dir "${CMake_BINARY_DIR}/Tests/FindGTK2/GTK2Targets/gthread"
    --force-new-ctest-process
    --test-command ${CMAKE_CTEST_COMMAND} -V
    )
endif()

if(TARGET GTK2::atk)
  add_test(GTK2Targets.atk ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/FindGTK2/atk"
    "${CMake_BINARY_DIR}/Tests/FindGTK2/GTK2Targets/atk"
    ${build_generator_args}
    --build-project atk
    --build-exe-dir "${CMake_BINARY_DIR}/Tests/FindGTK2/GTK2Targets/atk"
    --force-new-ctest-process
    --test-command ${CMAKE_CTEST_COMMAND} -V
    )
endif()

if(TARGET GTK2::gdk_pixbuf)
  add_test(GTK2Targets.gdk_pixbuf ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/FindGTK2/gdk_pixbuf"
    "${CMake_BINARY_DIR}/Tests/FindGTK2/GTK2Targets/gdk_pixbuf"
    ${build_generator_args}
    --build-project gdk_pixbuf
    --build-exe-dir "${CMake_BINARY_DIR}/Tests/FindGTK2/GTK2Targets/gdk_pixbuf"
    --force-new-ctest-process
    --test-command ${CMAKE_CTEST_COMMAND} -V
    )
endif()

if(TARGET GTK2::cairo)
  add_test(GTK2Targets.cairo ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/FindGTK2/cairo"
    "${CMake_BINARY_DIR}/Tests/FindGTK2/GTK2Targets/cairo"
    ${build_generator_args}
    --build-project cairo
    --build-exe-dir "${CMake_BINARY_DIR}/Tests/FindGTK2/GTK2Targets/cairo"
    --force-new-ctest-process
    --test-command ${CMAKE_CTEST_COMMAND} -V
    )
endif()

if(TARGET GTK2::pango)
  add_test(GTK2Targets.pango ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/FindGTK2/pango"
    "${CMake_BINARY_DIR}/Tests/FindGTK2/GTK2Targets/pango"
    ${build_generator_args}
    --build-project pango
    --build-exe-dir "${CMake_BINARY_DIR}/Tests/FindGTK2/GTK2Targets/pango"
    --force-new-ctest-process
    --test-command ${CMAKE_CTEST_COMMAND} -V
    )
endif()

if(TARGET GTK2::pangocairo)
  add_test(GTK2Targets.pangocairo ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/FindGTK2/pangocairo"
    "${CMake_BINARY_DIR}/Tests/FindGTK2/GTK2Targets/pangocairo"
    ${build_generator_args}
    --build-project pangocairo
    --build-exe-dir "${CMake_BINARY_DIR}/Tests/FindGTK2/GTK2Targets/pangocairo"
    --force-new-ctest-process
    --test-command ${CMAKE_CTEST_COMMAND} -V
    )
endif()

if(TARGET GTK2::pangoxft)
  add_test(GTK2Targets.pangoxft ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/FindGTK2/pangoxft"
    "${CMake_BINARY_DIR}/Tests/FindGTK2/GTK2Targets/pangoxft"
    ${build_generator_args}
    --build-project pangoxft
    --build-exe-dir "${CMake_BINARY_DIR}/Tests/FindGTK2/GTK2Targets/pangoxft"
    --force-new-ctest-process
    --test-command ${CMAKE_CTEST_COMMAND} -V
    )
endif()

if(TARGET GTK2::pangoft2)
  add_test(GTK2Targets.pangoft2 ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/FindGTK2/pangoft2"
    "${CMake_BINARY_DIR}/Tests/FindGTK2/GTK2Targets/pangoft2"
    ${build_generator_args}
    --build-project pangoft2
    --build-exe-dir "${CMake_BINARY_DIR}/Tests/FindGTK2/GTK2Targets/pangoft2"
    --force-new-ctest-process
    --test-command ${CMAKE_CTEST_COMMAND} -V
    )
endif()

if(TARGET GTK2::gdk)
  add_test(GTK2Targets.gdk ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/FindGTK2/gdk"
    "${CMake_BINARY_DIR}/Tests/FindGTK2/GTK2Targets/gdk"
    ${build_generator_args}
    --build-project gdk
    --build-exe-dir "${CMake_BINARY_DIR}/Tests/FindGTK2/GTK2Targets/gdk"
    --force-new-ctest-process
    --test-command ${CMAKE_CTEST_COMMAND} -V
    )
endif()

if(TARGET GTK2::gtk)
  add_test(GTK2Targets.gtk ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/FindGTK2/gtk"
    "${CMake_BINARY_DIR}/Tests/FindGTK2/GTK2Targets/gtk"
    ${build_generator_args}
    --build-project gtk
    --build-exe-dir "${CMake_BINARY_DIR}/Tests/FindGTK2/GTK2Targets/gtk"
    --force-new-ctest-process
    --test-command ${CMAKE_CTEST_COMMAND} -V
    )
endif()

if(TARGET GTK2::sigc++)
  add_test(GTK2Targets.sigc++ ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/FindGTK2/sigc++"
    "${CMake_BINARY_DIR}/Tests/FindGTK2/GTK2Targets/sigc++"
     ${build_generator_args}
    --build-project sigc++
    --build-exe-dir "${CMake_BINARY_DIR}/Tests/FindGTK2/GTK2Targets/sigc++"
    --force-new-ctest-process
    --test-command ${CMAKE_CTEST_COMMAND} -V
    )
endif()

if(TARGET GTK2::glibmm)
  add_test(GTK2Targets.glibmm ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/FindGTK2/glibmm"
    "${CMake_BINARY_DIR}/Tests/FindGTK2/GTK2Targets/glibmm"
     ${build_generator_args}
    --build-project glibmm
    --build-exe-dir "${CMake_BINARY_DIR}/Tests/FindGTK2/GTK2Targets/glibmm"
    --force-new-ctest-process
    --test-command ${CMAKE_CTEST_COMMAND} -V
    )
endif()

if(TARGET GTK2::giomm)
  add_test(GTK2Targets.giomm ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/FindGTK2/giomm"
    "${CMake_BINARY_DIR}/Tests/FindGTK2/GTK2Targets/giomm"
     ${build_generator_args}
    --build-project giomm
    --build-exe-dir "${CMake_BINARY_DIR}/Tests/FindGTK2/GTK2Targets/giomm"
    --force-new-ctest-process
    --test-command ${CMAKE_CTEST_COMMAND} -V
    )
endif()

if(TARGET GTK2::atkmm)
  add_test(GTK2Targets.atkmm ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/FindGTK2/atkmm"
    "${CMake_BINARY_DIR}/Tests/FindGTK2/GTK2Targets/atkmm"
     ${build_generator_args}
    --build-project atkmm
    --build-exe-dir "${CMake_BINARY_DIR}/Tests/FindGTK2/GTK2Targets/atkmm"
    --force-new-ctest-process
    --test-command ${CMAKE_CTEST_COMMAND} -V
    )
endif()

if(TARGET GTK2::cairomm)
  add_test(GTK2Targets.cairomm ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/FindGTK2/cairomm"
    "${CMake_BINARY_DIR}/Tests/FindGTK2/GTK2Targets/cairomm"
     ${build_generator_args}
    --build-project cairomm
    --build-exe-dir "${CMake_BINARY_DIR}/Tests/FindGTK2/GTK2Targets/cairomm"
    --force-new-ctest-process
    --test-command ${CMAKE_CTEST_COMMAND} -V
    )
endif()

if(TARGET GTK2::pangomm)
  add_test(GTK2Targets.pangomm ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/FindGTK2/pangomm"
    "${CMake_BINARY_DIR}/Tests/FindGTK2/GTK2Targets/pangomm"
     ${build_generator_args}
    --build-project pangomm
    --build-exe-dir "${CMake_BINARY_DIR}/Tests/FindGTK2/GTK2Targets/pangomm"
    --force-new-ctest-process
    --test-command ${CMAKE_CTEST_COMMAND} -V
    )
endif()

if(TARGET GTK2::gdkmm)
  add_test(GTK2Targets.gdkmm ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/FindGTK2/gdkmm"
    "${CMake_BINARY_DIR}/Tests/FindGTK2/GTK2Targets/GTK2Targets/gdkmm"
     ${build_generator_args}
    --build-project gdkmm
    --build-exe-dir "${CMake_BINARY_DIR}/Tests/FindGTK2/GTK2Targets/GTK2Targets/gdkmm"
    --force-new-ctest-process
    --test-command ${CMAKE_CTEST_COMMAND} -V
    )
endif()

if(TARGET GTK2::gtkmm)
  add_test(GTK2Targets.gtkmm ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/FindGTK2/gtkmm"
    "${CMake_BINARY_DIR}/Tests/FindGTK2/GTK2Targets/gtkmm"
     ${build_generator_args}
    --build-target gtkmm-target
    --build-project gtkmm
    --build-exe-dir "${CMake_BINARY_DIR}/Tests/FindGTK2/GTK2Targets/gtkmm"
    --force-new-ctest-process
    --test-command ${CMAKE_CTEST_COMMAND} -V
    )
endif()
