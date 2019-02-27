cmake_minimum_required(VERSION 3.12)
project(OutName C)

add_executable(OutName main.c)
set_property(TARGET OutName PROPERTY PREFIX exe.)
set_property(TARGET OutName PROPERTY SUFFIX .exe)
