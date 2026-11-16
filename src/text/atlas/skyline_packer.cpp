#include <pingdart/text/glyph_atlas.h>
#include <algorithm>
#include <limits>

namespace pingdart::text {

SkylinePacker::SkylinePacker(uint32_t width, uint32_t height)
    : width_(width), height_(height) {
    clear();
}

void SkylinePacker::clear() {
    nodes_.clear();
    nodes_.push_back(Node{0, 0, width_});
    used_surface_ = 0;
}

float SkylinePacker::occupancy() const noexcept {
    uint64_t total = static_cast<uint64_t>(width_) * static_cast<uint64_t>(height_);
    return total > 0 ? static_cast<float>(used_surface_) / static_cast<float>(total) : 0.0f;
}

bool SkylinePacker::allocate(uint32_t w, uint32_t h, uint32_t& out_x, uint32_t& out_y) {
    if (w == 0 || h == 0) {
        out_x = 0;
        out_y = 0;
        return true;
    }

    // Add 1px padding between glyphs to prevent bilinear texture bleeding
    uint32_t pad_w = w + 1;
    uint32_t pad_h = h + 1;

    if (pad_w > width_ || pad_h > height_) {
        return false;
    }

    int best_index = -1;
    uint32_t best_y = std::numeric_limits<uint32_t>::max();
    uint32_t best_width = std::numeric_limits<uint32_t>::max();

    for (size_t i = 0; i < nodes_.size(); ++i) {
        uint32_t x = nodes_[i].x;
        if (x + pad_w > width_) break;

        uint32_t y = nodes_[i].y;
        size_t j = i;
        uint32_t accumulated_w = 0;

        while (j < nodes_.size() && accumulated_w < pad_w) {
            y = std::max(y, nodes_[j].y);
            accumulated_w += nodes_[j].width;
            j++;
        }

        if (accumulated_w >= pad_w && (y + pad_h <= height_)) {
            if (y < best_y || (y == best_y && nodes_[i].width < best_width)) {
                best_y = y;
                best_width = nodes_[i].width;
                best_index = static_cast<int>(i);
            }
        }
    }

    if (best_index == -1) {
        return false;
    }

    out_x = nodes_[static_cast<size_t>(best_index)].x;
    out_y = best_y;

    Node new_node{out_x, best_y + pad_h, pad_w};

    size_t insert_index = static_cast<size_t>(best_index);
    nodes_.insert(nodes_.begin() + static_cast<std::ptrdiff_t>(insert_index), new_node);

    for (size_t i = insert_index + 1; i < nodes_.size();) {
        auto prev_x = nodes_[i - 1].x;
        auto prev_w = nodes_[i - 1].width;
        if (nodes_[i].x < prev_x + prev_w) {
            uint32_t shrink = (prev_x + prev_w) - nodes_[i].x;
            if (nodes_[i].width <= shrink) {
                nodes_.erase(nodes_.begin() + static_cast<std::ptrdiff_t>(i));
                continue;
            } else {
                nodes_[i].x += shrink;
                nodes_[i].width -= shrink;
                break;
            }
        }
        break;
    }

    // Merge adjacent nodes with identical Y
    for (size_t i = 0; i + 1 < nodes_.size();) {
        if (nodes_[i].y == nodes_[i + 1].y) {
            nodes_[i].width += nodes_[i + 1].width;
            nodes_.erase(nodes_.begin() + static_cast<std::ptrdiff_t>(i + 1));
        } else {
            i++;
        }
    }

    used_surface_ += static_cast<uint64_t>(pad_w) * static_cast<uint64_t>(pad_h);
    return true;
}

} // namespace pingdart::text
