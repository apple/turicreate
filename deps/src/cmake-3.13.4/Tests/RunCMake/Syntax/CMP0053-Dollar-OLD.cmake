cmake_policy(SET CMP0053 OLD)

set($ value)
set(dollar $)
message("-->${${dollar}}<--")
message("-->${$}<--")
