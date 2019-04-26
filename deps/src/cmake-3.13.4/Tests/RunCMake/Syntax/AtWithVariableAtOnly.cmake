set(right "wrong")
set(var "\${right}")
# Expanded here.
set(ref "@var@")

# No dereference done at all.
string(CONFIGURE "${ref}" output @ONLY)
message("-->${output}<--")
