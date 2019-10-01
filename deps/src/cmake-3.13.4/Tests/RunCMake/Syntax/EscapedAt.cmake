set(var "n")
set(ref "\\@var@")

string(CONFIGURE "${ref}" output)
message("-->${output}<--")
