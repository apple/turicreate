#include <fstream>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/text_format.h>
#include <iostream>
#include <string>

int main(int argc, char* argv[])
{
  std::ifstream fs;
  fs.open(argv[1], std::ifstream::in);
  google::protobuf::FileDescriptorSet file_descriptor_set;
  file_descriptor_set.ParseFromIstream(&fs);

  const google::protobuf::DescriptorPool* compiled_pool =
    google::protobuf::DescriptorPool::generated_pool();

  if (compiled_pool == NULL) {
    std::cerr << "compiled pool is NULL." << std::endl;
    return 1;
  }

  google::protobuf::DescriptorPool pool(compiled_pool);
  google::protobuf::DynamicMessageFactory dynamic_message_factory(&pool);

  for (const google::protobuf::FileDescriptorProto& file_descriptor_proto :
       file_descriptor_set.file()) {
    const google::protobuf::FileDescriptor* file_descriptor =
      pool.BuildFile(file_descriptor_proto);
    if (file_descriptor == NULL) {
      continue;
    }

    const google::protobuf::Descriptor* descriptor =
      pool.FindMessageTypeByName("example.msgs.ExampleDesc");

    if (descriptor == NULL) {
      continue;
    }

    google::protobuf::Message* msg =
      dynamic_message_factory.GetPrototype(descriptor)->New();
    std::string data = "data: 1";
    bool success = google::protobuf::TextFormat::ParseFromString(data, msg);

    if (success) {
      return 0;
    } else {
      std::cerr << "Failed to parse message." << std::endl;
      return 2;
    }
  }

  std::cerr << "No matching message found." << std::endl;
  return 3;
}
