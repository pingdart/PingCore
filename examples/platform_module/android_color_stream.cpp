#include "color_stream.h"

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <utility>

#include <pingcore/android/external_texture.h>
#include <pingcore/android/jni.h>
#include <pingcore/android/platform_registry.h>
#include <pingcore/app.h>

namespace {

constexpr char platform_color_stream_class[] = "org/pingcore/examples/platformmodule/PlatformColorStream";

bool ClearJavaException(JNIEnv* environment) {
  if (!environment->ExceptionCheck()) {
    return false;
  }
  environment->ExceptionClear();
  return true;
}

struct AndroidColorStreamService : pingcore::example::ColorStreamService {
  AndroidColorStreamService(pingcore::UiWindow& window_value, JNIEnv* environment_value)
      : ui_window(&window_value), environment(environment_value),
        texture(std::make_shared<pingcore::android::BitmapTexture>(
            pingcore::Size{320.0F, 180.0F}
        )) {}

  ~AndroidColorStreamService() override {
    Dispose();
  }

  pingcore::PlatformRequestId
  Texture(std::function<void(pingcore::PlatformResult<std::shared_ptr<pingcore::ExternalTexture>>)> completion) override {
    if (!completion) {
      throw std::invalid_argument("PingCore example color stream completion must not be empty");
    }
    environment->CallVoidMethod(producer, start);
    if (ClearJavaException(environment)) {
      pingcore::PlatformError error{
          "example/color-stream-java",
          "The Android color stream could not be started",
          {},
      };
      ui_window->DispatchToUiThread(
          [completion = std::move(completion), error = std::move(error)]() mutable { completion(std::move(error)); });
      return 0;
    }
    ui_window->DispatchToUiThread([completion = std::move(completion), texture = texture]() mutable {
      completion(std::move(texture));
    });
    return ++request_id;
  }

  void Publish(JNIEnv* callback_environment, jobject bitmap) {
    texture->Publish(callback_environment, bitmap);
  }

  void Dispose() noexcept {
    if (producer != nullptr) {
      environment->CallVoidMethod(producer, dispose_bridge);
      ClearJavaException(environment);
      environment->DeleteGlobalRef(producer);
      producer = nullptr;
    }
    delete bridge;
    bridge = nullptr;
    texture->Finish();
  }

  pingcore::UiWindow* ui_window = nullptr;
  JNIEnv* environment = nullptr;
  std::shared_ptr<pingcore::android::BitmapTexture> texture;
  jobject producer = nullptr;
  jmethodID start = nullptr;
  jmethodID dispose_bridge = nullptr;
  std::weak_ptr<AndroidColorStreamService>* bridge = nullptr;
  pingcore::PlatformRequestId request_id = 0;
};

std::shared_ptr<pingcore::example::ColorStreamService> CreateAndroidColorStream(pingcore::UiWindow& ui_window,
                                                                               JNIEnv* environment, jobject context) {
  pingcore::android::LocalRef<jclass> producer_class(environment, environment->FindClass(platform_color_stream_class));
  if (!producer_class) {
    ClearJavaException(environment);
    throw std::logic_error("PingCore example could not find the Android platform color stream class");
  }
  const jmethodID constructor =
      environment->GetMethodID(producer_class.Get(), "<init>", "(Landroid/content/Context;J)V");
  const jmethodID start = environment->GetMethodID(producer_class.Get(), "start", "()V");
  const jmethodID dispose_bridge = environment->GetMethodID(producer_class.Get(), "disposePlatformBridge", "()V");
  if (constructor == nullptr || start == nullptr || dispose_bridge == nullptr) {
    ClearJavaException(environment);
    throw std::logic_error("PingCore example Android color stream methods do not match the platform bridge");
  }

  auto service = std::make_shared<AndroidColorStreamService>(ui_window, environment);
  auto bridge = std::make_unique<std::weak_ptr<AndroidColorStreamService>>(service);
  pingcore::android::LocalRef<jobject> local_producer(
      environment,
      environment->NewObject(
          producer_class.Get(),
          constructor,
          context,
          static_cast<jlong>(reinterpret_cast<std::uintptr_t>(bridge.get()))
      )
  );
  if (!local_producer || environment->ExceptionCheck()) {
    ClearJavaException(environment);
    throw std::logic_error("PingCore example could not create the Android platform color stream");
  }
  service->producer = environment->NewGlobalRef(local_producer.Get());
  if (service->producer == nullptr) {
    ClearJavaException(environment);
    throw std::runtime_error("PingCore example could not retain the Android platform color stream");
  }
  service->start = start;
  service->dispose_bridge = dispose_bridge;
  service->bridge = bridge.release();
  return service;
}

} // namespace

namespace pingcore::example {

void InstallColorStream(ApplicationContext& root) {
  root.RegisterPlatformModule<std::shared_ptr<ColorStreamService>>(
      color_stream::type, android::PlatformModuleFactory<std::shared_ptr<ColorStreamService>>{
                              .create = CreateAndroidColorStream,
                          });
}

} // namespace pingcore::example

extern "C" JNIEXPORT void JNICALL
Java_org_pingcore_examples_platformmodule_PlatformColorStream_nativePublish(
    JNIEnv* environment, jclass, jlong bridge, jobject bitmap
) {
  auto* weak_service = reinterpret_cast<std::weak_ptr<AndroidColorStreamService>*>(static_cast<std::uintptr_t>(bridge));
  if (weak_service != nullptr) {
    try {
      if (const std::shared_ptr<AndroidColorStreamService> service = weak_service->lock()) {
        service->Publish(environment, bitmap);
      }
    } catch (...) {
    }
  }
}
