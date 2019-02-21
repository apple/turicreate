set("\${varname}" bar)
set(var foo)
set(ref "@\${var}@")

string(CONFIGURE "${ref}" output)
message("-->${output}<--")
