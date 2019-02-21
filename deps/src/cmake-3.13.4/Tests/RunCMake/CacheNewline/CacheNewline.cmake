cmake_minimum_required(VERSION 3.5)

project(CacheNewlineTest NONE)

set(NEWLINE_VARIABLE "a\nb" CACHE STRING "Offending entry")
