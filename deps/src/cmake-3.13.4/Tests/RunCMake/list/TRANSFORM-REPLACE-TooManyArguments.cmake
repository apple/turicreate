set(mylist alpha bravo charlie)
list(TRANSFORM mylist REPLACE "^alpha$" "zulu" "one_too_many")
