cmake_minimum_required(VERSION 3.3)
project(SwiftMix C Swift)

add_executable(SwiftMix CMain.c ObjCMain.m SwiftMain.swift ObjC-Swift.h)
set_property(TARGET SwiftMix PROPERTY XCODE_ATTRIBUTE_SWIFT_OBJC_BRIDGING_HEADER "ObjC-Swift.h")
