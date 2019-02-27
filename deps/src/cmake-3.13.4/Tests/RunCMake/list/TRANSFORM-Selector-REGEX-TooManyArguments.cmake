set(mylist alpha bravo charlie)
list(TRANSFORM mylist TOUPPER REGEX "^alpha$" "one_too_many")
