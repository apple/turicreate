%ifdef TESTx64
foo:
%else
_foo:
%endif
    mov  	eax, 0
    ret
