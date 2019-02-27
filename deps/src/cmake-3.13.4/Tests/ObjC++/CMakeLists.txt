# a simple objc++ test case that uses Cocoa framework
project (ObjC++)

add_executable (ObjC++ objc++.mm)
target_link_libraries(ObjC++ "-framework Cocoa")
