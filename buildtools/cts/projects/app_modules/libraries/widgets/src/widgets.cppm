export module ctskit.widgets;

import std;
import pingcore;
import ctskit_widgets.resources;
import ctskit.core;

using namespace pingcore;

export namespace ctskit {

/// Displays the widgets library's caption resource.
[[pingcore::composable]]
View Caption() {
  return Text(ctskit_widgets::strings::caption);
}

/// Resolves the widgets library's own string resource in the caller's composition scope.
std::string UseWidgetsCaption() {
  return UseString(ctskit_widgets::strings::caption);
}

/// Resolves the string of the core library that the widgets library depends on.
std::string UseWidgetsCoreName() {
  return UseCoreName();
}

} // namespace ctskit
