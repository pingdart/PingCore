#pragma once

#include <memory>

namespace pingcore::detail {

class HttpTransport;

std::shared_ptr<HttpTransport> CreateWin32HttpTransport();

} // namespace pingcore::detail
