/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UTIL_TRACEPOINT_HPP
#define TURI_UTIL_TRACEPOINT_HPP
#include <iostream>
#include <vector>
#include <string>
#include <timer/timer.hpp>
#include <core/util/branch_hints.hpp>
#include <core/parallel/atomic.hpp>
#include <core/parallel/atomic_ops.hpp>

/**
 * \defgroup perfmonitoring Intrusive Performance Monitoring
 * \brief A tracepoint utility that provides intrusive (requires code change) performance monitoring.
 *
 * The tracepoint utility provides a extremely low overhead way of profiling a
 * section of code, counting the number of times the section is entered, the
 * average, maximum and mimimum runtimes of the section.
 *
 * The tracepoint utility can be enabled file by file, or in an entire project by
 * setting the USE_TRACEPOINT macro before including tracepoint.hpp, or
 * predefining USE_TRACEPOINT globally.
 * \code
 * #define USE_TRACEPOINT
 * #include "perf/tracepoint.hpp"
 * \endcode
 *
 * Example Usage:
 * \code
 *    DECLARE_TRACER(event)
 *    INITIALIZE_TRACER(event, "event counter name");
 *    Then later on...
 *    BEGIN_TRACEPOINT(event)
 *    ... Do stuff ...
 *    END_TRACEPOINT(event)
 * \endcode
 * See \ref DECLARE_TRACER for details.
 */

namespace turi{
/**
 * \ingroup perfmonitoring
 * Implementation detail of the tracing macros.
 */
struct trace_count{
  std::string name;
  std::string description;
  bool print_on_destruct;
  turi::atomic<unsigned long long> count;
  turi::atomic<unsigned long long> total;
  unsigned long long minimum;
  unsigned long long maximum;
  inline trace_count(std::string name = "",
                    std::string description = "",
                    bool print_on_destruct = true):
                      name(name),
                      description(description),
                      print_on_destruct(print_on_destruct),
                      count(0),
                      total(0),
                      minimum(std::numeric_limits<unsigned long long>::max()),
                      maximum(0) { }

  /**
   * Initializes the tracer with a name, a description
   * and whether to print on destruction
   */
  inline void initialize(std::string n,
                  std::string desc,
                  bool print_out = true) {
    name = n;
    description = desc;
    print_on_destruct = print_out;
  }

  /**
   * Adds an event time to the trace
   */
  inline void incorporate(unsigned long long val)  __attribute__((always_inline)) {
    count.inc();
    total.inc(val);
    while(1) {
      unsigned long long m = minimum;
      if (__likely__(val > m || turi::atomic_compare_and_swap(minimum, m, val))) break;
    }
    while(1) {
      unsigned long long m = maximum;
      if (__likely__(val < m || turi::atomic_compare_and_swap(maximum, m, val))) break;
    }
  }

  /**
   * Adds the counts in a second tracer to the current tracer.
   */
  inline void incorporate(const trace_count &val)  __attribute__((always_inline)) {
    count.inc(val.count.value);
    total.inc(val.total.value);
    while(1) {
      unsigned long long m = minimum;
      if (__likely__(val.minimum > m || turi::atomic_compare_and_swap(minimum, m, val.minimum))) break;
    }
    while(1) {
      unsigned long long m = maximum;
      if (__likely__(val.maximum < m || turi::atomic_compare_and_swap(maximum, m, val.maximum))) break;
    }
  }

  /**
   * Adds the counts in a second tracer to the current tracer.
   */
  inline trace_count& operator+=(trace_count &val) {
    incorporate(val);
    return *this;
  }

  /**
   * Destructor. Will print to cerr if initialize() is called
   * with "true" as the 3rd argument
   */
  ~trace_count();

  /**
   * Prints the tracer counts
   */
  void print(std::ostream& out, unsigned long long tpersec = 0) const;
};

} // namespace


/**
 * \ingroup perfmonitoring
 * \def DECLARE_TRACER(name)
 * Creates a tracing object with a given name. This creates a variable
 * called "name" which is of type trace_count. and is equivalent to:
 *
 * turi::trace_count name;
 *
 * The primary reason to use this macro instead of just writing
 * the code above directly, is that the macro is ignored and compiles
 * to nothing when tracepoints are disabled.
 *
 * Example Usage:
 * \code
 *    DECLARE_TRACER(event)
 *    INITIALIZE_TRACER(event, "event counter name");
 *    Then later on...
 *    BEGIN_TRACEPOINT(event)
 *    ... Do stuff ...
 *    END_TRACEPOINT(event)
 * \endcode
 *
 * There are some minor caveats. BEGIN_TRACEPOINT really declares a variable,
 * so you must begin and end a tracepoint within the same scope, and you cannot
 * call BEGIN_TRACEPOINT on the same event within the same scope.
 *
 * At program termination the contents of the tracepoint will be emitted.
 * It will print the number times the tracepoint is entered, the total
 * program time spent in the tracepoint, as well as average, minimum and
 * maximum time spent in the tracepoint. Note that the tracepoint
 * is safe to use even when running in parallel. The total time spent in the
 * tracepoint is hence the sum over all threads.
 *
 * The tracepoint uses rdtsc for fast clock cycle counting, hence might be
 * inaccurate on systems where rdtsc is not necessarily monotonic.
 */

/**
 * \ingroup perfmonitoring
 * \def INITIALIZE_TRACER(name, desc)
 * Initializes the tracer created by \ref DECLARE_TRACER with a description.
 * The object with name "name" created by DECLARE_TRACER must be in scope.
 * This initializes the tracer "name" with a description, and
 * configures the tracer to print when the tracer "name" is destroyed.
 *
 *
 */

/**
 * \ingroup perfmonitoring
 * \def INITIALIZE_TRACER_NO_PRINT(name, desc)
 * Initializes the tracer created by \ref DECLARE_TRACER with a description.
 * The object with name "name" created by DECLARE_TRACER must be in scope.
 * This initializes the tracer "name" with a description, and
 * configures the tracer to NOT print when the tracer "name" is destroyed.
 */

/**
 * \ingroup perfmonitoring
 * \def BEGIN_TRACEPOINT(name)
 * Begins a tracepoint.
 * The object with name "name" created by DECLARE_TRACER must be in scope.
 * Times a block of code. Every END_TRACEPOINT must be matched with a
 * BEGIN_TRACEPOINT within the same scope. Tracepoints are safe to use in
 * concurrent use.
 */

/**
 * \ingroup perfmonitoring
 * \def END_TRACEPOINT(name)
 * Ends a tracepoint; see \ref BEGIN_TRACEPOINT.
 */

/**
 * \ingroup perfmonitoring
 * \def END_AND_BEGIN_TRACEPOINT(endname, beginname)
 * Ends the tracepoint with the name "endname" and begins a tracepoint with
 * name "beginname". Conceptually equivalent to
 * \code
 * END_TRACEPOINT(endname)
 * BEGIN_TRACEPOINT(beginname)
 * \endcode
 * but with slightly less overhead.
 *
 */

#ifdef DOXYGEN_DOCUMENTATION
#define USE_TRACEPOINT
#endif
#ifdef USE_TRACEPOINT
#define DECLARE_TRACER(name) turi::trace_count name;

#define INITIALIZE_TRACER(name, description) name.initialize(#name, description);
#define INITIALIZE_TRACER_NO_PRINT(name, description) name.initialize(#name, description, false);

#define BEGIN_TRACEPOINT(name) unsigned long long __ ## name ## _trace_ = rdtsc();
#define END_TRACEPOINT(name) name.incorporate(rdtsc() - __ ## name ## _trace_);
#define END_AND_BEGIN_TRACEPOINT(endname, beginname) unsigned long long __ ## beginname ## _trace_ = rdtsc(); \
                                                     endname.incorporate(__ ## beginname ## _trace_ - __ ## endname ## _trace_);
#else
#define DECLARE_TRACER(name)
#define INITIALIZE_TRACER(name, description)
#define INITIALIZE_TRACER_NO_PRINT(name, description)

#define BEGIN_TRACEPOINT(name)
#define END_TRACEPOINT(name)
#define END_AND_BEGIN_TRACEPOINT(endname, beginname)
#endif

#endif
