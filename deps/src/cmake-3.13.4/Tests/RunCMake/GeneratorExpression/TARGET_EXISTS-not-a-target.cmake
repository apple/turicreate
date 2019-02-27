cmake_policy(SET CMP0070 NEW)
file(GENERATE OUTPUT TARGET_EXISTS-not-a-target-generated.txt CONTENT "$<TARGET_EXISTS:just-random-string>")
