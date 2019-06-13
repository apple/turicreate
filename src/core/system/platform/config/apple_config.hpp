#ifndef TURI_APPLE_SYSTEM_CONFIG_H_
#define TURI_APPLE_SYSTEM_CONFIG_H_

#include <string>


#ifdef __APPLE__

namespace turi { namespace config {

void init_cocoa_multithreaded_runtime();

std::string get_apple_system_temporary_directory();

}}

#endif

#endif
