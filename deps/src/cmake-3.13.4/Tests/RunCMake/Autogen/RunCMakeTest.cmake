include(RunCMake)

run_cmake(NoQt)
if (with_qt5)
  run_cmake(QtInFunction)
  run_cmake(QtInFunctionNested)
  run_cmake(QtInFunctionProperty)
endif ()
