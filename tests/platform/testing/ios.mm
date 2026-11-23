#include "smoke.h"

#import <XCTest/XCTest.h>

#include "uikit_renderer.h"
#include "../support/apple_text_test_support.h"

@interface PingCoreUiTestingTests : XCTestCase
@end

@implementation PingCoreUiTestingTests
- (void)testWindowlessRuntime {
  NSString* root = [[[NSBundle bundleForClass:self.class] resourcePath] stringByAppendingPathComponent:@"package"];
  const auto error = RunUiTestingSmoke(root.UTF8String);
  XCTAssertTrue(error.empty(), @"%s", error.c_str());
}

- (void)testFallbackTextCentersAtTheExpectedHeight {
  const auto error = pingcore::test::CheckAppleFallbackTextCentering<pingcore::detail::UIKitRenderer>();
  XCTAssertTrue(error.empty(), @"%s", error.data());
}

- (void)testMixedParagraphGeometryMatchesColdAndWarmDrawing {
  const auto error = pingcore::test::CheckAppleParagraphReplay<pingcore::detail::UIKitRenderer>();
  XCTAssertTrue(error.empty(), @"%.*s", static_cast<int>(error.size()), error.data());
}
@end
