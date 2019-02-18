cmake_policy(SET CMP0070 NEW)
file(GENERATE OUTPUT TARGET_NAME_IF_EXISTS-not-a-target-generated.txt CONTENT "$<TARGET_NAME_IF_EXISTS:just-random-string>")
