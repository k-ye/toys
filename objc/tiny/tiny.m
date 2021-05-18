/* Tiny.m
 * A tiny Cocoa application that creates a window
 * and then displays graphics in it.
 * IB is not used to create this application.
 */

// https://www.oreilly.com/library/view/building-cocoa-applications/0596002351/ch04.html
// cc -Wall -o tiny tiny.m -framework Cocoa

#import <Cocoa/Cocoa.h>  // include the Cocoa Frameworks

int frame = 0;
NSWindow *myWindow;  // typed pointer to NSWindow object
NSView *myView;      // typed pointer to NSView object
/************************************************************
 ** A DemoView instance object of this class draws the image.
 ************************************************************/

@interface DemoView : NSView  // interface of DemoView class
{                             // (subclass of NSView class)
}
// - (void)drawRect:(NSRect)rect;  // instance method interface
- (void)updateLayer;
@end

@implementation DemoView  // implementation of DemoView class

- (void)updateLayer {
  // get the size of the application's window and view objects
  int width = [self bounds].size.width;
  int height = [self bounds].size.height;

  CGColorSpaceRef rgb = CGColorSpaceCreateWithName(kCGColorSpaceLinearSRGB);
  CGContextRef gc = CGBitmapContextCreate(NULL, width, height, 8, 0, rgb,
                                          kCGImageByteOrder32Big | kCGImageAlphaPremultipliedLast);
  CGColorSpaceRelease(rgb);

  size_t pitch = CGBitmapContextGetBytesPerRow(gc);
  uint8_t *buffer = CGBitmapContextGetData(gc);
  int color = (frame / 500) % (256 - 20) + 20;
  for (size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < width; ++x) {
      int index = 4 * (x + y * width);
      buffer[index++] = color;
      buffer[index++] = 0;
      buffer[index++] = 0;
      buffer[index++] = 255;  // alpha
    }
  }

  CGImageRef image = CGBitmapContextCreateImage(gc);
  CGContextRelease(gc);
  // CGContextRef context = [[NSGraphicsContext currentContext] graphicsPort];
  // CGContextDrawImage(context, rect, image);
  // self.window.contentView.wantsLayer = YES;
  myView.layer.contents = (__bridge id)image;
  CGImageRelease(image);
}  // end of drawRect: override method

/* windowWillClose: is a delegate method that gets invoked when
 * the on-screen window is about to close (user clicked close box).
 * In this case, we force the entire application to terminate.
 */

- (void)windowWillClose:(NSNotification *)notification {
  [NSApp terminate:self];
}

// - (BOOL) wantsUpdateLayer { return YES; }
@end  // end of DemoView implementation

/*
 * setup(  ) performs the functions that would normally be performed by
 * loading a nib file.
 */

void setup() {
  NSRect graphicsRect;  // contains an origin, width, height

  // initialize the rectangle variable
  graphicsRect = NSMakeRect(0, 0, 400.0, 400.0);

  myWindow = [[NSWindow alloc]  // create the window
      initWithContentRect:graphicsRect
                styleMask:NSTitledWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask
                  backing:NSBackingStoreBuffered
                    defer:NO];

  [myWindow setTitle:@"Tiny Application Window"];

  // create amd initialize the DemoView instance
  myView = [[[DemoView alloc] initWithFrame:graphicsRect] autorelease];
  [myView setWantsLayer:YES];

  [myWindow setContentView:myView];  // set window's view

  [myWindow setDelegate:myView];             // set window's delegate
  [myWindow makeKeyAndOrderFront:myWindow];  // display window
}

int main() {
  // create the autorelease pool
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

  // create the application object
  NSApp = [NSApplication sharedApplication];
  [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
  printf("ActivationPolicy=%d\n", [NSApp activationPolicy]);
  [NSApp activateIgnoringOtherApps:YES];

  // set up the window and drawing mechanism
  setup();

  // run the main event loop
  while (true) {
    ++frame;
    [myView setNeedsDisplay:YES];
    // NSDate *start = [NSDate date];
    [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode beforeDate:[NSDate distantPast]];
    NSEvent *event = [NSApp nextEventMatchingMask:NSAnyEventMask
                                        untilDate:[NSDate distantPast]
                                           inMode:NSDefaultRunLoopMode
                                          dequeue:YES];
    // NSTimeInterval timeInterval = [start timeIntervalSinceNow];
    // printf("timeInterval=%f millis\n", timeInterval * 1000.0);
    [NSApp sendEvent:event];
  }

  // we get here when the window is closed

  [NSApp release];  // release the app
  [pool release];   // release the pool
  return (EXIT_SUCCESS);
}
