# Quoted arguments may be immediately followed by another argument.
foreach(x "1 \${var} \\n 4""x"y)
  message("[${x}]")
endforeach()
