cmake_minimum_required(VERSION 3.7)
project(ReadOnlyProperty C)

add_library(a a.c)

set_property(TARGET a PROPERTY MANUALLY_ADDED_DEPENDENCIES DEPENDENCIES foo)
