#include "MacOSGamepadBridge.h"

#import <CoreHaptics/CoreHaptics.h>
#import <GameController/GameController.h>

#include <algorithm>

API_AVAILABLE(macos(11.0))
@interface ArtifactGamepadMotor : NSObject
- (instancetype)initWithHaptics:(GCDeviceHaptics*)InHaptics locality:(GCHapticsLocality)InLocality;
- (void)setIntensity:(float)InIntensity;
- (void)shutdown;
@end

static dispatch_queue_t GetHapticsQueue() {
    static dispatch_queue_t s_Queue = dispatch_queue_create("com.artifact.gamepad-haptics", DISPATCH_QUEUE_SERIAL);
    return s_Queue;
}

@implementation ArtifactGamepadMotor {
    CHHapticEngine* m_Engine;
    id<CHHapticPatternPlayer> m_Player;
    BOOL m_Playing;
}

- (instancetype)initWithHaptics:(GCDeviceHaptics*)InHaptics locality:(GCHapticsLocality)InLocality {
    if ((self = [super init])) {
        m_Engine = [InHaptics createEngineWithLocality:InLocality];
        __weak ArtifactGamepadMotor* weakSelf = self;
        m_Engine.stoppedHandler = ^(CHHapticEngineStoppedReason) {
            dispatch_async(GetHapticsQueue(), ^{ [weakSelf invalidatePlayer]; });
        };
        m_Engine.resetHandler = ^{
            dispatch_async(GetHapticsQueue(), ^{ [weakSelf invalidatePlayer]; });
        };
        [self createPlayer];
    }
    return self;
}

- (void)invalidatePlayer {
    m_Player = nil;
    m_Playing = NO;
}

- (void)createPlayer {
    if (![m_Engine startAndReturnError:nil]) {
        return;
    }
    CHHapticEventParameter* intensity =
        [[CHHapticEventParameter alloc] initWithParameterID:CHHapticEventParameterIDHapticIntensity value:1.0f];
    CHHapticEvent* event = [[CHHapticEvent alloc] initWithEventType:CHHapticEventTypeHapticContinuous
                                                         parameters:@[ intensity ]
                                                       relativeTime:0.0
                                                           duration:GCHapticDurationInfinite];
    CHHapticPattern* pattern = [[CHHapticPattern alloc] initWithEvents:@[ event ] parameters:@[] error:nil];
    if (pattern) {
        m_Player = [m_Engine createPlayerWithPattern:pattern error:nil];
    }
}

- (void)setIntensity:(float)InIntensity {
    if (InIntensity <= 0.0f) {
        if (m_Playing) {
            [m_Player stopAtTime:CHHapticTimeImmediate error:nil];
            m_Playing = NO;
        }
        return;
    }
    if (!m_Player) {
        [self createPlayer];
    }
    CHHapticDynamicParameter* intensity =
        [[CHHapticDynamicParameter alloc] initWithParameterID:CHHapticDynamicParameterIDHapticIntensityControl
                                                       value:InIntensity
                                                relativeTime:0.0];
    [m_Player sendParameters:@[ intensity ] atTime:CHHapticTimeImmediate error:nil];
    if (!m_Playing) {
        m_Playing = [m_Player startAtTime:CHHapticTimeImmediate error:nil];
    }
}

- (void)shutdown {
    [self setIntensity:0.0f];
    [self invalidatePlayer];
    [m_Engine stopWithCompletionHandler:nil];
}

@end

static GCController* s_Controller = nil;
static float s_LowFrequency = 0.0f;
static float s_HighFrequency = 0.0f;

API_AVAILABLE(macos(11.0))
static ArtifactGamepadMotor* s_LowFrequencyMotor = nil;
API_AVAILABLE(macos(11.0))
static ArtifactGamepadMotor* s_HighFrequencyMotor = nil;

void SetMacOSGamepadMotorSpeeds(float InLowFrequency, float InHighFrequency) {
    s_LowFrequency = InLowFrequency;
    s_HighFrequency = InHighFrequency;

    if (@available(macOS 11.0, *)) {
        dispatch_async(GetHapticsQueue(), ^{
            if (s_LowFrequencyMotor == s_HighFrequencyMotor) {
                [s_LowFrequencyMotor setIntensity:std::max(InLowFrequency, InHighFrequency)];
            } else {
                [s_LowFrequencyMotor setIntensity:InLowFrequency];
                [s_HighFrequencyMotor setIntensity:InHighFrequency];
            }
        });
    }
}

static void SelectController(GCController* InController) {
    if (InController == s_Controller) {
        return;
    }
    s_Controller = InController;

    if (@available(macOS 11.0, *)) {
        GCDeviceHaptics* haptics = InController.haptics;
        dispatch_async(GetHapticsQueue(), ^{
            [s_LowFrequencyMotor shutdown];
            [s_HighFrequencyMotor shutdown];
            s_LowFrequencyMotor = nil;
            s_HighFrequencyMotor = nil;
            if (!haptics) {
                return;
            }

            NSSet<GCHapticsLocality>* localities = haptics.supportedLocalities;
            if ([localities containsObject:GCHapticsLocalityLeftHandle] && [localities containsObject:GCHapticsLocalityRightHandle]) {
                s_LowFrequencyMotor = [[ArtifactGamepadMotor alloc] initWithHaptics:haptics locality:GCHapticsLocalityLeftHandle];
                s_HighFrequencyMotor = [[ArtifactGamepadMotor alloc] initWithHaptics:haptics locality:GCHapticsLocalityRightHandle];
            } else {
                s_LowFrequencyMotor = [[ArtifactGamepadMotor alloc] initWithHaptics:haptics locality:GCHapticsLocalityDefault];
                s_HighFrequencyMotor = s_LowFrequencyMotor;
            }
        });
    }

    if (!InController) {
        s_LowFrequency = 0.0f;
        s_HighFrequency = 0.0f;
    }
    SetMacOSGamepadMotorSpeeds(s_LowFrequency, s_HighFrequency);
}

bool ReadMacOSGamepad(MacOSGamepadState& OutState) {
    GCController* activeController = nil;
    for (GCController* controller in GCController.controllers) {
        if (controller.extendedGamepad) {
            activeController = controller;
            break;
        }
    }
    SelectController(activeController);

    GCExtendedGamepad* pad = activeController.extendedGamepad;
    if (!pad) {
        return false;
    }

    // Face buttons by position: South/East/West/North = A/B/X/Y = Cross/Circle/Square/Triangle.
    GCControllerButtonInput* buttons[15] = {
        pad.buttonA, pad.buttonB, pad.buttonX, pad.buttonY,
        pad.leftShoulder, pad.rightShoulder, pad.buttonOptions, pad.buttonMenu,
        nil, pad.leftThumbstickButton, pad.rightThumbstickButton,
        pad.dpad.up, pad.dpad.right, pad.dpad.down, pad.dpad.left,
    };
    if (@available(macOS 11.0, *)) {
        buttons[8] = pad.buttonHome;
    }
    for (int i = 0; i < 15; i++) {
        OutState.Buttons[i] = buttons[i] && buttons[i].isPressed;
    }

    // GameController already reports stick Y up-positive.
    OutState.LeftStick[0] = pad.leftThumbstick.xAxis.value;
    OutState.LeftStick[1] = pad.leftThumbstick.yAxis.value;
    OutState.RightStick[0] = pad.rightThumbstick.xAxis.value;
    OutState.RightStick[1] = pad.rightThumbstick.yAxis.value;
    OutState.LeftTrigger = pad.leftTrigger.value;
    OutState.RightTrigger = pad.rightTrigger.value;
    return true;
}
