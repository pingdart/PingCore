# PingDartUI Complete 70-Component Production Catalog

**Framework**: PingDartUI (C++20, Qt 6.8 LTS, PingCore Unified Runtime)  
**Package**: PKG-137  
**Status**: Alpha Reference Specification  

---

## 1. Catalog Summary (70 Components)

| Category | Component Count | Components |
| :--- | :--- | :--- |
| **1. Layout** | 7 | `Container`, `Row`, `Column`, `Stack`, `Flex`, `Grid`, `ScrollView` |
| **2. Basic** | 6 | `Text`, `Heading`, `Image`, `Icon`, `Divider`, `Spacer` |
| **3. Actions & Buttons** | 4 | `Button`, `IconButton`, `FloatingButton`, `ButtonGroup` |
| **4. Forms & Input** | 7 | `Input`, `TextArea`, `Checkbox`, `Radio`, `Switch`, `Select`, `Slider` |
| **5. Date & Time** | 3 | `DatePicker`, `TimePicker`, `Calendar` |
| **6. Navigation** | 6 | `Navbar`, `Sidebar`, `Tabs`, `Drawer`, `Breadcrumb`, `BottomNavigation` |
| **7. Display & Data** | 7 | `Card`, `Avatar`, `Badge`, `Table`, `DataGrid`, `List`, `Tree` |
| **8. Feedback & Progress** | 6 | `Alert`, `Toast`, `Snackbar`, `ProgressBar`, `Spinner`, `Skeleton` |
| **9. Overlays & Dialogs** | 5 | `Dialog`, `Modal`, `Popover`, `Tooltip`, `BottomSheet` |
| **10. Application Shell** | 5 | `AppShell`, `Page`, `Outlet`, `SafeArea`, `StatCard` |
| **11. Media & Advanced** | 7 | `VideoPlayer`, `AudioPlayer`, `ImageGallery`, `Charts`, `RichTextEditor`, `VirtualList`, `FilePicker` |
| **12. Compound Workflows**| 7 | `Form`, `FormField`, `SearchBox`, `Pagination`, `Accordion`, `Carousel`, `CommandPalette` |
| **Total** | **70** | **Complete Unified Catalog** |

---

## 2. Component Reference & Developer API

### 2.1 Layout Components (7)
- `Container({ children })`: Primary column-oriented layout container.
- `Row({ children })`: Horizontal flex layout with `alignItems(AlignItems::Center)`.
- `Column({ children })`: Vertical flex layout.
- `Stack({ children })`: Layered container with `position("relative")` for z-index overlays.
- `Flex({ children })`: Generic flex container for custom flex properties.
- `Grid(cols, gap, { children })`: CSS Grid container with column template and gap.
- `ScrollView({ children })`: Scrollable viewport with `overflowY("auto")` and max height clipping.

### 2.2 Basic Components (6)
- `Text(content)` / `Text(reactive_fn)`: Semantic text span.
- `Heading(content, level)`: Semantic heading (h1–h6) with proportional typographic hierarchy.
- `Image(src, alt)`: Image tag with safe URL sanitization.
- `Icon(name)`: Accessible icon with `data-icon` attribute.
- `Divider()`: Horizontal 1px border line separator.
- `Spacer(size = 16)`: Flexible spacing box.

### 2.3 Actions & Buttons (4)
- `Button(label)`: Standard interactive trigger button with variants (`primary`, `secondary`, `destructive`).
- `IconButton(icon_name, label)`: Button containing icon and accessible `aria-label`.
- `FloatingButton(icon_name, label)`: Floating circular action button with elevated shadow.
- `ButtonGroup({ buttons })`: Horizontally merged button group.

### 2.4 Forms & Input Controls (7)
- `Input(placeholder, value)`: Text entry field.
- `TextArea(placeholder, rows)`: Multi-line text field.
- `Checkbox(label, is_checked)`: Accessible checkbox with label binding.
- `Radio(name, value, label, is_checked)`: Grouped radio button with label.
- `Switch(is_checked)`: Accessible toggle switch with thumb knob slider.
- `Select({ Option(...) })`: Dropdown select menu.
- `Slider(min, max, value)`: Numeric range slider.

### 2.5 Date & Time Components (3)
- `DatePicker(date)`: Form date selector.
- `TimePicker(time)`: Form time selector.
- `Calendar(year, month)`: Monthly date grid with day headers and interactive date cells.

### 2.6 Navigation Components (6)
- `Navbar({ children })`: Top application header bar with branding and links.
- `Sidebar({ children })`: Side navigation drawer bar.
- `Tabs({ Tab(title, content) })`: Accessible tabbed navigation interface.
- `Drawer({ children }, is_open)`: Slide-out side drawer with backdrop.
- `Breadcrumb({ segments })`: Breadcrumb trail with slash separators.
- `BottomNavigation({ items })`: Mobile navigation bottom bar.

### 2.7 Display & Data Components (7)
- `Card({ children })`: Elevated surface container with border, radius, shadow, and padding.
- `Avatar(src_or_initials, is_image, size)`: User avatar with image or initials fallback.
- `Badge(label)`: Small status indicator pill.
- `Table({ TableRow({ TableCell(...) }) })`: Semantic HTML/SceneGraph table.
- `DataGrid()`: High-level data grid with sortable columns, search input, and pagination.
- `List({ ListItem(...) })`: Ordered or unordered list container.
- `Tree({ TreeNode(...) })`: Hierarchical tree view.

### 2.8 Feedback & Progress (6)
- `Alert(message)`: Notice and warning callout box.
- `Toast(message, type)`: Floating notification toast.
- `Snackbar(message, action_label)`: Bottom feedback snackbar.
- `ProgressBar(percent)`: Horizontal visual progress bar.
- `Spinner(size)`: Circular loading indicator with CSS/SceneGraph animation.
- `Skeleton(w, h)`: Shimmer placeholder box for asynchronous loading.

### 2.9 Overlays & Dialogs (5)
- `Dialog(title, content, is_open)`: Modal dialog with backdrop and central card.
- `Modal(title, content, is_open)`: Generic modal container.
- `Popover(trigger, content)`: Floating popover anchored to trigger button.
- `Tooltip(tip_text, child)`: Hover tooltip wrapper.
- `BottomSheet({ children }, is_open)`: Bottom slide-up sheet drawer.

### 2.10 Application Shell & Views (5)
- `AppShell({ children })`: Master application responsive frame.
- `Page({ children })`: Primary content page container.
- `Outlet()`: Routing view container anchor.
- `SafeArea({ children })`: Device notch/home indicator safe-area wrapper.
- `StatCard(title, value, subtitle)`: Standard metric KPI card.

### 2.11 Media & Advanced Components (7)
- `VideoPlayer(src, poster)`: Video playback surface.
- `AudioPlayer(src)`: Audio playback surface.
- `ImageGallery({ urls })`: Responsive image grid gallery.
- `Charts(type, labels, values)`: Bar and sparkline data visualizer.
- `RichTextEditor(placeholder)`: Rich text editor toolbar and surface.
- `VirtualList(count, item_height)`: Virtualized scrolling list.
- `FilePicker(accept)`: Accessible file upload control.

### 2.12 Compound Workflows (7)
- `Form({ fields }).onSubmit(callback)`: Form container with `FormData` submission handling.
- `FormField(label, input, error_msg)`: Labeled input field with validation error text.
- `SearchBox(placeholder)`: Integrated search input with magnifying icon and clear action.
- `Pagination(total_pages, current_page)`: Multi-page navigation control.
- `Accordion({ sections })`: Collapsible disclosure panel list.
- `Carousel({ slides })`: Carousel slide viewer with previous/next controls.
- `CommandPalette(placeholder)`: Command palette modal with search and command list.

---

## 3. High-Level Developer Example

```cpp
#include <pingdart/ui.h>

using namespace pingdart::ui;

Component DoctorDashboard() {
    std::vector<std::unordered_map<std::string, std::string>> doctors = {
        {{"name", "Dr. Jane Smith"}, {"dept", "Cardiology"}, {"status", "Available"}},
        {{"name", "Dr. Raj Kumar"}, {"dept", "Neurology"}, {"status", "Busy"}}
    };

    return AppShell({
        Sidebar({
            Link("Dashboard", "/dashboard"),
            Link("Doctors", "/doctors"),
            Link("Appointments", "/appointments")
        }),
        Page({
            Heading("Doctor Overview", 1),
            Row({
                StatCard("Total Doctors", 125, "+12% this month"),
                StatCard("Appointments", 248, "98% on-time"),
                StatCard("Patients", 1250, "+54 today")
            }).gap(16),
            DataGrid()
                .columns({
                    ColumnDef("name", "Doctor Name"),
                    ColumnDef("dept", "Department"),
                    ColumnDef("status", "Status")
                })
                .data(doctors)
                .searchable(true)
                .sortable(true)
                .pagination(10)
        }).gap(24)
    });
}
```
