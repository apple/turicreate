#ifndef _FORMAT_HPP
#define _FORMAT_HPP

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#pragma clang diagnostic ignored "-Wshadow"
#if __apple_build_version__ < 10010028
#pragma clang diagnostic ignored "-Wextended-offsetof"
#endif

#include <google/protobuf/text_format.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

#include "../build/format/Model.pb.h"
#include "../build/format/Model_enums.h"

#pragma clang diagnostic pop

#endif
