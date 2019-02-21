cmake_minimum_required(VERSION 3.12)
project(Test LANGUAGES C)

configure_file(PreventConfigureFileDupBuildRule.cmake PreventTargetAliasesDupBuildRule.cmake @ONLY)
add_subdirectory(SubDirConfigureFileDup)
