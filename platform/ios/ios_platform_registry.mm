#import <pingcore/ios/external_texture.h>
#import <pingcore/ios/platform_registry.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "ios_external_texture_internal.h"
#include "ios_application_internal.h"
#include "ios_file_internal.h"
#include "application/platform_registry_internal.h"

using pingcore::BufferReference;
using pingcore::Bytes;
using pingcore::ExternalTexture;
using pingcore::FileReference;
using pingcore::PlatformError;
using pingcore::PlatformEventEmitter;
using pingcore::PlatformPayload;
using pingcore::PlatformPayloadKind;
using pingcore::PlatformResult;

[[noreturn]] static void RaiseInvalidArgument(NSString* reason) {
  @throw [NSException exceptionWithName:NSInvalidArgumentException reason:reason userInfo:nil];
}

[[noreturn]] static void RaiseCppException(const std::exception& exception) {
  NSString* reason = [NSString stringWithUTF8String:exception.what()];
  RaiseInvalidArgument(reason == nil ? @"PingCore Apple platform bridge rejected a value" : reason);
}

static std::string ToCppString(NSString* value, const char* description) {
  if (value == nil) {
    throw std::invalid_argument(std::string("PingCore ") + description + " must not be nil");
  }
  NSData* data = [value dataUsingEncoding:NSUTF8StringEncoding allowLossyConversion:NO];
  if (data == nil) {
    throw std::invalid_argument(std::string("PingCore ") + description + " must contain valid Unicode");
  }
  return std::string(static_cast<const char*>(data.bytes), data.length);
}

static NSString* ToNSString(std::string_view value) {
  NSString* result = [[NSString alloc] initWithBytes:value.data()
                                              length:value.size()
                                            encoding:NSUTF8StringEncoding];
  if (result == nil) {
    throw std::logic_error("PingCore PlatformPayload contained invalid UTF-8");
  }
  return result;
}

// The NSURL identifies the resource. Keep the FileReference beside it to retain coordinated access
// during asynchronous native media work.
@interface HUXFileReference () {
@private
  std::optional<FileReference> reference_;
  __strong NSURL* file_url_;
}
- (instancetype)initForPingCoreWithReference:(FileReference)reference fileURL:(NSURL*)file_url;
- (FileReference)referenceForPingCore;
@end

static HUXFileReference* WrapFileReference(FileReference reference) {
  const std::optional<pingcore::File> file = reference.AsFile();
  if (!file) {
    throw std::invalid_argument("PingCore iOS FileReference has no native file URL");
  }
  NSURL* url = [NSURL fileURLWithPath:ToNSString(file->Path())
                          isDirectory:reference.Type() == pingcore::FileType::Directory];
  if (url == nil) {
    throw std::runtime_error("PingCore iOS file URL could not be created");
  }
  return [[HUXFileReference alloc] initForPingCoreWithReference:std::move(reference) fileURL:url];
}

static FileReference UnwrapFileReference(HUXFileReference* reference) {
  if (reference == nil) {
    throw std::invalid_argument("PingCore Apple platform boundary requires a FileReference value");
  }
  return [reference referenceForPingCore];
}

@interface HUXBufferReference () {
@private
  BufferReference reference_;
}
- (instancetype)initForPingCoreWithReference:(BufferReference)reference;
- (BufferReference)referenceForPingCore;
@end

static HUXBufferReference* WrapBufferReference(BufferReference reference) {
  return [[HUXBufferReference alloc] initForPingCoreWithReference:std::move(reference)];
}

static BufferReference UnwrapBufferReference(HUXBufferReference* reference) {
  if (reference == nil) {
    throw std::invalid_argument("PingCore platform boundary requires a BufferReference value");
  }
  return [reference referenceForPingCore];
}

// HUXP capability slots index these retained wrapper arrays, so bytes and wrappers must share one object lifetime.
@interface HUXPlatformPayload () {
@private
  Bytes bytes_;
  __strong NSArray<HUXExternalTexture*>* textures_;
  __strong NSArray<HUXFileReference*>* file_references_;
  __strong NSArray<HUXBufferReference*>* buffer_references_;
}
- (instancetype)initForPingCoreWithEnvelope:(PlatformPayload::Envelope)envelope;
- (PlatformPayload)platformPayloadForPingCore;
@end

static PlatformPayload DecodePayload(HUXPlatformPayload* payload) {
  if (payload == nil) {
    throw std::invalid_argument("PingCore Apple platform boundary requires a PlatformPayload value");
  }
  return [payload platformPayloadForPingCore];
}

static HUXPlatformPayload* EncodePayload(PlatformPayload payload) {
  return [[HUXPlatformPayload alloc] initForPingCoreWithEnvelope:payload.Encode()];
}

HUXPlatformPayload* HUXGetLocalNotificationData(UNNotificationContent* content) {
  try {
    @try {
      return EncodePayload(pingcore::detail::DecodeIosLocalNotificationData(content));
    } @catch (NSException*) {
      return nil;
    }
  } catch (...) {
    return nil;
  }
}

static HUXPlatformPayloadKind ToObjectiveCKind(PlatformPayloadKind kind) {
  switch (kind) {
  case PlatformPayloadKind::Null:
    return HUXPlatformPayloadKindNull;
  case PlatformPayloadKind::Boolean:
    return HUXPlatformPayloadKindBoolean;
  case PlatformPayloadKind::Integer:
    return HUXPlatformPayloadKindInteger;
  case PlatformPayloadKind::Double:
    return HUXPlatformPayloadKindDouble;
  case PlatformPayloadKind::String:
    return HUXPlatformPayloadKindString;
  case PlatformPayloadKind::Bytes:
    return HUXPlatformPayloadKindBytes;
  case PlatformPayloadKind::List:
    return HUXPlatformPayloadKindList;
  case PlatformPayloadKind::Object:
    return HUXPlatformPayloadKindObject;
  case PlatformPayloadKind::ExternalTexture:
    return HUXPlatformPayloadKindExternalTexture;
  case PlatformPayloadKind::FileReference:
    return HUXPlatformPayloadKindFileReference;
  case PlatformPayloadKind::BufferReference:
    return HUXPlatformPayloadKindBufferReference;
  }
  throw std::logic_error("PingCore PlatformPayload contained an unknown kind");
}

@interface HUXIOSPlatformEventEmitter : NSObject <HUXPlatformEventEmitter> {
@private
  std::mutex mutex_;
  PlatformEventEmitter events_;
  bool active_;
}
- (instancetype)initWithEvents:(PlatformEventEmitter)events;
- (void)close;
@end

@implementation HUXIOSPlatformEventEmitter

- (instancetype)initWithEvents:(PlatformEventEmitter)events {
  self = [super init];
  if (self != nil) {
    events_ = std::move(events);
    active_ = true;
  }
  return self;
}

- (nullable HUXPlatformPayload*)emit:(NSString*)event payload:(HUXPlatformPayload*)payload {
  try {
    const std::string name = ToCppString(event, "platform event name");
    PlatformPayload value = DecodePayload(payload);
    PlatformEventEmitter emitter;
    {
      std::lock_guard lock(mutex_);
      if (!active_) {
        return nil;
      }
      emitter = events_;
    }
    // Native handlers may synchronously close or reenter this bridge, so invoke a retained copy outside the lock.
    std::optional<PlatformPayload> result = emitter.Emit(name, std::move(value));
    return result.has_value() ? EncodePayload(std::move(*result)) : nil;
  } catch (...) {
    return nil;
  }
}

- (void)close {
  std::lock_guard lock(mutex_);
  active_ = false;
  events_ = {};
}

@end

@interface HUXIOSPlatformResult : NSObject <HUXPlatformResult> {
@private
  std::mutex mutex_;
  std::function<void(PlatformResult<PlatformPayload>)> completion_;
}
- (instancetype)initWithCompletion:(std::function<void(PlatformResult<PlatformPayload>)>)completion;
- (std::function<void(PlatformResult<PlatformPayload>)>)takeCompletion;
- (void)close;
@end

@implementation HUXIOSPlatformResult

- (instancetype)initWithCompletion:(std::function<void(PlatformResult<PlatformPayload>)>)completion {
  self = [super init];
  if (self != nil) {
    completion_ = std::move(completion);
  }
  return self;
}

- (std::function<void(PlatformResult<PlatformPayload>)>)takeCompletion {
  std::lock_guard lock(mutex_);
  return std::exchange(completion_, {});
}

- (void)complete:(HUXPlatformPayload*)value {
  PlatformPayload payload;
  try {
    payload = DecodePayload(value);
  } catch (...) {
    if (auto completion = [self takeCompletion]) {
      completion(PlatformError{"pingcore/invalid-result",
                               "PingCore Apple platform call returned an invalid result payload", {}});
    }
    return;
  }
  std::function<void(PlatformResult<PlatformPayload>)> completion = [self takeCompletion];
  if (completion) {
    completion(std::move(payload));
  }
}

- (void)failWithCode:(NSString*)code
             message:(NSString*)message
             details:(HUXPlatformPayload*)details {
  PlatformError error;
  try {
    error = {ToCppString(code, "platform error code"), ToCppString(message, "platform error message"),
             DecodePayload(details)};
  } catch (...) {
    if (auto completion = [self takeCompletion]) {
      completion(PlatformError{"pingcore/invalid-error", "PingCore Apple platform call returned an invalid error", {}});
    }
    return;
  }
  std::function<void(PlatformResult<PlatformPayload>)> completion = [self takeCompletion];
  if (completion) {
    completion(std::move(error));
  }
}

- (void)close {
  std::lock_guard lock(mutex_);
  completion_ = {};
}

@end

class ObjectiveCPlatformInstanceBridge final {
public:
  ObjectiveCPlatformInstanceBridge(id instance, HUXIOSPlatformEventEmitter* events)
      : instance_(instance), events_(events) {}

  ~ObjectiveCPlatformInstanceBridge() {
    Dispose();
  }

  std::function<void()> Invoke(std::string method, PlatformPayload arguments,
                               std::function<void(PlatformResult<PlatformPayload>)> completion) {
    if (disposed_) {
      throw std::logic_error("PingCore iOS Objective-C platform instance is disposed");
    }
    NSString* method_name = ToNSString(method);
    HUXPlatformPayload* payload = EncodePayload(std::move(arguments));
    HUXIOSPlatformResult* result = [[HUXIOSPlatformResult alloc] initWithCompletion:std::move(completion)];
    __strong id<HUXPlatformCancellation> cancellation = nil;
    @try {
      cancellation = [instance_ invoke:method_name arguments:payload result:result];
      if (cancellation != nil && ![cancellation respondsToSelector:@selector(cancel)]) {
        throw std::logic_error("PingCore iOS Objective-C invocation returned an invalid cancellation object");
      }
    } @catch (NSException* exception) {
      [result close];
      throw std::logic_error("PingCore iOS Objective-C platform invocation raised an exception");
    }
    return [result, cancellation] {
      [result close];
      if (cancellation != nil) {
        @try {
          [cancellation cancel];
        } @catch (NSException*) {}
      }
    };
  }

  void CloseEvents() noexcept {
    [events_ close];
  }

  void Dispose() noexcept {
    if (disposed_) {
      return;
    }
    disposed_ = true;
    [events_ close];
    @try {
      [instance_ dispose];
    } @catch (NSException*) {}
    instance_ = nil;
    events_ = nil;
  }

  [[nodiscard]] id Instance() const noexcept {
    return instance_;
  }

  [[nodiscard]] bool Disposed() const noexcept {
    return disposed_;
  }

private:
  __strong id instance_ = nil;
  __strong HUXIOSPlatformEventEmitter* events_ = nil;
  bool disposed_ = false;
};

static void ConnectInstance(const pingcore::detail::PlatformChannelEndpoint& endpoint,
                            const std::shared_ptr<ObjectiveCPlatformInstanceBridge>& instance) {
  endpoint.Connect({
      .invoke = [instance](std::string method, PlatformPayload arguments,
                           std::function<void(PlatformResult<PlatformPayload>)> completion) {
        return instance->Invoke(std::move(method), std::move(arguments), std::move(completion));
      },
      .dispose = [instance] { instance->Dispose(); },
  });
}

@implementation HUXFileReference

+ (instancetype)copyFromFileURL:(NSURL*)url error:(NSError**)error {
  if (error != nullptr) { *error = nil; }
  try {
    return WrapFileReference(pingcore::detail::MakeCopiedIosFileReference(url, error));
  } catch (const std::exception& exception) {
    if (error != nullptr && *error == nil) {
      NSString* message = [NSString stringWithUTF8String:exception.what()];
      *error = [NSError errorWithDomain:NSCocoaErrorDomain code:NSFileReadUnknownError
                              userInfo:@{NSLocalizedDescriptionKey:
                                  message != nil ? message : @"PingCore could not copy the iOS file reference"}];
    }
    return nil;
  }
}

- (instancetype)initForPingCoreWithReference:(FileReference)reference fileURL:(NSURL*)file_url {
  self = [super init];
  if (self != nil) {
    reference_.emplace(std::move(reference));
    file_url_ = file_url;
  }
  return self;
}

- (FileReference)referenceForPingCore {
  if (!reference_.has_value() || file_url_ == nil) {
    throw std::invalid_argument("PingCore Apple platform boundary received an invalid FileReference value");
  }
  return *reference_;
}

- (NSURL*)fileURL {
  try {
    if (!reference_.has_value() || file_url_ == nil) {
      throw std::invalid_argument("PingCore Apple platform boundary received an invalid FileReference value");
    }
    return file_url_;
  } catch (const std::exception& exception) {
    RaiseCppException(exception);
  }
}

@end

@implementation HUXBufferReference

- (instancetype)initForPingCoreWithReference:(BufferReference)reference {
  self = [super init];
  if (self != nil) {
    reference_ = std::move(reference);
  }
  return self;
}

- (instancetype)initWithBytes:(const void*)bytes length:(NSUInteger)length owner:(id)owner {
  if (owner == nil || (bytes == nullptr && length != 0)) {
    RaiseInvalidArgument(@"PingCore buffer reference requires retained, address-stable storage");
  }
  try {
    struct Owner {
      __strong id object;
    };
    auto retained = std::make_shared<Owner>(owner);
    return [self initForPingCoreWithReference:BufferReference(
        {static_cast<const std::byte*>(bytes), length}, std::move(retained))];
  } catch (const std::exception& exception) {
    RaiseCppException(exception);
  }
}

- (BufferReference)referenceForPingCore {
  return reference_;
}

- (void)withUnsafeBytes:(void (^)(const void*, NSUInteger))reader {
  if (reader == nil) {
    RaiseInvalidArgument(@"PingCore buffer reader must not be nil");
  }
  const BufferReference retained = reference_;
  const auto bytes = retained.AsBytes();
  reader(bytes.data(), bytes.size());
}

- (HUXBufferReference*)sliceWithOffset:(NSUInteger)offset length:(NSUInteger)length {
  try {
    return WrapBufferReference(reference_.Slice(offset, length));
  } catch (const std::exception& exception) {
    RaiseCppException(exception);
  }
}

- (BOOL)isEqual:(id)object {
  return [object isKindOfClass:[HUXBufferReference class]] &&
         reference_ == [static_cast<HUXBufferReference*>(object) referenceForPingCore];
}

- (NSUInteger)hash {
  const auto bytes = reference_.AsBytes();
  return reinterpret_cast<std::uintptr_t>(bytes.data()) ^ bytes.size();
}

@end

@implementation HUXPlatformPayload

- (instancetype)initForPingCoreWithEnvelope:(PlatformPayload::Envelope)envelope {
  self = [super init];
  if (self != nil) {
    bytes_ = std::move(envelope.bytes);
    NSMutableArray<HUXExternalTexture*>* textures =
        [NSMutableArray arrayWithCapacity:envelope.external_textures.size()];
    for (std::shared_ptr<ExternalTexture>& texture : envelope.external_textures) {
      [textures addObject:pingcore::ios::detail::WrapExternalTexture(std::move(texture))];
    }
    textures_ = [textures copy];
    NSMutableArray<HUXFileReference*>* file_references =
        [NSMutableArray arrayWithCapacity:envelope.file_references.size()];
    for (FileReference& reference : envelope.file_references) {
      [file_references addObject:WrapFileReference(std::move(reference))];
    }
    file_references_ = [file_references copy];
    NSMutableArray<HUXBufferReference*>* buffers = [NSMutableArray arrayWithCapacity:envelope.buffer_references.size()];
    for (BufferReference& reference : envelope.buffer_references) {
      [buffers addObject:WrapBufferReference(std::move(reference))];
    }
    buffer_references_ = [buffers copy];
  }
  return self;
}

- (PlatformPayload)platformPayloadForPingCore {
  if (textures_ == nil || file_references_ == nil || buffer_references_ == nil) {
    throw std::invalid_argument("PingCore Apple platform boundary received an invalid PlatformPayload value");
  }
  PlatformPayload::Envelope envelope;
  envelope.bytes = bytes_;
  envelope.external_textures.reserve(textures_.count);
  for (HUXExternalTexture* texture in textures_) {
    envelope.external_textures.push_back(pingcore::ios::detail::UnwrapExternalTexture(texture));
  }
  envelope.file_references.reserve(file_references_.count);
  for (HUXFileReference* reference in file_references_) {
    envelope.file_references.push_back(UnwrapFileReference(reference));
  }
  envelope.buffer_references.reserve(buffer_references_.count);
  for (HUXBufferReference* reference in buffer_references_) {
    envelope.buffer_references.push_back(UnwrapBufferReference(reference));
  }
  return PlatformPayload::Decode(envelope);
}

+ (instancetype)nullValue {
  return EncodePayload({});
}

+ (instancetype)booleanValue:(BOOL)value {
  return EncodePayload(PlatformPayload(value == YES));
}

+ (instancetype)integerValue:(int64_t)value {
  return EncodePayload(PlatformPayload(value));
}

+ (instancetype)doubleValue:(double)value {
  try {
    return EncodePayload(PlatformPayload(value));
  } catch (const std::exception& exception) {
    RaiseCppException(exception);
  }
}

+ (instancetype)stringValue:(NSString*)value {
  try {
    return EncodePayload(PlatformPayload(ToCppString(value, "PlatformPayload String value")));
  } catch (const std::exception& exception) {
    RaiseCppException(exception);
  }
}

+ (instancetype)bytesValue:(NSData*)value {
  if (value == nil) {
    RaiseInvalidArgument(@"PingCore PlatformPayload Bytes value must not be nil");
  }
  try {
    Bytes bytes(value.length);
    if (!bytes.empty()) {
      std::memcpy(bytes.data(), value.bytes, bytes.size());
    }
    return EncodePayload(PlatformPayload(std::move(bytes)));
  } catch (const std::exception& exception) {
    RaiseCppException(exception);
  }
}

+ (instancetype)listValue:(NSArray<HUXPlatformPayload*>*)value {
  if (value == nil) {
    RaiseInvalidArgument(@"PingCore PlatformPayload List value must not be nil");
  }
  try {
    PlatformPayload::List list;
    list.reserve(value.count);
    for (HUXPlatformPayload* item in value) {
      list.push_back(DecodePayload(item));
    }
    return EncodePayload(PlatformPayload(std::move(list)));
  } catch (const std::exception& exception) {
    RaiseCppException(exception);
  }
}

+ (instancetype)objectValue:(NSDictionary<NSString*, HUXPlatformPayload*>*)value {
  if (value == nil) {
    RaiseInvalidArgument(@"PingCore PlatformPayload Object value must not be nil");
  }
  try {
    PlatformPayload::Object object;
    for (NSString* key in value) {
      object.emplace(ToCppString(key, "PlatformPayload Object key"), DecodePayload(value[key]));
    }
    return EncodePayload(PlatformPayload(std::move(object)));
  } catch (const std::exception& exception) {
    RaiseCppException(exception);
  }
}

+ (instancetype)externalTextureValue:(HUXExternalTexture*)texture {
  try {
    return EncodePayload(PlatformPayload(pingcore::ios::detail::UnwrapExternalTexture(texture)));
  } catch (const std::exception& exception) {
    RaiseCppException(exception);
  }
}

+ (instancetype)fileReferenceValue:(HUXFileReference*)reference {
  try {
    return EncodePayload(PlatformPayload(UnwrapFileReference(reference)));
  } catch (const std::exception& exception) {
    RaiseCppException(exception);
  }
}

+ (instancetype)bufferReferenceValue:(HUXBufferReference*)reference {
  try {
    return EncodePayload(PlatformPayload(UnwrapBufferReference(reference)));
  } catch (const std::exception& exception) {
    RaiseCppException(exception);
  }
}

- (HUXBufferReference*)bufferReferenceValue {
  try {
    return WrapBufferReference(DecodePayload(self).AsBufferReference());
  } catch (const std::exception& exception) {
    RaiseCppException(exception);
  }
}

- (HUXPlatformPayloadKind)kind {
  try {
    return ToObjectiveCKind(DecodePayload(self).Kind());
  } catch (const std::exception& exception) {
    RaiseCppException(exception);
  }
}

- (BOOL)booleanValue {
  try {
    return DecodePayload(self).AsBoolean();
  } catch (const std::exception& exception) {
    RaiseCppException(exception);
  }
}

- (int64_t)integerValue {
  try {
    return DecodePayload(self).AsInteger();
  } catch (const std::exception& exception) {
    RaiseCppException(exception);
  }
}

- (double)doubleValue {
  try {
    return DecodePayload(self).AsDouble();
  } catch (const std::exception& exception) {
    RaiseCppException(exception);
  }
}

- (NSString*)stringValue {
  try {
    return ToNSString(DecodePayload(self).AsString());
  } catch (const std::exception& exception) {
    RaiseCppException(exception);
  }
}

- (NSData*)bytesValue {
  try {
    const PlatformPayload payload = DecodePayload(self);
    const std::span bytes = payload.AsBytes();
    return [NSData dataWithBytes:bytes.data() length:bytes.size()];
  } catch (const std::exception& exception) {
    RaiseCppException(exception);
  }
}

- (HUXExternalTexture*)externalTextureValue {
  try {
    return pingcore::ios::detail::WrapExternalTexture(DecodePayload(self).AsExternalTexture());
  } catch (const std::exception& exception) {
    RaiseCppException(exception);
  }
}

- (HUXFileReference*)fileReferenceValue {
  try {
    return WrapFileReference(DecodePayload(self).AsFileReference());
  } catch (const std::exception& exception) {
    RaiseCppException(exception);
  }
}

- (HUXPlatformPayload*)field:(NSString*)name {
  try {
    const PlatformPayload payload = DecodePayload(self);
    const auto& object = payload.AsObject();
    const std::string key = ToCppString(name, "PlatformPayload field name");
    const auto found = object.find(key);
    if (found == object.end()) {
      throw std::invalid_argument("PingCore PlatformPayload is missing field " + key);
    }
    return EncodePayload(found->second);
  } catch (const std::exception& exception) {
    RaiseCppException(exception);
  }
}

- (HUXPlatformPayload*)elementAtIndex:(NSUInteger)index {
  try {
    const PlatformPayload payload = DecodePayload(self);
    const auto& list = payload.AsList();
    if (index >= list.size()) {
      throw std::out_of_range("PingCore PlatformPayload element index is outside the List");
    }
    return EncodePayload(list[index]);
  } catch (const std::exception& exception) {
    RaiseCppException(exception);
  }
}

- (void)validateFields:(NSSet<NSString*>*)fields {
  if (fields == nil) {
    RaiseInvalidArgument(@"PingCore PlatformPayload accepted fields must not be nil");
  }
  try {
    const PlatformPayload payload = DecodePayload(self);
    const auto& object = payload.AsObject();
    for (const auto& [name, value] : object) {
      if (![fields containsObject:ToNSString(name)]) {
        throw std::invalid_argument("PingCore PlatformPayload contains unknown field " + name);
      }
    }
  } catch (const std::exception& exception) {
    RaiseCppException(exception);
  }
}

@end

namespace pingcore::ios::detail {

class ObjectiveCPlatformViewInstance final {
public:
  ~ObjectiveCPlatformViewInstance() {
    if (instance_bridge) {
      instance_bridge->CloseEvents();
      if (channel_connected) {
        channel.Close();
      } else {
        instance_bridge->Dispose();
      }
    }
  }

  std::shared_ptr<ObjectiveCPlatformInstanceBridge> instance_bridge;
  PlatformChannel channel;
  __strong UIView* view = nil;
  bool channel_connected = false;
};

static PlatformChannel CreateModule(const pingcore::detail::PlatformChannelEndpoint& endpoint,
                                    const std::function<id<HUXPlatformModule>(id<HUXPlatformEventEmitter>)>& create) {
  HUXIOSPlatformEventEmitter* events = [[HUXIOSPlatformEventEmitter alloc] initWithEvents:endpoint.Events()];
  __strong id<HUXPlatformModule> instance = nil;
  @try {
    instance = create(events);
  } @catch (NSException* exception) {
    [events close];
    throw std::logic_error("PingCore iOS Objective-C PlatformModule factory raised an exception");
  }
  if (instance == nil || ![instance respondsToSelector:@selector(invoke:arguments:result:)] ||
      ![instance respondsToSelector:@selector(dispose)]) {
    [events close];
    throw std::logic_error("PingCore iOS Objective-C PlatformModule factory returned an invalid instance");
  }
  auto instance_bridge = std::make_shared<ObjectiveCPlatformInstanceBridge>(instance, events);
  ConnectInstance(endpoint, instance_bridge);
  return endpoint.Channel();
}

PlatformChannel CreateObjectiveCPlatformModule(Runtime& runtime, id<HUXPlatformModuleFactory> factory,
                                               PlatformPayload options) {
  if (factory == nil || ![factory respondsToSelector:@selector(createWithOptions:events:)]) {
    throw std::invalid_argument("PingCore iOS Objective-C PlatformModule factory must provide create");
  }
  return CreateModule(pingcore::detail::MakePlatformChannelEndpoint(runtime),
      [factory, options = std::move(options)](id<HUXPlatformEventEmitter> events) {
        return [factory createWithOptions:EncodePayload(options) events:events];
      });
}

PlatformChannel CreateObjectiveCWindowPlatformModule(UiWindow& ui_window, UIViewController* owner,
                                                     id<HUXUIKitWindowPlatformModuleFactory> factory,
                                                     PlatformPayload options) {
  if (owner == nil) {
    throw std::logic_error("PingCore iOS Objective-C PlatformModule requires an owning UIViewController");
  }
  if (factory == nil || ![factory respondsToSelector:@selector(createWithViewController:options:events:)]) {
    throw std::invalid_argument("PingCore iOS Objective-C PlatformModule factory must provide create");
  }
  return CreateModule(pingcore::detail::MakePlatformChannelEndpoint(ui_window),
      [owner, factory, options = std::move(options)](id<HUXPlatformEventEmitter> events) {
        return [factory createWithViewController:owner options:EncodePayload(options) events:events];
      });
}

std::shared_ptr<ObjectiveCPlatformViewInstance>
CreateObjectiveCPlatformView(UiWindow& ui_window, UIViewController* owner,
                             id<HUXUIKitPlatformViewFactory> factory, PlatformPayload properties,
                             PlatformEventEmitter events, bool update_required, bool channel_required) {
  if (owner == nil) {
    throw std::logic_error("PingCore iOS Objective-C PlatformView requires an owning UIViewController");
  }
  if (factory == nil || ![factory respondsToSelector:@selector(createWithViewController:properties:events:)]) {
    throw std::invalid_argument("PingCore iOS Objective-C PlatformView factory must provide create");
  }
  HUXIOSPlatformEventEmitter* event_endpoint = [[HUXIOSPlatformEventEmitter alloc] initWithEvents:std::move(events)];
  __strong id<HUXUIKitPlatformView> instance = nil;
  @try {
    instance = [factory createWithViewController:owner
                                      properties:EncodePayload(std::move(properties))
                                          events:event_endpoint];
  } @catch (NSException* exception) {
    [event_endpoint close];
    throw std::logic_error("PingCore iOS Objective-C PlatformView factory raised an exception");
  }
  if (instance == nil || ![instance respondsToSelector:@selector(view)] ||
      ![instance respondsToSelector:@selector(dispose)] ||
      (update_required && ![instance respondsToSelector:@selector(updateWithProperties:)]) ||
      (channel_required && ![instance respondsToSelector:@selector(invoke:arguments:result:)])) {
    [event_endpoint close];
    throw std::logic_error("PingCore iOS Objective-C PlatformView factory returned an invalid instance");
  }
  UIView* view = instance.view;
  if (view == nil) {
    [event_endpoint close];
    throw std::logic_error("PingCore iOS Objective-C PlatformView returned a null UIView");
  }
  if (view.superview != nil) {
    [event_endpoint close];
    throw std::logic_error("PingCore iOS Objective-C PlatformView returned an attached UIView");
  }

  auto result = std::make_shared<ObjectiveCPlatformViewInstance>();
  result->instance_bridge = std::make_shared<ObjectiveCPlatformInstanceBridge>(instance, event_endpoint);
  result->view = view;
  if (channel_required) {
    const pingcore::detail::PlatformChannelEndpoint endpoint = pingcore::detail::MakePlatformChannelEndpoint(ui_window);
    ConnectInstance(endpoint, result->instance_bridge);
    result->channel = endpoint.Channel();
    result->channel_connected = true;
  }
  return result;
}

UIView* GetObjectiveCPlatformView(const std::shared_ptr<ObjectiveCPlatformViewInstance>& instance) {
  return instance ? instance->view : nil;
}

void UpdateObjectiveCPlatformView(const std::shared_ptr<ObjectiveCPlatformViewInstance>& instance,
                                  PlatformPayload properties) {
  if (!instance || !instance->instance_bridge || instance->instance_bridge->Disposed()) {
    throw std::logic_error("PingCore iOS Objective-C PlatformView instance is disposed");
  }
  id<HUXUIKitPlatformView> platform_view = instance->instance_bridge->Instance();
  @try {
    [platform_view updateWithProperties:EncodePayload(std::move(properties))];
    if (platform_view.view != instance->view) {
      throw std::logic_error("PingCore iOS Objective-C PlatformView changed its UIView identity");
    }
  } @catch (NSException* exception) {
    throw std::logic_error("PingCore iOS Objective-C PlatformView update raised an exception");
  }
}

void DisposeObjectiveCPlatformView(const std::shared_ptr<ObjectiveCPlatformViewInstance>& instance) noexcept {
  if (!instance) {
    return;
  }
  if (instance->instance_bridge) {
    instance->instance_bridge->CloseEvents();
    if (instance->channel_connected) {
      instance->channel.Close();
    } else {
      instance->instance_bridge->Dispose();
    }
  }
  instance->view = nil;
}

PlatformChannel GetObjectiveCPlatformViewChannel(const std::shared_ptr<ObjectiveCPlatformViewInstance>& instance) {
  return instance ? instance->channel : PlatformChannel{};
}

} // namespace pingcore::ios::detail
