export module ctskit.core;

import std;
import pingcore;
import ctskit_core.resources;

export namespace ctskit {

/// Resolves the core library's own string resource in the caller's composition scope.
std::string UseCoreName() {
  return pingcore::UseString(ctskit_core::strings::name);
}

} // namespace ctskit
