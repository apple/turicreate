// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: VisionFeaturePrint.proto

#define INTERNAL_SUPPRESS_PROTOBUF_FIELD_DEPRECATION
#include "VisionFeaturePrint.pb.h"

#include <algorithm>

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/stubs/port.h>
#include <google/protobuf/stubs/once.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/wire_format_lite_inl.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
// @@protoc_insertion_point(includes)

namespace CoreML {
namespace Specification {
namespace CoreMLModels {
class VisionFeaturePrint_SceneDefaultTypeInternal : public ::google::protobuf::internal::ExplicitlyConstructed<VisionFeaturePrint_Scene> {
} _VisionFeaturePrint_Scene_default_instance_;
class VisionFeaturePrintDefaultTypeInternal : public ::google::protobuf::internal::ExplicitlyConstructed<VisionFeaturePrint> {
  public:
  const ::CoreML::Specification::CoreMLModels::VisionFeaturePrint_Scene* scene_;
} _VisionFeaturePrint_default_instance_;

namespace protobuf_VisionFeaturePrint_2eproto {

PROTOBUF_CONSTEXPR_VAR ::google::protobuf::internal::ParseTableField
    const TableStruct::entries[] = {
  {0, 0, 0, ::google::protobuf::internal::kInvalidMask, 0, 0},
};

PROTOBUF_CONSTEXPR_VAR ::google::protobuf::internal::AuxillaryParseTableField
    const TableStruct::aux[] = {
  ::google::protobuf::internal::AuxillaryParseTableField(),
};
PROTOBUF_CONSTEXPR_VAR ::google::protobuf::internal::ParseTable const
    TableStruct::schema[] = {
  { NULL, NULL, 0, -1, -1, false },
  { NULL, NULL, 0, -1, -1, false },
};


void TableStruct::Shutdown() {
  _VisionFeaturePrint_Scene_default_instance_.Shutdown();
  _VisionFeaturePrint_default_instance_.Shutdown();
}

void TableStruct::InitDefaultsImpl() {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  ::google::protobuf::internal::InitProtobufDefaults();
  _VisionFeaturePrint_Scene_default_instance_.DefaultConstruct();
  _VisionFeaturePrint_default_instance_.DefaultConstruct();
}

void InitDefaults() {
  static GOOGLE_PROTOBUF_DECLARE_ONCE(once);
  ::google::protobuf::GoogleOnceInit(&once, &TableStruct::InitDefaultsImpl);
}
void AddDescriptorsImpl() {
  InitDefaults();
  ::google::protobuf::internal::OnShutdown(&TableStruct::Shutdown);
}

void AddDescriptors() {
  static GOOGLE_PROTOBUF_DECLARE_ONCE(once);
  ::google::protobuf::GoogleOnceInit(&once, &AddDescriptorsImpl);
}
#ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
// Force AddDescriptors() to be called at static initialization time.
struct StaticDescriptorInitializer {
  StaticDescriptorInitializer() {
    AddDescriptors();
  }
} static_descriptor_initializer;
#endif  // GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER

}  // namespace protobuf_VisionFeaturePrint_2eproto

bool VisionFeaturePrint_Scene_SceneVersion_IsValid(int value) {
  switch (value) {
    case 0:
    case 1:
      return true;
    default:
      return false;
  }
}

#if !defined(_MSC_VER) || _MSC_VER >= 1900
const VisionFeaturePrint_Scene_SceneVersion VisionFeaturePrint_Scene::SCENE_VERSION_INVALID;
const VisionFeaturePrint_Scene_SceneVersion VisionFeaturePrint_Scene::SCENE_VERSION_1;
const VisionFeaturePrint_Scene_SceneVersion VisionFeaturePrint_Scene::SceneVersion_MIN;
const VisionFeaturePrint_Scene_SceneVersion VisionFeaturePrint_Scene::SceneVersion_MAX;
const int VisionFeaturePrint_Scene::SceneVersion_ARRAYSIZE;
#endif  // !defined(_MSC_VER) || _MSC_VER >= 1900

// ===================================================================

#if !defined(_MSC_VER) || _MSC_VER >= 1900
const int VisionFeaturePrint_Scene::kVersionFieldNumber;
#endif  // !defined(_MSC_VER) || _MSC_VER >= 1900

VisionFeaturePrint_Scene::VisionFeaturePrint_Scene()
  : ::google::protobuf::MessageLite(), _internal_metadata_(NULL) {
  if (GOOGLE_PREDICT_TRUE(this != internal_default_instance())) {
    protobuf_VisionFeaturePrint_2eproto::InitDefaults();
  }
  SharedCtor();
  // @@protoc_insertion_point(constructor:CoreML.Specification.CoreMLModels.VisionFeaturePrint.Scene)
}
VisionFeaturePrint_Scene::VisionFeaturePrint_Scene(const VisionFeaturePrint_Scene& from)
  : ::google::protobuf::MessageLite(),
      _internal_metadata_(NULL),
      _cached_size_(0) {
  _internal_metadata_.MergeFrom(from._internal_metadata_);
  version_ = from.version_;
  // @@protoc_insertion_point(copy_constructor:CoreML.Specification.CoreMLModels.VisionFeaturePrint.Scene)
}

void VisionFeaturePrint_Scene::SharedCtor() {
  version_ = 0;
  _cached_size_ = 0;
}

VisionFeaturePrint_Scene::~VisionFeaturePrint_Scene() {
  // @@protoc_insertion_point(destructor:CoreML.Specification.CoreMLModels.VisionFeaturePrint.Scene)
  SharedDtor();
}

void VisionFeaturePrint_Scene::SharedDtor() {
}

void VisionFeaturePrint_Scene::SetCachedSize(int size) const {
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
}
const VisionFeaturePrint_Scene& VisionFeaturePrint_Scene::default_instance() {
  protobuf_VisionFeaturePrint_2eproto::InitDefaults();
  return *internal_default_instance();
}

VisionFeaturePrint_Scene* VisionFeaturePrint_Scene::New(::google::protobuf::Arena* arena) const {
  VisionFeaturePrint_Scene* n = new VisionFeaturePrint_Scene;
  if (arena != NULL) {
    arena->Own(n);
  }
  return n;
}

void VisionFeaturePrint_Scene::Clear() {
// @@protoc_insertion_point(message_clear_start:CoreML.Specification.CoreMLModels.VisionFeaturePrint.Scene)
  version_ = 0;
}

bool VisionFeaturePrint_Scene::MergePartialFromCodedStream(
    ::google::protobuf::io::CodedInputStream* input) {
#define DO_(EXPRESSION) if (!GOOGLE_PREDICT_TRUE(EXPRESSION)) goto failure
  ::google::protobuf::uint32 tag;
  // @@protoc_insertion_point(parse_start:CoreML.Specification.CoreMLModels.VisionFeaturePrint.Scene)
  for (;;) {
    ::std::pair< ::google::protobuf::uint32, bool> p = input->ReadTagWithCutoffNoLastTag(127u);
    tag = p.first;
    if (!p.second) goto handle_unusual;
    switch (::google::protobuf::internal::WireFormatLite::GetTagFieldNumber(tag)) {
      // .CoreML.Specification.CoreMLModels.VisionFeaturePrint.Scene.SceneVersion version = 1;
      case 1: {
        if (static_cast< ::google::protobuf::uint8>(tag) ==
            static_cast< ::google::protobuf::uint8>(8u)) {
          int value;
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   int, ::google::protobuf::internal::WireFormatLite::TYPE_ENUM>(
                 input, &value)));
          set_version(static_cast< ::CoreML::Specification::CoreMLModels::VisionFeaturePrint_Scene_SceneVersion >(value));
        } else {
          goto handle_unusual;
        }
        break;
      }

      default: {
      handle_unusual:
        if (tag == 0 ||
            ::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_END_GROUP) {
          goto success;
        }
        DO_(::google::protobuf::internal::WireFormatLite::SkipField(input, tag));
        break;
      }
    }
  }
success:
  // @@protoc_insertion_point(parse_success:CoreML.Specification.CoreMLModels.VisionFeaturePrint.Scene)
  return true;
failure:
  // @@protoc_insertion_point(parse_failure:CoreML.Specification.CoreMLModels.VisionFeaturePrint.Scene)
  return false;
#undef DO_
}

void VisionFeaturePrint_Scene::SerializeWithCachedSizes(
    ::google::protobuf::io::CodedOutputStream* output) const {
  // @@protoc_insertion_point(serialize_start:CoreML.Specification.CoreMLModels.VisionFeaturePrint.Scene)
  ::google::protobuf::uint32 cached_has_bits = 0;
  (void) cached_has_bits;

  // .CoreML.Specification.CoreMLModels.VisionFeaturePrint.Scene.SceneVersion version = 1;
  if (this->version() != 0) {
    ::google::protobuf::internal::WireFormatLite::WriteEnum(
      1, this->version(), output);
  }

  // @@protoc_insertion_point(serialize_end:CoreML.Specification.CoreMLModels.VisionFeaturePrint.Scene)
}

size_t VisionFeaturePrint_Scene::ByteSizeLong() const {
// @@protoc_insertion_point(message_byte_size_start:CoreML.Specification.CoreMLModels.VisionFeaturePrint.Scene)
  size_t total_size = 0;

  // .CoreML.Specification.CoreMLModels.VisionFeaturePrint.Scene.SceneVersion version = 1;
  if (this->version() != 0) {
    total_size += 1 +
      ::google::protobuf::internal::WireFormatLite::EnumSize(this->version());
  }

  int cached_size = ::google::protobuf::internal::ToCachedSize(total_size);
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = cached_size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
  return total_size;
}

void VisionFeaturePrint_Scene::CheckTypeAndMergeFrom(
    const ::google::protobuf::MessageLite& from) {
  MergeFrom(*::google::protobuf::down_cast<const VisionFeaturePrint_Scene*>(&from));
}

void VisionFeaturePrint_Scene::MergeFrom(const VisionFeaturePrint_Scene& from) {
// @@protoc_insertion_point(class_specific_merge_from_start:CoreML.Specification.CoreMLModels.VisionFeaturePrint.Scene)
  GOOGLE_DCHECK_NE(&from, this);
  _internal_metadata_.MergeFrom(from._internal_metadata_);
  ::google::protobuf::uint32 cached_has_bits = 0;
  (void) cached_has_bits;

  if (from.version() != 0) {
    set_version(from.version());
  }
}

void VisionFeaturePrint_Scene::CopyFrom(const VisionFeaturePrint_Scene& from) {
// @@protoc_insertion_point(class_specific_copy_from_start:CoreML.Specification.CoreMLModels.VisionFeaturePrint.Scene)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool VisionFeaturePrint_Scene::IsInitialized() const {
  return true;
}

void VisionFeaturePrint_Scene::Swap(VisionFeaturePrint_Scene* other) {
  if (other == this) return;
  InternalSwap(other);
}
void VisionFeaturePrint_Scene::InternalSwap(VisionFeaturePrint_Scene* other) {
  std::swap(version_, other->version_);
  std::swap(_cached_size_, other->_cached_size_);
}

::std::string VisionFeaturePrint_Scene::GetTypeName() const {
  return "CoreML.Specification.CoreMLModels.VisionFeaturePrint.Scene";
}

#if PROTOBUF_INLINE_NOT_IN_HEADERS
// VisionFeaturePrint_Scene

// .CoreML.Specification.CoreMLModels.VisionFeaturePrint.Scene.SceneVersion version = 1;
void VisionFeaturePrint_Scene::clear_version() {
  version_ = 0;
}
::CoreML::Specification::CoreMLModels::VisionFeaturePrint_Scene_SceneVersion VisionFeaturePrint_Scene::version() const {
  // @@protoc_insertion_point(field_get:CoreML.Specification.CoreMLModels.VisionFeaturePrint.Scene.version)
  return static_cast< ::CoreML::Specification::CoreMLModels::VisionFeaturePrint_Scene_SceneVersion >(version_);
}
void VisionFeaturePrint_Scene::set_version(::CoreML::Specification::CoreMLModels::VisionFeaturePrint_Scene_SceneVersion value) {
  
  version_ = value;
  // @@protoc_insertion_point(field_set:CoreML.Specification.CoreMLModels.VisionFeaturePrint.Scene.version)
}

#endif  // PROTOBUF_INLINE_NOT_IN_HEADERS

// ===================================================================

#if !defined(_MSC_VER) || _MSC_VER >= 1900
const int VisionFeaturePrint::kSceneFieldNumber;
#endif  // !defined(_MSC_VER) || _MSC_VER >= 1900

VisionFeaturePrint::VisionFeaturePrint()
  : ::google::protobuf::MessageLite(), _internal_metadata_(NULL) {
  if (GOOGLE_PREDICT_TRUE(this != internal_default_instance())) {
    protobuf_VisionFeaturePrint_2eproto::InitDefaults();
  }
  SharedCtor();
  // @@protoc_insertion_point(constructor:CoreML.Specification.CoreMLModels.VisionFeaturePrint)
}
VisionFeaturePrint::VisionFeaturePrint(const VisionFeaturePrint& from)
  : ::google::protobuf::MessageLite(),
      _internal_metadata_(NULL),
      _cached_size_(0) {
  _internal_metadata_.MergeFrom(from._internal_metadata_);
  clear_has_VisionFeaturePrintType();
  switch (from.VisionFeaturePrintType_case()) {
    case kScene: {
      mutable_scene()->::CoreML::Specification::CoreMLModels::VisionFeaturePrint_Scene::MergeFrom(from.scene());
      break;
    }
    case VISIONFEATUREPRINTTYPE_NOT_SET: {
      break;
    }
  }
  // @@protoc_insertion_point(copy_constructor:CoreML.Specification.CoreMLModels.VisionFeaturePrint)
}

void VisionFeaturePrint::SharedCtor() {
  clear_has_VisionFeaturePrintType();
  _cached_size_ = 0;
}

VisionFeaturePrint::~VisionFeaturePrint() {
  // @@protoc_insertion_point(destructor:CoreML.Specification.CoreMLModels.VisionFeaturePrint)
  SharedDtor();
}

void VisionFeaturePrint::SharedDtor() {
  if (has_VisionFeaturePrintType()) {
    clear_VisionFeaturePrintType();
  }
}

void VisionFeaturePrint::SetCachedSize(int size) const {
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
}
const VisionFeaturePrint& VisionFeaturePrint::default_instance() {
  protobuf_VisionFeaturePrint_2eproto::InitDefaults();
  return *internal_default_instance();
}

VisionFeaturePrint* VisionFeaturePrint::New(::google::protobuf::Arena* arena) const {
  VisionFeaturePrint* n = new VisionFeaturePrint;
  if (arena != NULL) {
    arena->Own(n);
  }
  return n;
}

void VisionFeaturePrint::clear_VisionFeaturePrintType() {
// @@protoc_insertion_point(one_of_clear_start:CoreML.Specification.CoreMLModels.VisionFeaturePrint)
  switch (VisionFeaturePrintType_case()) {
    case kScene: {
      delete VisionFeaturePrintType_.scene_;
      break;
    }
    case VISIONFEATUREPRINTTYPE_NOT_SET: {
      break;
    }
  }
  _oneof_case_[0] = VISIONFEATUREPRINTTYPE_NOT_SET;
}


void VisionFeaturePrint::Clear() {
// @@protoc_insertion_point(message_clear_start:CoreML.Specification.CoreMLModels.VisionFeaturePrint)
  clear_VisionFeaturePrintType();
}

bool VisionFeaturePrint::MergePartialFromCodedStream(
    ::google::protobuf::io::CodedInputStream* input) {
#define DO_(EXPRESSION) if (!GOOGLE_PREDICT_TRUE(EXPRESSION)) goto failure
  ::google::protobuf::uint32 tag;
  // @@protoc_insertion_point(parse_start:CoreML.Specification.CoreMLModels.VisionFeaturePrint)
  for (;;) {
    ::std::pair< ::google::protobuf::uint32, bool> p = input->ReadTagWithCutoffNoLastTag(16383u);
    tag = p.first;
    if (!p.second) goto handle_unusual;
    switch (::google::protobuf::internal::WireFormatLite::GetTagFieldNumber(tag)) {
      // .CoreML.Specification.CoreMLModels.VisionFeaturePrint.Scene scene = 20;
      case 20: {
        if (static_cast< ::google::protobuf::uint8>(tag) ==
            static_cast< ::google::protobuf::uint8>(162u)) {
          DO_(::google::protobuf::internal::WireFormatLite::ReadMessageNoVirtual(
               input, mutable_scene()));
        } else {
          goto handle_unusual;
        }
        break;
      }

      default: {
      handle_unusual:
        if (tag == 0 ||
            ::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_END_GROUP) {
          goto success;
        }
        DO_(::google::protobuf::internal::WireFormatLite::SkipField(input, tag));
        break;
      }
    }
  }
success:
  // @@protoc_insertion_point(parse_success:CoreML.Specification.CoreMLModels.VisionFeaturePrint)
  return true;
failure:
  // @@protoc_insertion_point(parse_failure:CoreML.Specification.CoreMLModels.VisionFeaturePrint)
  return false;
#undef DO_
}

void VisionFeaturePrint::SerializeWithCachedSizes(
    ::google::protobuf::io::CodedOutputStream* output) const {
  // @@protoc_insertion_point(serialize_start:CoreML.Specification.CoreMLModels.VisionFeaturePrint)
  ::google::protobuf::uint32 cached_has_bits = 0;
  (void) cached_has_bits;

  // .CoreML.Specification.CoreMLModels.VisionFeaturePrint.Scene scene = 20;
  if (has_scene()) {
    ::google::protobuf::internal::WireFormatLite::WriteMessage(
      20, *VisionFeaturePrintType_.scene_, output);
  }

  // @@protoc_insertion_point(serialize_end:CoreML.Specification.CoreMLModels.VisionFeaturePrint)
}

size_t VisionFeaturePrint::ByteSizeLong() const {
// @@protoc_insertion_point(message_byte_size_start:CoreML.Specification.CoreMLModels.VisionFeaturePrint)
  size_t total_size = 0;

  switch (VisionFeaturePrintType_case()) {
    // .CoreML.Specification.CoreMLModels.VisionFeaturePrint.Scene scene = 20;
    case kScene: {
      total_size += 2 +
        ::google::protobuf::internal::WireFormatLite::MessageSizeNoVirtual(
          *VisionFeaturePrintType_.scene_);
      break;
    }
    case VISIONFEATUREPRINTTYPE_NOT_SET: {
      break;
    }
  }
  int cached_size = ::google::protobuf::internal::ToCachedSize(total_size);
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = cached_size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
  return total_size;
}

void VisionFeaturePrint::CheckTypeAndMergeFrom(
    const ::google::protobuf::MessageLite& from) {
  MergeFrom(*::google::protobuf::down_cast<const VisionFeaturePrint*>(&from));
}

void VisionFeaturePrint::MergeFrom(const VisionFeaturePrint& from) {
// @@protoc_insertion_point(class_specific_merge_from_start:CoreML.Specification.CoreMLModels.VisionFeaturePrint)
  GOOGLE_DCHECK_NE(&from, this);
  _internal_metadata_.MergeFrom(from._internal_metadata_);
  ::google::protobuf::uint32 cached_has_bits = 0;
  (void) cached_has_bits;

  switch (from.VisionFeaturePrintType_case()) {
    case kScene: {
      mutable_scene()->::CoreML::Specification::CoreMLModels::VisionFeaturePrint_Scene::MergeFrom(from.scene());
      break;
    }
    case VISIONFEATUREPRINTTYPE_NOT_SET: {
      break;
    }
  }
}

void VisionFeaturePrint::CopyFrom(const VisionFeaturePrint& from) {
// @@protoc_insertion_point(class_specific_copy_from_start:CoreML.Specification.CoreMLModels.VisionFeaturePrint)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool VisionFeaturePrint::IsInitialized() const {
  return true;
}

void VisionFeaturePrint::Swap(VisionFeaturePrint* other) {
  if (other == this) return;
  InternalSwap(other);
}
void VisionFeaturePrint::InternalSwap(VisionFeaturePrint* other) {
  std::swap(VisionFeaturePrintType_, other->VisionFeaturePrintType_);
  std::swap(_oneof_case_[0], other->_oneof_case_[0]);
  std::swap(_cached_size_, other->_cached_size_);
}

::std::string VisionFeaturePrint::GetTypeName() const {
  return "CoreML.Specification.CoreMLModels.VisionFeaturePrint";
}

#if PROTOBUF_INLINE_NOT_IN_HEADERS
// VisionFeaturePrint

// .CoreML.Specification.CoreMLModels.VisionFeaturePrint.Scene scene = 20;
bool VisionFeaturePrint::has_scene() const {
  return VisionFeaturePrintType_case() == kScene;
}
void VisionFeaturePrint::set_has_scene() {
  _oneof_case_[0] = kScene;
}
void VisionFeaturePrint::clear_scene() {
  if (has_scene()) {
    delete VisionFeaturePrintType_.scene_;
    clear_has_VisionFeaturePrintType();
  }
}
 const ::CoreML::Specification::CoreMLModels::VisionFeaturePrint_Scene& VisionFeaturePrint::scene() const {
  // @@protoc_insertion_point(field_get:CoreML.Specification.CoreMLModels.VisionFeaturePrint.scene)
  return has_scene()
      ? *VisionFeaturePrintType_.scene_
      : ::CoreML::Specification::CoreMLModels::VisionFeaturePrint_Scene::default_instance();
}
::CoreML::Specification::CoreMLModels::VisionFeaturePrint_Scene* VisionFeaturePrint::mutable_scene() {
  if (!has_scene()) {
    clear_VisionFeaturePrintType();
    set_has_scene();
    VisionFeaturePrintType_.scene_ = new ::CoreML::Specification::CoreMLModels::VisionFeaturePrint_Scene;
  }
  // @@protoc_insertion_point(field_mutable:CoreML.Specification.CoreMLModels.VisionFeaturePrint.scene)
  return VisionFeaturePrintType_.scene_;
}
::CoreML::Specification::CoreMLModels::VisionFeaturePrint_Scene* VisionFeaturePrint::release_scene() {
  // @@protoc_insertion_point(field_release:CoreML.Specification.CoreMLModels.VisionFeaturePrint.scene)
  if (has_scene()) {
    clear_has_VisionFeaturePrintType();
    ::CoreML::Specification::CoreMLModels::VisionFeaturePrint_Scene* temp = VisionFeaturePrintType_.scene_;
    VisionFeaturePrintType_.scene_ = NULL;
    return temp;
  } else {
    return NULL;
  }
}
void VisionFeaturePrint::set_allocated_scene(::CoreML::Specification::CoreMLModels::VisionFeaturePrint_Scene* scene) {
  clear_VisionFeaturePrintType();
  if (scene) {
    set_has_scene();
    VisionFeaturePrintType_.scene_ = scene;
  }
  // @@protoc_insertion_point(field_set_allocated:CoreML.Specification.CoreMLModels.VisionFeaturePrint.scene)
}

bool VisionFeaturePrint::has_VisionFeaturePrintType() const {
  return VisionFeaturePrintType_case() != VISIONFEATUREPRINTTYPE_NOT_SET;
}
void VisionFeaturePrint::clear_has_VisionFeaturePrintType() {
  _oneof_case_[0] = VISIONFEATUREPRINTTYPE_NOT_SET;
}
VisionFeaturePrint::VisionFeaturePrintTypeCase VisionFeaturePrint::VisionFeaturePrintType_case() const {
  return VisionFeaturePrint::VisionFeaturePrintTypeCase(_oneof_case_[0]);
}
#endif  // PROTOBUF_INLINE_NOT_IN_HEADERS

// @@protoc_insertion_point(namespace_scope)

}  // namespace CoreMLModels
}  // namespace Specification
}  // namespace CoreML

// @@protoc_insertion_point(global_scope)
