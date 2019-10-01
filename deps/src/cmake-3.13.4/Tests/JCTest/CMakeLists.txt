cmake_minimum_required(VERSION 2.6)
project(TestTime)
enable_testing()
add_executable(TestTime TestTime.cxx)

foreach(f 1 2 3 4 5 6 7 8 9 10 11 12 12 14 15 16 17 18 19
20 21 22 23 24 25 26 27 28 29 30)
  add_test(TestTime${f} TestTime 50000000)
endforeach()
