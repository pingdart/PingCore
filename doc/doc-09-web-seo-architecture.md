# PingDart Framework
## DOC-09: First-Class Web Platform & SEO Architecture

**Status:** Technical Architecture Specification  
**Version:** 1.0  
**Parent Documents:** [PingDart PRD](prd.md) · [DOC-01 Core Runtime](doc-01-core-runtime-architecture.md) · [DOC-02 Rendering Engine](doc-02-rendering-engine-architecture.md) · [DOC-05 Widget & Layout Engine](doc-05-widget-layout-engine.md) · [DOC-08 Task Breakdown](doc-08-v1-development-task-breakdown.md)  
**Target Milestone:** Web Platform & Universal SEO

---

# 1. Executive Summary & Philosophy

PingDart Framework treats the **Web as a first-class citizen**, standing alongside Windows, Linux, Android, iOS, and macOS.

### The Critical Web Fallacy: WebGPU/Canvas Alone is NOT Web
Many cross-platform native frameworks compile their GPU renderers directly to WebAssembly and blit pixels into a single full-screen `<canvas>` via WebGPU or WebGL. While this provides pixel-identical rendering for high-intensity desktop apps, it has catastrophic drawbacks for web applications:
1. **Zero SEO Indexability:** Search engine crawlers (Googlebot, Bingbot) see an empty `<canvas>` without parseable text, headlines, links, or metadata.
2. **Broken Browser Affordances:** Native text selection, right-click context menus, browser page search (Ctrl+F), screen readers (a11y), and password autofill are non-functional.
3. **Heavy Initial Payload:** Users must download megabytes of WASM binaries and GPU pipelines before seeing the first frame.

### The PingDart Dual-Rendering Architecture
PingDart resolves this by establishing **Two Parallel Rendering Paths** driven by a single unified C++ application codebase:

```text
+--------------------------------------------------------------------------+
|                  Single PingDart Application Codebase                    |
|             C++ Application Logic · Widgets · Routing · State            |
+--------------------------------------------------------------------------+
                                     |
                 +-------------------+-------------------+
                 |                                       |
                 v                                       v
+---------------------------------+     +----------------------------------+
|     Native Renderer Path        |     |        Web Renderer Path         |
|  (Vulkan · D3D12 · Metal)       |     |   (Semantic HTML5 · CSS3 · WASM) |
|                                 |     |                                  |
|  - Windows, Linux, macOS        |     |  - SSR (Server-Side Rendering)   |
|  - Android, iOS                 |     |  - SSG (Static Site Generation)  |
|  - GPU DisplayList rasterizer   |     |  - Hydration & Browser DOM       |
|  - Direct compositor swapchain  |     |  - Full SEO (JSON-LD, OpenGraph) |
+---------------------------------+     +----------------------------------+
```

---

# 2. Web Rendering Modes & Strategy

| Mode | Purpose | SEO Capabilities | Runtime Technology |
|---|---|---|---|
| **Static Site Generation (SSG)** | Marketing, docs, blogs, directory listings, landing pages | **Maximum**: Pre-rendered HTML + schema | Build-time native C++ headless generator |
| **Server-Side Rendering (SSR)** | Dynamic product catalogs, localized portals, authenticated public profiles | **Maximum**: On-demand HTML generated per request | High-performance C++ HTTP server / edge worker |
| **Hydrated Client Application** | Interactive forms, stateful flows, real-time dashboards | **High**: Interactive event listeners bound to server-rendered HTML | Lightweight C++ WebAssembly + DOM bindings |
| **Selective WebGPU/Canvas Islands** | Embedded charts, games, creative tools, 3D visualizations | **Scoped**: Contained inside semantic `<section>` / `<figure>` with `aria-label` | WebGPU/WASM embedded within semantic DOM tree |

---

# 3. Component Semantic Decomposition (`SemanticNode`)

Widgets in PingDart express both **Visual Presentation** and **Semantic Intent**. During widget tree layout:
- In Native mode: `paint()` calls `DisplayListBuilder` to record GPU draw primitives.
- In Web mode: `build_semantic_tree()` decomposes the widget into a hierarchy of semantic DOM elements:

```text
Widget Declaration (e.g. DoctorCardWidget)
       |
       +---> [Native Path] ----> Skia / Vulkan DisplayList
       |
       +---> [Web Path] -------> SemanticNode
                                    |
                                    v
                     <article class="pd-doctor-card">
                       <header>
                         <h2>Dr. Jane Smith</h2>
                         <p class="pd-subtitle">Cardiologist</p>
                       </header>
                       <p>15 years experience in pediatric cardiology.</p>
                       <a href="/doctors/jane-smith" class="pd-btn">
                         Book Appointment
                       </a>
                     </article>
```

### Semantic Tag Mapping
| Widget Category | PingDart Widget | Native Render | Web Semantic HTML Tag |
|---|---|---|---|
| Structural | `Page`, `Scaffold` | Surface background | `<main role="main">` |
| Grouping | `Container`, `Card` | Rounded rect + shadow | `<article>`, `<section>`, `<div>` |
| Text Headers | `Heading(level: 1..6)` | HarfBuzz shaped text | `<h1>` through `<h6>` |
| Text Paragraphs | `Text(body)` | HarfBuzz shaped text | `<p>`, `<span>` |
| Navigation | `Link(href, label)` | Styled interactive text | `<a href="...">` |
| Interaction | `Button(on_click)` | Vector box + ripple | `<button type="button">` |
| Input | `TextField`, `Form` | Native IME caret | `<input>`, `<textarea>`, `<form>` |
| Media | `Image(src, alt)` | Texture quad | `<img src="..." alt="..." loading="lazy">` |
| Graphical Island | `CanvasIsland` | Embedded sub-view | `<canvas role="img" aria-label="...">` (WebGPU) |

---

# 4. Universal SEO & Metadata Engine

Search engine indexing requires comprehensive, valid `<head>` metadata, social graph tags, and structured linked data.

### 4.1 Metadata Hierarchy (`SeoMetadata`)
Each route or widget page specifies a `SeoMetadata` object:
1. **Core Crawlability:**
   - `<title>`: Unique, concise page title.
   - `<meta name="description">`: 150-160 character crawl snippet.
   - `<link rel="canonical" href="...">`: Prevents duplicate content penalties across query parameters and subdomains.
   - `<meta name="robots" content="index, follow">`: Directives for crawl robots.
2. **Social Graph Optimization:**
   - Open Graph (`og:title`, `og:description`, `og:image`, `og:url`, `og:type`).
   - Twitter Card (`twitter:card`, `twitter:site`, `twitter:title`, `twitter:image`).
3. **Structured Data (Schema.org / JSON-LD):**
   - Emits valid `<script type="application/ld+json">` for Google Rich Results.
   - Standard schemas: `MedicalBusiness`, `Physician`, `Product`, `Article`, `Organization`, `BreadcrumbList`.
4. **Internationalization (i18n):**
   - `<link rel="alternate" hreflang="es" href="...">` for multi-language indexing.

---

# 5. Server-Side Rendering (SSR) & Static Site Generation (SSG)

### 5.1 SSG Build Pipeline (`pd build --platform web --mode static`)
1. **Route Discovery:** Scans route definitions (`/`, `/about`, `/doctors`, `/doctors/:id`).
2. **Headless Execution:** Instantiates `HeadlessPlatformHost` in pure native C++ (no browser required).
3. **Tree Inflation & Serialization:** Evaluates widgets, serializes HTML to `dist/`, and writes bundled CSS.
4. **Sitemap & Robots Generation:** Automatically emits `dist/sitemap.xml` with `<lastmod>` timestamps and `dist/robots.txt`.

### 5.2 SSR Request Pipeline (`pd build --platform web --mode ssr`)
```text
HTTP GET /doctors/jane-smith
       |
       v
PingDart C++ SSR Runtime (Worker Process)
       |
       +---> Match Route & Fetch Data
       |
       +---> Build Widget Tree
       |
       +---> Render Semantic HTML + Injected State JSON
       |
       +---> Transmit complete HTML stream to client (TTFB < 50ms)
       |
Browser receives valid HTML -> Paints immediately (First Contentful Paint < 200ms)
       |
WASM loads asynchronously -> Hydrates event listeners -> Instant Client Interactivity
```

---

# 6. Hydration Contract & Progressive Enhancement

When the browser receives the pre-rendered HTML:
1. **Zero Layout Shift:** The DOM structure emitted by SSR matches the component tree inflated by WebAssembly exactly.
2. **Progressive Hydration:**
   - Elements with active event listeners (buttons, text fields) receive unique deterministically hashed `data-pd-id` attributes.
   - The C++ WASM client initializes, queries `data-pd-id` nodes, and attaches DOM event listeners (`click`, `input`, `keydown`).
   - No DOM destruction or innerHTML replacement occurs during hydration.
3. **Client-Side Navigation:**
   - Links intercept clicks, push history state via the HTML5 History API (`pushState`), and update the DOM incrementally without full-page reloads.

---

# 7. Web CLI Tooling (`pd`)

The official developer CLI exposes streamlined web workflows:

```bash
# Start local live-reload development server for web
pd dev --platform web --port 8080

# Build static production website (SSG) with automatic sitemap and SEO pre-rendering
pd build --platform web --mode static --output-dir dist/

# Build standalone SSR binary (ultra-fast C++ server producing semantic HTML)
pd build --platform web --mode ssr --output-dir dist/

# Build client-side interactive WASM app (for dashboards)
pd build --platform web --mode spa --output-dir dist/
```
