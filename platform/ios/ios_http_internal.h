#pragma once

#include <memory>

namespace pingcore::detail {

class HttpTransport;

std::shared_ptr<HttpTransport> CreateIosHttpTransport();

} // namespace pingcore::detail
