file(STRINGS UTF-32LE.txt str ENCODING UTF-32LE LENGTH_MINIMUM 4)
message("${str}")
file(STRINGS UTF-32LE.txt str LENGTH_MINIMUM 4)
message("${str}")
