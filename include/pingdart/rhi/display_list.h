#pragma once

#include "geometry.h"
#include "paint.h"
#include <pingdart/core/ref_ptr.h>
#include <string>
#include <vector>
#include <variant>
#include <memory>
#include <cstdint>

namespace pingdart::rhi {

class ITexture;

struct DamageRegion {
    Rect dirty_rect{};

    constexpr DamageRegion() noexcept = default;
    constexpr explicit DamageRegion(const Rect& r) noexcept : dirty_rect(r) {}

    void union_with(const Rect& r) noexcept {
        dirty_rect = dirty_rect.union_with(r);
    }

    [[nodiscard]] constexpr bool is_empty() const noexcept {
        return dirty_rect.is_empty();
    }

    [[nodiscard]] constexpr bool intersects(const Rect& r) const noexcept {
        return dirty_rect.intersects(r);
    }

    [[nodiscard]] constexpr const Rect& bounds() const noexcept {
        return dirty_rect;
    }
};

struct TextBlob {
    std::string text;
    std::string font_family{"sans-serif"};
    float font_size{14.0f};
    bool bold{false};
    bool italic{false};

    bool operator==(const TextBlob&) const noexcept = default;
};

enum class DisplayListOpType : uint8_t {
    Save,
    Restore,
    PushClipRect,
    PushClipRRect,
    PopClip,
    PushTransform,
    PopTransform,
    PushOpacity,
    PopOpacity,
    DrawRect,
    DrawRRect,
    DrawCircle,
    DrawLine,
    DrawTextBlob,
    DrawImage,
    DrawShadow
};

struct OpSave {};
struct OpRestore {};

struct OpPushClipRect {
    Rect rect;
    ClipOp op;
};

struct OpPushClipRRect {
    RRect rrect;
    ClipOp op;
};

struct OpPopClip {};

struct OpPushTransform {
    Matrix2D matrix;
};

struct OpPopTransform {};

struct OpPushOpacity {
    float opacity;
};

struct OpPopOpacity {};

struct OpDrawRect {
    Rect rect;
    Paint paint;
};

struct OpDrawRRect {
    RRect rrect;
    Paint paint;
};

struct OpDrawCircle {
    Point center;
    float radius;
    Paint paint;
};

struct OpDrawLine {
    Point p0;
    Point p1;
    Paint paint;
};

struct OpDrawTextBlob {
    TextBlob blob;
    Point origin;
    Paint paint;
};

struct OpDrawImage {
    ITexture* texture;
    Rect src;
    Rect dst;
    Paint paint;
};

struct OpDrawShadow {
    RRect rrect;
    Color color;
    float elevation;
    float spread;
};

using DisplayListOpData = std::variant<
    OpSave,
    OpRestore,
    OpPushClipRect,
    OpPushClipRRect,
    OpPopClip,
    OpPushTransform,
    OpPopTransform,
    OpPushOpacity,
    OpPopOpacity,
    OpDrawRect,
    OpDrawRRect,
    OpDrawCircle,
    OpDrawLine,
    OpDrawTextBlob,
    OpDrawImage,
    OpDrawShadow
>;

struct DisplayListOp {
    DisplayListOpType type;
    DisplayListOpData data;
    Rect transformed_bounds{}; // Computed bounding box in canvas space
};

class IDisplayListReceiver {
public:
    virtual ~IDisplayListReceiver() = default;

    virtual void on_save() = 0;
    virtual void on_restore() = 0;
    virtual void on_push_clip_rect(const Rect& rect, ClipOp op) = 0;
    virtual void on_push_clip_rrect(const RRect& rrect, ClipOp op) = 0;
    virtual void on_pop_clip() = 0;
    virtual void on_push_transform(const Matrix2D& matrix) = 0;
    virtual void on_pop_transform() = 0;
    virtual void on_push_opacity(float opacity) = 0;
    virtual void on_pop_opacity() = 0;
    virtual void on_draw_rect(const Rect& rect, const Paint& paint) = 0;
    virtual void on_draw_rrect(const RRect& rrect, const Paint& paint) = 0;
    virtual void on_draw_circle(Point center, float radius, const Paint& paint) = 0;
    virtual void on_draw_line(Point p0, Point p1, const Paint& paint) = 0;
    virtual void on_draw_text_blob(const TextBlob& blob, Point origin, const Paint& paint) = 0;
    virtual void on_draw_image(ITexture* texture, const Rect& src, const Rect& dst, const Paint& paint) = 0;
    virtual void on_draw_shadow(const RRect& rrect, Color color, float elevation, float spread) = 0;
};

class DisplayList final : public core::RefCounted<DisplayList> {
public:
    DisplayList(std::vector<DisplayListOp> ops, DamageRegion bounds);
    ~DisplayList() override = default;

    DisplayList(const DisplayList&) = delete;
    DisplayList& operator=(const DisplayList&) = delete;

    [[nodiscard]] const std::vector<DisplayListOp>& ops() const noexcept { return ops_; }
    [[nodiscard]] size_t op_count() const noexcept { return ops_.size(); }
    [[nodiscard]] const DamageRegion& bounds() const noexcept { return bounds_; }

    // Replays commands. If damage is provided, skips commands outside the damaged area
    void replay(IDisplayListReceiver& receiver, const DamageRegion* damage = nullptr) const;

private:
    std::vector<DisplayListOp> ops_;
    DamageRegion bounds_;
};

class DisplayListBuilder {
public:
    DisplayListBuilder();
    ~DisplayListBuilder() = default;

    void save();
    void restore();

    void push_clip_rect(const Rect& rect, ClipOp op = ClipOp::Intersect);
    void push_clip_rrect(const RRect& rrect, ClipOp op = ClipOp::Intersect);
    void pop_clip();

    void push_transform(const Matrix2D& matrix);
    void pop_transform();
    void translate(float dx, float dy);
    void scale(float sx, float sy);
    void rotate(float radians);

    void push_opacity(float opacity);
    void pop_opacity();

    void draw_rect(const Rect& rect, const Paint& paint);
    void draw_rounded_rect(const RRect& rrect, const Paint& paint);
    void draw_circle(Point center, float radius, const Paint& paint);
    void draw_line(Point p0, Point p1, const Paint& paint);
    void draw_text_blob(TextBlob blob, Point origin, const Paint& paint);
    void draw_image(ITexture* texture, const Rect& src, const Rect& dst, const Paint& paint);
    void draw_shadow(const RRect& rrect, Color color, float elevation, float spread);

    [[nodiscard]] core::RefPtr<DisplayList> finish_recording();

    [[nodiscard]] size_t op_count() const noexcept { return ops_.size(); }
    [[nodiscard]] const DamageRegion& accumulated_bounds() const noexcept { return accumulated_bounds_; }

private:
    struct State {
        Matrix2D transform{Matrix2D::identity()};
        Rect clip{Rect::from_ltrb(-1e6f, -1e6f, 1e6f, 1e6f)};
        float opacity{1.0f};
    };

    void append_op(DisplayListOpType type, DisplayListOpData data, const Rect& local_bounds);

    std::vector<DisplayListOp> ops_;
    std::vector<State> state_stack_;
    DamageRegion accumulated_bounds_;
    int32_t save_count_{0};
};

} // namespace pingdart::rhi
