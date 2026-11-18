# PingDartUI Supported CSS Specification & Styling Architecture

**Version**: 1.0 (Qt 6.8 LTS & PingCore Unified Native Runtime)  
**Package**: PKG-136  
**Status**: Formal Specification  

---

## 1. Executive Overview

PingDartUI provides a **Unified Cross-Platform Native-Compiled Framework** combining the developer ergonomics of modern declarative web styling (CSS3, CSS Variables, and Tailwind-compatible utility classes) with compiled C++20 performance.

PingDartUI features a deterministic 5-tier styling cascade engine that resolves styles across Web Server-Side Rendering (SSR), Qt 6.8 LTS Quick SceneGraph, and PingCore Native Render Hardware Interface (RHI) without exposing Qt headers, raw pointers, or platform-specific rendering primitives to application code.

---

## 2. Five-Layer Styling Cascade & Precedence Engine

When resolving visual styling for any component, PingDartUI applies a strict 5-layer cascade hierarchy. Higher tiers override lower tiers unless overridden by an `!important` declaration:

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

### Specificity Scoring (Tier 3)
External CSS selectors are evaluated using the standard 4-tuple specificity vector:
$$\text{Specificity} = (a, b, c, d)$$
- $a$: Inline styles ($a = 1$ when applied via style attribute, else $0$).
- $b$: ID selectors (`#id`).
- $c$: Class selectors (`.class`) and pseudo-classes (`:hover`, `:focus`, `:active`, `:disabled`).
- $d$: Element/Tag selectors (`div`, `button`, `span`, `h1`).

Total specificity score is calculated as:
$$\text{Score} = (a \times 1{,}000{,}000) + (b \times 10{,}000) + (c \times 100) + d$$

When specificity scores are identical, later declarations in source order override earlier declarations. Any declaration tagged with `!important` immediately bypasses standard Tier 4/5 overrides.

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

## 5. Bundled Design Tokens & Palettes

PingDartUI bundles 8 standard color palettes from 50 to 950 shades:
- **Slate**: Neutral cool grays (`#F8FAFC` to `#020617`)
- **Zinc**: Neutral balanced grays (`#FAFAFA` to `#09090B`)
- **Red**: Error, destructive, and alerts (`#FEF2F2` to `#450A0A`)
- **Amber**: Warnings and cautions (`#FFFBEB` to `#451A03`)
- **Emerald**: Success, completions, and positive indicators (`#ECFDF5` to `#022C22`)
- **Blue**: Primary interactive brand actions (`#EFF6FF` to `#172554`)
- **Indigo**: Elevated accents (`#EEF2FF` to `#1E1B4B`)
- **Violet**: Secondary creative accents (`#F5F3FF` to `#2E1065`)

### CSS Variable Resolution
Dynamic CSS variables are declared and resolved via `ThemeEngine`:
```css
.card {
    background-color: var(--card, #FFFFFF);
    color: var(--foreground, #0F172A);
    border: 1px solid var(--border, #E2E8F0);
    border-radius: var(--radius, 12px);
}
```

Standard token names:
- `--primary`: Primary brand action color (`#2563EB` light, `#3B82F6` dark)
- `--secondary`: Supporting neutral color (`#64748B`)
- `--success`: Positive feedback color (`#16A34A`)
- `--danger`: Destructive/error action color (`#DC2626`)
- `--warning`: Warning alert color (`#D97706`)
- `--background`: Page background (`#FFFFFF` light, `#0F172A` dark)
- `--foreground`: High-contrast body text (`#0F172A` light, `#F8FAFC` dark)
- `--card`: Surface container background (`#FFFFFF` light, `#1E293B` dark)
- `--border`: Standard structural border color (`#E2E8F0` light, `#334155` dark)
- `--muted`: Low-emphasis background (`#F1F5F9` light, `#1E293B` dark)

---

## 6. Tailwind-Compatible Utility Classes

PingDartUI provides a high-throughput, thread-safe `UtilityClassCompiler` with LRU caching (`std::unordered_map<std::string, ComputedStyle>`):

| Utility Prefix | Examples | Resulting ComputedStyle |
| :--- | :--- | :--- |
| `bg-{color}-{shade}` | `bg-blue-600`, `bg-slate-100`, `bg-white` | `background_color = "#2563EB"` |
| `text-{color}-{shade}` | `text-slate-900`, `text-white`, `text-red-600` | `text_color = "#0F172A"` |
| `text-{size}` | `text-sm`, `text-base`, `text-lg`, `text-xl`, `text-2xl` | `font_size = 14`, `16`, `18`, `20`, `24` |
| `p-{n}`, `px-{n}`, `py-{n}` | `p-4` (16px), `px-6` (24px), `py-2` (8px) | `padding` calculated with `n * 4px` |
| `m-{n}`, `mx-{n}`, `my-{n}` | `m-4` (16px), `mx-auto` | `margin` calculated with `n * 4px` |
| `gap-{n}` | `gap-2` (8px), `gap-4` (16px), `gap-6` (24px) | `gap = n * 4px` |
| `rounded-{size}` | `rounded-sm` (4px), `rounded-md` (6px), `rounded-lg` (8px), `rounded-xl` (12px), `rounded-full` (9999px) | `border_radius` |
| `flex-row`, `flex-col` | `flex-row`, `flex-col` | `flex_direction = FlexDirection::Row / Column` |
| `items-{align}` | `items-center`, `items-start`, `items-end` | `align_items = AlignItems::Center / Start / End` |
| `justify-{align}` | `justify-between`, `justify-center`, `justify-end` | `justify_content = SpaceBetween / Center / End` |
| `w-full`, `h-full` | `w-full`, `h-full` | `width = "100%"`, `height = "100%"` |
| `font-{weight}` | `font-medium` (500), `font-semibold` (600), `font-bold` (700) | `font_weight` |
| `shadow-{size}` | `shadow-sm`, `shadow-md`, `shadow-lg` | Multi-layer drop shadow |
| `cursor-pointer` | `cursor-pointer` | `cursor = "pointer"` |
| `overflow-auto` | `overflow-auto`, `overflow-hidden` | `overflow = "auto" / "hidden"` |

---

## 7. Responsive Breakpoints

PingDartUI evaluates media queries and responsive utility prefixes against 5 standard breakpoints:
- `sm`: $\ge 640\text{ px}$ (Mobile landscape / small tablets)
- `md`: $\ge 768\text{ px}$ (Standard tablets)
- `lg`: $\ge 1024\text{ px}$ (Laptops / desktops)
- `xl`: $\ge 1280\text{ px}$ (Large desktops)
- `2xl`: $\ge 1536\text{ px}$ (High-resolution displays)

---

## 8. Unsupported Properties & Rationale

The following web-only or legacy CSS constructs are intentionally excluded from the unified native engine:

1. **`float` / `clear` Layout**:
   - *Rationale*: Legacy layout model superseded by Flexbox and CSS Grid. Direct mapping to Qt SceneGraph and Native Vulkan/Metal layouts is non-deterministic.
2. **Complex 3D CSS Transforms (`transform: rotate3d(...)`, `perspective`)**:
   - *Rationale*: PingDartUI delegates 3D transforms to dedicated hardware-accelerated RHI SceneGraph matrices in PKG-139 rather than parsing CSS matrix strings.
3. **CSS Counters (`counter-reset`, `counter-increment`)**:
   - *Rationale*: Dynamic counting is natively managed through reactive state (`useState<int>()`).
4. **CSS Preprocessor Directives (`@extend`, `@mixin`, `@include`)**:
   - *Rationale*: Compile-time composability is natively provided by C++20 templates and helper functions.

---

## 9. Security & XSS Mitigation

The SSR renderer (`WebSsrAdapter`) performs automatic, strict sanitization of all HTML entities:
- `&` $\rightarrow$ `&amp;`
- `<` $\rightarrow$ `&lt;`
- `>` $\rightarrow$ `&gt;`
- `"` $\rightarrow$ `&quot;`
- `'` $\rightarrow$ `&#39;`

Untrusted inputs embedded in component text or attributes cannot inject malicious script tags or execute arbitrary JavaScript during server-side hydration.
