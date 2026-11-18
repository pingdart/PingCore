// include/pingdart/ui/hooks/use_theme.h
// PingDartUI useTheme Hook (PKG-138)
#pragma once

#include <pingdart/ui/theme.h>

namespace pingdart::ui {

inline ThemeEngine& useTheme() {
    return ThemeEngine::instance();
}

} // namespace pingdart::ui
