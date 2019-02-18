# Literal since 'var' is not defined.
set(ref "@var@")
set(right "wrong")
set(var "\${right}")

# 'var' is dereferenced here.
string(CONFIGURE "${ref}" output)
message("-->${output}<--")
