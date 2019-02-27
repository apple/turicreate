include(RunCMake)

set(RunCMake_TEST_OPTIONS -DQT_QMAKE_EXECUTABLE:FILEPATH=${QT_QMAKE_EXECUTABLE})

run_cmake(UseModulesMacro-WARN)
run_cmake(AutomocMacro-WARN)
