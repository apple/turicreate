#
# Add test
#
add_executable(testing2 testing2.cxx)
add_test(testing.2 ${Testing_BINARY_DIR}/bin/testing2)

add_test(NotCycle.a ${CMAKE_COMMAND} -E echo a)
add_test(NotCycle.test1 ${CMAKE_COMMAND} -E echo test1)
set_property(TEST NotCycle.test1 PROPERTY DEPENDS NotCycle.a)

add_test(NotCycle.b ${CMAKE_COMMAND} -E echo b)
add_test(NotCycle.test2 ${CMAKE_COMMAND} -E echo test2)
set_property(TEST NotCycle.test2 PROPERTY DEPENDS NotCycle.b NotCycle.test1)

add_test(NotCycle.c ${CMAKE_COMMAND} -E echo c)
add_test(NotCycle.test3 ${CMAKE_COMMAND} -E echo test3)
set_property(TEST NotCycle.test3 PROPERTY DEPENDS NotCycle.c NotCycle.test1 NotCycle.test2)
