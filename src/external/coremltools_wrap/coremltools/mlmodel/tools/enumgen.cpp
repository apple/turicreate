#include "../../deps/protobuf/src/google/protobuf/compiler/plugin.pb.h"

#include <iostream>
#include <sstream>
#include <string>

using namespace google::protobuf;
using namespace google::protobuf::compiler;

const char *INDENT = "    ";

// extract the portion of filename before the first . character
static std::string basename(const std::string& filename) {
  size_t dotPosition = filename.find('.');
  assert(dotPosition != std::string::npos);
  return filename.substr(0, dotPosition);
}

// make a filename like foo_enums.h from foo.proto
static std::string makeExt(const std::string& filename) {
  std::stringstream result;
  result << basename(filename);
  result << "_enums.h";
  return result.str();
}

static std::string makeGuard(const std::string& filename) {
  std::stringstream result;
  std::string upperBasename = basename(filename);
  for (auto & c: upperBasename) c = std::toupper(c);
  result << "__" << upperBasename << "_ENUMS_H";
  return result.str();
}

// handle enum types inside either a message (Descriptor)
// or a top level file (FileDescriptor).
template<typename T>
static void handleContainer(const T& container, std::ostream& ret) {
  // flat structure
  for (size_t i=0; i<container.enum_type_count(); i++) {
    const auto* enumType = container.enum_type(i);
    ret << "enum ML" << enumType->name() << ": int {" << std::endl;
    for (size_t j=0; j<enumType->value_count(); j++) {
      const auto* enumValue = enumType->value(j);
      ret << INDENT << "ML" << enumType->name() << enumValue->name();
      ret << " = " << enumValue->number() << "," << std::endl;
    }
    ret << "};" << std::endl << std::endl;
  }
}

// recursively handle message types
static void handleMessage(const Descriptor& message, std::ostream& ret) {
    handleContainer(message, ret);

    for (size_t i=0; i<message.oneof_decl_count(); i++) {
        const auto* oneofType = message.oneof_decl(i);

        // generate an enum
        std::stringstream enumName;
        enumName << "ML" << message.name() << oneofType->name();
        ret << "enum " << enumName.str() << ": int {" << std::endl;
        for (size_t j=0; j<oneofType->field_count(); j++) {
            const auto* field = oneofType->field(j);
            ret << INDENT << enumName.str() << "_" << field->name();
            ret << " = " << field->number() << "," << std::endl;
        }
        ret << INDENT << enumName.str() << "_NOT_SET = 0," << std::endl;
        ret << "};" << std::endl << std::endl;

        // generate a name function (reverse lookup)
        ret << "__attribute__((__unused__))" << std::endl;
        ret << "static const char * " << enumName.str() << "_Name(" << enumName.str() << " x) {" << std::endl;
        ret << INDENT << "switch (x) {" << std::endl;
        for (size_t j=0; j<oneofType->field_count(); j++) {
            const auto* field = oneofType->field(j);
            ret << INDENT << INDENT << "case " << enumName.str() << "_" << field->name();
            ret << ":" << std::endl;
            ret << INDENT << INDENT << INDENT;
            ret << "return \"" << enumName.str() << "_" << field->name() << "\";" << std::endl;
        }
        ret << INDENT << INDENT << "case " << enumName.str() << "_NOT_SET:" << std::endl;
        ret << INDENT << INDENT << INDENT << "return \"INVALID\";" << std::endl;
        ret << INDENT << "}" << std::endl;
        ret << "}" << std::endl << std::endl;
  }

  for (size_t i=0; i<message.nested_type_count(); i++) {
    const auto* nested = message.nested_type(i);
    handleMessage(*nested, ret);
  }
}

// handle converting a FileDescriptor to enum strings
static std::string makeContents(const FileDescriptor& in) {
  std::stringstream ret;

  std::string guard = makeGuard(in.name());
  ret << "#ifndef " << guard << std::endl;
  ret << "#define " << guard << std::endl;

  handleContainer(in, ret);
  for (size_t i=0; i<in.message_type_count(); i++) {
    const auto* message = in.message_type(i);
    handleMessage(*message, ret);
  }

  ret << "#endif" << std::endl;

  return ret.str();
}

int main(int argc, char *argv[]) {
  CodeGeneratorRequest request;
  CodeGeneratorResponse response;
  DescriptorPool pool;

  bool result = request.ParseFromIstream(&std::cin);
  if (!result) { return 1; }

  for (const auto& fileDescriptorProto : request.proto_file()) {
    const auto* fileDescriptor = pool.BuildFile(fileDescriptorProto);
    auto* outputFile = response.add_file();
    outputFile->set_name(makeExt(fileDescriptor->name()));
    outputFile->set_content(makeContents(*fileDescriptor));
  }

  result = response.SerializeToOstream(&std::cout);
  if (!result) { return 2; }

  return 0;
}
