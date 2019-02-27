section .text
%ifdef TEST2x64
global bar
%else
global _bar
%endif
%ifdef TESTx64
bar:
%else
_bar:
%endif
    mov  	EAX_COMMA_SPACE_ZERO
    ret
