set(disallowed_chars
  a b c d e f g h i j l m   o p q   s   u v w x y z
  A B C D E F G H I J L M N O P Q R S T U V W X Y Z
  0 1 2 3 4 5 6 6 7 8 9)
set(testnum 0)

configure_file(
  "${RunCMake_SOURCE_DIR}/CMakeLists.txt"
  "${RunCMake_BINARY_DIR}/CMakeLists.txt"
  COPYONLY)

foreach (char IN LISTS disallowed_chars)
  configure_file(
    "${RunCMake_SOURCE_DIR}/EscapeChar-char.cmake.in"
    "${RunCMake_BINARY_DIR}/EscapeChar-${char}-${testnum}.cmake"
    @ONLY)
  configure_file(
    "${RunCMake_SOURCE_DIR}/EscapeChar-char-stderr.txt.in"
    "${RunCMake_BINARY_DIR}/EscapeChar-${char}-${testnum}-stderr.txt"
    @ONLY)
  configure_file(
    "${RunCMake_SOURCE_DIR}/EscapeChar-char-result.txt"
    "${RunCMake_BINARY_DIR}/EscapeChar-${char}-${testnum}-result.txt"
    COPYONLY)

  math(EXPR testnum "${testnum} + 1")
endforeach ()

function (run_tests)
  set(GENERATED_RUNCMAKE_TESTS TRUE)
  # Find the tests in the binary directory.
  set(RunCMake_SOURCE_DIR "${RunCMake_BINARY_DIR}")

  set(testnum 0)
  foreach (char IN LISTS disallowed_chars)
    run_cmake("EscapeChar-${char}-${testnum}")

    math(EXPR testnum "${testnum} + 1")
  endforeach ()
endfunction ()

run_tests()
