set(right "wrong")
set(var "\${right}")
# Expanded here.
set(ref "@var@")

# 'right' is dereferenced because 'var' was dereferenced when
# assigning to 'ref' above.
string(CONFIGURE "${ref}" output)
message("-->${output}<--")
