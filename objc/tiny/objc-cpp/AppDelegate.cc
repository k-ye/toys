#include <stdio.h>
#include <stdlib.h>
#include <vector>

#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#include <CoreGraphics/CGBase.h>
#include <CoreGraphics/CGGeometry.h>
#include <objc/NSObjCRuntime.h>

#include "CMacsTypes.h"
#include "Timer.h"

// clang++ -std=c++17 -Wall -o app AppDelegate.cc -framework Cocoa

/// A reference to NSApp.  Always a good idea, seeing has he's probably the most
/// helpful thing in CocoaLand
extern id NSApp;
extern id const NSDefaultRunLoopMode;

/// This is a strong reference to the class of the AppDelegate
/// (same as [AppDelegate class])
Class ViewClass;
Class AppDelClass;


// BOOL AppDel_didFinishLaunching(AppDelegate *self, SEL _cmd, id notification)
// { 	self->window = cmacs_simple_msgSend((id)objc_getClass("NSWindow"),
// sel_getUid("alloc"));

// 	/// Create an instance of the window.
// 	self->window = cmacs_window_init_msgSend(self->window,
// sel_getUid("initWithContentRect:styleMask:backing:defer:"),
// (CMRect){0,0,1024,460}, (NSTitledWindowMask | NSClosableWindowMask |
// NSResizableWindowMask | NSMiniaturizableWindowMask), 0, false);

// 	/// Create an instance of our view class.
// 	///
// 	/// Relies on the view having declared a constructor that allocates a
// class pair for it. 	id view =
// cmacs_rect_msgSend1(cmacs_simple_msgSend((id)objc_getClass("View"),
// sel_getUid("alloc")), sel_getUid("initWithFrame:"), (CMRect){ 0, 0, 320, 480
// });

// 	// here we simply add the view to the window.
// 	cmacs_void_msgSend1(self->window, sel_getUid("setContentView:"), view);
// 	cmacs_simple_msgSend(self->window, sel_getUid("becomeFirstResponder"));

// 	// Shows our window in the bottom-left hand corner of the screen.
// 	cmacs_void_msgSend1(self->window, sel_getUid("makeKeyAndOrderFront:"),
// self); 	printf("hey done\n"); 	return YES;
// }

static int frame = 0;
static constexpr int kWidth = 600;
static constexpr int kHeight = 400;
static constexpr int kNumPixels = kWidth * kHeight;

id window;
id view;

// void redraw(id self, SEL, CGRect) {
void updateLayer(id self, SEL) {
  printf("redrawing frame=%d\n", frame);
  std::vector<uint8_t> img_data(kNumPixels * 4, 0);
  const int color = (frame / 200) % (255 - 100) + 100;
  for (int i = 0; i < kNumPixels; ++i) {
    img_data[i * 4] = color;
    img_data[i * 4 + 3] = 255;
  }
  CGDataProviderRef provider = CGDataProviderCreateWithData(
      nullptr, img_data.data(), img_data.size(), nullptr);
  CGColorSpaceRef colorspace = CGColorSpaceCreateDeviceRGB();
  CGImageRef image =
      CGImageCreate(kWidth, kHeight, 8, 32, kWidth * 4, colorspace,
                    kCGBitmapByteOrder32Big | kCGImageAlphaPremultipliedLast,
                    provider, nullptr, true, kCGRenderingIntentDefault);
  // CGContextRef context = cast_call<CGContextRef>(
  //     clscall("NSGraphicsContext", "currentContext"), "graphicsPort");
  // CGRect rect{{0, 0}, {CGFloat(kWidth), CGFloat(kHeight)}};
  // CGContextDrawImage(context, rect, image);
  call(call(view, "layer"), "setContents:", image);
  printf("wantsUpdateLayer=%d\n", (intptr_t)call(view, "wantsUpdateLayer"));
  // [CATransaction flush];

  CGImageRelease(image);
  CGDataProviderRelease(provider);
  CGColorSpaceRelease(colorspace);
}

__attribute__((constructor)) static void CreateAppDelegate() {
  ViewClass = objc_allocateClassPair((Class)objc_getClass("NSView"), "View", 0);
  // and again, we tell the runtime to add a function called -drawRect:
  // to our custom view. Note that there is an error in the type-specification
  // of this method, as I do not know the @encode sequence of 'CGRect' off
  // of the top of my head. As a result, there is a chance that the rect
  // parameter of the method may not get passed properly.
  // class_addMethod(ViewClass, sel_getUid("drawRect:"), (IMP)redraw, "v@:");
  class_addMethod(ViewClass, sel_getUid("updateLayer"), (IMP)updateLayer, "v@:");
  objc_registerClassPair(ViewClass);

  AppDelClass = objc_allocateClassPair((Class)objc_getClass("NSObject"),
                                       "AppDelegate", 0);
  // class_addMethod(AppDelClass, sel_getUid("applicationDidFinishLaunching:"),
  // (IMP)AppDel_didFinishLaunching, "i@:@");
  objc_registerClassPair(AppDelClass);
}

void RunApplication(void) {
  clscall("NSApplication", "sharedApplication");

  if (NSApp == NULL) {
    fprintf(stderr, "Failed to initialized NSApplication...  terminating...\n");
    return;
  }
  call(NSApp,
       "setActivationPolicy:", /*NSApplicationActivationPolicyRegular=*/0);

  id appDelObj = clscall("AppDelegate", "alloc");
  appDelObj = call(appDelObj, "init");

  call(NSApp, "setDelegate:", appDelObj);

  window = clscall("NSWindow", "alloc");
  const auto rect = (CGRect){{0, 0}, {CGFloat(kWidth), CGFloat(kHeight)}};
  /// Create an instance of the window.
  window = call(window, "initWithContentRect:styleMask:backing:defer:", rect,
                (NSTitledWindowMask | NSClosableWindowMask |
                 NSResizableWindowMask | NSMiniaturizableWindowMask),
                0, false);

  /// Create an instance of our view class.
  ///
  /// Relies on the view having declared a constructor that allocates a class
  /// pair for it.
  view = call(clscall("View", "alloc"), "initWithFrame:", rect);
  call(view, "setWantsLayer:", YES);

  // here we simply add the view to the window.
  call(window, "setContentView:", view);
  call(window, "becomeFirstResponder");

  // Shows our window in the bottom-left hand corner of the screen.
  // call(window, "setAcceptsMouseMovedEvents:", YES);
  call(window, "makeKeyAndOrderFront:", window);
}

void Loop() {
  frame += 1;
  call(view, "setNeedsDisplay:", YES);

  Timer timer;
  while (true) {
    timer.start();
    auto event = call(
        NSApp, "nextEventMatchingMask:untilDate:inMode:dequeue:", NSUIntegerMax,
        nullptr, NSDefaultRunLoopMode, YES);
    timer.stop();
    if (event != nullptr) {
      printf("nextEventMatchingMask duration=%d us\n", timer.get_micros());
      auto event_type = cast_call<NSInteger>(event, "type");
      call(NSApp, "sendEvent:", event);
      call(NSApp, "updateWindows");
      switch (event_type) {
      case 1: // NSLeftMouseDown
        printf("NSLeftMouseDown\n");
        break;
      case 2: // NSLeftMouseUp
        printf("NSLeftMouseUp\n");
        break;
      case 3: // NSEventTypeRightMouseDown
        printf("NSEventTypeRightMouseDown\n");
        break;
      case 4: // NSEventTypeRightMouseUp
        printf("NSEventTypeRightMouseUp\n");
        break;
      case 5:  // NSMouseMoved
      case 6:  // NSLeftMouseDragged
      case 7:  // NSRightMouseDragged
      case 27: // NSNSOtherMouseDragged
        printf("MouseMoved\n");
        break;
      default:
        break;
      }
    } else {
      break;
    }
  }
}

int main(int argc, char** argv) {
  // CreateAppDelegate();
  RunApplication();
  // call(NSApp, "run");
  while (true) {
    Loop();
  }
  return EXIT_SUCCESS;
}