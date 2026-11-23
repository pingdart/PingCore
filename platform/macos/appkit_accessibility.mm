#import "appkit_accessibility.h"

#include <algorithm>
#include <string>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

#include <pingcore/app.h>

#include "appkit_platform_view.h"

@interface PingCoreAccessibilityElement : NSAccessibilityElement {
@public
  pingcore::detail::MacAccessibility* pingcoreAccessibility;
  pingcore::SemanticNodeId pingcoreNodeId;
}
@end

namespace {

NSString* AccessibilityRole(const pingcore::SemanticNode& node) {
  using pingcore::SemanticRole;
  switch (node.role) {
  case SemanticRole::Text:
  case SemanticRole::Heading:
    return NSAccessibilityStaticTextRole;
  case SemanticRole::Image:
    return NSAccessibilityImageRole;
  case SemanticRole::Button:
    return NSAccessibilityButtonRole;
  case SemanticRole::Link:
    return NSAccessibilityLinkRole;
  case SemanticRole::Checkbox:
  case SemanticRole::Switch:
    return NSAccessibilityCheckBoxRole;
  case SemanticRole::RadioButton:
    return NSAccessibilityRadioButtonRole;
  case SemanticRole::Slider:
    return NSAccessibilitySliderRole;
  case SemanticRole::Separator:
    return NSAccessibilitySplitterRole;
  case SemanticRole::ProgressIndicator:
    return NSAccessibilityProgressIndicatorRole;
  case SemanticRole::ComboBox:
    return node.read_only.value_or(true) ? NSAccessibilityPopUpButtonRole : NSAccessibilityComboBoxRole;
  case SemanticRole::TextField:
  case SemanticRole::SearchField:
    return NSAccessibilityTextFieldRole;
  case SemanticRole::Tab:
    return NSAccessibilityRadioButtonRole;
  case SemanticRole::TabList:
    return NSAccessibilityTabGroupRole;
  case SemanticRole::Menu:
    return NSAccessibilityMenuRole;
  case SemanticRole::MenuItem:
    return NSAccessibilityMenuItemRole;
  case SemanticRole::Dialog:
    return NSAccessibilityWindowRole;
  case SemanticRole::List:
    return NSAccessibilityListRole;
  case SemanticRole::Grid:
    return NSAccessibilityGridRole;
  case SemanticRole::GridCell:
    return NSAccessibilityCellRole;
  case SemanticRole::ScrollView:
    return NSAccessibilityScrollAreaRole;
  case SemanticRole::Tree:
    return NSAccessibilityOutlineRole;
  case SemanticRole::TreeItem:
    return NSAccessibilityRowRole;
  case SemanticRole::Generic:
  case SemanticRole::Navigation:
  case SemanticRole::ListItem:
    return NSAccessibilityGroupRole;
  }
  return NSAccessibilityGroupRole;
}

NSString* StringFromUtf8(const std::string& value) {
  if (value.empty()) {
    return nil;
  }
  return [[NSString alloc] initWithBytes:value.data() length:value.size() encoding:NSUTF8StringEncoding];
}

bool SemanticLayoutChanged(const pingcore::SemanticFrame* previous, const pingcore::SemanticFrame* current) {
  if (previous == nullptr || current == nullptr) {
    return previous != current;
  }
  if (previous->root != current->root || previous->nodes.size() != current->nodes.size()) {
    return true;
  }
  std::unordered_map<pingcore::SemanticNodeId, const pingcore::SemanticNode*> previous_nodes;
  previous_nodes.reserve(previous->nodes.size());
  for (const auto& node : previous->nodes) {
    previous_nodes.emplace(node.id, &node);
  }
  return std::ranges::any_of(current->nodes, [&previous_nodes](const pingcore::SemanticNode& node) {
    const auto found = previous_nodes.find(node.id);
    const pingcore::SemanticNode* old = found == previous_nodes.end() ? nullptr : found->second;
    return old == nullptr || old->parent != node.parent || old->children != node.children || old->role != node.role ||
           old->platform_view_identity != node.platform_view_identity || old->bounds != node.bounds;
  });
}

bool SemanticValueChanged(const pingcore::SemanticNode& previous, const pingcore::SemanticNode& current) {
  return previous.value != current.value || previous.placeholder != current.placeholder ||
         previous.hint != current.hint || previous.state_description != current.state_description ||
         previous.error != current.error || previous.identifier != current.identifier ||
         previous.checked != current.checked || previous.selected != current.selected ||
         previous.expanded != current.expanded || previous.busy != current.busy ||
         previous.read_only != current.read_only || previous.required != current.required ||
         previous.invalid != current.invalid || previous.heading_level != current.heading_level ||
         previous.range != current.range || previous.collection != current.collection ||
         previous.collection_item != current.collection_item || previous.live_region != current.live_region ||
         previous.enabled != current.enabled || previous.multiline != current.multiline ||
         previous.secure != current.secure || previous.actions != current.actions ||
         previous.custom_actions != current.custom_actions;
}

NSNumber* AccessibilityCheckedValue(pingcore::SemanticCheckedState checked) {
  using pingcore::SemanticCheckedState;
  switch (checked) {
  case SemanticCheckedState::Unchecked:
    return @0;
  case SemanticCheckedState::Checked:
    return @1;
  case SemanticCheckedState::Mixed:
    return @2;
  }
  return @0;
}

} // namespace

namespace pingcore::detail {

MacAccessibility::MacAccessibility(UiWindow& ui_window, NSView* root_view, AppKitPlatformViews& platform_views) noexcept
    : ui_window_(&ui_window), root_view_(root_view), platform_views_(&platform_views) {}

MacAccessibility::~MacAccessibility() {
  for (PingCoreAccessibilityElement* element in elements_.allValues) {
    element->pingcoreAccessibility = nullptr;
  }
}

void MacAccessibility::Commit(std::shared_ptr<const SemanticFrame> frame) {
  if (frame_ == frame) {
    return;
  }
  const std::shared_ptr<const SemanticFrame> previous = frame_;
  const bool layout_changed = SemanticLayoutChanged(previous.get(), frame.get());
  frame_ = std::move(frame);
  const auto previous_indices = std::move(node_indices_);
  node_indices_.clear();
  if (frame_) {
    node_indices_.reserve(frame_->nodes.size());
    for (std::size_t index = 0; index < frame_->nodes.size(); ++index) {
      node_indices_.emplace(frame_->nodes[index].id, index);
    }
  }
  if (elements_ != nil) {
    std::unordered_set<SemanticNodeId> retained;
    if (frame_) {
      for (const SemanticNode& node : frame_->nodes) {
        retained.insert(node.id);
      }
    }
    for (NSNumber* key in elements_.allKeys) {
      if (!retained.contains(key.unsignedLongLongValue)) {
        PingCoreAccessibilityElement* element = elements_[key];
        element->pingcoreAccessibility = nullptr;
        [elements_ removeObjectForKey:key];
      }
    }
  }
  if (root_view_ == nil) {
    return;
  }
  if (layout_changed) {
    NSAccessibilityPostNotification(root_view_, NSAccessibilityLayoutChangedNotification);
    return;
  }
  if (!previous || !frame_) {
    return;
  }
  for (const SemanticNode& node : frame_->nodes) {
    const auto found = previous_indices.find(node.id);
    const SemanticNode* old = found == previous_indices.end() ? nullptr : &previous->nodes[found->second];
    if (old == nullptr ||
        (old->label == node.label && !SemanticValueChanged(*old, node) && old->focused == node.focused)) {
      continue;
    }
    id element = Element(node.id);
    if (element == nil) {
      continue;
    }
    if (old->label != node.label) {
      NSAccessibilityPostNotification(element, NSAccessibilityTitleChangedNotification);
    }
    if (SemanticValueChanged(*old, node)) {
      NSAccessibilityPostNotification(element, NSAccessibilityValueChangedNotification);
    }
    if (old->focused != node.focused) {
      NSAccessibilityPostNotification(
          node.focused ? element : root_view_,
          NSAccessibilityFocusedUIElementChangedNotification
      );
    }
  }
}

NSArray* MacAccessibility::RootChildren() {
  return frame_ ? Children(frame_->root) : @[];
}

const SemanticNode* MacAccessibility::NodeForId(SemanticNodeId id) const noexcept {
  if (!frame_) {
    return nullptr;
  }
  const auto found = node_indices_.find(id);
  return found == node_indices_.end() ? nullptr : &frame_->nodes[found->second];
}

id MacAccessibility::Element(SemanticNodeId id) {
  if (id == frame_->root) {
    return root_view_;
  }
  const SemanticNode* node = NodeForId(id);
  if (node == nullptr) {
    return nil;
  }
  if (node->platform_view_identity.has_value()) {
    NSView* platform_view =
        platform_views_ == nullptr ? nil : platform_views_->AccessibilityView(*node->platform_view_identity);
    NSObject<NSAccessibility>* platform_element = static_cast<NSObject<NSAccessibility>*>(
        platform_view == nil ? nil : NSAccessibilityUnignoredDescendant(platform_view)
    );
    if (platform_element != nil && node->parent.has_value() &&
        [platform_element respondsToSelector:@selector(setAccessibilityParent:)]) {
      [platform_element setAccessibilityParent:Element(*node->parent)];
    }
    return platform_element;
  }
  if (elements_ == nil) {
    elements_ = [[NSMutableDictionary alloc] init];
  }
  NSNumber* key = @(id);
  PingCoreAccessibilityElement* element = elements_[key];
  if (element == nil) {
    element = [[PingCoreAccessibilityElement alloc] init];
    element->pingcoreAccessibility = this;
    element->pingcoreNodeId = id;
    elements_[key] = element;
  }
  return element;
}

NSArray* MacAccessibility::Children(SemanticNodeId id) {
  const SemanticNode* node = NodeForId(id);
  if (node == nullptr) {
    return @[];
  }
  NSMutableArray* children = [[NSMutableArray alloc] initWithCapacity:node->children.size()];
  for (SemanticNodeId child : node->children) {
    if (NSObject* child_element = Element(child)) {
      [children addObject:child_element];
    }
  }
  return children;
}

NSRect MacAccessibility::Frame(SemanticNodeId id) const {
  const SemanticNode* node = NodeForId(id);
  NSView* root_view = root_view_;
  if (node == nullptr || root_view == nil || root_view.window == nil) {
    return NSZeroRect;
  }
  const Rect& bounds = node->bounds;
  const NSRect window_rect = [root_view convertRect:NSMakeRect(bounds.x, bounds.y, bounds.width, bounds.height)
                                             toView:nil];
  return [root_view.window convertRectToScreen:window_rect];
}

bool MacAccessibility::PerformAction(SemanticNodeId id, SemanticAction action) {
  return ui_window_ != nullptr && ui_window_->PerformSemanticAction(id, action);
}

} // namespace pingcore::detail

@implementation PingCoreAccessibilityElement

- (BOOL)isAccessibilityElement {
  return YES;
}

- (NSString*)accessibilityRole {
  const pingcore::SemanticNode* node =
      pingcoreAccessibility == nullptr ? nullptr : pingcoreAccessibility->NodeForId(pingcoreNodeId);
  return node == nullptr ? NSAccessibilityGroupRole : AccessibilityRole(*node);
}

- (NSString*)accessibilityLabel {
  const pingcore::SemanticNode* node =
      pingcoreAccessibility == nullptr ? nullptr : pingcoreAccessibility->NodeForId(pingcoreNodeId);
  return node == nullptr ? nil : StringFromUtf8(node->label);
}

- (id)accessibilityValue {
  const pingcore::SemanticNode* node =
      pingcoreAccessibility == nullptr ? nullptr : pingcoreAccessibility->NodeForId(pingcoreNodeId);
  if (node == nullptr) {
    return nil;
  }
  if (node->checked.has_value()) {
    return AccessibilityCheckedValue(*node->checked);
  }
  if (node->range.has_value()) {
    return @(node->range->current);
  }
  return StringFromUtf8(node->value);
}

- (NSString*)accessibilityHelp {
  const pingcore::SemanticNode* node =
      pingcoreAccessibility == nullptr ? nullptr : pingcoreAccessibility->NodeForId(pingcoreNodeId);
  return node == nullptr ? nil : StringFromUtf8(node->hint);
}

- (NSArray*)accessibilityChildren {
  return pingcoreAccessibility == nullptr ? @[] : pingcoreAccessibility->Children(pingcoreNodeId);
}

- (id)accessibilityParent {
  const pingcore::SemanticNode* node =
      pingcoreAccessibility == nullptr ? nullptr : pingcoreAccessibility->NodeForId(pingcoreNodeId);
  return node == nullptr || !node->parent.has_value() ? nil : pingcoreAccessibility->Element(*node->parent);
}

- (NSRect)accessibilityFrame {
  return pingcoreAccessibility == nullptr ? NSZeroRect : pingcoreAccessibility->Frame(pingcoreNodeId);
}

- (BOOL)isAccessibilityEnabled {
  const pingcore::SemanticNode* node =
      pingcoreAccessibility == nullptr ? nullptr : pingcoreAccessibility->NodeForId(pingcoreNodeId);
  return node != nullptr && node->enabled;
}

- (BOOL)isAccessibilitySelected {
  const pingcore::SemanticNode* node =
      pingcoreAccessibility == nullptr ? nullptr : pingcoreAccessibility->NodeForId(pingcoreNodeId);
  return node != nullptr && node->selected.value_or(false);
}

- (BOOL)isAccessibilityFocused {
  const pingcore::SemanticNode* node =
      pingcoreAccessibility == nullptr ? nullptr : pingcoreAccessibility->NodeForId(pingcoreNodeId);
  return node != nullptr && node->focused;
}

- (void)setAccessibilitySelected:(BOOL)selected {
  const auto* node = pingcoreAccessibility == nullptr ? nullptr : pingcoreAccessibility->NodeForId(pingcoreNodeId);
  if (node == nullptr) {
    return;
  }
  if ((node->actions & pingcore::SemanticActionMask(pingcore::SemanticActionKind::SetSelected)) != 0) {
    pingcoreAccessibility->PerformAction(pingcoreNodeId,
                                        {pingcore::SemanticActionKind::SetSelected, selected != NO});
  } else if (selected != NO &&
             (node->actions & pingcore::SemanticActionMask(pingcore::SemanticActionKind::Activate)) != 0) {
    pingcoreAccessibility->PerformAction(pingcoreNodeId,
                                        {pingcore::SemanticActionKind::Activate, std::monostate{}});
  }
}

- (BOOL)isAccessibilityDisclosed {
  const auto* node = pingcoreAccessibility == nullptr ? nullptr : pingcoreAccessibility->NodeForId(pingcoreNodeId);
  return node != nullptr && node->expanded.value_or(false);
}

- (void)setAccessibilityDisclosed:(BOOL)disclosed {
  if (pingcoreAccessibility != nullptr) {
    pingcoreAccessibility->PerformAction(pingcoreNodeId,
        {disclosed ? pingcore::SemanticActionKind::Expand : pingcore::SemanticActionKind::Collapse, std::monostate{}});
  }
}

- (NSInteger)accessibilityDisclosureLevel {
  const auto* node = pingcoreAccessibility == nullptr ? nullptr : pingcoreAccessibility->NodeForId(pingcoreNodeId);
  NSInteger level = 0;
  while (node != nullptr && node->parent) {
    node = pingcoreAccessibility->NodeForId(*node->parent);
    if (node == nullptr || node->role != pingcore::SemanticRole::TreeItem) {
      break;
    }
    ++level;
  }
  return level;
}

- (id)accessibilityDisclosedByRow {
  const auto* node = pingcoreAccessibility == nullptr ? nullptr : pingcoreAccessibility->NodeForId(pingcoreNodeId);
  const auto* parent = node != nullptr && node->parent ? pingcoreAccessibility->NodeForId(*node->parent) : nullptr;
  return parent != nullptr && parent->role == pingcore::SemanticRole::TreeItem
             ? pingcoreAccessibility->Element(parent->id) : nil;
}

- (NSArray*)accessibilityDisclosedRows {
  const auto* node = pingcoreAccessibility == nullptr ? nullptr : pingcoreAccessibility->NodeForId(pingcoreNodeId);
  NSMutableArray* rows = [NSMutableArray array];
  if (node != nullptr) {
    for (const auto child_id : node->children) {
      const auto* child = pingcoreAccessibility->NodeForId(child_id);
      if (child != nullptr && child->role == pingcore::SemanticRole::TreeItem) {
        id element = pingcoreAccessibility->Element(child->id);
        if (element != nil) {
          [rows addObject:element];
        }
      }
    }
  }
  return rows;
}

- (void)setAccessibilityFocused:(BOOL)focused {
  if (focused && pingcoreAccessibility != nullptr) {
    pingcoreAccessibility->PerformAction(pingcoreNodeId, {pingcore::SemanticActionKind::Focus, std::monostate{}});
  }
}

- (NSArray*)accessibilityRows {
  const auto* node = pingcoreAccessibility == nullptr ? nullptr : pingcoreAccessibility->NodeForId(pingcoreNodeId);
  NSMutableArray* rows = [NSMutableArray array];
  if (node == nullptr || node->role != pingcore::SemanticRole::Tree) {
    return rows;
  }
  std::vector<pingcore::SemanticNodeId> pending(node->children.rbegin(), node->children.rend());
  while (!pending.empty()) {
    const auto* row = pingcoreAccessibility->NodeForId(pending.back());
    pending.pop_back();
    if (row == nullptr || row->role != pingcore::SemanticRole::TreeItem) {
      continue;
    }
    id element = pingcoreAccessibility->Element(row->id);
    if (element != nil) {
      [rows addObject:element];
    }
    pending.insert(pending.end(), row->children.rbegin(), row->children.rend());
  }
  return rows;
}

- (NSArray*)accessibilitySelectedRows {
  NSMutableArray* selected = [NSMutableArray array];
  for (PingCoreAccessibilityElement* row in [self accessibilityRows]) {
    if ([row isAccessibilitySelected]) {
      [selected addObject:row];
    }
  }
  return selected;
}

- (void)setAccessibilitySelectedRows:(NSArray*)rows {
  if (rows.count > 1) {
    return;
  }
  if (rows.count == 1) {
    id row = rows.firstObject;
    if ([[self accessibilityRows] containsObject:row]) {
      [row setAccessibilitySelected:YES];
    }
  } else {
    for (PingCoreAccessibilityElement* row in [self accessibilitySelectedRows]) {
      [row setAccessibilitySelected:NO];
    }
  }
}

- (NSArray<NSAccessibilityActionName>*)accessibilityActionNames {
  const pingcore::SemanticNode* node =
      pingcoreAccessibility == nullptr ? nullptr : pingcoreAccessibility->NodeForId(pingcoreNodeId);
  if (node == nullptr) {
    return @[];
  }
  NSMutableArray<NSAccessibilityActionName>* actions = [[NSMutableArray alloc] init];
  if ((node->actions & pingcore::SemanticActionMask(pingcore::SemanticActionKind::Activate)) != 0) {
    [actions addObject:NSAccessibilityPressAction];
  }
  if ((node->actions & pingcore::SemanticActionMask(pingcore::SemanticActionKind::Increment)) != 0) {
    [actions addObject:NSAccessibilityIncrementAction];
  }
  if ((node->actions & pingcore::SemanticActionMask(pingcore::SemanticActionKind::Decrement)) != 0) {
    [actions addObject:NSAccessibilityDecrementAction];
  }
  return actions;
}

- (void)accessibilityPerformAction:(NSAccessibilityActionName)action {
  if (pingcoreAccessibility == nullptr) {
    return;
  }
  if ([action isEqualToString:NSAccessibilityPressAction]) {
    pingcoreAccessibility->PerformAction(pingcoreNodeId, {pingcore::SemanticActionKind::Activate, std::monostate{}});
  } else if ([action isEqualToString:NSAccessibilityIncrementAction]) {
    pingcoreAccessibility->PerformAction(pingcoreNodeId, {pingcore::SemanticActionKind::Increment, std::monostate{}});
  } else if ([action isEqualToString:NSAccessibilityDecrementAction]) {
    pingcoreAccessibility->PerformAction(pingcoreNodeId, {pingcore::SemanticActionKind::Decrement, std::monostate{}});
  }
}

@end
