#ifndef _FORMAT_HPP
#define _FORMAT_HPP

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#pragma clang diagnostic ignored "-Wshadow"
#pragma clang diagnostic ignored "-Wextended-offsetof"

#include <google/protobuf/text_format.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

#include "../build/format/Model.pb.h"

#pragma clang diagnostic pop

#endif
