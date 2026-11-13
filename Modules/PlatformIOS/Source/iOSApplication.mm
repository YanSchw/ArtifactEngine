#import <UIKit/UIKit.h>
#import <QuartzCore/CAMetalLayer.h>

#include "iOSBridge.h"

static int s_Argc = 0;
static char** s_Argv = nullptr;
static UIView* s_View = nil;

static void SubmitTouches(NSSet<UITouch*>* InTouches, UIView* InView, iOSTouchPhase InPhase) {
    const CGFloat scale = InView.contentScaleFactor;
    for (UITouch* touch in InTouches) {
        const CGPoint location = [touch locationInView:InView];
        iOSTouchEvent event;
        event.Id = (uint64_t)(uintptr_t)(__bridge void*)touch;
        event.Phase = InPhase;
        event.X = (float)(location.x * scale);
        event.Y = (float)(location.y * scale);
        event.Pressure = touch.maximumPossibleForce > 0.0 ? (float)(touch.force / touch.maximumPossibleForce) : 1.0f;
        event.Radius = (float)(touch.majorRadius * scale);
        iOSBridge::SubmitTouch(event);
    }
}

@interface ArtifactView : UIView
@end

@implementation ArtifactView

+ (Class)layerClass {
    return [CAMetalLayer class];
}

- (instancetype)initWithFrame:(CGRect)InFrame {
    if ((self = [super initWithFrame:InFrame])) {
        self.multipleTouchEnabled = YES;
        self.backgroundColor = UIColor.blackColor;
    }
    return self;
}

- (void)didMoveToWindow {
    [super didMoveToWindow];
    if (self.window.windowScene) {
        self.contentScaleFactor = self.window.windowScene.screen.nativeScale;
    }
}

- (void)touchesBegan:(NSSet<UITouch*>*)InTouches withEvent:(UIEvent*)InEvent {
    SubmitTouches(InTouches, self, iOSTouchPhase::Began);
}

- (void)touchesMoved:(NSSet<UITouch*>*)InTouches withEvent:(UIEvent*)InEvent {
    SubmitTouches(InTouches, self, iOSTouchPhase::Moved);
}

- (void)touchesEnded:(NSSet<UITouch*>*)InTouches withEvent:(UIEvent*)InEvent {
    SubmitTouches(InTouches, self, iOSTouchPhase::Ended);
}

- (void)touchesCancelled:(NSSet<UITouch*>*)InTouches withEvent:(UIEvent*)InEvent {
    SubmitTouches(InTouches, self, iOSTouchPhase::Cancelled);
}

@end

@interface ArtifactViewController : UIViewController
@end

@implementation ArtifactViewController

- (void)loadView {
    self.view = [[ArtifactView alloc] initWithFrame:CGRectZero];
    s_View = self.view;
}

- (BOOL)prefersStatusBarHidden {
    return YES;
}

- (BOOL)prefersHomeIndicatorAutoHidden {
    return YES;
}

- (UIRectEdge)preferredScreenEdgesDeferringSystemGestures {
    return UIRectEdgeAll;
}

@end

@interface ArtifactSceneDelegate : UIResponder <UIWindowSceneDelegate>
@property (strong, nonatomic) UIWindow* window;
@end

@implementation ArtifactSceneDelegate

- (void)scene:(UIScene*)InScene willConnectToSession:(UISceneSession*)InSession options:(UISceneConnectionOptions*)InOptions {
    self.window = [[UIWindow alloc] initWithWindowScene:(UIWindowScene*)InScene];
    self.window.rootViewController = [[ArtifactViewController alloc] init];
    [self.window makeKeyAndVisible];
    [self performSelector:@selector(startEngine) withObject:nil afterDelay:0.0];
}

- (void)startEngine {
    static bool s_Started = false;
    if (s_Started) {
        return;
    }
    s_Started = true;
    iOSBridge::RunEngine(s_Argc, s_Argv);
}

@end

@interface ArtifactAppDelegate : UIResponder <UIApplicationDelegate>
@end

@implementation ArtifactAppDelegate
@end

void* iOSBridge::GetMetalLayer() {
    return (__bridge void*)s_View.layer;
}

void iOSBridge::GetDrawableSize(uint32_t& OutWidth, uint32_t& OutHeight) {
    const CGSize size = s_View.bounds.size;
    const CGFloat scale = s_View.contentScaleFactor;
    OutWidth = (uint32_t)(size.width * scale);
    OutHeight = (uint32_t)(size.height * scale);
}

bool iOSBridge::IsApplicationActive() {
    return UIApplication.sharedApplication.applicationState == UIApplicationStateActive;
}

void iOSBridge::PumpEvents() {
    @autoreleasepool {
        while (CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.0, TRUE) == kCFRunLoopRunHandledSource) {
        }
    }
}

int main(int argc, char* argv[]) {
    s_Argc = argc;
    s_Argv = argv;
    @autoreleasepool {
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([ArtifactAppDelegate class]));
    }
}
