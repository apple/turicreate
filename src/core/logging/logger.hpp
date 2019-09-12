/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
/**
 * \defgroup turilogger Logging Utilities
 */

/**
 * @file logger.hpp
 * Usage:
 * First include logger.hpp. To logger, use the logger() function
 * There are 2 output levels. A "soft" output level which is
 * set by calling global_logger.set_log_level(), as well as a "hard" output
 * level OUTPUTLEVEL which is set in the source code (logger.h).
 *
 * when you call "logger()" with a loglevel and if the loglevel is greater than
 * both of the output levels, the string will be written.
 * written to a logger file. Otherwise, logger() has no effect.
 *
 * The difference between the hard level and the soft level is that the
 * soft level can be changed at runtime, while the hard level optimizes away
 * logging calls at compile time.
 */

#ifndef TURI_LOG_LOG_HPP
#define TURI_LOG_LOG_HPP

#ifndef TURI_LOGGER_THROW_ON_FAILURE
#define TURI_LOGGER_THROW_ON_FAILURE
#endif

#if defined(COMPILER_HAS_IOS_BASE_FAILURE_WITH_ERROR_CODE) && (_MSC_VER < 1600)
#undef COMPILER_HAS_IOS_BASE_FAILURE_WITH_ERROR_CODE
#endif

#include <fstream>
#include <sstream>
#include <cstdlib>
#include <cassert>
#include <cstring>
#include <cstdarg>
#ifdef COMPILER_HAS_IOS_BASE_FAILURE_WITH_ERROR_CODE
#include <system_error>
#endif
#include <functional>
#include <core/parallel/pthread_h.h>
#include <timer/timer.hpp>
#include <core/logging/fail_method.hpp>
#include <core/logging/backtrace.hpp>
#include <core/logging/error.hpp>
#include <core/system/cppipc/server/cancel_ops.hpp>
#include <core/util/code_optimization.hpp>
#include <process/process_util.hpp>

/**
 * \ingroup turilogger
 *
 * \addtogroup levels Log Levels
 * \brief Log Levels
 * \{
 * \def LOG_FATAL
 *   Used for fatal and probably irrecoverable conditions.
 */
/**
 * \def LOG_ERROR
 *   Used for errors which are recoverable within the scope of the function
 */
/**
 * \def LOG_WARNING
 *   Logs interesting conditions which are probably not fatal
 */
/**
 * \def LOG_EMPH
 *   Outputs as LOG_INFO, but in LOG_WARNING colors. Useful for
 *   outputting information you want to emphasize.
 */
/**
 * \def LOG_INFO
 *   Used for providing general useful information
 */
/**
 * \def LOG_DEBUG
 *   Debugging purposes only
 */
/**
 * \def LOG_EVERYTHING
 *   Log everything
 * \}
 */
// sgr - needed additional debug levels. I can undo this change if
// necessary. although it seems to me that log levels should count
// up and saturate so the messages label array can always be used.
#define LOG_NONE 8
#define LOG_FATAL 7
#define LOG_ERROR 6
#define LOG_WARNING 5
#define LOG_PROGRESS 4
#define LOG_EMPH 3
#define LOG_INFO 2
#define LOG_DEBUG 1
#define LOG_EVERYTHING 0

/**
 * \ingroup turilogger
 * \def OUTPUTLEVEL
 *  The minimum level to logger at. Set to LOG_NONE
 *  OUTPUTLEVEL to LOG_NONE to disable logging
 */

#ifndef OUTPUTLEVEL
#define OUTPUTLEVEL LOG_DEBUG
#endif
/// If set, logs to screen will be printed in color
#define COLOROUTPUT


/**
 * \ingroup turilogger
 * \addtogroup Global Logging Statements
 * \brief Global Logging Statements
 * \{
 * \def logger(lvl, fmt,...)
 *    Emits a log line output in printf format at a particular log level. lvl
 *    must be one of the log levels from LOG_DEBUG to LOG_FATAL. Emitting a
 *    LOG_FATAL will kill the process.
 *
 *    Example:
 *    \code
 *    logger(LOG_INFO, "hello world: %d", 10);
 *    \endcode
 */

/**
 * \def logstream(lvl)
 *    An output stream for specified log level. lvl must be one of the
 *    log levels from LOG_DEBUG to LOG_FATAL. Emitting a LOG_FATAL will
 *    kill the process. The stream must terminate with a "\n" or std::endl
 *    at the end of the message.
 *
 *    Example:
 *    \code
 *    logstream(LOG_INFO << "hello world: " << 10 << std::endl;
 *    \endcode
 */

/**
 * \def logger_once(lvl, fmt,...)
 *    Emits a log line output in printf format at a particular log level, only
 *    the first time the line is encountered. lvl must be one of the log levels
 *    from LOG_DEBUG to LOG_FATAL. Emitting a LOG_FATAL will kill the process.
 *
 *    Example:
 *    \code
 *    logger_once(LOG_INFO, "Class %s constructed", str);
 *    \endcode
 */

/**
 * \def logstream_once(lvl)
 *    An output stream for specified log level. This will only output the first
 *    time the line is encountered. lvl must be one of the log levels from
 *    LOG_DEBUG to LOG_FATAL. Emitting a LOG_FATAL will kill the process. The
 *    stream must terminate with a "\n" or std::endl at the end of the message.
 *
 *    Example:
 *    \code
 *    logstream_once(LOG_INFO) << "Class " << str << " constructed" << std::endl;
 *    \endcode
 */

/**
 * \def logger_ontick(sec, lvl, fmt,...)
 *    Emits a log line output in printf format at a particular log level, but
 *    will only print approximately once every "sec" seconds. lvl must be one
 *    of the log levels from LOG_DEBUG to LOG_FATAL. Emitting a LOG_FATAL will
 *    kill the process.
 *
 *    Example:
 *    \code
 *    // only print once every 5 seconds
 *    logger_ontick(5, LOG_INFO, "Class %s constructed", str);
 *    \endcode
 */

/**
 * \def logstream_ontick(sec, lvl)
 *    An output stream for specified log level. This will only print
 *    approximately once every "sec" seconds. lvl must be one of the log levels
 *    from LOG_DEBUG to LOG_FATAL. Emitting a LOG_FATAL will kill the process.
 *    The stream must terminate with a "\n" or std::endl at the end of the
 *    message.
 *
 *    Example:
 *    \code
 *    logstream_ontick(5, LOG_INFO) << "Class " << str << " constructed"
 *                                  << std::endl;
 *    \endcode
 */

/**
 * \def logprogress(fmt,...)
 *    Emits a progress message using printf format.
 *
 *    Example:
 *    \code
 *    logprogress("hello world: %d", 10);
 *    \endcode
 */

/**
 * \def logprogress_stream
 *    Emits a progress message using a stream. The stream must terminate with a
 *    "\n" or std::endl at the end of the message.
 *
 *    Example:
 *    \code
 *    logprogress_stream << "hello world " << 10 << std::endl;
 *    \endcode
 */

/**
 * \def logprogress_ontick(sec, fmt,...)
 *    Emits a progress message using printf format, but will only print
 *    approximately once every "sec" seconds.
 *    Example:
 *    \code
 *    // only print once every 5 seconds
 *    logprogress_ontick(5, "Class %s constructed", str);
 *    \endcode
 */

/**
 * \def logprogress_stream_ontick(sec)
 *    Emits a progress message using a stream. The stream must terminate with a
 *    "\n" or std::endl at the end of the message. This will only print
 *    approximately once every "sec" seconds. The stream must terminate with
 *    a "\n" or std::endl at the end of the message.
 *
 *    Example:
 *    \code
 *    logprogress_stream_ontick(5) << "Class " << str << " constructed"
 *                                 << std::endl;
 *    \endcode
 * \}
 */
#if OUTPUTLEVEL == LOG_NONE
// totally disable logging
#define logger(lvl,fmt,...)
#define logbuf(lvl,buflen,
#define logstream(lvl) if(0) null_stream()

#define logger_once(lvl,fmt,...)
#define logstream_once(lvl) if(0) null_stream()

#define logger_ontick(sec,lvl,fmt,...)
#define logstream_ontick(sec, lvl) if(0) null_stream()

#define logprogress(fmt,...)
#define logprogress_stream if(0) null_stream()

#define logprogress_ontick(sec,fmt,...)
#define logprogress_stream_ontick(sec) if(0) null_stream()

#else

#define logger(lvl, fmt, ...)                                                  \
  (log_dispatch<(lvl >= OUTPUTLEVEL)>::exec(lvl, __FILE__, __func__, __LINE__, \
                                            fmt, ##__VA_ARGS__))

#define logbuf(lvl, buf, len)                                                  \
  (log_dispatch<(lvl >= OUTPUTLEVEL)>::exec(lvl, __FILE__, __func__, __LINE__, \
                                            buf, len))

#define logstream(lvl)                                                      \
  if (lvl >= global_logger().get_log_level())                               \
  (log_stream_dispatch<(lvl >= OUTPUTLEVEL)>::exec(lvl, __FILE__, __func__, \
                                                   __LINE__))

#define logger_once(lvl, fmt, ...)                                 \
  {                                                                \
    static bool __printed__ = false;                               \
    if (!__printed__) {                                            \
      __printed__ = true;                                          \
      (log_dispatch<(lvl >= OUTPUTLEVEL)>::exec(                   \
          lvl, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)); \
    }                                                              \
  }

#define logstream_once(lvl)                                     \
  (*({                                                          \
    static bool __printed__ = false;                            \
    bool __prev_printed__ = __printed__;                        \
    if (!__printed__) __printed__ = true;                       \
    &(log_stream_dispatch<(lvl >= OUTPUTLEVEL)>::exec(          \
        lvl, __FILE__, __func__, __LINE__, !__prev_printed__)); \
  }))

#define logger_ontick(sec, lvl, fmt, ...)                          \
  {                                                                \
    static float last_print = -sec - 1;                            \
    float curtime = turi::timer::approx_time_seconds();            \
    if (last_print + sec <= curtime) {                             \
      last_print = curtime;                                        \
      (log_dispatch<(lvl >= OUTPUTLEVEL)>::exec(                   \
          lvl, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)); \
    }                                                              \
  }

#define logstream_ontick(sec, lvl)                                             \
  (*({                                                                         \
    static float last_print = -sec - 1;                                        \
    float curtime = turi::timer::approx_time_seconds();                        \
    bool print_now = false;                                                    \
    if (last_print + sec <= curtime) {                                         \
      last_print = curtime;                                                    \
      print_now = true;                                                        \
    }                                                                          \
    &(log_stream_dispatch<(lvl >= OUTPUTLEVEL)>::exec(lvl, __FILE__, __func__, \
                                                      __LINE__, print_now));   \
  }))

#define logprogress(fmt,...) logger(LOG_PROGRESS, fmt, ##__VA_ARGS__)
#define logprogress_stream logstream(LOG_PROGRESS)

#define logprogress_ontick(sec,fmt,...) logger_ontick(sec, LOG_PROGRESS, fmt, ##__VA_ARGS__)
#define logprogress_stream_ontick(sec) logstream_ontick(sec, LOG_PROGRESS)

#endif

#ifdef NDEBUG

#define log_and_throw(message)                        \
  do {                                                \
    auto throw_error = [&]() GL_COLD_NOINLINE_ERROR { \
      logstream(LOG_ERROR) << (message) << std::endl; \
      throw(std::string(message));                    \
    };                                                \
    throw_error();                                    \
  } while (0)

#define std_log_and_throw(key_type, message)          \
  do {                                                \
    auto throw_error = [&]() GL_COLD_NOINLINE_ERROR { \
      logstream(LOG_ERROR) << (message) << std::endl; \
      throw(key_type(message));                       \
    };                                                \
    throw_error();                                    \
  } while (0)

#ifdef COMPILER_HAS_IOS_BASE_FAILURE_WITH_ERROR_CODE
#define log_and_throw_io_failure(message)                       \
  do {                                                          \
    auto throw_error = [&]() GL_COLD_NOINLINE_ERROR {           \
      logstream(LOG_ERROR) << (message) << std::endl;           \
      throw(turi::error::io_error(message, std::error_code())); \
    };                                                          \
    throw_error();                                              \
  } while (0)
#else
#define log_and_throw_io_failure(message)             \
  do {                                                \
    auto throw_error = [&]() GL_COLD_NOINLINE_ERROR { \
      logstream(LOG_ERROR) << (message) << std::endl; \
      throw(turi::error::io_error(message));          \
    };                                                \
    throw_error();                                    \
  } while (0)
#endif

#else // debug mode

#define log_and_throw(message)                                          \
  do {                                                                  \
    auto throw_error = [&]() GL_COLD_NOINLINE_ERROR {                   \
      std::stringstream _turi_ss;                                       \
      _turi_ss << (message) << ". " << __func__ << " from " << __FILE__ \
               << " at " << __LINE__ << std::endl;                      \
      logstream(LOG_ERROR) << message << std::endl;                     \
      throw(_turi_ss.str());                                            \
    };                                                                  \
    throw_error();                                                      \
  } while (0)

#define std_log_and_throw(key_type, message)                            \
  do {                                                                  \
    auto throw_error = [&]() GL_COLD_NOINLINE_ERROR {                   \
      std::stringstream _turi_ss;                                       \
      _turi_ss << (message) << ". " << __func__ << " from " << __FILE__ \
               << " at " << __LINE__ << std::endl;                      \
      logstream(LOG_ERROR) << message << std::endl;                     \
      throw(key_type(_turi_ss.str()));                                  \
    };                                                                  \
    throw_error();                                                      \
  } while (0)

#ifdef COMPILER_HAS_IOS_BASE_FAILURE_WITH_ERROR_CODE
#define log_and_throw_io_failure(message)                               \
  do {                                                                  \
    auto throw_error = [&]() GL_COLD_NOINLINE_ERROR {                   \
      std::stringstream _turi_ss;                                       \
      _turi_ss << (message) << ". " << __func__ << " from " << __FILE__ \
               << " at " << __LINE__ << std::endl;                      \
      logstream(LOG_ERROR) << message << std::endl;                     \
      throw(turi::error::io_error(__turi_ss.str(), std::error_code())); \
    };                                                                  \
    throw_error();                                                      \
  } while (0)
#else
#define log_and_throw_io_failure(message)                               \
  do {                                                                  \
    auto throw_error = [&]() GL_COLD_NOINLINE_ERROR {                   \
      std::stringstream _turi_ss;                                       \
      _turi_ss << (message) << ". " << __func__ << " from " << __FILE__ \
               << " at " << __LINE__ << std::endl;                      \
      logstream(LOG_ERROR) << message << std::endl;                     \
      throw(turi::error::io_error(_turi_ss.str()));                     \
    };                                                                  \
    throw_error();                                                      \
  } while (0)
#endif

#endif // end of NDEBUG

#define log_and_throw_current_io_failure()                 \
  do {                                                     \
    auto error_code = errno;                               \
    std::string error_message = std::strerror(error_code); \
    errno = 0; /* clear errno */                           \
    log_and_throw_io_failure(error_message);               \
  } while (0)

#define log_func_entry()                                  \
  do {                                                    \
    logstream(LOG_INFO) << "Function entry" << std::endl; \
  } while (0)

#define Dlog_func_entry()                                  \
  do {                                                     \
    logstream(LOG_DEBUG) << "Function entry" << std::endl; \
  } while (0)

namespace logger_impl {
struct streambuff_tls_entry {
  std::stringstream streambuffer;
  bool streamactive;
  size_t header_len;
  int loglevel;
};
}

#define LOG_DEBUG_WITH_PID(...)                                         \
  do {                                                                  \
    auto __log_funct = [&]() {                                          \
      std::ostringstream ss;                                            \
      ss << "PID-" << global_logger().get_pid() << ": ";                \
      ss << __VA_ARGS__;                                                \
      logstream(LOG_DEBUG) << ss.str() << std::endl;                    \
    };                                                                  \
    if(LOG_DEBUG >= global_logger().get_log_level()) { __log_funct(); } \
  } while(0)


extern void __print_back_trace();

/**
 * \ingroup turilogger
 * The main logging class.
 *
 * This writes to a file, and/or the system console.
 * The main logger \ref global_logger is a global instance of this class.
 */
class file_logger{
 public:
  /** Default constructor. By default, log_to_console is on,
      there is no logger file, and logger level is set to LOG_EMPH
  */
  file_logger();

  ~file_logger();   /// destructor. flushes and closes the current logger file

  /** Closes the current logger file if one exists.
      if 'file' is not an empty string, it will be opened and
      all subsequent logger output will be written into 'file'.
      Any existing content of 'file' will be cleared.
      Return true on success and false on failure.
  */
  bool set_log_file(std::string file);

  /// If consolelog is true, subsequent logger output will be written
  /// to stdout / stderr.  If log_to_stderr is true, all output is
  /// logged to stderr.
  void set_log_to_console(bool consolelog, bool _log_to_stderr = false) {
    log_to_console = consolelog;
    log_to_stderr = _log_to_stderr;
  }

  /// If consolelog is true, subsequent logger output will be written
  /// to stdout / stderr.  If log_to_stderr is true, all output is
  /// logged to stderr.
  void set_pid(size_t pid) {
    reference_pid = pid;
  }

  /// If consolelog is true, subsequent logger output will be written
  /// to stdout / stderr.  If log_to_stderr is true, all output is
  /// logged to stderr.
  size_t get_pid() const {
    return reference_pid;
  }

  /// Returns the current logger file.
  std::string get_log_file(void) {
    return log_file;
  }

  /// Returns true if output is being written to stderr
  bool get_log_to_console() {
    return log_to_console;
  }

  /// Returns the current logger level
  int get_log_level() {
    return log_level;
  }

  /**
   * Set a callback to be called whenever a log message at a particular
   * log level is issued. Only one observer can be set per log level.
   */
  void add_observer(int loglevel,
                    std::function<void(int lineloglevel, const char* buf, size_t len)> callback_fn) {
    pthread_mutex_lock(&mut);
    callback[loglevel] = callback_fn;
    // has callback if callback_fn is not empty
    has_callback[loglevel] = !!callback_fn;
    pthread_mutex_unlock(&mut);
  }

  /**
   * Gets the callback called when a log message at a particular
   * log level is issued. Note that only one observer can be set per log level.
   */
  inline std::function<void(int lineloglevel, const char* buf, size_t len)> get_observer(int loglevel) {
    return callback[loglevel];
  }

  file_logger& start_stream(int lineloglevel,const char* file,const char* function, int line, bool do_start = true);

  /**
   * Streams a value into the logger.
   */
  template <typename T>
  file_logger& operator<<(T a) {
    // get the stream buffer
    logger_impl::streambuff_tls_entry* streambufentry = reinterpret_cast<logger_impl::streambuff_tls_entry*>(
                                          pthread_getspecific(streambuffkey));
    if (streambufentry != NULL) {
      std::stringstream& streambuffer = streambufentry->streambuffer;
      bool& streamactive = streambufentry->streamactive;

      if (streamactive) {
        streambuffer << a;
      }
    }
    return *this;
  }

  /**
   * Streams a value into the logger.
   */
  inline file_logger& operator<<(const char* a) {
    // get the stream buffer
    logger_impl::streambuff_tls_entry* streambufentry = reinterpret_cast<logger_impl::streambuff_tls_entry*>(
                                          pthread_getspecific(streambuffkey));
    if (streambufentry != NULL) {
      std::stringstream& streambuffer = streambufentry->streambuffer;
      bool& streamactive = streambufentry->streamactive;

      if (streamactive) {
        streambuffer << a;
        size_t s = strlen(a);
        if (s > 0 && a[s-1] == '\n') {
          stream_flush();
        }
      }
    }
    return *this;
  }

  /**
   * Streams an std::endl into the logger.
   */
  inline file_logger& operator<<(std::ostream& (*f)(std::ostream&)){
    // get the stream buffer
    logger_impl::streambuff_tls_entry* streambufentry = reinterpret_cast<logger_impl::streambuff_tls_entry*>(
                                          pthread_getspecific(streambuffkey));
    if (streambufentry != NULL) {
      std::stringstream& streambuffer = streambufentry->streambuffer;
      bool& streamactive = streambufentry->streamactive;

      if (streamactive) {
        // TODO: previously, we had a check for if (endltype(f) == endltype(std::endl))
        // and only flushed the stream on endl (ignoring all other stream modifiers).
        // On recent clang compilers, this check seems to always return false in debug.
        // (tested with Apple LLVM version 10.0.0 (clang-1000.11.45.5))
        // As a workaround, let's just flush the stream on all modifiers.
        // In practice they're usually endl anyway, so the perf hit should not be too bad.
        streambuffer << f;
        stream_flush();
        if(streamloglevel == LOG_FATAL) {
          __print_back_trace();
          TURI_LOGGER_FAIL_METHOD("LOG_FATAL encountered");
        }
      }
    }
    return *this;
  }



  /** Sets the current logger level. All logging commands below the current
      logger level will not be written. */
  void set_log_level(int new_log_level) {
    log_level = new_log_level;
  }

  /**
  * logs the message if loglevel>=OUTPUTLEVEL
  * This function should not be used directly. Use logger()
  *
  * @param loglevel Type of message \see LOG_DEBUG LOG_INFO LOG_WARNING LOG_ERROR LOG_FATAL
  * @param file File where the logger call originated
  * @param function Function where the logger call originated
  * @param line Line number where the logger call originated
  * @param fmt printf format string
  * @param arg var args. The parameters that match the format string
  */
  void _log(int loglevel,const char* file,const char* function,
                int line,const char* fmt, va_list arg );

  void _logbuf(int loglevel,const char* file,const char* function,
                int line,  const char* buf, int len);

  void _lograw(int loglevel, const char* buf, int len);

  inline void stream_flush() {
    // get the stream buffer
    logger_impl::streambuff_tls_entry* streambufentry =
        reinterpret_cast<logger_impl::streambuff_tls_entry*>(
                                          pthread_getspecific(streambuffkey));
    if (streambufentry != NULL) {
      std::stringstream& streambuffer = streambufentry->streambuffer;
      int lineloglevel = streambufentry->loglevel;

      streambuffer.flush();
      std::string msg = streambuffer.str();
      _lograw(streamloglevel, msg.c_str(), msg.length());

      // call the callback on the message if any
      if (has_callback[lineloglevel]) {
        pthread_mutex_lock(&mut);
        if (callback[lineloglevel]) {
          callback[lineloglevel](lineloglevel,
                                 msg.c_str() + streambufentry->header_len,
                                 msg.length() - streambufentry->header_len);
        }
        streambufentry->header_len = 0;
        pthread_mutex_unlock(&mut);
      }
      streambuffer.str("");
    }
  }
 private:
  std::ofstream fout;
  std::string log_file;

  pthread_key_t streambuffkey;

  int streamloglevel;
  pthread_mutex_t mut;

  bool log_to_console;
  bool log_to_stderr;
  int log_level;

  size_t reference_pid = size_t(-1);

  // LOG_NONE is the "highest" log level
  std::function<void(int lineloglevel, const char* buf, size_t len)> callback[LOG_NONE];
  int has_callback[LOG_NONE];
};


/**
 * Gets a reference to the global logger which all the logging macros use.
 */
file_logger& global_logger();

/**
Wrapper to generate 0 code if the output level is lower than the log level
*/
template <bool dostuff>
struct log_dispatch {};

template <>
struct log_dispatch<true> {
  inline static void exec(int loglevel,const char* file,const char* function,
                          int line,const char* fmt, ... ) {
    va_list argp;
    va_start(argp, fmt);
    global_logger()._log(loglevel, file, function, line, fmt, argp);
    va_end(argp);
    if(loglevel == LOG_FATAL) {
      __print_back_trace();
      TURI_LOGGER_FAIL_METHOD("LOG_FATAL encountered");
    }
  }
};

template <>
struct log_dispatch<false> {
  inline static void exec(int loglevel,const char* file,const char* function,
                int line,const char* fmt, ... ) {}
};


struct null_stream {
  template<typename T>
  inline null_stream operator<<(T t) { return null_stream(); }
  inline null_stream operator<<(const char* a) { return null_stream(); }
  inline null_stream operator<<(std::ostream& (*f)(std::ostream&)) { return null_stream(); }
};


template <bool dostuff>
struct log_stream_dispatch {};

template <>
struct log_stream_dispatch<true> {
  inline static file_logger& exec(int lineloglevel,const char* file,const char* function, int line, bool do_start = true) {
    // First see if there is an interupt waiting.  This is a convenient place that a lot of people call.
    if(cppipc::must_cancel()) {
      log_and_throw("Canceled by user.");
    }


    return global_logger().start_stream(lineloglevel, file, function, line, do_start);
  }
};

template <>
struct log_stream_dispatch<false> {
  inline static null_stream exec(int lineloglevel,const char* file,const char* function, int line, bool do_start = true) {

    // First see if there is an interupt waiting.  This is a convenient place that a lot of people call.
    if(cppipc::must_cancel()) {
      log_and_throw("Canceled by user.");
    }

    return null_stream();
  }
};


#define TEXTCOLOR_RESET   0
#define TEXTCOLOR_BRIGHT    1
#define TEXTCOLOR_DIM   2
#define TEXTCOLOR_UNDERLINE   3
#define TEXTCOLOR_BLINK   4
#define TEXTCOLOR_REVERSE   7
#define TEXTCOLOR_HIDDEN    8

#define TEXTCOLOR_BLACK     0
#define TEXTCOLOR_RED   1
#define TEXTCOLOR_GREEN   2
#define TEXTCOLOR_YELLOW    3
#define TEXTCOLOR_BLUE    4
#define TEXTCOLOR_MAGENTA   5
#define TEXTCOLOR_CYAN    6
#define TEXTCOLOR_WHITE   7

void textcolor(FILE* handle, int attr, int fg);
std::string textcolor(int attr, int fg);
void reset_color(FILE* handle);
std::string reset_color();

#endif
