cmake_minimum_required(VERSION 3.0)
project(Minimal NONE)

function(report where)
    message("----------")
    message("variable values at ${where}:")
    foreach(var IN ITEMS
            top_implicit_inner_set top_implicit_inner_unset
            top_explicit_inner_set top_explicit_inner_unset top_explicit_inner_tounset
            top_implicit_outer_set top_explicit_outer_unset
            top_explicit_outer_set top_explicit_outer_unset top_explicit_outer_tounset

            outer_implicit_inner_set outer_implicit_inner_unset
            outer_explicit_inner_set outer_explicit_inner_unset outer_explicit_inner_tounset)
        if(DEFINED ${var})
            message("${var}: -->${${var}}<--")
        else()
            message("${var}: <undefined>")
        endif()
    endforeach()
    message("----------")
endfunction()

macro(set_values upscope downscope value)
    # Pull the value in implicitly.
    set(dummy ${${upscope}_implicit_${downscope}_set})
    set(dummy ${${upscope}_implicit_${downscope}_unset})
    # Pull it down explicitly.
    set(${upscope}_explicit_${downscope}_set "${value}" PARENT_SCOPE)
    set(${upscope}_explicit_${downscope}_unset "${value}" PARENT_SCOPE)
    set(${upscope}_explicit_${downscope}_tounset PARENT_SCOPE)
endmacro()

function(inner)
    report("inner start")

    set_values(top inner inner)
    set_values(outer inner inner)

    report("inner end")
endfunction()

function(outer)
    report("outer start")

    set_values(top outer outer)

    # Set values for inner to manipulate.
    set(outer_implicit_inner_set outer)
    set(outer_implicit_inner_unset)
    set(outer_explicit_inner_set outer)
    set(outer_explicit_inner_unset)
    set(outer_explicit_inner_tounset outer)

    report("outer before inner")

    inner()

    report("outer after inner")

    # Do what inner does so that we can test the values that inner should have
    # pulled through to here.
    set_values(top inner outer)

    report("outer end")
endfunction()

# variable name is:
#
#    <upscope>_<pulltype>_<downscope>_<settype>
#
# where the value is the name of the scope it was set in. The scopes available
# are "top", "outer", and "inner". The pull type may either be "implicit" or
# "explicit" based on whether the pull is due to a variable dereference or a
# PARENT_SCOPE setting. The settype is "set" where both scopes set a value,
# "unset" where upscope unsets it and downscope sets it, and "tounset" where
# upscope sets it and downscope unsets it.
#
# We test the following combinations:
#
#   - outer overriding top's values;
#   - inner overriding top's values;
#   - inner overriding outer's values; and
#   - outer overriding inner's values in top after inner has run.

# Set values for inner to manipulate.
set(top_implicit_inner_set top)
set(top_implicit_inner_unset)
set(top_explicit_inner_set top)
set(top_explicit_inner_unset)
set(top_explicit_inner_tounset top)

# Set values for outer to manipulate.
set(top_implicit_outer_set top)
set(top_implicit_outer_unset)
set(top_explicit_outer_set top)
set(top_explicit_outer_unset)
set(top_explicit_outer_tounset top)

report("top before calls")

outer()

report("top after calls")
