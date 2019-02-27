cmake_minimum_required(VERSION 3.0)
project(Minimal NONE)

function(test_set)
    set(blah "value2")
    message("before PARENT_SCOPE blah=${blah}")
    set(blah ${blah} PARENT_SCOPE)
    message("after PARENT_SCOPE blah=${blah}")
endfunction()

set(blah value1)
test_set()
message("in parent scope, blah=${blah}")
