#pragma once

#include <jni.h>

#include <memory>

namespace pingcore::detail {

class HttpTransport;

std::shared_ptr<HttpTransport> CreateAndroidHttpTransport(JavaVM* virtual_machine, JNIEnv* environment);

} // namespace pingcore::detail
