add_executable(TSD TSD.cxx TSD_utils.cxx)
install_files(/MyTest/include FILES TSD.h)
install_targets(/MyTest/bin TSD)
