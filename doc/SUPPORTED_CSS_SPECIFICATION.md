# PingDartUI Supported CSS Specification & Styling Architecture

**Version**: 1.1 (Qt 6.8 LTS & PingCore Unified Native Runtime)  
**Package**: PKG-137  
**Status**: Formal Specification  

---

## 1. Executive Overview

PingDartUI provides a **Unified Cross-Platform Native-Compiled Framework** combining the developer ergonomics of modern declarative web styling (CSS3, CSS Variables, and Tailwind-compatible utility classes) with compiled C++20 performance.

PingDartUI features a deterministic 5-tier styling cascade engine that resolves styles across Web Server-Side Rendering (SSR), Qt 6.8 LTS Quick SceneGraph, and PingCore Native Render Hardware Interface (RHI) without exposing Qt headers, raw pointers, or platform-specific rendering primitives to application code.

---

## 2. Five-Layer Styling Cascade & Lexicographical Specificity Engine

When resolving visual styling for any component, PingDartUI applies a strict 5-layer cascade hierarchy. Higher tiers override lower tiers unless overridden by an `!important` declaration:

```
┌─────────────────────────────────────────────────────────────┐
│  Tier 5: Fluent C++ Method Chaining (.padding().color())    │  (Highest precedence)
├─────────────────────────────────────────────────────────────┤
│  Tier 4: Utility Classes (.className("bg-blue-600 p-4"))    │
├─────────────────────────────────────────────────────────────┤
│  Tier 3: External CSS Rules (.card, #main, button:hover)    │  (Lexicographically compared)
├─────────────────────────────────────────────────────────────┤
│  Tier 2: Theme Configuration (pingdart.config.json / Tokens)│
├─────────────────────────────────────────────────────────────┤
│  Tier 1: Component Defaults (e.g. Button padding, Card bg)  │  (Lowest precedence)
└─────────────────────────────────────────────────────────────┘
```

### Lexicographical Specificity Comparison (P0 Precision)
External CSS selectors are evaluated using strict C++20 three-way comparison (`operator<=>`) over the 4-tuple specificity vector:

```cpp
struct CssSpecificity {
    uint32_t a{0}; // Inline style count
    uint32_t b{0}; // ID count
    uint32_t c{0}; // Class / pseudo-class count
    uint32_t d{0}; // Tag / element count

    // C++20 member-wise lexicographical comparison: a -> b -> c -> d
    auto operator<=>(const CssSpecificity&) const = default;
    bool operator==(const CssSpecificity&) const = default;
};
```

This guarantees zero arithmetic overflow: an ID selector `(0, 1, 0, 0)` strictly outranks any combination of class and element selectors `(0, 0, 250, 500)`, and inline styles `(1, 0, 0, 0)` strictly outrank any selector combination.

When specificity tuples are identical, later declarations in source order override earlier declarations. Any declaration tagged with `!important` immediately bypasses standard Tier 4/5 overrides.

---

## 3. Supported Selectors & States

| Selector Type | Syntax Example | Specificity $(a,b,c,d)$ | Notes |
| :--- | :--- | :--- | :--- |
| **Universal** | `*` | $(0, 0, 0, 0)$ | Matches any component node. |
| **Type / Element** | `button`, `div`, `span`, `h1` | $(0, 0, 0, 1)$ | Matches node tag. |
| **Class** | `.card`, `.badge` | $(0, 0, 1, 0)$ | Matches assigned class names. |
| **Multi-Class** | `.card.elevated` | $(0, 0, 2, 0)$ | Requires all listed classes. |
| **ID** | `#header`, `#submit-btn` | $(0, 1, 0, 0)$ | Matches unique component ID. |
| **Element + Class** | `button.primary` | $(0, 0, 1, 1)$ | Matches element having class. |
| **Element + ID** | `div#container` | $(0, 1, 0, 1)$ | Matches element having ID. |
| **Pseudo: hover** | `button:hover` | $(0, 0, 1, 1)$ | Evaluated when pointer hovers. |
| **Pseudo: focus** | `input:focus` | $(0, 0, 1, 1)$ | Evaluated when component is focused. |
| **Pseudo: active** | `button:active` | $(0, 0, 1, 1)$ | Evaluated during click/press. |
| **Pseudo: disabled**| `button:disabled` | $(0, 0, 1, 1)$ | Evaluated when disabled flag is true. |

---

## 4. Supported Properties, Units, & Values

### 4.1 Measurement Units
- `px`: Absolute pixels (1px = 1 device-independent pixel).
- `rem`: Root font size multiple (default root = 16px).
- `em`: Current font size multiple.
- `%`: Relative percentage of parent dimension.
- `vw`, `vh`: Viewport width and height percentage units.

### 4.2 Property Matrix

| CSS Property | Supported Values / Types | Default Value | Internal ComputedStyle Field |
| :--- | :--- | :--- | :--- |
| `background`, `background-color` | Hex (`#FFFFFF`, `#1E293B`), `var(--...)`, CSS colors | `""` (transparent) | `background_color` |
| `color` | Hex, `var(--...)`, CSS color names | `""` (inherit) | `text_color` |
| `opacity` | Float `0.0` to `1.0` | `1.0` | `opacity` |
| `border`, `border-width` | Dimension (`1px`, `2px solid #CBD5E1`) | `0.0` | `border_width`, `border_color` |
| `border-radius` | Dimension (`4px`, `8px`, `12px`, `9999px`) | `0.0` | `border_radius` |
| `border-color` | Hex, `var(--...)` | `""` | `border_color` |
| `border-style` | `solid`, `dashed`, `dotted`, `none` | `"solid"` | `border_style` |
| `padding` | 1, 2, or 4 values (`12px`, `8px 16px`) | `0` | `padding` (`Insets`) |
| `margin` | 1, 2, or 4 values (`16px`, `0 auto`) | `0` | `margin` (`Insets`) |
| `width`, `height` | Dimension (`100%`, `480px`, `auto`) | `""` | `width`, `height` |
| `min-width`, `max-width` | Dimension (`200px`, `1200px`) | `0.0` | `min_width`, `max_width` |
| `min-height`, `max-height` | Dimension (`100px`, `600px`) | `0.0` | `min_height`, `max_height` |
| `display` | `flex`, `grid`, `block`, `none` | `"flex"` | `is_flex`, `is_grid`, `display` |
| `position` | `relative`, `absolute` | `""` | `position` |
| `overflow`, `overflow-y` | `auto`, `hidden`, `scroll`, `visible` | `""` | `overflow` |
| `cursor` | `pointer`, `default`, `not-allowed` | `""` | `cursor` |
| `gap` | Dimension (`8px`, `16px`, `1rem`) | `0.0` | `gap` |
| `flex-direction` | `row`, `column`, `row-reverse`, `column-reverse` | `column` | `flex_direction` |
| `flex-wrap` | `nowrap`, `wrap`, `wrap-reverse` | `nowrap` | `flex_wrap` |
| `align-items` | `flex-start`, `center`, `flex-end`, `stretch` | `flex-start` | `align_items` |
| `justify-content` | `flex-start`, `center`, `flex-end`, `space-between` | `flex-start` | `justify_content` |
| `grid-template-columns` | CSS grid definition (`repeat(3, 1fr)`, `1fr 2fr`) | `""` | `grid_template_columns` |
| `font-size` | Dimension (`14px`, `16px`, `1.5rem`) | `16.0` | `font_size` |
| `font-weight` | `400` (normal), `500` (medium), `600` (semibold), `700` (bold) | `400` | `font_weight` |
| `box-shadow` | `x y blur spread color` | `""` | `shadow` (`BoxShadow`) |

---

## 5. Bundled Design Tokens & Scoped ThemeContext

PingDartUI bundles 8 standard color palettes from 50 to 950 shades:
- **Slate**: Neutral cool grays (`#F8FAFC` to `#020617`)
- **Zinc**: Neutral balanced grays (`#FAFAFA` to `#09090B`)
- **Red**: Error, destructive, and alerts (`#FEF2F2` to `#450A0A`)
- **Amber**: Warnings and cautions (`#FFFBEB` to `#451A03`)
- **Emerald**: Success, completions, and positive indicators (`#ECFDF5` to `#022C22`)
- **Blue**: Primary interactive brand actions (`#EFF6FF` to `#172554`)
- **Indigo**: Elevated accents (`#EEF2FF` to `#1E1B4B`)
- **Violet**: Secondary creative accents (`#F5F3FF` to `#2E1065`)

### Scoped ThemeContext
In addition to the global `ThemeEngine::instance()`, applications can instantiate independent `ThemeContext` objects for windows, modal panels, or subtrees:

```cpp
auto window_theme = std::make_shared<ThemeContext>();
window_theme->set_dark_mode(true);
window_theme->tokens().primary = "#7C3AED"; // Scoped violet theme

Component modal = Card({ ... }).theme(window_theme);
```

---

## 6. Security: Context-Aware Sanitization

To guarantee zero code execution across all supported renderers:
1. **HTML Entity Escaping**: Strict conversion of `&`, `<`, `>`, `"`, `'` prevents DOM injection during SSR hydration.
2. **URL Sanitization**: Blocks dangerous execution schemes (`javascript:`, `vbscript:`, `data:text/html`). Only safe schemes (`http://`, `https://`, `mailto:`, `tel:`, `/`, `#`) are allowed; malicious schemes resolve to `#blocked-unsafe-url`.
3. **QML String Escaping**: Escapes quotes, newlines, and backslashes in generated SceneGraph strings.
4. **CSS Value Sanitization**: Strips `expression(...)`, `behavior:`, and embedded scripts.

---

## 7. Functional QML Layout Bindings (Qt 6.8 LTS)

PingDartUI does not emit dimensions as dead comments. All layout properties translate directly into functional Qt Quick QML properties and bindings:
- `width: "100%"` $\rightarrow$ `width: parent.width`
- `height: "100%"` $\rightarrow$ `height: parent.height`
- `border_radius` $\rightarrow$ `radius: <val>`
- `gap` $\rightarrow$ `spacing: <val>`
- `font_size` $\rightarrow$ `font.pixelSize: <val>`
- `font_weight` $\rightarrow$ `font.bold: true`
