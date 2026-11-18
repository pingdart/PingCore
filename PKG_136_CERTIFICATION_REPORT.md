# PingCore PKG-136 — PingDartUI Unified CSS & Utility Styling Engine Certification Report

**Release Channel**: **PingDartUI v0.2.0-preview** (Internal Core & Styling Release)  
**Document ID**: `PKG-136-STYLING-ENGINE-CERT`  
**Date**: November 18, 2026  
**Status**: **72 / 72 CTest Test Suites Passing (100%)** — Formally Certified  
**Target Architecture**: C++20 / Darwin arm64 / Linux x86_64 / Android AArch64 / iOS arm64 Metal  
**Repository**: `https://github.com/pingdart/PingCore.git`  
**Engine**: PingDart PingCore Native Reactive UI & Styling Runtime  

---

## Executive Summary

**PKG-136** establishes the **Unified CSS & Utility Styling Engine** for PingDartUI, providing a modern declarative component and styling architecture built on C++20, Qt 6.8 LTS, and PingCore.

The engine fulfills all architectural goals established in the PingDartUI Master Plan:
1. **Developer API Simplicity**: Zero raw pointers, no `std::unique_ptr` leaks, no mandatory Qt headers in `<pingdart/ui.h>`.
2. **Deterministic 5-Tier Styling Cascade**: Component Defaults < Theme Tokens < External CSS (Specificity Scored) < Utility Classes < Fluent C++ Method Chaining (with `!important` support).
3. **Robust CSS Parser & Lexer**: Complete support for selectors (tags, classes, IDs, pseudo-classes), CSS variables (`var(--name, fallback)`), dimension units (`px`, `rem`, `em`, `%`), comments, and syntax diagnostics without runtime aborts.
4. **Bundled Design Tokens & Palettes**: 8 full color palettes (50–950 shades for Slate, Zinc, Red, Amber, Emerald, Blue, Indigo, Violet) and deterministic `pingdart.config.json` loading.
5. **First 25 Production Components**: Complete implementation of all 25 core components in PKG-137A dependency order across Layout (7), Basic (6), Actions (2), Forms (6), Display (3), and Overlay (1).
6. **Multi-Renderer Security & Isolation**: Pluggable Web SSR (with strict HTML/XSS entity escaping), Qt 6.8 LTS Quick SceneGraph declarative output, and PingCore Native RHI DisplayLists.
7. **Production Verification**: 72 out of 72 CTest test suites passing (100%), verified with an interactive Component Gallery (`example_component_gallery`).

---

## 1. Five-Tier Styling Cascade & Specificity Engine

PingDartUI evaluates styling using the strict 5-tier cascade hierarchy:

```
┌─────────────────────────────────────────────────────────────┐
│  Tier 5: Fluent C++ Method Chaining (.padding().color())    │  (Highest precedence)
├─────────────────────────────────────────────────────────────┤
│  Tier 4: Utility Classes (.className("bg-blue-600 p-4"))    │
├─────────────────────────────────────────────────────────────┤
│  Tier 3: External CSS Rules (.card, #main, button:hover)    │  (Specificity scored)
├─────────────────────────────────────────────────────────────┤
│  Tier 2: Theme Configuration (pingdart.config.json / Tokens)│
├─────────────────────────────────────────────────────────────┤
│  Tier 1: Component Defaults (e.g. Button padding, Card bg)  │  (Lowest precedence)
└─────────────────────────────────────────────────────────────┘
```

### Specificity Scoring
External CSS selectors are evaluated using the standard 4-tuple specificity vector:
$$\text{Specificity} = (a, b, c, d)$$
- $a$: Inline styles ($a = 1$ when applied via style attribute, else $0$).
- $b$: ID selectors (`#id`).
- $c$: Class selectors (`.class`) and pseudo-classes (`:hover`, `:focus`, `:active`, `:disabled`).
- $d$: Element/Tag selectors (`div`, `button`, `span`, `h1`).

Total specificity score is calculated as:
$$\text{Score} = (a \times 1{,}000{,}000) + (b \times 10{,}000) + (c \times 100) + d$$

Any rule marked with `!important` immediately bypasses standard Tier 4/5 overrides.

---

## 2. First 25 Core Components Implementation Matrix

All 25 components were implemented in `include/pingdart/ui/components.h` adhering to the PKG-137A dependency order:

| Category | Component | Semantic Tag | Key Capabilities & Styling Defaults |
| :--- | :--- | :--- | :--- |
| **Layout** (7) | `Container` | `div` | Responsive column flow, 100% width, container padding |
| | `Row` | `div` | Flex row, `align-items: center`, gap support |
| | `Column` | `div` | Flex column, vertical flow |
| | `Stack` | `div` | Layered relative positioning for overlays |
| | `Flex` | `div` | Direct control of direction, wrap, justify, align |
| | `Grid` | `div` | CSS Grid (`grid-template-columns`, `grid-gap`) |
| | `ScrollView` | `div` | Viewport clipping with `overflow-y: auto`, max height |
| **Basic** (6) | `Text` | `span` | Text span and reactive string callback binding |
| | `Heading` | `h1`–`h6` | Semantic typographic scale, bold/semibold weights |
| | `Image` | `img` | Void tag, `src`, `alt`, dimension control |
| | `Icon` | `i` | Accessible icon with `data-icon` and icon class |
| | `Divider` | `hr` | Void tag, subtle 1px border separator |
| | `Spacer` | `div` | Configurable spacing box |
| **Actions** (2) | `Button` | `button` | Variants (primary, secondary, destructive), hover, click |
| | `IconButton` | `button` | Accessible icon button with `aria-label` |
| **Forms** (6) | `Input` | `input` | Void tag, text input, placeholder, `onChange` |
| | `TextArea` | `textarea` | Multi-line text input with configurable rows |
| | `Checkbox` | `label`+`input`| Checked state, accessible label binding |
| | `Radio` | `label`+`input`| Radio group name, value, checked attribute |
| | `Switch` | `button`+`div` | Accessible toggle switch with `role="switch"`, thumb slider |
| | `Select` / `Option` | `select` | Dropdown menu with child `option` elements |
| **Display** (3) | `Card` | `div` | Elevated container with border, radius, shadow, padding |
| | `Avatar` | `img`/`div` | Circular user avatar (image or initials fallback) |
| | `Badge` | `span` | Pill-shaped status indicator badge |
| **Overlay** (1) | `Dialog` | `div` | Modal dialog with semi-transparent backdrop and card body |

---

## 3. Interactive Component Gallery Verification

The component gallery executable (`example_component_gallery`) compiles and runs cleanly, validating all 25 components, custom CSS parsing, Tailwind classes, and dark mode toggling:

```text
==========================================================
 PingDartUI — Production Component Gallery (25 Components) 
==========================================================
[CSS Parser]: Parsed 8 CSS rules with 0 syntax diagnostics.

[Web SSR HTML Output generated (16604 bytes)]
Sample snippet:
<div style="background-color: #F8FAFC; font-size: 16px; padding: 24px 24px 24px 24px; display: flex; flex-direction: column; gap: 24px; width: 100%; " class="gallery-container"><div style="font-size: 16px; display: flex; flex-direction: row; align-items: center; "><h1 style="color: #0F172A; font-size: 24px; font-weight: 700; display: flex; flex-direction: column; " class="gallery-header">PingDartU...

[Qt Quick SceneGraph QML generated (8790 bytes)]
Sample snippet:
// PingDartUI Generated Qt Quick SceneGraph (Qt 6.8 LTS)
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Column {
    spacing: 24
    // width: 100%
    Row {
        Text {
            text: "PingDartUI Component Gallery"
        }
        Column {
            // width: 16px
            // height: 16px
        }
        Button {
            text: "Toggle Dark Mode"
            // r...

[Testing Dark Mode Toggle]...
Dark Mode active in SSR: YES (data-theme="dark" injected)

Gallery validation successfully completed.
```

---

## 4. Full CTest Regression Summary (72 / 72 Suites Passing)

```text
Test project /Volumes/Development/PingDart/PingDartCore/build
      Start  1: CoreSmokeTest ..............................   Passed    0.00 sec
      ...
      Start 66: Pkg128FullStackCertificationGoogleTest .....   Passed    3.15 sec
      Start 67: Pkg129RuntimeDeploymentGoogleTest ..........   Passed    3.60 sec
      Start 68: Pkg130NativeRuntimeIntegrationGoogleTest ...   Passed    3.08 sec
      Start 69: Pkg132NativeRuntimeExecutionGoogleTest .....   Passed    1.79 sec
      Start 70: Pkg133RealDeviceAcceptanceGoogleTest .......   Passed    1.74 sec
      Start 71: Pkg135PingDartUiFoundationGoogleTest .......   Passed    0.01 sec
      Start 72: Pkg136CssStylingEngineGoogleTest ...........   Passed    0.01 sec

100% tests passed out of 72
Total Test time (real) = 21.72 sec
```

### Dedicated PKG-136 Acceptance Suite (`test_pkg136_css_styling_engine`):
1. `Pkg136CssStylingEngineTest.CssParserBasicRules` — Passed
2. `Pkg136CssStylingEngineTest.CssParserDiagnosticsResilience` — Passed
3. `Pkg136CssStylingEngineTest.CssVariableResolution` — Passed
4. `Pkg136CssStylingEngineTest.SpecificityOrdering` — Passed
5. `Pkg136CssStylingEngineTest.ImportantFlagOverrides` — Passed
6. `Pkg136CssStylingEngineTest.FiveLayerCascadePrecedence` — Passed
7. `Pkg136CssStylingEngineTest.DimensionAndUnitParsing` — Passed
8. `Pkg136CssStylingEngineTest.FlexboxAndGridLayout` — Passed
9. `Pkg136CssStylingEngineTest.BundledPalettesAndConfigLoading` — Passed
10. `Pkg136CssStylingEngineTest.DarkModeTogglingAndSsrInjection` — Passed
11. `Pkg136CssStylingEngineTest.XssEntitySanitizationInSsr` — Passed
12. `Pkg136CssStylingEngineTest.ThreadSafeUtilityClassCache` — Passed (8 concurrent worker threads)
13. `Pkg136CssStylingEngineTest.All25ComponentsCatalogValidation` — Passed (all 25 components verified)

---

## 5. Formal Verification Decision

**PKG-136 is fully approved and certified.**  
The PingDartUI Unified CSS & Utility Styling Engine satisfies all requirements and provides the foundational styling infrastructure for subsequent UI component libraries and platform targets.
