//
//  CMacsTypes.h
//  CMacs
//
//  Created by Robert Widmann on 11/29/14.
//  Copyright (c) 2014 CodaFi. All rights reserved.
//

#include <objc/message.h>
#include <objc/runtime.h>

#ifndef CMACS_CMACSTYPES_H
#define CMACS_CMACSTYPES_H

typedef struct CMPoint {
	double x;
	double y;
} CMPoint;

typedef struct CMSize {
	double width;
	double height;
} CMSize;

typedef struct CMRect {
	CMPoint origin;
	CMSize size;
} CMRect;

typedef struct AppDel {
	Class isa;
	
	// Will be an NSWindow later.
	id window;
} AppDelegate;

enum {
	NSBorderlessWindowMask		= 0,
	NSTitledWindowMask			= 1 << 0,
	NSClosableWindowMask		= 1 << 1,
	NSMiniaturizableWindowMask	= 1 << 2,
	NSResizableWindowMask		= 1 << 3,
};

template <typename R, typename O, typename... Args>
R cast_call(O *i, const char *select, Args... args) {
  using func = R (*)(id, SEL, Args...);
  return ((func)(objc_msgSend))(reinterpret_cast<id>(i), sel_getUid(select),
                                args...);
}

template <typename O, typename... Args>
id call(O *i, const char *select, Args... args) {
  return cast_call<id>(i, select, args...);
}

template <typename R = id, typename... Args>
R clscall(const char *class_name, const char *select, Args... args) {
  using func = R (*)(id, SEL, Args...);
  return ((func)(objc_msgSend))((id)objc_getClass(class_name),
                                sel_getUid(select), args...);
}

#endif