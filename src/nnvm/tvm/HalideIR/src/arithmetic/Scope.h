#ifndef HALIDE_SCOPE_H
#define HALIDE_SCOPE_H

#include <string>
#include <map>
#include <stack>
#include <utility>
#include <iostream>

#include "base/Util.h"
#include "base/Debug.h"
#include "base/Error.h"
#include "ir/IR.h"

/** \file
 * Defines the Scope class, which is used for keeping track of names in a scope while traversing IR
 */

namespace Halide {
namespace Internal {

/** A stack which can store one item very efficiently. Using this
 * instead of std::stack speeds up Scope substantially. */
template<typename T>
class SmallStack {
private:
    T _top;
    std::vector<T> _rest;
    bool _empty;

public:
    SmallStack() : _empty(true) {}

    void pop() {
        if (_rest.empty()) {
            _empty = true;
            _top = T();
        } else {
            _top = _rest.back();
            _rest.pop_back();
        }
    }

    void push(const T &t) {
        if (_empty) {
            _empty = false;
        } else {
            _rest.push_back(_top);
        }
        _top = t;
    }

    T top() const {
        return _top;
    }

    T &top_ref() {
        return _top;
    }

    const T &top_ref() const {
        return _top;
    }

    bool empty() const {
        return _empty;
    }
};

/** A common pattern when traversing Halide IR is that you need to
 * keep track of stuff when you find a Let or a LetStmt, and that it
 * should hide previous values with the same name until you leave the
 * Let or LetStmt nodes This class helps with that. */
template<typename T>
class Scope {
private:
    std::map<const Variable*, SmallStack<T>> table;

    // Copying a scope object copies a large table full of strings and
    // stacks. Bad idea.
    Scope(const Scope<T> &);
    Scope<T> &operator=(const Scope<T> &);

    const Scope<T> *containing_scope;


public:
    Scope() : containing_scope(nullptr) {}

    /** Set the parent scope. If lookups fail in this scope, they
     * check the containing scope before returning an error. Caller is
     * responsible for managing the memory of the containing scope. */
    void set_containing_scope(const Scope<T> *s) {
        containing_scope = s;
    }

    /** A const ref to an empty scope. Useful for default function
     * arguments, which would otherwise require a copy constructor
     * (with llvm in c++98 mode) */
    static const Scope<T> &empty_scope() {
        static Scope<T> *_empty_scope = new Scope<T>();
        return *_empty_scope;
    }

    /** Retrieve the value referred to by a name */
    T get(const Variable* var) const {
        typename std::map<const Variable*, SmallStack<T>>::const_iterator iter = table.find(var);
        if (iter == table.end() || iter->second.empty()) {
            if (containing_scope) {
                return containing_scope->get(var);
            } else {
                internal_error << "Symbol '" << var->name_hint << "' not found\n";
            }
        }
        return iter->second.top();
    }

    /** Return a reference to an entry. Does not consider the containing scope. */
    T &ref(const Variable* var) {
        typename std::map<const Variable*, SmallStack<T>>::iterator iter = table.find(var);
        if (iter == table.end() || iter->second.empty()) {
            internal_error << "Symbol '" << var->name_hint << "' not found\n";
        }
        return iter->second.top_ref();
    }

    /** Tests if a name is in scope */
    bool contains(const Variable* var) const {
        typename std::map<const Variable*, SmallStack<T>>::const_iterator iter = table.find(var);
        if (iter == table.end() || iter->second.empty()) {
            if (containing_scope) {
                return containing_scope->contains(var);
            } else {
                return false;
            }
        }
        return true;
    }

    /** Add a new (name, value) pair to the current scope. Hide old
     * values that have this name until we pop this name.
     */
    void push(const Variable* var, const T &value) {
        table[var].push(value);
    }

    /** A name goes out of scope. Restore whatever its old value
     * was (or remove it entirely if there was nothing else of the
     * same name in an outer scope) */
    void pop(const Variable* var) {
        typename std::map<const Variable*, SmallStack<T>>::iterator iter = table.find(var);
        internal_assert(iter != table.end()) << "Name not in symbol table: " << var->name_hint << "\n";
        iter->second.pop();
        if (iter->second.empty()) {
            table.erase(iter);
        }
    }

    /** Iterate through the scope. Does not capture any containing scope. */
    class const_iterator {
        typename std::map<const Variable*, SmallStack<T>>::const_iterator iter;
    public:
        explicit const_iterator(const typename std::map<const Variable*, SmallStack<T>>::const_iterator &i) :
            iter(i) {
        }

        const_iterator() {}

        bool operator!=(const const_iterator &other) {
            return iter != other.iter;
        }

        void operator++() {
            ++iter;
        }

        const Variable* var() {
            return iter->first;
        }

        const SmallStack<T> &stack() {
            return iter->second;
        }

        const T &value() {
            return iter->second.top_ref();
        }
    };

    const_iterator cbegin() const {
        return const_iterator(table.begin());
    }

    const_iterator cend() const {
        return const_iterator(table.end());
    }

    class iterator {
        typename std::map<Variable*, SmallStack<T>>::iterator iter;
    public:
        explicit iterator(typename std::map<Variable*, SmallStack<T>>::iterator i) :
            iter(i) {
        }

        iterator() {}

        bool operator!=(const iterator &other) {
            return iter != other.iter;
        }

        void operator++() {
            ++iter;
        }

        const Variable* var() {
            return iter->first;
        }

        SmallStack<T> &stack() {
            return iter->second;
        }

        T &value() {
            return iter->second.top_ref();
        }
    };

    iterator begin() {
        return iterator(table.begin());
    }

    iterator end() {
        return iterator(table.end());
    }

    void swap(Scope<T> &other) {
        table.swap(other.table);
        std::swap(containing_scope, other.containing_scope);
    }
};

template<typename T>
std::ostream &operator<<(std::ostream &stream, const Scope<T>& s) {
    stream << "{\n";
    typename Scope<T>::const_iterator iter;
    for (iter = s.cbegin(); iter != s.cend(); ++iter) {
        stream << "  " << iter.var()->name_hint << "\n";
    }
    stream << "}";
    return stream;
}

}
}

#endif
