cmake_minimum_required(VERSION 3.0)
project(${RunCMake_TEST} NONE)

cmake_policy(SET CMP0069 NEW)

include(CheckIPOSupported)
include(${RunCMake_TEST}.cmake)
