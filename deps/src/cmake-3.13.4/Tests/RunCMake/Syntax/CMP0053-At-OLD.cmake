cmake_policy(SET CMP0053 OLD)

set(right "wrong")
set(var "\${right}")
# Expanded here with the old policy.
set(ref "@var@")

string(CONFIGURE "${ref}" output)
message("-->${output}<--")
