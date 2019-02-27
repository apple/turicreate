cmake_policy(SET CMP0053 NEW)

set($ value)
set(dollar $)
message("-->${${dollar}}<--")
message("-->${$}<--")
