cmake_policy(SET CMP0070 NEW)
file(GENERATE OUTPUT TARGET_EXISTS-generated.txt CONTENT "$<TARGET_EXISTS:${empty}>")
