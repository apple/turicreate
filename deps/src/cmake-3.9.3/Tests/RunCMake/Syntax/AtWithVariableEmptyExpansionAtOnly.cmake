# Literal since 'var' is not defined.
set(ref "@var@")
set(right "wrong")
set(var "\${right}")

# 'var' is dereferenced, but now 'right'
string(CONFIGURE "${ref}" output @ONLY)
message("-->${output}<--")
