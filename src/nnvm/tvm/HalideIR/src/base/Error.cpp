#include "Error.h"

namespace Halide {

namespace {

CompileTimeErrorReporter* custom_error_reporter = nullptr;

}  // namespace

void set_custom_compile_time_error_reporter(CompileTimeErrorReporter* error_reporter) {
    custom_error_reporter = error_reporter;
}

bool exceptions_enabled() {
    #ifdef WITH_EXCEPTIONS
    return true;
    #else
    return false;
    #endif
}

Error::Error(const std::string &msg) : std::runtime_error(msg) {
}

CompileError::CompileError(const std::string &msg) : Error(msg) {
}

RuntimeError::RuntimeError(const std::string &msg) : Error(msg) {
}

InternalError::InternalError(const std::string &msg) : Error(msg) {
}


namespace Internal {

// Force the classes to exist, even if exceptions are off
namespace {
CompileError _compile_error("");
RuntimeError _runtime_error("");
InternalError _internal_error("");
}

ErrorReport::ErrorReport(const char *file, int line, const char *condition_string, int flags) : flags(flags) {

    const std::string &source_loc = "";

    if (flags & User) {
        // Only mention where inside of libHalide the error tripped if we have debug level > 0
        debug(1) << "User error triggered at " << file << ":" << line << "\n";
        if (condition_string) {
            debug(1) << "Condition failed: " << condition_string << "\n";
        }
        if (flags & Warning) {
            msg << "Warning";
        } else {
            msg << "Error";
        }
        if (source_loc.empty()) {
            msg << ":\n";
        } else {
            msg << " at " << source_loc << ":\n";
        }

    } else {
        msg << "Internal ";
        if (flags & Warning) {
            msg << "warning";
        } else {
            msg << "error";
        }
        msg << " at " << file << ":" << line;
        if (!source_loc.empty()) {
            msg << " triggered by user code at " << source_loc << ":\n";
        } else {
            msg << "\n";
        }
        if (condition_string) {
            msg << "Condition failed: " << condition_string << "\n";
        }
    }
}

ErrorReport::~ErrorReport()
#if __cplusplus >= 201100 || _MSC_VER >= 1900
    noexcept(false)
#endif
{
    if (!msg.str().empty() && msg.str().back() != '\n') {
        msg << '\n';
    }

    if (custom_error_reporter != nullptr) {
        if (flags & Warning) {
            custom_error_reporter->warning(msg.str().c_str());
            return;
        } else {
            custom_error_reporter->error(msg.str().c_str());
            // error() should not have returned to us, but just in case
            // it does, make sure we don't continue.
            abort();
        }
    }

    // TODO: Add an option to error out on warnings too
    if (flags & Warning) {
        std::cerr << msg.str();
        return;
    }

#ifdef WITH_EXCEPTIONS
    if (std::uncaught_exception()) {
        // This should never happen - evaluating one of the arguments
        // to the error message would have to throw an
        // exception. Nonetheless, in case it does, preserve the
        // exception already in flight and suppress this one.
        return;
    } else if (flags & Runtime) {
        RuntimeError err(msg.str());
        throw err;
    } else if (flags & User) {
        CompileError err(msg.str());
        throw err;
    } else {
        InternalError err(msg.str());
        throw err;
    }
#else
    std::cerr << msg.str();
    abort();
#endif
}
}

}
