/* Tiny.m
 * A tiny Cocoa application that creates a window
 * and then displays graphics in it.
 * IB is not used to create this application.
 */

// https://www.oreilly.com/library/view/building-cocoa-applications/0596002351/ch04.html
// cc -Wall -o tiny tiny.m -framework Cocoa

#import <Cocoa/Cocoa.h>  // include the Cocoa Frameworks

/************************************************************
 ** A DemoView instance object of this class draws the image.
 ************************************************************/

@interface DemoView : NSView  // interface of DemoView class
{                             // (subclass of NSView class)
}
- (void)drawRect:(NSRect)rect;  // instance method interface
@end

@implementation DemoView  // implementation of DemoView class

#define X(t) (sin(t) + 1) * width * 0.5   // macro for X(t)
#define Y(t) (cos(t) + 1) * height * 0.5  // macro for Y(t)

- (void)drawRect:(NSRect)rect  // instance method implementation
{
  double f, g;
  double const pi = 2 * acos(0.0);

  int n = 12;  // number of sides of the polygon

  // get the size of the application's window and view objects
  float width = [self bounds].size.width;
  float height = [self bounds].size.height;

  [[NSColor whiteColor] set];  // set the drawing color to white
  NSRectFill([self bounds]);   // fill the view with white

  // the following statements trace two polygons with n sides
  // and connect all of the vertices with lines

  [[NSColor blackColor] set];  // set the drawing color to black

  for (f = 0; f < 2 * pi; f += 2 * pi / n) {  // draw the fancy pattern
    for (g = 0; g < 2 * pi; g += 2 * pi / n) {
      NSPoint p1 = NSMakePoint(X(f), Y(f));
      NSPoint p2 = NSMakePoint(X(g), Y(g));
      [NSBezierPath strokeLineFromPoint:p1 toPoint:p2];
    }
  }

}  // end of drawRect: override method

/* windowWillClose: is a delegate method that gets invoked when
 * the on-screen window is about to close (user clicked close box).
 * In this case, we force the entire application to terminate.
 */

- (void)windowWillClose:(NSNotification *)notification {
  [NSApp terminate:self];
}

- (BOOL)canBecomeKeyWindow {
  return YES;
}

- (BOOL)canBecomeMainWindow {
  return YES;
}

- (void)keyDown:(NSEvent *)theEvent {
  printf("keydown\n");
}

@end  // end of DemoView implementation

/*
 * setup(  ) performs the functions that would normally be performed by
 * loading a nib file.
 */

NSWindow *myWindow;  // typed pointer to NSWindow object

void setup() {
  NSView *myView;       // typed pointer to NSView object
  NSRect graphicsRect;  // contains an origin, width, height

  // initialize the rectangle variable
  graphicsRect = NSMakeRect(100.0, 350.0, 400.0, 400.0);

  myWindow = [[NSWindow alloc]  // create the window
      initWithContentRect:graphicsRect
                styleMask:NSTitledWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask
                  backing:NSBackingStoreBuffered
                    defer:NO];

  [myWindow setTitle:@"Tiny Application Window"];

  // create amd initialize the DemoView instance
  myView = [[[DemoView alloc] initWithFrame:graphicsRect] autorelease];

  [myWindow setContentView:myView];  // set window's view

  [myWindow setDelegate:myView];  // set window's delegate
  [myWindow makeKeyAndOrderFront:myWindow];  // display window
}

int main() {
  // create the autorelease pool
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

  // create the application object
  NSApp = [NSApplication sharedApplication];
  // [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
  printf("ActivationPolicy=%d\n", [NSApp activationPolicy]);
  [NSApp activateIgnoringOtherApps:YES];

  // set up the window and drawing mechanism
  setup();

  // run the main event loop
  while (true) {
    [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode beforeDate:[NSDate distantPast]];
    NSEvent *event = [NSApp nextEventMatchingMask:NSAnyEventMask
                                        untilDate:[NSDate distantPast]
                                           inMode:NSDefaultRunLoopMode
                                          dequeue:YES];
    [NSApp sendEvent:event];

    // Some code is execute here every frame to do some tasks...
    if (event != NULL) {
      // printf("got event! type=%d\n", [event type]);
    }
  }

  // we get here when the window is closed

  [NSApp release];  // release the app
  [pool release];   // release the pool
  return (EXIT_SUCCESS);
}
