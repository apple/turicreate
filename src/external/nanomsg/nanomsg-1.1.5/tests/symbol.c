/*
    Copyright (c) 2013 Evan Wies <evan@neomantra.net>

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom
    the Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
    IN THE SOFTWARE.
*/

#include "../src/utils/err.c"

int main ()
{
    int i;
    struct nn_symbol_properties sym;
    int value;

    nn_assert (nn_symbol (-1, NULL) == NULL);
    nn_assert (nn_errno () == EINVAL);
    nn_assert (nn_symbol_info (-1, &sym, (int) sizeof (sym)) == 0);

    nn_assert (nn_symbol (2000, NULL) == NULL);
    nn_assert (nn_errno () == EINVAL);
    nn_assert (nn_symbol_info (2000, &sym, (int) sizeof (sym)) == 0);

    nn_assert (nn_symbol (6, &value) != NULL);
    nn_assert (value != 0);
    nn_assert (nn_symbol_info (6, &sym, (int) sizeof (sym)) == sizeof (sym));

    for (i = 0; ; ++i) {
        const char* name = nn_symbol (i, &value);
        if (name == NULL) {
            nn_assert (nn_errno () == EINVAL);
            break;
        }
    }

    for (i = 0; ; ++i) {
        if (nn_symbol_info (i, &sym, sizeof (sym)) == 0)
            break;
    }

    return 0;
}

