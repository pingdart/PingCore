#pragma once

#include <memory>

namespace pingcore::detail {

class HttpTransport;

std::shared_ptr<HttpTransport> CreateWebHttpTransport();

} // namespace pingcore::detail
