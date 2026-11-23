#include "ios_file_internal.h"

#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#import <dispatch/dispatch.h>

#include <algorithm>
#include <atomic>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include <pingcore/app.h>
#include <pingcore/file_drop.h>

#include "uikit_view.h"
#include "macros_internal.h"

namespace pingcore::detail {
namespace {

NSString* FileRepresentationType(NSItemProvider* provider) {
  if ([provider hasItemConformingToTypeIdentifier:UTTypeFolder.identifier]) {
    return nil;
  }
  for (NSString* identifier in provider.registeredTypeIdentifiers) {
    UTType* type = [UTType typeWithIdentifier:identifier];
    if ([type conformsToType:UTTypeData] && ![type conformsToType:UTTypeURL]) {
      return identifier;
    }
  }
  return nil;
}

FileDropOffer IosDropOffer(id<UIDropSession> session) {
  FileDropOffer offer{.item_count = session.items.count};
  for (UIDragItem* item in session.items) {
    NSString* identifier = FileRepresentationType(item.itemProvider);
    NSString* mime = identifier == nil ? nil : [UTType typeWithIdentifier:identifier].preferredMIMEType;
    if (mime.UTF8String != nullptr) {
      const std::string value(mime.UTF8String);
      if (std::ranges::find(offer.content_types, value) == offer.content_types.end()) {
        offer.content_types.push_back(value);
      }
    }
  }
  return offer;
}

class IosDropPreparation final : public std::enable_shared_from_this<IosDropPreparation> {
public:
  IosDropPreparation(NSArray<UIDragItem*>* items, FileDropCompletion completion)
      : items_(items), completion_(std::move(completion)) {}

  void Next() {
    if (canceled_) {
      return;
    }
    if (files_.size() == items_.count) {
      auto completion = std::move(completion_);
      completion(IoResult<std::vector<FileReference>>(std::move(files_)));
      return;
    }
    NSItemProvider* provider = items_[files_.size()].itemProvider;
    NSString* type = FileRepresentationType(provider);
    if (type == nil) {
      Fail({IoErrorCode::Unsupported, "PingCore dropped item has no ordinary file representation"});
      return;
    }
    const std::weak_ptr<IosDropPreparation> weak = shared_from_this();
    progress_ = [provider loadInPlaceFileRepresentationForTypeIdentifier:type
                                                    completionHandler:^(NSURL* url, BOOL in_place, NSError* error) {
      const auto self = weak.lock();
      if (!self || self->canceled_) {
        return;
      }
      auto result = std::make_shared<IoResult<FileReference>>(
          IoError{IoErrorCode::Io, "PingCore could not retain the iOS dropped file"});
      if (error == nil && url != nil && url.isFileURL) {
        try {
          // Temporary provider URLs must be secured while this callback still owns their readable lifetime.
          *result =
              IoResult<FileReference>(in_place ? MakeIosFileReference(url, false) : MakeCopiedIosFileReference(url));
        } catch (...) {
        }
      }
      dispatch_async(dispatch_get_main_queue(), ^{
        const auto active = weak.lock();
        if (!active || active->canceled_) {
          return;
        }
        active->progress_ = nil;
        if (!result->Succeeded()) {
          active->Fail(result->Error());
        } else {
          try {
            active->files_.push_back(std::move(*result).Value());
            active->Next();
          } catch (...) {
            active->Fail({IoErrorCode::Io, "PingCore could not prepare the next iOS dropped file"});
          }
        }
      });
    }];
  }

  void Cancel() {
    canceled_ = true;
    [progress_ cancel];
    progress_ = nil;
    completion_ = {};
    files_.clear();
  }

private:
  void Fail(IoError error) {
    auto completion = std::move(completion_);
    files_.clear();
    if (completion) {
      completion(IoResult<std::vector<FileReference>>(std::move(error)));
    }
  }

  __strong NSArray<UIDragItem*>* items_;
  __strong NSProgress* progress_ = nil;
  FileDropCompletion completion_;
  std::vector<FileReference> files_;
  std::atomic<bool> canceled_ = false;
};

FileDropPreparation CaptureIosFileDrop(id<UIDropSession> session) {
  __strong NSArray<UIDragItem*>* items = [session.items copy];
  return {[items](FileDropCompletion completion) {
    auto preparation = std::make_shared<IosDropPreparation>(items, std::move(completion));
    preparation->Next();
    return [preparation] { preparation->Cancel(); };
  }};
}

} // namespace
} // namespace pingcore::detail

@interface PingCoreFileDropDelegate : NSObject <UIDropInteractionDelegate>
- (instancetype)initWithView:(PingCoreView*)view;
@end

@implementation PingCoreFileDropDelegate {
  __weak PingCoreView* _view;
  __strong id<UIDropSession> _hover;
  std::uint64_t _session;
}

- (instancetype)initWithView:(PingCoreView*)view {
  self = [super init];
  if (self != nil) {
    _view = view;
  }
  return self;
}

- (BOOL)dropInteraction:(UIDropInteraction*)interaction canHandleSession:(id<UIDropSession>)session {
  if (session.items.count == 0) {
    return NO;
  }
  for (UIDragItem* item in session.items) {
    if (pingcore::detail::FileRepresentationType(item.itemProvider) == nil) {
      return NO;
    }
  }
  return YES;
}

- (void)dropInteraction:(UIDropInteraction*)interaction sessionDidEnter:(id<UIDropSession>)session {
  if (_hover != nil) {
    [self dropInteraction:interaction sessionDidExit:_hover];
  }
  PingCoreView* view = _view;
  if (view == nil || view->pingcoreWindow == nullptr) {
    return;
  }
  _hover = session;
  ++_session;
  const CGPoint point = [session locationInView:view];
  try {
    PINGCORE_DISCARD(view->pingcoreWindow->HandleFileDragEntered(
        _session, pingcore::detail::IosDropOffer(session), {static_cast<float>(point.x), static_cast<float>(point.y)}
    ));
  } catch (...) {
    [self dropInteraction:interaction sessionDidExit:session];
  }
}

- (UIDropProposal*)dropInteraction:(UIDropInteraction*)interaction sessionDidUpdate:(id<UIDropSession>)session {
  PingCoreView* view = _view;
  bool accepted = false;
  if (view != nil && view->pingcoreWindow != nullptr && _hover == session) {
    const CGPoint point = [session locationInView:view];
    try {
      accepted = view->pingcoreWindow->HandleFileDragMoved(
          _session, pingcore::detail::IosDropOffer(session), {static_cast<float>(point.x), static_cast<float>(point.y)}
      );
    } catch (...) {
      [self dropInteraction:interaction sessionDidExit:session];
    }
  }
  return [[UIDropProposal alloc] initWithDropOperation:accepted ? UIDropOperationCopy : UIDropOperationForbidden];
}

- (void)dropInteraction:(UIDropInteraction*)interaction sessionDidExit:(id<UIDropSession>)session {
  PingCoreView* view = _view;
  if (_hover != session) {
    return;
  }
  _hover = nil;
  if (view != nil && view->pingcoreWindow != nullptr) {
    try {
      view->pingcoreWindow->HandleFileDragExited(_session);
    } catch (...) {
    }
  }
}

- (void)dropInteraction:(UIDropInteraction*)interaction performDrop:(id<UIDropSession>)session {
  PingCoreView* view = _view;
  if (view != nil && view->pingcoreWindow != nullptr && _hover == session) {
    const CGPoint point = [session locationInView:view];
    try {
      PINGCORE_DISCARD(view->pingcoreWindow->HandleFileDrop(
          _session, pingcore::detail::IosDropOffer(session), {static_cast<float>(point.x), static_cast<float>(point.y)},
          pingcore::detail::CaptureIosFileDrop(session)
      ));
    } catch (...) {
    }
  }
  [self dropInteraction:interaction sessionDidExit:session];
}

- (void)dropInteraction:(UIDropInteraction*)interaction sessionDidEnd:(id<UIDropSession>)session {
  [self dropInteraction:interaction sessionDidExit:session];
}

@end

namespace pingcore::detail {

void InstallIosFileDrop(PingCoreView* view) {
  view->pingcoreFileDropDelegate = [[PingCoreFileDropDelegate alloc] initWithView:view];
  [view addInteraction:[[UIDropInteraction alloc] initWithDelegate:view->pingcoreFileDropDelegate]];
}

} // namespace pingcore::detail
