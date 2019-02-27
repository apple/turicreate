#!/usr/bin/env bash

set -e

forced=1
if [[ "${1}" = "make" ]]; then
    forced=0
fi

pushd "${BASH_SOURCE%/*}/../../Source/LexerParser" > /dev/null

for parser in            \
    CommandArgument     \
    DependsJava         \
    Expr                \
    Fortran
do
    in_file=cm${parser}Parser.y
    cxx_file=cm${parser}Parser.cxx
    h_file=cm${parser}ParserTokens.h
    prefix=cm${parser}_yy

    if [[ (${in_file} -nt ${cxx_file}) || (${in_file} -nt ${h_file}) || (${forced} -gt 0) ]]; then
        echo "Generating Parser ${parser}"
          bison --yacc --name-prefix=${prefix} --defines=${h_file} -o${cxx_file}  ${in_file}
          sed -i '/\/\* Else will try to reuse/ i\
#if 0
/^yyerrlab1:/ a\
#endif
' ${cxx_file}
    else
        echo "Skipped generating Parser ${parser}"
    fi
done


popd > /dev/null
