# PingDart Framework
## DOC-05: Widget & Layout Engine Specification

**Status:** Technical Architecture Specification  
**Version:** 1.0  
**Language Baseline:** C++20  
**Parent Documents:** [PingDart PRD](prd.md) · [DOC-01: Core Runtime](doc-01-core-runtime-architecture.md) · [DOC-03: Platform Adapter SDK](doc-03-platform-adapter-sdk.md) · [DOC-02: Rendering Engine](doc-02-rendering-engine-architecture.md)  
**Target Phase:** Phase 1 (Desktop Proof-of-Concept)

---

# 1. Executive Summary

The **Widget & Layout Engine** is the developer-facing UI foundation of PingDart. It provides a declarative, reactive programming model modeled on modern UI frameworks while maintaining high performance, deterministic memory management, and C++20 type safety.

PingDart implements a strict **Three-Tree Architecture**:
1. **The Widget Tree:** Declarative, immutable configuration descriptions. Lightweight and ephemeral.
2. **The Element Tree:** Persistent structural representation managing component lifecycle, state retention, and diffing/reconciliation.
3. **The Render Tree:** Geometry calculation, constraint solving, hit testing, and paint command emission into the DOC-02 `DisplayList`.

```text
+--------------------------------------------------------------------------+
|                        APPLICATION WIDGET TREE                           |
|       Declarative, Immutable C++20 UI Configuration (Ephemeral)          |
+--------------------------------------------------------------------------+
                                     |  creates / inflates
                                     v
+--------------------------------------------------------------------------+
|                         PERSISTENT ELEMENT TREE                          |
|         Component Identity · State<T> Retention · Reconciliation         |
+--------------------------------------------------------------------------+
                                     |  mounts / updates
                                     v
+--------------------------------------------------------------------------+
|                           PHYSICAL RENDER TREE                           |
|      BoxConstraints Solver · Geometry · Hit Testing · SemanticsTree       |
+--------------------------------------------------------------------------+
                                     |  records paint commands
                                     v
+--------------------------------------------------------------------------+
|                       DOC-02 DISPLAY LIST & COMPOSITOR                   |
|           Immutable Commands · Layers · Scissored Damage Regions         |
+--------------------------------------------------------------------------+
                                     |  GPU submit
                                     v
+--------------------------------------------------------------------------+
|                  DOC-03 NATIVE PLATFORM SURFACE (PAL)                    |
|                      (Win32 / Wayland / X11)                             |
+--------------------------------------------------------------------------+
```

---

# 2. The Three-Tree Architecture & Object Ownership

### 2.1 Ownership and Thread-Affinity Contracts
1. **Thread Affinity:** All three trees (Widgets, Elements, RenderObjects) reside strictly on the **UI Thread**. Cross-thread access is strictly prohibited.
2. **Widget Ownership:** Widgets are created as value objects or temporary heap objects passed by value or `std::shared_ptr`/`RefPtr`. They do not own native resources.
3. **Element Ownership:** The root `Element` is owned by `BuildOwner` / `Window`. Parent elements strictly own their child elements via `std::vector<std::unique_ptr<Element>>`.
4. **RenderObject Ownership:** `RenderElement` instances own their corresponding `RenderObject` instances (`std::unique_ptr<RenderObject>`). In the render tree, parent `RenderObject` instances maintain raw pointers or non-owning references to their children.

```text
[Element Tree] (Owner)               [Render Tree] (Hierarchy)
      |                                    |
ContainerElement                             ContainerRenderObject
  |-(owns unique_ptr)--> ColumnElement ---->   |-(children)--> ColumnRenderObject
                           |-(owns)--> TextElement                 |-(child)--> TextRenderObject
```

---

# 3. Widget & Element Lifecycle Specification

### 3.1 Widget Identity & Keys
Widgets are distinguished by runtime type and optional `Key`:
- `Key`: Base polymorphic identity key.
- `ValueKey<T>`: Key matching by value (e.g., `ValueKey<std::string>("item_42")`).
- `ObjectKey`: Key matching by pointer identity of an underlying model.
- `UniqueKey`: Guaranteed unique random token.

### 3.2 Reconciliation Algorithm & Multi-Child Matching
When a widget tree rebuilds, PingDart avoids recreating elements and render objects whenever possible:
```cpp
bool can_update(const Widget& old_widget, const Widget& new_widget) {
    return old_widget.type_id() == new_widget.type_id() &&
           old_widget.key() == new_widget.key();
}
```

#### Sibling Key Scoping & Reordering Rules
1. **Sibling Scoping:** Keys are evaluated strictly within their immediate sibling collection (e.g. within a single `Row` or `Column`'s `children` array). Keys do not conflict across different branches of the tree.
2. **Duplicate Key Assertion:** Duplicate non-null keys within the same sibling collection violate tree integrity. In Debug builds, this raises an immediate assertion (`"Duplicate key detected in sibling collection"`); in Release builds, the first occurrence is matched and subsequent occurrences fall back to keyless diffing.
3. **Child Reordering:**
   - Multi-child elements (e.g., `MultiChildRenderObjectElement`) index existing keyed children into a temporary hash table (`std::unordered_map<Key, std::unique_ptr<Element>>`).
   - Reordered keyed children are matched by key, preserving their underlying persistent `Element`, their state (`State<T>`), and their `RenderObject` without reallocations.
   - Remaining unkeyed or unmatched old elements are gracefully unmounted and cleaned up.

### 3.3 Element Lifecycle State Machine & State Invariants
```text
           [Instantiated]
                 |
               mount()
                 v
              [Active] <------+
                 |            | update()
       deactivate()           |
                 v            |
             [Inactive] ------+
                 |
              unmount()
                 v
             [Defunct / Destroyed]
```
1. **`mount(Element* parent, Slot slot)`:** Element enters the tree, registers dependencies, and creates its `RenderObject` if applicable.
2. **`update(const Widget& new_widget)`:** Replaces widget reference and marks render object dirty if properties changed.
3. **`deactivate()`:** Temporarily detached (e.g., during tree restructuring or Hero animations).
4. **`unmount()`:** Permanently detached; cleans up child elements, disposes persistent state, and destroys render objects.

#### State Mutation Safety Rules (`set_state`)
- **UI-Thread Affiliation:** `set_state()` asserts `EventLoop::current().is_on_loop_thread()`. Calling it from a background thread asserts in Debug builds or logs an error and redirects via `EventLoop::post_task()` in Release builds.
- **Defunct Protection:** If `set_state()` is invoked on a state whose element has already been unmounted (`element_ == nullptr` or defunct), the call is a no-op and logs a warning: `"set_state() called after dispose() / unmount()"`.
- **Reentrant Build Protection:** If `set_state()` is invoked while `BuildOwner` is in the middle of executing a tree build pass, mutating state synchronously could corrupt the active build traversal. PingDart detects `build_owner_->is_building()` and automatically defers the rebuild request to a post-frame microtask (`post_microtask`).

---

# 4. Constraint Solver & Layout Model

PingDart employs a **One-Pass Constraint Solver** (identical to the box model established by Flutter):
1. **Constraints go Down:** Parent passes `BoxConstraints` to each child.
2. **Sizes go Up:** Child decides its own size (`Size`) within those constraints and returns it to parent.
3. **Parent sets Position:** Parent positions the child (`Offset`) relative to its own coordinate space.

### 4.1 BoxConstraints Rules
```cpp
struct BoxConstraints {
    float min_width{0.0f};
    float max_width{std::numeric_limits<float>::infinity()};
    float min_height{0.0f};
    float max_height{std::numeric_limits<float>::infinity()};

    [[nodiscard]] bool is_tight() const noexcept {
        return min_width == max_width && min_height == max_height;
    }

    [[nodiscard]] bool is_unbounded_width() const noexcept {
        return std::isinf(max_width);
    }

    [[nodiscard]] bool is_unbounded_height() const noexcept {
        return std::isinf(max_height);
    }

    [[nodiscard]] Size constrain(Size size) const noexcept {
        return Size{
            std::clamp(size.width, min_width, max_width),
            std::clamp(size.height, min_height, max_height)
        };
    }
};
```
- **Rule 1 (Invariant & NaN Rejection):** $0 \le min\_width \le max\_width$ and $0 \le min\_height \le max\_height$. Neither dimension may be `NaN` (`!std::isnan(...)`). Any NaN detected during constraint calculation raises an immediate fatal assertion in Debug builds, or clamps to 0.0f in Release builds.
- **Rule 2 (Tight Constraints):** $min = max$. Child has zero flexibility and must occupy exact dimensions.
- **Rule 3 (Loose Constraints):** $min = 0$. Child can choose any size up to $max$.
- **Rule 4 (Unbounded Constraints & Flex Protection):** $max = \infty$. Used inside scrollable viewports. An `Expanded` or `Flexible` widget requires bounded incoming constraints along its flex axis to allocate remaining space. Placing an `Expanded` widget inside an unbounded axis (e.g., inside an unconstrained vertical `ScrollView` or unbounded `Column`) raises a clear, actionable assertion in Debug builds:
  `"RenderFlex children have non-zero flex but incoming constraints are unbounded along the main axis"`.
- **Rule 5 (Overflow Detection & Visual Warnings):**
  - When children exceed parent bounds (e.g. Row children wider than available window width):
  - In **Debug Mode**: The render engine paints an amber-and-black hazard striped warning banner along the overflowing edge, reporting the exact overflow delta (e.g., `A RenderFlex overflowed by 32.4 pixels on the right`).
  - In **Release Mode**: The overflowing content is cleanly clipped to the parent bounds to prevent visual corruption.

---

# 5. Core Layout Primitives

PingDart Phase 1 provides standard structural layout primitives:

| Widget | Responsibility | Constraint Behavior |
|---|---|---|
| **`Row`** | Horizontal flex layout | Provides unbounded width to flexible children; enforces cross-axis height constraint. |
| **`Column`** | Vertical flex layout | Provides unbounded height to flexible children; enforces cross-axis width constraint. |
| **`Expanded` / `Flexible`** | Flex ratio sizing inside Row/Column | Distributes remaining free space proportional to `flex` factor. |
| **`Stack` & `Positioned`** | Z-axis overlapping layout | Stacks non-positioned children; anchors positioned children to edges via `top, right, bottom, left`. |
| **`Padding`** | Insets child by `EdgeInsets` | Deflates incoming constraints by padding dimensions. |
| **`Center` / `Align`** | Aligns child within available space | Takes loose constraints and positions child using `Alignment` coordinates (-1.0 to 1.0). |
| **`SizedBox`** | Fixed dimensional box | Imposes tight constraints of exact width/height on its child. |
| **`ScrollView`** | Scrollable viewport | Translates bounded screen constraint into an infinite scrollable axis. |

---

# 6. State Management & Reactive Rebuilding

Stateful UI components derive from `StatefulWidget` and instantiate a persistent `State<T>` object:

```cpp
class CounterState : public State<CounterWidget> {
    int count_ = 0;

public:
    Widget build(BuildContext& context) override {
        return Row({
            Text("Count: " + std::to_string(count_)),
            Button("+", [this]() {
                set_state([this]() {
                    count_++;
                });
            })
        });
    }
};
```

### 6.1 `setState()` Batching Architecture
- Calling `set_state(lambda)` immediately executes the mutation lambda.
- It then marks the owning `StatefulElement` as **dirty** and registers it with the global `BuildOwner`.
- The `BuildOwner` does **not** rebuild synchronously. It posts a microtask to DOC-01's microtask queue.
- On the next turn of the event loop before the frame paint phase, `BuildOwner::flush_build()` sorts dirty elements by depth (parents before children) and rebuilds them in a single optimized pass.

---

# 7. Event Propagation & Input Routing

Input events received from DOC-03 PAL are routed through hit testing and gesture recognizers:

```text
Normalized PointerDown (from DOC-03)
                 |
                 v
        RenderTree::hit_test()
   (Depth-first reverse traversal)
                 |
+----------------v--------------------------------------+
|                   HitTestResult                       |
|   [ButtonRenderObject, ContainerRenderObject, Root]   |
+-------------------------------------------------------+
                 |
                 v
   GestureArena / Event Dispatch
                 |
                 v
   ButtonWidget::on_click callback invoked
```

### 7.1 Focus Tree & Keyboard Shortcuts
- Widgets requiring keyboard interaction register a `FocusNode`.
- The `FocusManager` maintains the active `focused_node`.
- Keyboard events from DOC-03 are routed directly to the active `focused_node`.
- Directional navigation (D-Pad / TV remotes) and Tab key traversal compute the nearest geometric sibling focus target.

---

# 8. Text Layout, Typography & Accessibility Synchronization

### 8.1 Multi-Stage Text Pipeline
1. **Unicode Normalization & BiDi:** Resolves text direction (LTR / RTL) according to the Unicode Bidirectional Algorithm (UAX #9).
2. **Text Shaping (HarfBuzz):** Takes Unicode codepoints and font metrics, applies OpenType features (ligatures, kerning), and outputs shaped glyph indices and cluster positions.
3. **Line Breaking & Ellipsis:** Breaks shaped runs into visual lines based on max layout width constraints. Applies trailing ellipsis (`TextOverflow::Ellipsis`) or clipping (`TextOverflow::Clip`).
4. **Rasterization Pass:** Records `draw_text_blob()` commands into the DOC-02 `DisplayList` for GPU rendering.

### 8.2 Layout-Driven Synchronization Contracts
- **DPI Text Scaling:** When DOC-03 fires a DPI change, the root element updates font scale factors across the tree, triggering an invalidation of all text layout caches.
- **IME Composition Bounds:** As user types with an IME, `RenderEditable` computes the exact pixel bounds of the cursor and active pre-edit text run, reporting these coordinates to DOC-03 (`IPlatformHost::set_ime_cursor_area`) so the native IME candidate window anchors directly below the cursor.
- **Text Selection Geometry:** Dragging a mouse or holding Shift+Arrow routes through hit testing to update `TextSelection` offsets.
- **Semantic Tree Synchronization:** Whenever a `RenderObject` performs layout or changes position, its corresponding `SemanticsNode` bounds (`Rect`) and scroll offsets are recalculated and synchronized to the DOC-03 accessibility bridge (UI Automation on Windows, AT-SPI2 on Linux).

---

# 9. Performance & Invalidation Boundaries

To ensure 60/120 FPS responsiveness on complex applications:

### 9.1 Relayout Boundaries (`relayout_boundary`)
A `RenderObject` is designated as its own **Relayout Boundary** using the following baseline heuristic:
1. `!parent_uses_size` (the parent's layout calculation does not read or depend on this child's geometry), OR
2. `constraints.is_tight()` (the child has zero sizing flexibility along both axes), OR
3. The node is the root of the render tree.

*Empirical Validation Rule:* This formula serves as an initial working heuristic. Because complex layouts (such as baseline alignment, intrinsic sizing passes, and multi-pass flex children) can introduce subtle parent-size coupling, relayout boundary behavior must be validated against automated layout stress suites. If a parent queries `child->get_max_intrinsic_width()` or relies on multi-pass sizing, the child must **not** act as an isolated relayout boundary.

**Optimization Guarantee:** When an isolated child calls `mark_needs_layout()`, layout dirty flags propagate upward until reaching the nearest `relayout_boundary`. Sibling nodes and unaffected ancestor branches are **not** re-laid out.

### 9.2 Repaint Boundaries (`RepaintBoundary`)
- Isolates a subtree's paint output into a cached sub-display list.
- If an animated element repaints, only its own display list slice is re-recorded; static parent/sibling widgets are completely bypassed.

---

# 10. Core C++20 Header Specifications

### 10.1 Widget & Element (`pingdart/ui/widget.h`, `pingdart/ui/element.h`)

```cpp
#pragma once
#include <memory>
#include <string>
#include <vector>

namespace pingdart::ui {

class Element;
class RenderObject;
class BuildContext;

class Key {
public:
    virtual ~Key() = default;
    [[nodiscard]] virtual bool equals(const Key& other) const = 0;
};

class Widget {
public:
    virtual ~Widget() = default;

    [[nodiscard]] virtual uint64_t type_id() const noexcept = 0;
    [[nodiscard]] virtual const Key* key() const noexcept { return key_.get(); }

    [[nodiscard]] virtual std::unique_ptr<Element> create_element() const = 0;

protected:
    std::shared_ptr<Key> key_{nullptr};
};

class BuildContext {
public:
    virtual ~BuildContext() = default;
    [[nodiscard]] virtual Element& element() = 0;
};

class Element : public BuildContext {
public:
    explicit Element(const Widget& widget) : widget_(&widget) {}
    ~Element() override = default;

    virtual void mount(Element* parent);
    virtual void update(const Widget& new_widget);
    virtual void unmount();

    virtual void mark_needs_build();
    virtual void perform_rebuild() = 0;

    [[nodiscard]] Element& element() override { return *this; }
    [[nodiscard]] const Widget& widget() const noexcept { return *widget_; }
    [[nodiscard]] virtual RenderObject* render_object() const noexcept { return nullptr; }

protected:
    const Widget* widget_;
    Element* parent_{nullptr};
    bool is_dirty_{true};
};

class StatelessWidget : public Widget {
public:
    [[nodiscard]] std::unique_ptr<Element> create_element() const override;
    [[nodiscard]] virtual std::unique_ptr<Widget> build(BuildContext& context) const = 0;
};

class StatefulWidget : public Widget {
public:
    [[nodiscard]] std::unique_ptr<Element> create_element() const override;
    [[nodiscard]] virtual std::unique_ptr<class StateBase> create_state() const = 0;
};

class StateBase {
public:
    virtual ~StateBase() = default;
    virtual void init_state() {}
    virtual void dispose() {}
    [[nodiscard]] virtual std::unique_ptr<Widget> build(BuildContext& context) = 0;

    void set_state(std::function<void()> fn);

    void set_element(Element* element) { element_ = element; }

protected:
    Element* element_{nullptr};
};

} // namespace pingdart::ui
```

### 10.2 Constraints & Geometry (`pingdart/ui/geometry.h`, `pingdart/ui/box_constraints.h`)

```cpp
#pragma once
#include <algorithm>
#include <cmath>
#include <limits>

namespace pingdart::ui {

struct Size {
    float width{0.0f};
    float height{0.0f};

    bool operator==(const Size& o) const noexcept {
        return width == o.width && height == o.height;
    }
};

struct Offset {
    float dx{0.0f};
    float dy{0.0f};
};

struct Rect {
    float left{0.0f};
    float top{0.0f};
    float right{0.0f};
    float bottom{0.0f};

    [[nodiscard]] float width() const noexcept { return right - left; }
    [[nodiscard]] float height() const noexcept { return bottom - top; }
    [[nodiscard]] bool contains(float x, float y) const noexcept {
        return x >= left && x <= right && y >= top && y <= bottom;
    }
};

struct BoxConstraints {
    float min_width{0.0f};
    float max_width{std::numeric_limits<float>::infinity()};
    float min_height{0.0f};
    float max_height{std::numeric_limits<float>::infinity()};

    [[nodiscard]] static BoxConstraints tight(Size size) noexcept {
        return BoxConstraints{size.width, size.width, size.height, size.height};
    }

    [[nodiscard]] static BoxConstraints loose(Size size) noexcept {
        return BoxConstraints{0.0f, size.width, 0.0f, size.height};
    }

    [[nodiscard]] bool is_valid() const noexcept {
        return min_width >= 0.0f && max_width >= min_width &&
               min_height >= 0.0f && max_height >= min_height;
    }

    [[nodiscard]] Size constrain(Size size) const noexcept {
        return Size{
            std::clamp(size.width, min_width, max_width),
            std::clamp(size.height, min_height, max_height)
        };
    }
};

} // namespace pingdart::ui
```

### 10.3 Render Object & Paint Context (`pingdart/ui/render_object.h`)

```cpp
#pragma once
#include "geometry.h"
#include "box_constraints.h"
#include <vector>

namespace pingdart::rhi {
class DisplayListBuilder;
}

namespace pingdart::ui {

class HitTestResult {
public:
    void add(class RenderObject* target) { path_.push_back(target); }
    [[nodiscard]] const std::vector<RenderObject*>& path() const noexcept { return path_; }
private:
    std::vector<RenderObject*> path_;
};

class PaintContext {
public:
    explicit PaintContext(rhi::DisplayListBuilder& builder) : builder_(builder) {}
    [[nodiscard]] rhi::DisplayListBuilder& builder() noexcept { return builder_; }
private:
    rhi::DisplayListBuilder& builder_;
};

class RenderObject {
public:
    virtual ~RenderObject() = default;

    void layout(const BoxConstraints& constraints) {
        if (!needs_layout_ && constraints_ == constraints) return;
        constraints_ = constraints;
        perform_layout();
        needs_layout_ = false;
        mark_needs_paint();
    }

    void mark_needs_layout() {
        if (needs_layout_) return;
        needs_layout_ = true;
        if (parent_) parent_->mark_needs_layout();
    }

    void mark_needs_paint() {
        if (needs_paint_) return;
        needs_paint_ = true;
        if (parent_) parent_->mark_needs_paint();
    }

    virtual void paint(PaintContext& context, Offset offset) = 0;
    virtual bool hit_test(HitTestResult& result, Offset position) = 0;

    [[nodiscard]] Size size() const noexcept { return size_; }
    [[nodiscard]] Offset offset() const noexcept { return offset_; }
    void set_offset(Offset offset) noexcept { offset_ = offset; }

protected:
    virtual void perform_layout() = 0;

    RenderObject* parent_{nullptr};
    BoxConstraints constraints_;
    Size size_;
    Offset offset_;
    bool needs_layout_{true};
    bool needs_paint_{true};
};

} // namespace pingdart::ui
```

---

# 11. First Widget Engine Milestone Acceptance Test

The initial implementation of DOC-05 concludes with a self-contained demonstration application:

```cpp
class CounterApp : public StatefulWidget {
public:
    uint64_t type_id() const noexcept override { return 1001; }
    std::unique_ptr<StateBase> create_state() const override {
        return std::make_unique<CounterState>();
    }
};

class CounterState : public StateBase {
    int counter_ = 0;

public:
    std::unique_ptr<Widget> build(BuildContext& context) override {
        return std::make_unique<Column>(std::vector<std::unique_ptr<Widget>>{
            std::make_unique<Text>("PingDart Counter Desktop Demo"),
            std::make_unique<Text>("Value: " + std::to_string(counter_)),
            std::make_unique<Button>("Click Me", [this]() {
                set_state([this]() { counter_++; });
            })
        });
    }
};
```

### Acceptance Criteria:
1. **Subtree Rebuilding:** Clicking "Click Me" updates only the `Text("Value: ...")` node without rebuilding or re-laying out unchanged sibling/parent widgets.
2. **Deterministic Layout:** Constraint solving guarantees Column fits client bounds and centers children cleanly.
3. **Display List Generation:** UI mutations cleanly produce an immutable DOC-02 `DisplayList` submitted to Vulkan/D3D12.
4. **Cross-Platform Consistency:** The exact same application source runs identically on Windows (Win32) and Linux (Wayland & X11), handling resize, keyboard focus, and clean exit.
