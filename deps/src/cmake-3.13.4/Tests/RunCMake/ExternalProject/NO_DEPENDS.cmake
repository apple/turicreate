cmake_minimum_required(VERSION 2.8.12)

include(ExternalProject RESULT_VARIABLE GOO)

set_property(DIRECTORY PROPERTY EP_INDEPENDENT_STEP_TARGETS download patch update configure build)

ExternalProject_Add(FOO
                    URL https://example.org/foo.tar.gz)

ExternalProject_Add(BAR
                    URL https://example.org/bar.tar.gz
                    TEST_COMMAND echo test
                    INDEPENDENT_STEP_TARGETS install)
# This one should not give a warning
ExternalProject_Add_Step(BAR bar
                         COMMAND echo bar)

ExternalProject_Add_StepTargets(BAR NO_DEPENDS test bar)
