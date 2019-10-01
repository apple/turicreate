section .text
%ifdef TEST2x64
global foo
%else
global _foo
%endif
%include "foo-proc.asm"
