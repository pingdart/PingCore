#pragma once

#include "box_constraints.h"
#include <pingdart/rhi/display_list.h>
#include <vector>
#include <functional>
#include <memory>

namespace pingdart::ui {

class RenderObject;

struct HitTestEntry {
    RenderObject* target{nullptr};
    Offset local_position{0.0f, 0.0f};
};

class HitTestResult {
public:
    void add(HitTestEntry entry) {
        path_.push_back(entry);
    }

    [[nodiscard]] const std::vector<HitTestEntry>& path() const noexcept {
        return path_;
    }

    [[nodiscard]] bool empty() const noexcept {
        return path_.empty();
    }

    void clear() noexcept {
        path_.clear();
    }

private:
    std::vector<HitTestEntry> path_;
};

class PaintContext {
public:
    explicit PaintContext(pingdart::rhi::DisplayListBuilder& builder) : builder_(builder) {}

    [[nodiscard]] pingdart::rhi::DisplayListBuilder& builder() noexcept {
        return builder_;
    }

    void paint_child(RenderObject& child, Offset offset);

private:
    pingdart::rhi::DisplayListBuilder& builder_;
};

class RenderObject {
public:
    virtual ~RenderObject();

    [[nodiscard]] RenderObject* parent() const noexcept { return parent_; }
    void set_parent(RenderObject* p) noexcept { parent_ = p; }

    [[nodiscard]] bool needs_layout() const noexcept { return needs_layout_; }
    [[nodiscard]] bool needs_paint() const noexcept { return needs_paint_; }

    virtual void mark_needs_layout();
    virtual void mark_needs_paint();

    virtual void adopt_child(RenderObject* child);
    virtual void drop_child(RenderObject* child);

    virtual void paint(PaintContext& context, Offset offset) = 0;
    virtual bool hit_test(HitTestResult& result, Offset position) = 0;
    virtual void visit_children(const std::function<void(RenderObject&)>& visitor) = 0;

protected:
    RenderObject* parent_{nullptr};
    bool needs_layout_{true};
    bool needs_paint_{true};
    bool attached_{false};
};

class RenderBox : public RenderObject {
public:
    ~RenderBox() override = default;

    [[nodiscard]] const BoxConstraints& constraints() const noexcept { return constraints_; }
    [[nodiscard]] const Size& size() const noexcept { return size_; }
    void set_size(Size s) noexcept { size_ = s; }

    [[nodiscard]] const Offset& offset() const noexcept { return offset_; }
    void set_offset(Offset o) noexcept { offset_ = o; }

    void layout(const BoxConstraints& constraints, bool parent_uses_size = false);
    virtual void perform_layout() = 0;

    [[nodiscard]] virtual Size compute_dry_layout(const BoxConstraints& constraints) const;

    bool hit_test(HitTestResult& result, Offset position) override;
    virtual bool hit_test_children(HitTestResult& result, Offset position);
    virtual bool hit_test_self(Offset position);

    [[nodiscard]] Rect paint_bounds() const noexcept {
        return Rect::from_xywh(0.0f, 0.0f, size_.width, size_.height);
    }

    [[nodiscard]] Rect global_bounds(Offset parent_offset = Offset{0.0f, 0.0f}) const noexcept {
        return Rect::from_xywh(parent_offset.dx + offset_.dx,
                              parent_offset.dy + offset_.dy,
                              size_.width,
                              size_.height);
    }

    void visit_children(const std::function<void(RenderObject&)>& visitor) override {
        (void)visitor;
    }

protected:
    BoxConstraints constraints_{};
    Size size_{0.0f, 0.0f};
    Offset offset_{0.0f, 0.0f};
    bool parent_uses_size_{false};
};

} // namespace pingdart::ui
