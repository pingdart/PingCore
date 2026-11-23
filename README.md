<p align="center"><picture><source media="(prefers-color-scheme: dark)" srcset="docs/assets/PingCore-logo-dark.png"><source media="(prefers-color-scheme: light)" srcset="docs/assets/PingCore-logo-light.png"><img src="docs/assets/PingCore-logo-light.png" width="220" alt="PingCore logo"></picture></p>

<h1 align="center">PingCore</h1>

<p align="center"><strong>Build human-centered user experiences.</strong></p>

PingCore is a declarative, cross-platform UI framework for C++20. The name *PingCore* combines **H** for *Human*, **UX** for *User Experience*, and **-er** for the person who creates that experience.

## Quick Start

This quick start targets PingCore **0.5.0**. The installers select the latest published SDK.

### Install the SDK

**Windows PowerShell**

```powershell
irm https://github.com/pingdart/PingCore/releases/latest/download/install.ps1 | iex
```

**macOS, Linux, or ARM64 Termux**

```bash
curl -fsSL https://github.com/pingdart/PingCore/releases/latest/download/install.sh | sh
```

On desktop computers, `xlings install pingcore -y` installs the SDK through [xlings](https://github.com/d2learn/xlings).

For toolchain requirements and SDK version selection, see [Installation](docs/guide/installation.md).

### Run an application

Open a new terminal to create an application. Replace `windows` with a target you can build and run from your development machine.

```bash
pingcore create app hello_pingcore
cd hello_pingcore
pingcore doctor
pingcore run windows
```

The project above uses headers (`#include <pingcore/pingcore.h>`) and CMake. For a C++ modules project, whose sources write `import pingcore;` and which mcpp builds:

```bash
pingcore create app hello_modules --build-system mcpp
```

A library defaults to, and is recommended with, `--build-system cmake,mcpp`: you maintain its headers, the mcpp build generates its module interface, and consumers can either include or `import` it. See [C++ Modules and mcpp](skills/pingcore-app-development/references/cpp-modules-and-mcpp.md).

## Example

Replace `src/app.cpp` in the generated CMake project with this counter. `UseState` retains the count and updates the view when the button changes it.

```cpp
#include <pingcore/pingcore.h>

using namespace pingcore;

[[pingcore::composable]]
View Counter() {
  auto count = UseState(0);

  return Column {
    Text::Format("Count: {}", count),
    Button("Increment").OnClick([count] { count += 1; }),
  }.With(Padding(24.0F), Spacing(12.0F));
}

View App() {
  return MaterialTheme {
    Counter(),
  };
}

const Application application{App, {.window = {.title = "Counter"}}};
```

Repeat the run command to try the counter.

Explore more components in the [UI Gallery](examples/ui_gallery/) and complete applications in [PingCore-Demos](https://github.com/pingdart/PingCore-Demos).

## Platforms

| Platform | Platform integration | Renderer |
|---|---|---|
| Windows | Win32 | Direct2D and DirectWrite |
| macOS | AppKit | Core Graphics and Core Text |
| Linux | GTK 4.14 | GSK, Cairo, and Pango |
| Web | Emscripten and browser APIs | Canvas 2D |
| Android | Android View and InputConnection | Android Canvas |
| iOS | UIKit and UITextInput | Core Graphics and Core Text |

For build requirements and backend capabilities, see [Platform Support](docs/guide/platforms.md).

### Web SEO

Web projects can opt into semantic HTML, server-side rendering, static site generation, hydration, page metadata, JSON-LD structured data, dynamic routes, `sitemap.xml`, and `robots.txt` through [`<pingcore/web/seo.h>`](include/pingcore/web/seo.h). The existing Emscripten/Canvas renderer remains available for interactive applications and native applications continue to use the shared component model.

## Official Libraries

| Library | Purpose |
|---|---|
| [Lib-WebView](https://github.com/PingCore/Lib-WebView) | Embeds web content with navigation, JavaScript evaluation, and messaging between pages and the app. |
| [Lib-RichEditor](https://github.com/PingCore/Lib-RichEditor) | Edits structured rich-text documents with Markdown and JSON import and export. |
| [Lib-CodeEditor](https://github.com/PingCore/Lib-CodeEditor) | Provides a code editor with syntax highlighting, completion, and search. |
| [Lib-Media](https://github.com/PingCore/Lib-Media) | Plays audio and video and provides encoding, decoding, and container I/O. |
| [Lib-SQLite](https://github.com/PingCore/Lib-SQLite) | Maps C++ records to SQLite with typed queries, transactions, and schema migrations. |
| [Lib-Markdown](https://github.com/PingCore/Lib-Markdown) | Parses CommonMark with GFM extensions and renders themed Markdown views. |
| [Lib-DataTable](https://github.com/PingCore/Lib-DataTable) | Displays virtualized tables with resizable columns and application-controlled sorting, selection, and editing. |
| [Lib-Charts](https://github.com/PingCore/Lib-Charts) | Displays interactive charts with tooltips, selection, and animated data updates. |
| [Lib-PDF](https://github.com/PingCore/Lib-PDF) | Displays PDF pages and supports asynchronous rendering, text extraction, and search. |
| [Lib-Camera](https://github.com/PingCore/Lib-Camera) | Provides camera previews, photo capture, and silent video recording. |
| [Lib-AudioCapture](https://github.com/PingCore/Lib-AudioCapture) | Captures timestamped microphone audio as PCM and exports recordings. |
| [Lib-QRCode](https://github.com/PingCore/Lib-QRCode) | Generates and decodes QR codes and barcodes with declarative views and SVG export. |
| [Lib-Lottie](https://github.com/PingCore/Lib-Lottie) | Plays Lottie animations with playback and seeking controls. |
| [Lib-Map](https://github.com/PingCore/Lib-Map) | Embeds AMap and Baidu Map views on Android and iOS. |
| [Lib-Live2D](https://github.com/PingCore/Lib-Live2D) | Displays interactive Live2D Cubism characters with motion and expression controls. |
| [Lib-PluginKit](https://github.com/PingCore/Lib-PluginKit) | Design for a C++20 plugin toolkit with typed interfaces, package management, and isolated execution. |
| [Lib-AVKit](https://github.com/PingCore/Lib-AVKit) | Planned toolkit for recording, real-time effects, timeline editing, and media export. |

## Documentation

- [Documentation index](docs/README.md)
- [Getting Started](docs/guide/getting-started.md)
- [Building PingCore](docs/development/building.md)

## Community

Join the PingCore community on Discord or QQ.

| Discord | QQ group `1090609035` |
|:---:|:---:|
| <img src="docs/assets/community-discord.png" width="280" alt="Discord invitation QR code"> | <img src="docs/assets/community-qq.jpg" width="280" alt="QQ group invitation QR code"> |

## License

PingCore is available under the terms in [LICENSE](LICENSE).
