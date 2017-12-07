#ifndef HALIDE_IR_PRINTER_H
#define HALIDE_IR_PRINTER_H

/** \file
 * This header file defines operators that let you dump a Halide
 * expression, statement, or type directly into an output stream
 * in a human readable form.
 * E.g:
 \code
 Expr foo = ...
 std::cout << "Foo is " << foo << std::endl;
 \endcode
 *
 * These operators are implemented using \ref Halide::Internal::IRPrinter
 */

#include <ostream>
#include "./IR.h"
#include "./IRVisitor.h"

namespace Halide {

/** Emit an expression on an output stream (such as std::cout) in a
 * human-readable form */
EXPORT std::ostream &operator<<(std::ostream &stream, const Expr &);

/** Emit a halide type on an output stream (such as std::cout) in a
 * human-readable form */
EXPORT std::ostream &operator<<(std::ostream &stream, const Type &);


/** Emit a halide device api type in a human readable form */
EXPORT std::ostream &operator<<(std::ostream &stream, const DeviceAPI &);

namespace Internal {

/** Emit a halide statement on an output stream (such as std::cout) in
 * a human-readable form */
EXPORT std::ostream &operator<<(std::ostream &stream, const Stmt &);

/** Emit a halide for loop type (vectorized, serial, etc) in a human
 * readable form */
EXPORT std::ostream &operator<<(std::ostream &stream, const ForType &);

/**
 * An IRVisitor that emits IR to the given output stream in a human
 * readable form. Can be subclassed if you want to modify the way in
 * which it prints.
 *
 * IRPrinter is re-implemeneted using IRFunctor, as a demonstration
 * example on how Visitor based printing can be adopted to IRFunctor.
 *
 */
class IRPrinter {
public:
    /** Construct an IRPrinter pointed at a given output stream
     * (e.g. std::cout, or a std::ofstream) */
    EXPORT IRPrinter(std::ostream &);

    /** emit an expression on the output stream */
    EXPORT void print(const NodeRef&);

    EXPORT static void test();

    /** The stream we're outputting on */
    std::ostream &stream;

    /** The current indentation level, useful for pretty-printing
     * statements */
    int indent;

    /** Emit spaces according to the current indentation level */
    void do_indent();

    using FType = tvm::IRFunctor<void(const NodeRef&, IRPrinter *)>;

    EXPORT static FType& vtable();
};
}
}

#endif
