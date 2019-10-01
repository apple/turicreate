file(STRINGS UTF-32BE.txt str ENCODING UTF-32BE LENGTH_MINIMUM 4)
message("${str}")
file(STRINGS UTF-32BE.txt str LENGTH_MINIMUM 4)
message("${str}")
