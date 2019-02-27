set(var "\"")
set(ref "@var@")
set(rref "\${var}")

string(CONFIGURE "${ref}" output ESCAPE_QUOTES)
message("-->${output}<--")

string(CONFIGURE "${rref}" output ESCAPE_QUOTES)
message("-->${output}<--")
