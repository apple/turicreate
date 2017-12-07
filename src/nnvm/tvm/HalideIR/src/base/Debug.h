#ifndef HALIDE_DEBUG_H
#define HALIDE_DEBUG_H

/** \file
 * Defines functions for debug logging during code generation.
 */

#include <iostream>
#include <string>
#include <stdlib.h>
#include "Util.h"

namespace Halide {

struct Expr;
struct Type;
// Forward declare some things from IRPrinter, which we can't include yet.
EXPORT std::ostream &operator<<(std::ostream &stream, const Expr &);
EXPORT std::ostream &operator<<(std::ostream &stream, const Type &);

class Module;
EXPORT std::ostream &operator<<(std::ostream &stream, const Module &);

namespace Internal {

struct Stmt;
EXPORT std::ostream &operator<<(std::ostream &stream, const Stmt &);

struct LoweredFunc;
EXPORT std::ostream &operator << (std::ostream &, const LoweredFunc &);

/** For optional debugging during codegen, use the debug class as
 * follows:
 *
 \code
 debug(verbosity) << "The expression is " << expr << std::endl;
 \endcode
 *
 * verbosity of 0 always prints, 1 should print after every major
 * stage, 2 should be used for more detail, and 3 should be used for
 * tracing everything that occurs. The verbosity with which to print
 * is determined by the value of the environment variable
 * HL_DEBUG_CODEGEN
 */

class debug {
    const bool logging;

public:
    debug(int verbosity) : logging(verbosity <= debug_level()) {}

    template<typename T>
    debug &operator<<(T&& x) {
        if (logging) {
            std::cerr << std::forward<T>(x);
        }
        return *this;
    }

    EXPORT static int debug_level();
};

}
}

#endif
