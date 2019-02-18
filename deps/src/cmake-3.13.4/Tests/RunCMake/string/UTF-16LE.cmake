file(STRINGS UTF-16LE.txt str ENCODING UTF-16LE LENGTH_MINIMUM 4)
message("${str}")
file(STRINGS UTF-16LE.txt str LENGTH_MINIMUM 4)
message("${str}")
