#include <pingcore/android/external_texture.h>

#include <limits>
#include <mutex>
#include <vector>
#include <stdexcept>
#include <utility>

#include <pingcore/android/platform_registry.h>
#include <pingcore/resource.h>

#include "android_external_texture_internal.h"
#include "application/application_internal.h"
#include "graphics/bitmap_internal.h"

namespace pingcore::detail {

namespace {

void DeleteGlobalReference(JavaVM* virtual_machine, jobject reference) noexcept {
  if (virtual_machine == nullptr || reference == nullptr) {
    return;
  }
  JNIEnv* environment = nullptr;
  bool attached = false;
  const jint result = virtual_machine->GetEnv(reinterpret_cast<void**>(&environment), JNI_VERSION_1_6);
  if (result == JNI_EDETACHED) {
    attached = virtual_machine->AttachCurrentThread(&environment, nullptr) == JNI_OK;
  }
  if (environment != nullptr) {
    environment->DeleteGlobalRef(reference);
  }
  if (attached) {
    virtual_machine->DetachCurrentThread();
  }
}

bool ClearPendingJavaException(JNIEnv* environment) noexcept {
  if (environment == nullptr || !environment->ExceptionCheck()) {
    return false;
  }
  environment->ExceptionClear();
  return true;
}

jmethodID GetRequiredMethod(JNIEnv* environment, jclass type, const char* name, const char* signature) {
  const jmethodID method = environment->GetMethodID(type, name, signature);
  const bool lookup_failed = ClearPendingJavaException(environment);
  if (method == nullptr || lookup_failed) {
    throw std::runtime_error("PingCore android.graphics.Bitmap methods do not match the platform backend");
  }
  return method;
}

class BitmapEnvironment {
public:
  explicit BitmapEnvironment(JavaVM& vm) : vm_(vm) {
    const auto result = vm_.GetEnv(reinterpret_cast<void**>(&environment_), JNI_VERSION_1_6);
    if (result == JNI_EDETACHED) {
      attached_ = vm_.AttachCurrentThread(&environment_, nullptr) == JNI_OK;
    }
    if (!environment_) throw std::runtime_error("PingCore could not attach the bitmap worker to Android");
  }
  ~BitmapEnvironment() {
    if (attached_) vm_.DetachCurrentThread();
  }
  BitmapEnvironment(const BitmapEnvironment&) = delete;
  BitmapEnvironment& operator=(const BitmapEnvironment&) = delete;
  JNIEnv* Get() const noexcept { return environment_; }

private:
  JavaVM& vm_;
  JNIEnv* environment_ = nullptr;
  bool attached_ = false;
};

JavaVM* ApplicationBitmapVm() {
  auto* environment = android::GetPlatformEnv(*CurrentApplicationRuntime()->owner).jni;
  JavaVM* vm = nullptr;
  if (environment->GetJavaVM(&vm) != JNI_OK || !vm) {
    throw std::runtime_error("PingCore could not access the Android Java VM for bitmap processing");
  }
  return vm;
}

template <class T> T RequireBitmapJni(JNIEnv* env, T value, const char* message) {
  if (ClearPendingJavaException(env) || !value) throw std::runtime_error(message);
  return value;
}

android::LocalRef<jclass> BitmapClass(JNIEnv* env, const char* name) {
  return {env, RequireBitmapJni(env, env->FindClass(name), "PingCore could not load the Android bitmap codec class")};
}

android::LocalRef<jobject> ArgbBitmapConfig(JNIEnv* env) {
  const auto config = BitmapClass(env, "android/graphics/Bitmap$Config");
  const auto field = RequireBitmapJni(
      env, env->GetStaticFieldID(config.Get(), "ARGB_8888", "Landroid/graphics/Bitmap$Config;"),
      "PingCore could not find the Android ARGB bitmap format");
  return {env, RequireBitmapJni(env, env->GetStaticObjectField(config.Get(), field),
                                "PingCore could not read the Android ARGB bitmap format")};
}

Bitmap ReadAndroidPixels(JNIEnv* env, jobject bitmap, jint width, jint height, float scale) {
  if (width <= 0 || height <= 0) throw std::runtime_error("PingCore Android bitmap dimensions are invalid");
  const auto count = BitmapByteSize(width, height) / 4;
  if (count > static_cast<std::size_t>(std::numeric_limits<jsize>::max())) {
    throw std::runtime_error("PingCore bitmap exceeds the Android pixel array limit");
  }
  const auto klass = BitmapClass(env, "android/graphics/Bitmap");
  const auto get_pixels = GetRequiredMethod(env, klass.Get(), "getPixels", "([IIIIIII)V");
  const android::LocalRef<jintArray> colors(env, RequireBitmapJni(
      env, env->NewIntArray(static_cast<jsize>(count)), "PingCore could not allocate Android bitmap pixels"));
  env->CallVoidMethod(bitmap, get_pixels, colors.Get(), 0, width, 0, 0, width, height);
  if (ClearPendingJavaException(env)) {
    // Hardware-backed Android bitmaps need a software copy before Java can read their pixels.
    const auto config = ArgbBitmapConfig(env);
    const auto copy = GetRequiredMethod(
        env, klass.Get(), "copy", "(Landroid/graphics/Bitmap$Config;Z)Landroid/graphics/Bitmap;");
    const android::LocalRef<jobject> software(env, RequireBitmapJni(
        env, env->CallObjectMethod(bitmap, copy, config.Get(), JNI_FALSE),
        "PingCore could not copy the Android bitmap for readback"));
    env->CallVoidMethod(software.Get(), get_pixels, colors.Get(), 0, width, 0, 0, width, height);
    if (ClearPendingJavaException(env)) throw std::runtime_error("PingCore could not read Android bitmap pixels");
  }

  std::vector<jint> argb(count);
  env->GetIntArrayRegion(colors.Get(), 0, static_cast<jsize>(count), argb.data());
  if (ClearPendingJavaException(env)) throw std::runtime_error("PingCore could not copy Android bitmap pixels");
  Bytes pixels(count * 4);
  auto* output = pixels.data();
  for (const auto value : argb) {
    const auto packed = static_cast<std::uint32_t>(value);
    *output++ = static_cast<std::byte>(packed >> 16U);
    *output++ = static_cast<std::byte>(packed >> 8U);
    *output++ = static_cast<std::byte>(packed);
    *output++ = static_cast<std::byte>(packed >> 24U);
  }
  return Bitmap::FromRgba8(width, height, std::move(pixels), scale);
}

} // namespace

android::LocalRef<jobject> CreateAndroidBitmap(JNIEnv* env, const Bitmap& bitmap) {
  const auto count = bitmap.Pixels().size() / 4;
  if (count == 0 || count > static_cast<std::size_t>(std::numeric_limits<jsize>::max())) {
    throw std::invalid_argument("PingCore bitmap exceeds the Android pixel array limit");
  }
  const auto klass = BitmapClass(env, "android/graphics/Bitmap");
  const auto config = ArgbBitmapConfig(env);
  const auto create = RequireBitmapJni(env, env->GetStaticMethodID(
      klass.Get(), "createBitmap", "([IIILandroid/graphics/Bitmap$Config;)Landroid/graphics/Bitmap;"),
      "PingCore could not find the Android bitmap constructor");
  const android::LocalRef<jintArray> colors(env, RequireBitmapJni(
      env, env->NewIntArray(static_cast<jsize>(count)), "PingCore could not allocate Android bitmap pixels"));
  std::vector<jint> argb(count);
  const auto* source = bitmap.Pixels().data();
  for (auto& value : argb) {
    value = static_cast<jint>(std::to_integer<std::uint32_t>(source[3]) << 24U |
                             std::to_integer<std::uint32_t>(source[0]) << 16U |
                             std::to_integer<std::uint32_t>(source[1]) << 8U |
                             std::to_integer<std::uint32_t>(source[2]));
    source += 4;
  }
  env->SetIntArrayRegion(colors.Get(), 0, static_cast<jsize>(count), argb.data());
  if (ClearPendingJavaException(env)) throw std::runtime_error("PingCore could not store Android bitmap pixels");
  return {env, RequireBitmapJni(env, env->CallStaticObjectMethod(
      klass.Get(), create, colors.Get(),
      static_cast<jint>(bitmap.PixelWidth()), static_cast<jint>(bitmap.PixelHeight()),
      config.Get()), "PingCore could not create an Android bitmap")};
}

Task<Bitmap> DecodeBitmap(ImageAsset image, std::uint16_t orientation) {
  if (!image.HasValue()) throw std::logic_error("PingCore cannot decode a missing image asset");
  co_return co_await RunWorker([orientation](JavaVM* vm, ImageAsset source) {
    const BitmapEnvironment attached(*vm);
    auto* env = attached.Get();
    const auto factory = BitmapClass(env, "android/graphics/BitmapFactory");
    const auto klass = BitmapClass(env, "android/graphics/Bitmap");
    const auto decode = RequireBitmapJni(env, env->GetStaticMethodID(
        factory.Get(), "decodeByteArray", "([BII)Landroid/graphics/Bitmap;"),
        "PingCore could not find the Android bitmap decoder");
    const auto encoded = android::BytesToJavaByteArray(env, source.EncodedBytes());
    RequireBitmapJni(env, encoded.Get(), "PingCore could not transfer encoded image bytes to Android");
    const android::LocalRef<jobject> decoded(env, RequireBitmapJni(env, env->CallStaticObjectMethod(
        factory.Get(), decode, encoded.Get(), 0, static_cast<jint>(source.EncodedBytes().size())),
        "PingCore could not decode the Android image"));
    const auto get_width = GetRequiredMethod(env, klass.Get(), "getWidth", "()I");
    const auto get_height = GetRequiredMethod(env, klass.Get(), "getHeight", "()I");
    const auto width = env->CallIntMethod(decoded.Get(), get_width);
    const auto height = env->CallIntMethod(decoded.Get(), get_height);
    return ApplyEncodedOrientation(ReadAndroidPixels(env, decoded.Get(), width, height, source.Scale()), orientation);
  }, ApplicationBitmapVm(), std::move(image));
}

Task<Bytes> EncodeBitmap(Bitmap bitmap, ImageEncodeOptions options) {
  co_return co_await RunWorker([](JavaVM* vm, Bitmap pixels, ImageEncodeOptions settings) {
    const BitmapEnvironment attached(*vm);
    auto* env = attached.Get();
    const bool jpeg = settings.format == ImageFormat::Jpeg;
    if (jpeg) pixels = FlattenBitmap(std::move(pixels), settings.jpeg_background);
    const auto image = CreateAndroidBitmap(env, pixels);
    const auto klass = BitmapClass(env, "android/graphics/Bitmap");
    const auto format = BitmapClass(env, "android/graphics/Bitmap$CompressFormat");
    const auto stream = BitmapClass(env, "java/io/ByteArrayOutputStream");
    const auto field = RequireBitmapJni(env, env->GetStaticFieldID(
        format.Get(), jpeg ? "JPEG" : "PNG", "Landroid/graphics/Bitmap$CompressFormat;"),
        "PingCore could not find the Android image encoding format");
    const android::LocalRef<jobject> encoding(env, RequireBitmapJni(
        env, env->GetStaticObjectField(format.Get(), field),
        "PingCore could not read the Android image encoding format"));
    const auto constructor = GetRequiredMethod(env, stream.Get(), "<init>", "()V");
    const android::LocalRef<jobject> output(env, RequireBitmapJni(
        env, env->NewObject(stream.Get(), constructor), "PingCore could not create Android image output"));
    const auto compress = GetRequiredMethod(env, klass.Get(), "compress",
        "(Landroid/graphics/Bitmap$CompressFormat;ILjava/io/OutputStream;)Z");
    const auto encoded = env->CallBooleanMethod(
        image.Get(), compress, encoding.Get(), settings.jpeg_quality, output.Get());
    if (ClearPendingJavaException(env) || encoded != JNI_TRUE) {
      throw std::runtime_error("PingCore could not encode the Android bitmap");
    }
    const auto to_bytes = GetRequiredMethod(env, stream.Get(), "toByteArray", "()[B");
    const android::LocalRef<jbyteArray> bytes(env, RequireBitmapJni(
        env, static_cast<jbyteArray>(env->CallObjectMethod(output.Get(), to_bytes)),
        "PingCore could not read the encoded Android image"));
    auto result = android::JavaByteArrayToBytes(env, bytes.Get());
    if (ClearPendingJavaException(env) || result.empty()) {
      throw std::runtime_error("PingCore could not copy the encoded Android image");
    }
    return result;
  }, ApplicationBitmapVm(), std::move(bitmap), options);
}

AndroidBitmapFrame::AndroidBitmapFrame(
    JavaVM* virtual_machine, jobject bitmap, jint pixel_width, jint pixel_height, jint generation
) noexcept
    : virtual_machine_(virtual_machine), bitmap_(bitmap), pixel_width_(pixel_width), pixel_height_(pixel_height),
      generation_(generation) {}

AndroidBitmapFrame::~AndroidBitmapFrame() {
  DeleteGlobalReference(virtual_machine_, bitmap_);
}

AndroidBitmapFrame::operator bool() const noexcept {
  return bitmap_ != nullptr;
}

jobject AndroidBitmapFrame::Bitmap() const noexcept {
  return bitmap_;
}

jint AndroidBitmapFrame::PixelWidth() const noexcept {
  return pixel_width_;
}

jint AndroidBitmapFrame::PixelHeight() const noexcept {
  return pixel_height_;
}

jint AndroidBitmapFrame::Generation() const noexcept {
  return generation_;
}

} // namespace pingcore::detail

namespace pingcore::android {

struct BitmapTexture::Storage {
  std::mutex mutex;
  std::shared_ptr<const pingcore::detail::AndroidBitmapFrame> frame;
  JavaVM* virtual_machine = nullptr;
  jclass bitmap_class = nullptr;
  jmethodID bitmap_get_width = nullptr;
  jmethodID bitmap_get_height = nullptr;
  jmethodID bitmap_get_generation = nullptr;
  jmethodID bitmap_is_recycled = nullptr;
  bool finished = false;
};

BitmapTexture::BitmapTexture(Size intrinsic_size)
    : pingcore::ExternalTexture(intrinsic_size), storage_(std::make_unique<Storage>()) {}

BitmapTexture::~BitmapTexture() {
  std::shared_ptr<const pingcore::detail::AndroidBitmapFrame> frame;
  JavaVM* virtual_machine = nullptr;
  jclass bitmap_class = nullptr;
  {
    std::lock_guard lock(storage_->mutex);
    frame = std::move(storage_->frame);
    virtual_machine = storage_->virtual_machine;
    bitmap_class = std::exchange(storage_->bitmap_class, nullptr);
  }
  pingcore::detail::DeleteGlobalReference(virtual_machine, bitmap_class);
}

void BitmapTexture::InitializeJni(JNIEnv* environment) {
  JavaVM* virtual_machine = nullptr;
  if (environment->GetJavaVM(&virtual_machine) != JNI_OK || virtual_machine == nullptr) {
    throw std::runtime_error("PingCore could not access the Android Java VM for an external texture");
  }
  if (storage_->virtual_machine != nullptr) {
    if (storage_->virtual_machine != virtual_machine) {
      throw std::invalid_argument("PingCore Android external texture frames must belong to the same Java VM");
    }
    return;
  }

  jclass local_bitmap_class = environment->FindClass("android/graphics/Bitmap");
  if (local_bitmap_class == nullptr) {
    pingcore::detail::ClearPendingJavaException(environment);
    throw std::runtime_error("PingCore could not resolve android.graphics.Bitmap");
  }
  jclass bitmap_class = static_cast<jclass>(environment->NewGlobalRef(local_bitmap_class));
  environment->DeleteLocalRef(local_bitmap_class);
  const bool retain_class_failed = pingcore::detail::ClearPendingJavaException(environment);
  if (bitmap_class == nullptr || retain_class_failed) {
    if (bitmap_class != nullptr) {
      environment->DeleteGlobalRef(bitmap_class);
    }
    throw std::runtime_error("PingCore could not retain android.graphics.Bitmap");
  }

  jmethodID bitmap_get_width = nullptr;
  jmethodID bitmap_get_height = nullptr;
  jmethodID bitmap_get_generation = nullptr;
  jmethodID bitmap_is_recycled = nullptr;
  try {
    bitmap_get_width = pingcore::detail::GetRequiredMethod(environment, bitmap_class, "getWidth", "()I");
    bitmap_get_height = pingcore::detail::GetRequiredMethod(environment, bitmap_class, "getHeight", "()I");
    bitmap_get_generation = pingcore::detail::GetRequiredMethod(environment, bitmap_class, "getGenerationId", "()I");
    bitmap_is_recycled = pingcore::detail::GetRequiredMethod(environment, bitmap_class, "isRecycled", "()Z");
  } catch (...) {
    environment->DeleteGlobalRef(bitmap_class);
    throw;
  }
  storage_->bitmap_class = bitmap_class;
  storage_->bitmap_get_width = bitmap_get_width;
  storage_->bitmap_get_height = bitmap_get_height;
  storage_->bitmap_get_generation = bitmap_get_generation;
  storage_->bitmap_is_recycled = bitmap_is_recycled;
  storage_->virtual_machine = virtual_machine;
}

void BitmapTexture::Publish(JNIEnv* environment, jobject bitmap) {
  if (environment == nullptr) {
    throw std::invalid_argument("PingCore Android external texture JNI environment must not be null");
  }
  if (bitmap == nullptr) {
    throw std::invalid_argument("PingCore Android external texture Bitmap must not be null");
  }

  std::shared_ptr<const pingcore::detail::AndroidBitmapFrame> replaced;
  {
    std::lock_guard lock(storage_->mutex);
    if (storage_->finished) {
      throw std::logic_error("PingCore Android external texture is finished");
    }
    InitializeJni(environment);
    const jboolean is_bitmap = environment->IsInstanceOf(bitmap, storage_->bitmap_class);
    if (pingcore::detail::ClearPendingJavaException(environment)) {
      throw std::runtime_error("PingCore could not inspect the Android external texture frame type");
    }
    if (is_bitmap != JNI_TRUE) {
      throw std::invalid_argument("PingCore Android external texture frame must be an android.graphics.Bitmap");
    }
    const jboolean recycled = environment->CallBooleanMethod(bitmap, storage_->bitmap_is_recycled);
    if (pingcore::detail::ClearPendingJavaException(environment)) {
      throw std::runtime_error("PingCore could not inspect the Android external texture Bitmap state");
    }
    if (recycled == JNI_TRUE) {
      throw std::invalid_argument("PingCore Android external texture Bitmap must not be recycled");
    }
    const jint pixel_width = environment->CallIntMethod(bitmap, storage_->bitmap_get_width);
    if (pingcore::detail::ClearPendingJavaException(environment)) {
      throw std::runtime_error("PingCore could not inspect the Android external texture Bitmap width");
    }
    const jint pixel_height = environment->CallIntMethod(bitmap, storage_->bitmap_get_height);
    if (pingcore::detail::ClearPendingJavaException(environment)) {
      throw std::runtime_error("PingCore could not inspect the Android external texture Bitmap height");
    }
    const jint generation = environment->CallIntMethod(bitmap, storage_->bitmap_get_generation);
    if (pingcore::detail::ClearPendingJavaException(environment)) {
      throw std::runtime_error("PingCore could not inspect the Android external texture Bitmap generation");
    }
    if (pixel_width <= 0 || pixel_height <= 0) {
      throw std::invalid_argument("PingCore Android external texture Bitmap dimensions must be positive");
    }
    jobject retained_bitmap = environment->NewGlobalRef(bitmap);
    const bool retain_bitmap_failed = pingcore::detail::ClearPendingJavaException(environment);
    if (retained_bitmap == nullptr || retain_bitmap_failed) {
      if (retained_bitmap != nullptr) {
        environment->DeleteGlobalRef(retained_bitmap);
      }
      throw std::runtime_error("PingCore could not retain the Android external texture Bitmap");
    }
    auto frame = std::make_shared<const pingcore::detail::AndroidBitmapFrame>(
        storage_->virtual_machine, retained_bitmap, pixel_width, pixel_height, generation
    );
    replaced = std::move(storage_->frame);
    storage_->frame = std::move(frame);
  }
  NotifyFrameAvailable();
}

void BitmapTexture::Finish() noexcept {
  std::lock_guard lock(storage_->mutex);
  storage_->finished = true;
}

std::shared_ptr<const pingcore::detail::AndroidBitmapFrame> BitmapTexture::AcquireFrame() const noexcept {
  std::lock_guard lock(storage_->mutex);
  return storage_->frame;
}

Task<std::optional<pingcore::Bitmap>> BitmapTexture::SnapshotAsync() const {
  std::shared_ptr<const pingcore::detail::AndroidBitmapFrame> frame;
  JavaVM* vm = nullptr;
  {
    std::lock_guard lock(storage_->mutex);
    frame = storage_->frame;
    vm = storage_->virtual_machine;
  }
  return RunWorker([](const auto& retained, JavaVM* virtual_machine) -> std::optional<pingcore::Bitmap> {
    if (!retained) return std::nullopt;
    const pingcore::detail::BitmapEnvironment attached(*virtual_machine);
    auto* env = attached.Get();
    const auto klass = pingcore::detail::BitmapClass(env, "android/graphics/Bitmap");
    const auto generation = pingcore::detail::GetRequiredMethod(env, klass.Get(), "getGenerationId", "()I");
    const auto current = env->CallIntMethod(retained->Bitmap(), generation);
    if (pingcore::detail::ClearPendingJavaException(env) || current != retained->Generation()) {
      throw std::runtime_error("PingCore Android external texture Bitmap changed after publication");
    }
    return pingcore::detail::ReadAndroidPixels(
        env, retained->Bitmap(), retained->PixelWidth(), retained->PixelHeight(), 1.0F);
  }, std::move(frame), vm);
}

} // namespace pingcore::android
