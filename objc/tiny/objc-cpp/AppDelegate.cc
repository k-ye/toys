#include <stdio.h>
#include <stdlib.h>

#include "CMacsTypes.h"

// clang++ -std=c++17 -Wall -o app AppDelegate.cc -framework Cocoa

/// A reference to NSApp.  Always a good idea, seeing has he's probably the most
/// helpful thing in CocoaLand
extern id NSApp;

/// This is a strong reference to the class of the AppDelegate
/// (same as [AppDelegate class])
Class AppDelClass;

id window;
id view;

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

static void CreateAppDelegate() {
  AppDelClass = objc_allocateClassPair((Class)objc_getClass("NSObject"),
                                       "AppDelegate", 0);
  // class_addMethod(AppDelClass, sel_getUid("applicationDidFinishLaunching:"),
  // (IMP)AppDel_didFinishLaunching, "i@:@");
  objc_registerClassPair(AppDelClass);
}

void InitWindow() {
  window = clscall("NSWindow", "alloc");

  /// Create an instance of the window.
  window = call(window, "initWithContentRect:styleMask:backing:defer:",
                (CMRect){{0, 0}, {640, 360}},
                (NSTitledWindowMask | NSClosableWindowMask |
                 NSResizableWindowMask | NSMiniaturizableWindowMask),
                0, false);

  /// Create an instance of our view class.
  ///
  /// Relies on the view having declared a constructor that allocates a class
  /// pair for it.
  view = call(clscall("View", "alloc"),
              "initWithFrame:", (CMRect){{0, 0}, {320, 480}});

  // here we simply add the view to the window.
  call(window, "setContentView:", view);
  call(window, "becomeFirstResponder");

  // Shows our window in the bottom-left hand corner of the screen.
  call(window, "makeKeyAndOrderFront:", window);
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
  InitWindow();
  call(NSApp, "run");
}

int main(int argc, char** argv) {
  CreateAppDelegate();
  RunApplication();
  return EXIT_SUCCESS;
}