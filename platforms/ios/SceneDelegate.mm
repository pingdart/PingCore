// platforms/ios/SceneDelegate.mm
#import <UIKit/UIKit.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#include <pingdart/pal/ios/ios_platform.h>
#include <pingdart/rhi/i_render_device.h>
#include <pingdart/rhi/i_swapchain.h>
#include <pingdart/rhi/compositor.h>
#include "../../examples/shared_login/login_app.h"

@interface PingCoreViewController : UIViewController
@end

@implementation PingCoreViewController {
    CAMetalLayer* _metalLayer;
    std::unique_ptr<pingdart::pal::ios::IOSPlatformHost> _platformHost;
    std::unique_ptr<pingdart::pal::ios::IOSWindow> _window;
    std::unique_ptr<pingdart::examples::SharedLoginApp> _loginApp;
    std::unique_ptr<pingdart::rhi::IRenderDevice> _device;
    std::unique_ptr<pingdart::rhi::ISwapchain> _swapchain;
    std::unique_ptr<pingdart::rhi::Compositor> _compositor;
    CADisplayLink* _displayLink;
}

- (void)viewDidLoad {
    [super viewDidLoad];
    self.view.backgroundColor = [UIColor colorWithRed:0.06f green:0.09f blue:0.16f alpha:1.0f];

    _metalLayer = [CAMetalLayer layer];
    _metalLayer.device = MTLCreateSystemDefaultDevice();
    _metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    _metalLayer.frame = self.view.bounds;
    _metalLayer.contentsScale = [UIScreen mainScreen].scale;
    [self.view.layer addSublayer:_metalLayer];

    _platformHost = std::make_unique<pingdart::pal::ios::IOSPlatformHost>();
    (void)_platformHost->initialize();

    auto win_res = _platformHost->create_window("PingDart iOS Login",
        {static_cast<uint32_t>(self.view.bounds.size.width),
         static_cast<uint32_t>(self.view.bounds.size.height)});
    if (win_res.has_value()) {
        _window.reset(static_cast<pingdart::pal::ios::IOSWindow*>(win_res.value().release()));
        _window->setup_metal_surface(
            (__bridge void*)self.view,
            (__bridge void*)_metalLayer,
            static_cast<uint32_t>(self.view.bounds.size.width),
            static_cast<uint32_t>(self.view.bounds.size.height),
            static_cast<float>([UIScreen mainScreen].scale)
        );
    }

    _loginApp = std::make_unique<pingdart::examples::SharedLoginApp>();
    UIEdgeInsets insets = self.view.safeAreaInsets;
    _loginApp->set_screen_geometry(
        static_cast<float>(self.view.bounds.size.width),
        static_cast<float>(self.view.bounds.size.height),
        {static_cast<float>(insets.top),
         static_cast<float>(insets.bottom),
         static_cast<float>(insets.left),
         static_cast<float>(insets.right)}
    );

    // Setup Metal graphics pipeline
    pingdart::rhi::RenderDeviceDescriptor dev_desc{
        .preferred_api = pingdart::rhi::GraphicsApi::Metal,
        .enable_validation_layers = false,
        .prefer_discrete_gpu = true,
        .require_hardware_adapter = false
    };
    auto dev_res = pingdart::rhi::IRenderDevice::create(dev_desc);
    if (dev_res.has_value() && _window) {
        _device = std::move(dev_res).value();
        auto sc_res = _device->create_swapchain(
            _window->native_surface_handle(),
            static_cast<uint32_t>(self.view.bounds.size.width),
            static_cast<uint32_t>(self.view.bounds.size.height),
            pingdart::rhi::PresentMode::Mailbox
        );
        if (sc_res.has_value()) {
            _swapchain = std::move(sc_res).value();
            _compositor = std::make_unique<pingdart::rhi::Compositor>(_device.get());
        }
    }

    _displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(renderFrame)];
    [_displayLink addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSRunLoopCommonModes];
}

- (void)viewDidLayoutSubviews {
    [super viewDidLayoutSubviews];
    _metalLayer.frame = self.view.bounds;
    if (_window) {
        _window->update_drawable_size(
            static_cast<uint32_t>(self.view.bounds.size.width),
            static_cast<uint32_t>(self.view.bounds.size.height)
        );
    }
    if (_swapchain) {
        (void)_swapchain->resize(
            static_cast<uint32_t>(self.view.bounds.size.width),
            static_cast<uint32_t>(self.view.bounds.size.height)
        );
    }
}

- (void)viewSafeAreaInsetsDidChange {
    [super viewSafeAreaInsetsDidChange];
    UIEdgeInsets insets = self.view.safeAreaInsets;
    if (_window) {
        pingdart::pal::ios::IOSSafeAreaInsets pd_insets{
            static_cast<float>(insets.top),
            static_cast<float>(insets.bottom),
            static_cast<float>(insets.left),
            static_cast<float>(insets.right)
        };
        _window->update_safe_area_insets(pd_insets);
    }
    if (_loginApp) {
        _loginApp->set_screen_geometry(
            static_cast<float>(self.view.bounds.size.width),
            static_cast<float>(self.view.bounds.size.height),
            {static_cast<float>(insets.top),
             static_cast<float>(insets.bottom),
             static_cast<float>(insets.left),
             static_cast<float>(insets.right)}
        );
    }
}

- (void)renderFrame {
    if (_loginApp && _device && _swapchain && _compositor) {
        (void)_loginApp->render(_device.get(), _swapchain.get(), _compositor.get());
    }
}

- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    for (UITouch* t in touches) {
        CGPoint pt = [t locationInView:self.view];
        if (_loginApp) {
            _loginApp->handle_pointer_down(pt.x, pt.y, static_cast<uint32_t>((uintptr_t)t));
        }
    }
}

- (void)touchesMoved:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    for (UITouch* t in touches) {
        CGPoint pt = [t locationInView:self.view];
        if (_loginApp) {
            _loginApp->handle_pointer_move(pt.x, pt.y, static_cast<uint32_t>((uintptr_t)t));
        }
    }
}

- (void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    for (UITouch* t in touches) {
        CGPoint pt = [t locationInView:self.view];
        if (_loginApp) {
            _loginApp->handle_pointer_up(pt.x, pt.y, static_cast<uint32_t>((uintptr_t)t));
        }
    }
}

- (void)touchesCancelled:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    for (UITouch* t in touches) {
        if (_loginApp) {
            _loginApp->handle_pointer_cancel(static_cast<uint32_t>((uintptr_t)t));
        }
    }
}

- (void)dealloc {
    [_displayLink invalidate];
    if (_device) {
        _device->wait_idle();
    }
    _swapchain.reset();
    _compositor.reset();
    _device.reset();
    if (_window) {
        _window->detach_metal_surface();
    }
    _window.reset();
    if (_platformHost) {
        _platformHost->shutdown();
    }
    _platformHost.reset();
    _loginApp.reset();
}

@end

@interface SceneDelegate : UIResponder <UIWindowSceneDelegate>
@property (strong, nonatomic) UIWindow* window;
@end

@implementation SceneDelegate

- (void)scene:(UIScene *)scene willConnectToSession:(UISceneSession *)session options:(UISceneConnectionOptions *)connectionOptions {
    if ([scene isKindOfClass:[UIWindowScene class]]) {
        UIWindowScene *windowScene = (UIWindowScene *)scene;
        self.window = [[UIWindow alloc] initWithWindowScene:windowScene];
        self.window.rootViewController = [[PingCoreViewController alloc] init];
        [self.window makeKeyAndVisible];
    }
}

@end
