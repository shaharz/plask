// Plask.
// (c) Dean McNamee <dean@gmail.com>, 2010.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

#include "plask_bindings.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "bindings_common.h"

#include "FreeImage.h"

#include <string>
#include <map>

#include <CoreFoundation/CoreFoundation.h>
#include <ScriptingBridge/SBApplication.h>
#include <Foundation/NSObjCRuntime.h>
#include <objc/runtime.h>

#define SK_RELEASE 1  // Hmmmm, really? SkPreConfig is thinking we are debug.
#include "core/SkBitmap.h"
#include "core/SkCanvas.h"
#include "core/SkColorPriv.h"  // For color ordering.
#include "core/SkDevice.h"
#include "core/SkString.h"
#include "core/SkTypeface.h"
#include "core/SkUnPreMultiply.h"
#include "core/SkXfermode.h"
#include "utils/SkParsePath.h"
#include "effects/SkGradientShader.h"
#include "effects/SkDashPathEffect.h"
#include "ports/SkTypeface_mac.h"  // SkCreateTypefaceFromCTFont.
#include "gpu/GrContext.h"
#include "gpu/skgpudevice.h"
#include "core/SkSurface.h"

#import <Syphon/Syphon.h>

#include "gl_bindings.h"
#include "midi_bindings.h"
#include "nvg_bindings.h"

#include "svgparser.h"

template <typename T>
T Clamp(T v, T a, T b) {
  if (v < a) return a;
  if (v > b) return b;
  return v;
}

@interface WrappedNSWindow: NSWindow {
  v8::Persistent<v8::Function> event_callback_;
  v8::Persistent<v8::Function> update_callback_;
  NSTimer* animationTimer;
}

-(void)setEventCallbackWithHandle:(v8::Handle<v8::Function>)func;
-(void)setFrameUpdateCallbackWithHandle:(v8::Handle<v8::Function>)func frameRate:(float)fps;

@end

@interface WindowDelegate : NSObject <NSWindowDelegate> {
}

@end

@interface BlitImageView : NSView {
  SkBitmap* bitmap_;
}

-(id)initWithSkBitmap:(SkBitmap*)bitmap;

@end

class NSWindowWrapper {
 public:
  static v8::Persistent<v8::FunctionTemplate> GetTemplate() {
    static v8::Persistent<v8::FunctionTemplate> ft_cache;
    if (!ft_cache.IsEmpty())
      return ft_cache;

    v8::HandleScope scope;
    ft_cache = v8::Persistent<v8::FunctionTemplate>::New(
        v8::FunctionTemplate::New(&NSWindowWrapper::V8New));
    v8::Local<v8::ObjectTemplate> instance = ft_cache->InstanceTemplate();
    instance->SetInternalFieldCount(3);  // NSWindow, bitmap, and context.

    v8::Local<v8::Signature> default_signature = v8::Signature::New(ft_cache);

    // Configure the template...
    static BatchedConstants constants[] = {
      { "kValue", 12 },
    };

    static BatchedMethods methods[] = {
      { "blit", &NSWindowWrapper::blit },
      { "mouseLocationOutsideOfEventStream",
        &NSWindowWrapper::mouseLocationOutsideOfEventStream },
      { "setAcceptsMouseMovedEvents",
        &NSWindowWrapper::setAcceptsMouseMovedEvents },
      { "setAcceptsFileDrag", &NSWindowWrapper::setAcceptsFileDrag },
      { "setEventCallback",
        &NSWindowWrapper::setEventCallback },
    { "setFrameUpdateCallback",
        &NSWindowWrapper::setFrameUpdateCallback },
      { "setTitle", &NSWindowWrapper::setTitle },
      { "setFrameTopLeftPoint", &NSWindowWrapper::setFrameTopLeftPoint },
      { "center", &NSWindowWrapper::center },
      { "hideCursor", &NSWindowWrapper::hideCursor },
      { "showCursor", &NSWindowWrapper::showCursor },
      { "hide", &NSWindowWrapper::hide },
      { "show", &NSWindowWrapper::show },
      { "screenSize", &NSWindowWrapper::screenSize },
    };

    for (size_t i = 0; i < arraysize(constants); ++i) {
      ft_cache->Set(v8::String::New(constants[i].name),
                    v8::Uint32::New(constants[i].val), v8::ReadOnly);
      instance->Set(v8::String::New(constants[i].name),
                    v8::Uint32::New(constants[i].val), v8::ReadOnly);
    }

    for (size_t i = 0; i < arraysize(methods); ++i) {
      instance->Set(v8::String::New(methods[i].name),
                    v8::FunctionTemplate::New(methods[i].func,
                                              v8::Handle<v8::Value>(),
                                              default_signature));
    }

    return ft_cache;
  }

  static WrappedNSWindow* ExtractWindowPointer(v8::Handle<v8::Object> obj) {
    return v8_utils::UnwrapCPointer<WrappedNSWindow>(obj->GetInternalField(0));
  }

  static SkBitmap* ExtractSkBitmapPointer(v8::Handle<v8::Object> obj) {
    return v8_utils::UnwrapCPointer<SkBitmap>(obj->GetInternalField(1));
  }

  static NSOpenGLContext* ExtractContextPointer(v8::Handle<v8::Object> obj) {
    return v8_utils::UnwrapCPointer<NSOpenGLContext>(obj->GetInternalField(2));
  }

  static bool HasInstance(v8::Handle<v8::Value> value) {
    return GetTemplate()->HasInstance(value);
  }

 private:
  static v8::Handle<v8::Value> V8New(const v8::Arguments& args) {
    if (!args.IsConstructCall())
      return v8_utils::ThrowTypeError(kMsgNonConstructCall);

    if (args.Length() != 8)
      return v8_utils::ThrowError("Wrong number of arguments.");
    uint32_t type = args[0]->Uint32Value();
    uint32_t bwidth = args[1]->Uint32Value();
    uint32_t bheight = args[2]->Uint32Value();
    bool multisample = args[3]->BooleanValue();
    int display = args[4]->Int32Value();
    bool borderless = args[5]->BooleanValue();
    bool fullscreen = args[6]->BooleanValue();
    uint32_t dpi_factor = args[7]->Uint32Value();

    NSScreen* screen = [NSScreen mainScreen];
    NSArray* screens = [NSScreen screens];

    if (display < [screens count]) {
      screen = [screens objectAtIndex:display];
      NSLog(@"Using alternate screen: %@", screen);
    }

    bool use_highdpi = false;
    uint32_t width = bwidth;
    uint32_t height = bheight;

    if (dpi_factor == 2) {
      if ((bwidth & 1) || (bheight & 1)) {
        NSLog(@"Warning, width/height must be multiple of 2 for highdpi.");
      } else if (![screen respondsToSelector:@selector(backingScaleFactor)]) {
        NSLog(@"Warning, OSX version doesn't support highdpi (<10.7?).");
      } else if ([screen backingScaleFactor] != dpi_factor) {
        NSLog(@"Warning, screen didn't support highdpi.");
      } else if (type != 1) {
        NSLog(@"Warning, highdpi only supported for 3d windows.");
      } else {
        use_highdpi = true;
        width = bwidth >> 1;
        height = bheight >> 1;
      }
    }

    int style_mask = NSTitledWindowMask; // | NSClosableWindowMask

    if (borderless)
      style_mask = NSBorderlessWindowMask;

    WrappedNSWindow* window = [[WrappedNSWindow alloc]
        initWithContentRect:NSMakeRect(0.0, 0.0,
                                       width,
                                       height)
        styleMask:style_mask
        backing:NSBackingStoreBuffered
        defer:NO
        screen: screen];

    // Don't really see the point of the "User Interface Preservation" (added
    // in 10.7) for something like Plask.  Disable it on our windows.
    if ([window respondsToSelector:@selector(setRestorable:)])
      [window setRestorable:NO];

    // CGColorSpaceRef rgb_space = CGColorSpaceCreateDeviceRGB();
    // CGBitmapContextCreate(NULL, width, height, 8, width * 4, rgb_space,
    //                       kCGBitmapByteOrder32Little |
    //                       kCGImageAlphaNoneSkipFirst);
    // CGColorSpaceRelease(rgb_space)
    //
    // kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Little;
    SkBitmap* bitmap = NULL;
    NSOpenGLContext* context = NULL;

    if (type == 0) {  // 2d window.
        v8_utils::ThrowError("Unimplemented");
    } else if (type == 1) {  // 3d window.
      NSOpenGLPixelFormatAttribute attrs[] = {
          NSOpenGLPFAColorSize, 24,
          NSOpenGLPFADepthSize, 16,
          NSOpenGLPFADoubleBuffer,
          NSOpenGLPFAAccelerated,
          NSOpenGLPFAStencilSize, 8,
          // Truncate here for non-multisampling
          NSOpenGLPFAMultisample,
          NSOpenGLPFASampleBuffers, 1,
          NSOpenGLPFASamples, 4,
          NSOpenGLPFANoRecovery,
          0
      };

      if (!multisample)
        attrs[8] = 0;

      NSOpenGLPixelFormat* format = [[NSOpenGLPixelFormat alloc]
                                        initWithAttributes:attrs];
      NSView* view = [[NSView alloc] initWithFrame:NSMakeRect(0.0, 0.0,
                                                              width, height)];
      if (use_highdpi)
        [view setWantsBestResolutionOpenGLSurface:YES];
      context = [[NSOpenGLContext alloc] initWithFormat:format
                                         shareContext:nil];
      [format release];
      [window setContentView:view];
      [context setView:view];
      [view release];

      // Make sure both sides of the buffer are cleared.
      [context makeCurrentContext];
      for (int i = 0; i < 2; ++i) {
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT |
                GL_DEPTH_BUFFER_BIT |
                GL_STENCIL_BUFFER_BIT);
        [context flushBuffer];
      }

      if (multisample) {
        glEnable(GL_MULTISAMPLE_ARB);
        glHint(GL_MULTISAMPLE_FILTER_HINT_NV, GL_NICEST);
      }

      // Point sprite support.
      glEnable(GL_POINT_SPRITE);
      glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);

      v8::Local<v8::Object> context_wrapper =
          NSOpenGLContextWrapper::GetTemplate()->
              InstanceTemplate()->NewInstance();
      context_wrapper->SetInternalField(0, v8_utils::WrapCPointer(context));
      args.This()->Set(v8::String::New("context"), context_wrapper);
    }

    if (fullscreen)
      [window setLevel:NSMainMenuWindowLevel+1];
    // NOTE(deanm): We currently aren't even using the delegate for anything,
    // so might as well leave it to nil for now.
    // And oh yeah, setDelegate doesn't retain (because delegates could create
    // cycles, or be the same object, etc).  There was previously a bug here
    // where the delegate was autoreleased.  clang's memory static analysis
    // was no help for that one either.
    // [window setDelegate:[[WindowDelegate alloc] init]];
    [window makeKeyAndOrderFront:nil];

    args.This()->SetInternalField(0, v8_utils::WrapCPointer(window));
    args.This()->SetInternalField(1, v8_utils::WrapCPointer(bitmap));
    args.This()->SetInternalField(2, v8_utils::WrapCPointer(context));

    return args.This();
  }

  static v8::Handle<v8::Value> blit(const v8::Arguments& args) {
    WrappedNSWindow* window = ExtractWindowPointer(args.Holder());
    NSOpenGLContext* context = ExtractContextPointer(args.Holder());
    if (context) {  // 3d, swap the buffers.
      [context flushBuffer];
    } else {  // 2d, redisplay the view.
      [[window contentView] display];
    }
    return v8::Undefined();
  }

  static v8::Handle<v8::Value> mouseLocationOutsideOfEventStream(
      const v8::Arguments& args) {
    WrappedNSWindow* window = ExtractWindowPointer(args.Holder());
    NSPoint pos = [window mouseLocationOutsideOfEventStream];
    v8::Local<v8::Object> res = v8::Object::New();
    res->Set(v8::String::New("x"), v8::Number::New(pos.x));
    res->Set(v8::String::New("y"), v8::Number::New(pos.y));
    return res;
  }

  static v8::Handle<v8::Value> setAcceptsMouseMovedEvents(
      const v8::Arguments& args) {
    WrappedNSWindow* window = ExtractWindowPointer(args.Holder());
    [window setAcceptsMouseMovedEvents:args[0]->BooleanValue()];
    return v8::Undefined();
  }

  static v8::Handle<v8::Value> setAcceptsFileDrag(
      const v8::Arguments& args) {
    WrappedNSWindow* window = ExtractWindowPointer(args.Holder());
    if (args[0]->BooleanValue()) {
      [window registerForDraggedTypes:
          [NSArray arrayWithObject:NSFilenamesPboardType]];
    } else {
      [window unregisterDraggedTypes];
    }
    return v8::Undefined();
  }

  // You should only really call this once, it's a pretty raw function.
  static v8::Handle<v8::Value> setEventCallback(const v8::Arguments& args) {
    if (args.Length() != 1 || !args[0]->IsFunction())
      return v8_utils::ThrowError("Incorrect invocation of setEventCallback.");
    WrappedNSWindow* window = ExtractWindowPointer(args.Holder());
    [window setEventCallbackWithHandle:v8::Handle<v8::Function>::Cast(args[0])];
    return v8::Undefined();
  }

    static v8::Handle<v8::Value> setFrameUpdateCallback(const v8::Arguments& args) {
        if (args.Length() != 2 || !args[0]->IsFunction())
            return v8_utils::ThrowError("Incorrect invocation of setFrameUpdateCallback.");
        WrappedNSWindow* window = ExtractWindowPointer(args.Holder());
        
        [window setFrameUpdateCallbackWithHandle:v8::Handle<v8::Function>::Cast(args[0]) frameRate:args[1]->NumberValue()];
        return v8::Undefined();
    }

  static v8::Handle<v8::Value> setTitle(const v8::Arguments& args) {
    WrappedNSWindow* window = ExtractWindowPointer(args.Holder());
    v8::String::Utf8Value title(args[0]);
    [window setTitle:[NSString stringWithUTF8String:*title]];
    return v8::Undefined();
  }
  static v8::Handle<v8::Value> setFrameTopLeftPoint(const v8::Arguments& args) {
    WrappedNSWindow* window = ExtractWindowPointer(args.Holder());
    if (args.Length() != 2)
      return v8_utils::ThrowError("Wrong number of arguments.");
    [window setFrameTopLeftPoint:NSMakePoint(args[0]->NumberValue(),
                                             args[1]->NumberValue())];
    return v8::Undefined();
  }

  static v8::Handle<v8::Value> center(const v8::Arguments& args) {
    WrappedNSWindow* window = ExtractWindowPointer(args.Holder());
    [window center];
    return v8::Undefined();
  }

  static v8::Handle<v8::Value> hideCursor(const v8::Arguments& args) {
    CGDisplayHideCursor(kCGDirectMainDisplay);
    return v8::Undefined();
  }

  static v8::Handle<v8::Value> showCursor(const v8::Arguments& args) {
    CGDisplayShowCursor(kCGDirectMainDisplay);
    return v8::Undefined();
  }

  static v8::Handle<v8::Value> hide(const v8::Arguments& args) {
    WrappedNSWindow* window = ExtractWindowPointer(args.Holder());
    [window orderOut:nil];
    return v8::Undefined();
  }

  static v8::Handle<v8::Value> show(const v8::Arguments& args) {
    WrappedNSWindow* window = ExtractWindowPointer(args.Holder());
    switch (args[0]->Uint32Value()) {
      case 0:  // Also when no argument was passed.
        [window makeKeyAndOrderFront:nil];
        break;
      case 1:
        [window orderFront:nil];
        break;
      case 2:
        [window orderBack:nil];
        break;
      default:
        return v8_utils::ThrowError("Unknown argument to show().");
        break;
    }

    return v8::Undefined();
  }

  static v8::Handle<v8::Value> screenSize(const v8::Arguments& args) {
    WrappedNSWindow* window = ExtractWindowPointer(args.Holder());
    NSRect frame = [[window screen] frame];
    v8::Local<v8::Object> res = v8::Object::New();
    res->Set(v8::String::New("width"), v8::Number::New(frame.size.width));
    res->Set(v8::String::New("height"), v8::Number::New(frame.size.height));
    return res;
  }

};


class NSEventWrapper {
 public:
  static v8::Persistent<v8::FunctionTemplate> GetTemplate() {
    static v8::Persistent<v8::FunctionTemplate> ft_cache;
    if (!ft_cache.IsEmpty())
      return ft_cache;

    v8::HandleScope scope;
    ft_cache = v8::Persistent<v8::FunctionTemplate>::New(
        v8::FunctionTemplate::New(&NSEventWrapper::V8New));
    v8::Local<v8::ObjectTemplate> instance = ft_cache->InstanceTemplate();
    instance->SetInternalFieldCount(1);  // NSEvent pointer.

    v8::Local<v8::Signature> default_signature = v8::Signature::New(ft_cache);

    // Configure the template...
    static BatchedConstants constants[] = {
      { "NSLeftMouseDown", NSLeftMouseDown },
      { "NSLeftMouseUp", NSLeftMouseUp },
      { "NSRightMouseDown", NSRightMouseDown },
      { "NSRightMouseUp", NSRightMouseUp },
      { "NSMouseMoved", NSMouseMoved },
      { "NSLeftMouseDragged", NSLeftMouseDragged },
      { "NSRightMouseDragged", NSRightMouseDragged },
      { "NSMouseEntered", NSMouseEntered },
      { "NSMouseExited", NSMouseExited },
      { "NSKeyDown", NSKeyDown },
      { "NSKeyUp", NSKeyUp },
      { "NSFlagsChanged", NSFlagsChanged },
      { "NSAppKitDefined", NSAppKitDefined },
      { "NSSystemDefined", NSSystemDefined },
      { "NSApplicationDefined", NSApplicationDefined },
      { "NSPeriodic", NSPeriodic },
      { "NSCursorUpdate", NSCursorUpdate },
      { "NSScrollWheel", NSScrollWheel },
      { "NSTabletPoint", NSTabletPoint },
      { "NSTabletProximity", NSTabletProximity },
      { "NSOtherMouseDown", NSOtherMouseDown },
      { "NSOtherMouseUp", NSOtherMouseUp },
      { "NSOtherMouseDragged", NSOtherMouseDragged },
      { "NSEventTypeGesture", NSEventTypeGesture },
      { "NSEventTypeMagnify", NSEventTypeMagnify },
      { "NSEventTypeSwipe", NSEventTypeSwipe },
      { "NSEventTypeRotate", NSEventTypeRotate },
      { "NSEventTypeBeginGesture", NSEventTypeBeginGesture },
      { "NSEventTypeEndGesture", NSEventTypeEndGesture },
      { "NSAlphaShiftKeyMask", NSAlphaShiftKeyMask },
      { "NSShiftKeyMask", NSShiftKeyMask },
      { "NSControlKeyMask", NSControlKeyMask },
      { "NSAlternateKeyMask", NSAlternateKeyMask },
      { "NSCommandKeyMask", NSCommandKeyMask },
      { "NSNumericPadKeyMask", NSNumericPadKeyMask },
      { "NSHelpKeyMask", NSHelpKeyMask },
      { "NSFunctionKeyMask", NSFunctionKeyMask },
      { "NSDeviceIndependentModifierFlagsMask",
          NSDeviceIndependentModifierFlagsMask },
    };

    static BatchedMethods class_methods[] = {
      { "pressedMouseButtons", &NSEventWrapper::class_pressedMouseButtons },
    };

    static BatchedMethods methods[] = {
      { "type", &NSEventWrapper::type },
      { "buttonNumber", &NSEventWrapper::buttonNumber },
      { "characters", &NSEventWrapper::characters },
      { "keyCode", &NSEventWrapper::keyCode },
      { "locationInWindow", &NSEventWrapper::locationInWindow },
      { "deltaX", &NSEventWrapper::deltaX },
      { "deltaY", &NSEventWrapper::deltaY },
      { "deltaZ", &NSEventWrapper::deltaZ },
      { "pressure", &NSEventWrapper::pressure },
      { "isEnteringProximity", &NSEventWrapper::isEnteringProximity },
      { "modifierFlags", &NSEventWrapper::modifierFlags },
    };

    for (size_t i = 0; i < arraysize(constants); ++i) {
      ft_cache->Set(v8::String::New(constants[i].name),
                    v8::Uint32::New(constants[i].val), v8::ReadOnly);
      instance->Set(v8::String::New(constants[i].name),
                    v8::Uint32::New(constants[i].val), v8::ReadOnly);
    }

    for (size_t i = 0; i < arraysize(class_methods); ++i) {
      ft_cache->Set(v8::String::New(class_methods[i].name),
                    v8::FunctionTemplate::New(class_methods[i].func,
                                              v8::Handle<v8::Value>()));
    }

    for (size_t i = 0; i < arraysize(methods); ++i) {
      instance->Set(v8::String::New(methods[i].name),
                    v8::FunctionTemplate::New(methods[i].func,
                                              v8::Handle<v8::Value>(),
                                              default_signature));
    }

    return ft_cache;
  }

  static bool HasInstance(v8::Handle<v8::Value> value) {
    return GetTemplate()->HasInstance(value);
  }

  static NSEvent* ExtractPointer(v8::Handle<v8::Object> obj) {
    return v8_utils::UnwrapCPointer<NSEvent>(obj->GetInternalField(0));
  }

 private:
  static void WeakCallback(v8::Persistent<v8::Value> value, void* data) {
    v8::Handle<v8::Object> obj = v8::Handle<v8::Object>::Cast(value);
    NSEvent* event = ExtractPointer(obj);

    value.ClearWeak();
    value.Dispose();

    [event release];  // Okay even if event is nil.
  }

  // This will be called when we create a new instance from the instance
  // template, wrapping a NSEvent*.  It can also be called directly from
  // JavaScript, which is a bit of a problem, but we'll survive.
  static v8::Handle<v8::Value> V8New(const v8::Arguments& args) {
    if (!args.IsConstructCall())
      return v8_utils::ThrowTypeError(kMsgNonConstructCall);

    args.This()->SetInternalField(0, v8_utils::WrapCPointer(NULL));

    v8::Persistent<v8::Object> persistent =
        v8::Persistent<v8::Object>::New(args.This());
    persistent.MakeWeak(NULL, &NSEventWrapper::WeakCallback);

    return args.This();
  }

  static v8::Handle<v8::Value> class_pressedMouseButtons(
      const v8::Arguments& args) {
    return v8::Integer::NewFromUnsigned([NSEvent pressedMouseButtons]);
  }

  static v8::Handle<v8::Value> type(const v8::Arguments& args) {
    NSEvent* event = ExtractPointer(args.Holder());
    return v8::Integer::NewFromUnsigned([event type]);
  }

  static v8::Handle<v8::Value> buttonNumber(const v8::Arguments& args) {
    NSEvent* event = ExtractPointer(args.Holder());
    return v8::Integer::NewFromUnsigned([event buttonNumber]);
  }

  static v8::Handle<v8::Value> characters(const v8::Arguments& args) {
    NSEvent* event = ExtractPointer(args.Holder());
    NSString* characters = [event characters];
    return v8::String::New(
        [characters UTF8String],
        [characters lengthOfBytesUsingEncoding:NSUTF8StringEncoding]);
  }

  static v8::Handle<v8::Value> keyCode(const v8::Arguments& args) {
    NSEvent* event = ExtractPointer(args.Holder());
    return v8::Integer::NewFromUnsigned([event keyCode]);
  }

  static v8::Handle<v8::Value> locationInWindow(const v8::Arguments& args) {
    NSEvent* event = ExtractPointer(args.Holder());
    // If window is nil we'll instead get screen coordinates.
    if ([event window] == nil)
      return v8_utils::ThrowError("Calling locationInWindow with nil window.");
    NSPoint pos = [event locationInWindow];
    v8::Local<v8::Object> res = v8::Object::New();
    res->Set(v8::String::New("x"), v8::Number::New(pos.x));
    res->Set(v8::String::New("y"), v8::Number::New(pos.y));
    return res;
  }

  static v8::Handle<v8::Value> deltaX(const v8::Arguments& args) {
    NSEvent* event = ExtractPointer(args.Holder());
    return v8::Number::New([event deltaX]);
  }

  static v8::Handle<v8::Value> deltaY(const v8::Arguments& args) {
    NSEvent* event = ExtractPointer(args.Holder());
    return v8::Number::New([event deltaY]);
  }

  static v8::Handle<v8::Value> deltaZ(const v8::Arguments& args) {
    NSEvent* event = ExtractPointer(args.Holder());
    return v8::Number::New([event deltaZ]);
  }

  static v8::Handle<v8::Value> pressure(const v8::Arguments& args) {
    NSEvent* event = ExtractPointer(args.Holder());
    return v8::Number::New([event pressure]);
  }

  static v8::Handle<v8::Value> isEnteringProximity(const v8::Arguments& args) {
    NSEvent* event = ExtractPointer(args.Holder());
    return v8::Boolean::New([event isEnteringProximity]);
  }

  static v8::Handle<v8::Value> modifierFlags(const v8::Arguments& args) {
    NSEvent* event = ExtractPointer(args.Holder());
    return v8::Integer::NewFromUnsigned([event modifierFlags]);
  }
};

class SkPathWrapper {
 public:
  static v8::Persistent<v8::FunctionTemplate> GetTemplate() {
    static v8::Persistent<v8::FunctionTemplate> ft_cache;
    if (!ft_cache.IsEmpty())
      return ft_cache;

    v8::HandleScope scope;
    ft_cache = v8::Persistent<v8::FunctionTemplate>::New(
        v8::FunctionTemplate::New(&SkPathWrapper::V8New));
    v8::Local<v8::ObjectTemplate> instance = ft_cache->InstanceTemplate();
    instance->SetInternalFieldCount(1);  // SkPath pointer.

    v8::Local<v8::Signature> default_signature = v8::Signature::New(ft_cache);

    // Configure the template...
    static BatchedConstants constants[] = {
      // FillType.
      { "kWindingFillType", SkPath::kWinding_FillType },
      { "kEvenOddFillType", SkPath::kEvenOdd_FillType },
      { "kInverseWindingFillType", SkPath::kInverseWinding_FillType },
      { "kInverseEvenOddFillType", SkPath::kInverseEvenOdd_FillType },
    };

    static BatchedMethods methods[] = {
      { "reset", &SkPathWrapper::reset },
      { "rewind", &SkPathWrapper::rewind },
      { "moveTo", &SkPathWrapper::moveTo },
      { "lineTo", &SkPathWrapper::lineTo },
      { "rLineTo", &SkPathWrapper::rLineTo },
      { "quadTo", &SkPathWrapper::quadTo },
      { "cubicTo", &SkPathWrapper::cubicTo },
      { "arcTo", &SkPathWrapper::arcTo },
      { "arct", &SkPathWrapper::arct },
      { "addRect", &SkPathWrapper::addRect },
      { "addOval", &SkPathWrapper::addOval },
      { "addCircle", &SkPathWrapper::addCircle },
      { "close", &SkPathWrapper::close },
      { "offset", &SkPathWrapper::offset },
      { "getBounds", &SkPathWrapper::getBounds },
      { "toSVGString", &SkPathWrapper::toSVGString },
      { "fromSVGString", &SkPathWrapper::fromSVGString },
      { "contains", &SkPathWrapper::contains },
    };

    for (size_t i = 0; i < arraysize(constants); ++i) {
      ft_cache->Set(v8::String::New(constants[i].name),
                    v8::Uint32::New(constants[i].val), v8::ReadOnly);
      instance->Set(v8::String::New(constants[i].name),
                    v8::Uint32::New(constants[i].val), v8::ReadOnly);
    }

    for (size_t i = 0; i < arraysize(methods); ++i) {
      instance->Set(v8::String::New(methods[i].name),
                    v8::FunctionTemplate::New(methods[i].func,
                                              v8::Handle<v8::Value>(),
                                              default_signature));
    }

    return ft_cache;
  }

  static SkPath* ExtractPointer(v8::Handle<v8::Object> obj) {
    return v8_utils::UnwrapCPointer<SkPath>(obj->GetInternalField(0));
  }

  static bool HasInstance(v8::Handle<v8::Value> value) {
    return GetTemplate()->HasInstance(value);
  }

 private:
  static v8::Handle<v8::Value> reset(const v8::Arguments& args) {
    SkPath* path = ExtractPointer(args.Holder());
    path->reset();
    return v8::Undefined();
  }

  static v8::Handle<v8::Value> rewind(const v8::Arguments& args) {
    SkPath* path = ExtractPointer(args.Holder());
    path->rewind();
    return v8::Undefined();
  }

  static v8::Handle<v8::Value> moveTo(const v8::Arguments& args) {
    SkPath* path = ExtractPointer(args.Holder());

    path->moveTo(SkDoubleToScalar(args[0]->NumberValue()),
                 SkDoubleToScalar(args[1]->NumberValue()));
    return v8::Undefined();
  }

  static v8::Handle<v8::Value> lineTo(const v8::Arguments& args) {
    SkPath* path = ExtractPointer(args.Holder());

    path->lineTo(SkDoubleToScalar(args[0]->NumberValue()),
                 SkDoubleToScalar(args[1]->NumberValue()));
    return v8::Undefined();
  }

  static v8::Handle<v8::Value> rLineTo(const v8::Arguments& args) {
    SkPath* path = ExtractPointer(args.Holder());

    path->rLineTo(SkDoubleToScalar(args[0]->NumberValue()),
                  SkDoubleToScalar(args[1]->NumberValue()));
    return v8::Undefined();
  }

  static v8::Handle<v8::Value> quadTo(const v8::Arguments& args) {
    SkPath* path = ExtractPointer(args.Holder());

    path->quadTo(SkDoubleToScalar(args[0]->NumberValue()),
                 SkDoubleToScalar(args[1]->NumberValue()),
                 SkDoubleToScalar(args[2]->NumberValue()),
                 SkDoubleToScalar(args[3]->NumberValue()));
    return v8::Undefined();
  }

  static v8::Handle<v8::Value> cubicTo(const v8::Arguments& args) {
    SkPath* path = ExtractPointer(args.Holder());

    path->cubicTo(SkDoubleToScalar(args[0]->NumberValue()),
                  SkDoubleToScalar(args[1]->NumberValue()),
                  SkDoubleToScalar(args[2]->NumberValue()),
                  SkDoubleToScalar(args[3]->NumberValue()),
                  SkDoubleToScalar(args[4]->NumberValue()),
                  SkDoubleToScalar(args[5]->NumberValue()));
    return v8::Undefined();
  }

  static v8::Handle<v8::Value> arcTo(const v8::Arguments& args) {
    SkPath* path = ExtractPointer(args.Holder());

    SkRect rect = { SkDoubleToScalar(args[0]->NumberValue()),
                    SkDoubleToScalar(args[1]->NumberValue()),
                    SkDoubleToScalar(args[2]->NumberValue()),
                    SkDoubleToScalar(args[3]->NumberValue()) };
    path->arcTo(rect,
                SkDoubleToScalar(args[4]->NumberValue()),
                SkDoubleToScalar(args[5]->NumberValue()),
                args[6]->BooleanValue());
    return v8::Undefined();
  }

  static v8::Handle<v8::Value> arct(const v8::Arguments& args) {
    SkPath* path = ExtractPointer(args.Holder());

    path->arcTo(SkDoubleToScalar(args[0]->NumberValue()),
                SkDoubleToScalar(args[1]->NumberValue()),
                SkDoubleToScalar(args[2]->NumberValue()),
                SkDoubleToScalar(args[3]->NumberValue()),
                SkDoubleToScalar(args[4]->NumberValue()));
    return v8::Undefined();
  }

  static v8::Handle<v8::Value> addRect(const v8::Arguments& args) {
    SkPath* path = ExtractPointer(args.Holder());

    path->addRect(SkDoubleToScalar(args[0]->NumberValue()),
                  SkDoubleToScalar(args[1]->NumberValue()),
                  SkDoubleToScalar(args[2]->NumberValue()),
                  SkDoubleToScalar(args[3]->NumberValue()),
                  args[4]->BooleanValue() ? SkPath::kCCW_Direction :
                                            SkPath::kCW_Direction);
    return v8::Undefined();
  }

  static v8::Handle<v8::Value> addOval(const v8::Arguments& args) {
    SkPath* path = ExtractPointer(args.Holder());

    SkRect rect = { SkDoubleToScalar(args[0]->NumberValue()),
                    SkDoubleToScalar(args[1]->NumberValue()),
                    SkDoubleToScalar(args[2]->NumberValue()),
                    SkDoubleToScalar(args[3]->NumberValue()) };
    path->addOval(rect, args[4]->BooleanValue() ? SkPath::kCCW_Direction :
                                                  SkPath::kCW_Direction);
    return v8::Undefined();
  }

  static v8::Handle<v8::Value> addCircle(const v8::Arguments& args) {
    SkPath* path = ExtractPointer(args.Holder());

    path->addCircle(SkDoubleToScalar(args[0]->NumberValue()),
                    SkDoubleToScalar(args[1]->NumberValue()),
                    SkDoubleToScalar(args[2]->NumberValue()),
                    args[3]->BooleanValue() ? SkPath::kCCW_Direction :
                                              SkPath::kCW_Direction);
    return v8::Undefined();
  }

  static v8::Handle<v8::Value> close(const v8::Arguments& args) {
    SkPath* path = ExtractPointer(args.Holder());
    path->close();
    return v8::Undefined();
  }

  static v8::Handle<v8::Value> offset(const v8::Arguments& args) {
    SkPath* path = ExtractPointer(args.Holder());
    path->offset(SkDoubleToScalar(args[0]->NumberValue()),
                 SkDoubleToScalar(args[1]->NumberValue()));
    return v8::Undefined();
  }

  static v8::Handle<v8::Value> getBounds(const v8::Arguments& args) {
    SkPath* path = ExtractPointer(args.Holder());
    SkRect bounds = path->getBounds();
    v8::Local<v8::Array> res = v8::Array::New(4);
    res->Set(v8::Integer::New(0), v8::Number::New(bounds.fLeft));
    res->Set(v8::Integer::New(1), v8::Number::New(bounds.fTop));
    res->Set(v8::Integer::New(2), v8::Number::New(bounds.fRight));
    res->Set(v8::Integer::New(3), v8::Number::New(bounds.fBottom));
    return res;
  }

  static v8::Handle<v8::Value> toSVGString(const v8::Arguments& args) {
    SkPath* path = ExtractPointer(args.Holder());
    SkString str;
    SkParsePath::ToSVGString(*path, &str);
    return v8::String::New(str.c_str(), str.size());
  }
    
    static v8::Handle<v8::Value> fromSVGString(const v8::Arguments& args) {
        SkPath* path = ExtractPointer(args.Holder());
        try {
            SvgParser::parsePath(*v8::String::Utf8Value(args[0]), path);
        } catch (std::exception) {
            return v8_utils::ThrowError("Parse error");
        }
        return v8::Undefined();
    }
    
    static v8::Handle<v8::Value> contains(const v8::Arguments& args) {
        SkPath* path = ExtractPointer(args.Holder());
        bool ret = path->contains(SkDoubleToScalar(args[0]->NumberValue()), SkDoubleToScalar(args[1]->NumberValue()));
        return v8::Boolean::New(ret);
    }

  static v8::Handle<v8::Value> V8New(const v8::Arguments& args) {
    if (!args.IsConstructCall())
      return v8_utils::ThrowTypeError(kMsgNonConstructCall);

    SkPath* prev_path = NULL;
    if (SkPathWrapper::HasInstance(args[0])) {
      prev_path = SkPathWrapper::ExtractPointer(
          v8::Handle<v8::Object>::Cast(args[0]));
    }

    SkPath* path = prev_path ? new SkPath(*prev_path) : new SkPath;
    args.This()->SetInternalField(0, v8_utils::WrapCPointer(path));
    return args.This();
  }
};


class SkBitmapWrapper {
public:
    static v8::Persistent<v8::FunctionTemplate> GetTemplate() {
        static v8::Persistent<v8::FunctionTemplate> ft_cache;
        if (!ft_cache.IsEmpty())
            return ft_cache;
        
        v8::HandleScope scope;
        ft_cache = v8::Persistent<v8::FunctionTemplate>::New(
                                                             v8::FunctionTemplate::New(&SkBitmapWrapper::V8New));
        v8::Local<v8::ObjectTemplate> instance = ft_cache->InstanceTemplate();
        instance->SetInternalFieldCount(1);  // SkBitmapWrapper pointer.
        
        v8::Local<v8::Signature> default_signature = v8::Signature::New(ft_cache);
        
        // Configure the template...
//        static BatchedConstants constants[] = {
//            // FillType.
//            { "kWindingFillType", SkPath::kWinding_FillType },
//            { "kEvenOddFillType", SkPath::kEvenOdd_FillType },
//            { "kInverseWindingFillType", SkPath::kInverseWinding_FillType },
//            { "kInverseEvenOddFillType", SkPath::kInverseEvenOdd_FillType },
//        };
        
        static BatchedMethods methods[] = {
            { "width", &SkBitmapWrapper::width },
            { "height", &SkBitmapWrapper::height },
        };
        
//        for (size_t i = 0; i < arraysize(constants); ++i) {
//            ft_cache->Set(v8::String::New(constants[i].name),
//                          v8::Uint32::New(constants[i].val), v8::ReadOnly);
//            instance->Set(v8::String::New(constants[i].name),
//                          v8::Uint32::New(constants[i].val), v8::ReadOnly);
//        }
        
        for (size_t i = 0; i < arraysize(methods); ++i) {
            instance->Set(v8::String::New(methods[i].name),
                          v8::FunctionTemplate::New(methods[i].func,
                                                    v8::Handle<v8::Value>(),
                                                    default_signature));
        }
        
        return ft_cache;
    }
    
    static SkBitmap* ExtractPointer(v8::Handle<v8::Object> obj) {
        return v8_utils::UnwrapCPointer<SkBitmap>(obj->GetInternalField(0));
    }
    
    static bool HasInstance(v8::Handle<v8::Value> value) {
        return GetTemplate()->HasInstance(value);
    }
    
private:
    static void WeakCallback(v8::Persistent<v8::Value> value, void* data) {
        v8::Handle<v8::Object> obj = v8::Handle<v8::Object>::Cast(value);
        auto p = ExtractPointer(obj);
        
        v8::V8::AdjustAmountOfExternalAllocatedMemory(-p->getSize());
        
        value.ClearWeak();
        value.Dispose();
        
        delete p;
    }

    static v8::Handle<v8::Value> width(const v8::Arguments& args) {
        SkBitmap* bitmap = ExtractPointer(args.Holder());
        return v8::Integer::New(bitmap->width());
    }
    
    static v8::Handle<v8::Value> height(const v8::Arguments& args) {
        SkBitmap* bitmap = ExtractPointer(args.Holder());
        return v8::Integer::New(bitmap->height());
    }
    
    static v8::Handle<v8::Value> V8New(const v8::Arguments& args) {
        if (!args.IsConstructCall())
            return v8_utils::ThrowTypeError(kMsgNonConstructCall);
        
        SkBitmap* bitmap = nullptr;
        if (SkBitmapWrapper::HasInstance(args[0])) {
            SkBitmap* prev = SkBitmapWrapper::ExtractPointer(v8::Handle<v8::Object>::Cast(args[0]));
            bitmap = new SkBitmap(*prev);
        } else if (args[0]->IsString()) {
            bitmap = new SkBitmap();
            SkImageDecoder::DecodeFile(*v8::String::Utf8Value(args[0]), bitmap);
        }
        
        args.This()->SetInternalField(0, v8_utils::WrapCPointer(bitmap));

        v8::V8::AdjustAmountOfExternalAllocatedMemory(bitmap->getSize());
        v8::Persistent<v8::Object> persistent =
        v8::Persistent<v8::Object>::New(args.This());
        persistent.MakeWeak(NULL, &SkBitmapWrapper::WeakCallback);
        
        return args.This();
    }
};


class SkPaintWrapper {
 public:
  static v8::Persistent<v8::FunctionTemplate> GetTemplate() {
    static v8::Persistent<v8::FunctionTemplate> ft_cache;
    if (!ft_cache.IsEmpty())
      return ft_cache;

    v8::HandleScope scope;
    ft_cache = v8::Persistent<v8::FunctionTemplate>::New(
        v8::FunctionTemplate::New(&SkPaintWrapper::V8New));
    v8::Local<v8::ObjectTemplate> instance = ft_cache->InstanceTemplate();
    instance->SetInternalFieldCount(1);  // SkPaint pointer.

    v8::Local<v8::Signature> default_signature = v8::Signature::New(ft_cache);

    // Configure the template...
    static BatchedConstants constants[] = {
      // Flags.
      { "kAntiAliasFlag", SkPaint::kAntiAlias_Flag },
      { "kDitherFlag", SkPaint::kDither_Flag },
      { "kUnderlineTextFlag", SkPaint::kUnderlineText_Flag },
      { "kStrikeThruTextFlag", SkPaint::kStrikeThruText_Flag },
      { "kFakeBoldTextFlag", SkPaint::kFakeBoldText_Flag },
      { "kLinearTextFlag", SkPaint::kLinearText_Flag },
      { "kSubpixelTextFlag", SkPaint::kSubpixelText_Flag },
      { "kDevKernTextFlag", SkPaint::kDevKernText_Flag },
      { "kAllFlags", SkPaint::kAllFlags },
      // Style.
      { "kFillStyle", SkPaint::kFill_Style },
      { "kStrokeStyle", SkPaint::kStroke_Style },
      { "kStrokeAndFillStyle", SkPaint::kStrokeAndFill_Style },
      // Port duff modes SkXfermode::Mode.
      { "kClearMode", SkXfermode::kClear_Mode },
      { "kSrcMode", SkXfermode::kSrc_Mode },
      { "kDstMode", SkXfermode::kDst_Mode },
      { "kSrcOverMode", SkXfermode::kSrcOver_Mode },
      { "kDstOverMode", SkXfermode::kDstOver_Mode },
      { "kSrcInMode", SkXfermode::kSrcIn_Mode },
      { "kDstInMode", SkXfermode::kDstIn_Mode },
      { "kSrcOutMode", SkXfermode::kSrcOut_Mode },
      { "kDstOutMode", SkXfermode::kDstOut_Mode },
      { "kSrcATopMode", SkXfermode::kSrcATop_Mode },
      { "kDstATopMode", SkXfermode::kDstATop_Mode },
      { "kXorMode", SkXfermode::kXor_Mode },
      { "kPlusMode", SkXfermode::kPlus_Mode },
      { "kMultiplyMode", SkXfermode::kMultiply_Mode },
      { "kScreenMode", SkXfermode::kScreen_Mode },
      { "kOverlayMode", SkXfermode::kOverlay_Mode },
      { "kDarkenMode", SkXfermode::kDarken_Mode },
      { "kLightenMode", SkXfermode::kLighten_Mode },
      { "kColorDodgeMode", SkXfermode::kColorDodge_Mode },
      { "kColorBurnMode", SkXfermode::kColorBurn_Mode },
      { "kHardLightMode", SkXfermode::kHardLight_Mode },
      { "kSoftLightMode", SkXfermode::kSoftLight_Mode },
      { "kDifferenceMode", SkXfermode::kDifference_Mode },
      { "kExclusionMode", SkXfermode::kExclusion_Mode },
      // Cap
      { "kButtCap", SkPaint::kButt_Cap },
      { "kRoundCap", SkPaint::kRound_Cap },
      { "kSquareCap", SkPaint::kSquare_Cap },
      { "kDefaultCap", SkPaint::kDefault_Cap },
      // Join
      { "kMiterJoin", SkPaint::kMiter_Join },
      { "kRoundJoin", SkPaint::kRound_Join },
      { "kBevelJoin", SkPaint::kBevel_Join },
      { "kDefaultJoin", SkPaint::kDefault_Join },
      // Bitmap filter
      { "kHighFilterLevel", SkPaint::kHigh_FilterLevel },
      { "kMediumFilterLevel", SkPaint::kMedium_FilterLevel },
      { "kLowFilterLevel", SkPaint::kLow_FilterLevel },
      { "kNoneFilterLevel", SkPaint::kNone_FilterLevel },
      // Text align
      { "kLeftAlign", SkPaint::kLeft_Align },
      { "kCenterAlign", SkPaint::kCenter_Align },
      { "kRightAlign", SkPaint::kRight_Align },
    };

    static BatchedMethods methods[] = {
      { "reset", &SkPaintWrapper::reset },
      { "getFlags", &SkPaintWrapper::getFlags },
      { "setFlags", &SkPaintWrapper::setFlags },
      { "setAntiAlias", &SkPaintWrapper::setAntiAlias },
      { "setFilterLevel", &SkPaintWrapper::setFilterLevel },
      { "setDither", &SkPaintWrapper::setDither },
      { "setUnderlineText", &SkPaintWrapper::setUnderlineText },
      { "setStrikeThruText", &SkPaintWrapper::setStrikeThruText },
      { "setFakeBoldText", &SkPaintWrapper::setFakeBoldText },
      { "setSubpixelText", &SkPaintWrapper::setSubpixelText },
      { "setDevKernText", &SkPaintWrapper::setDevKernText },
      { "setLCDRenderText", &SkPaintWrapper::setLCDRenderText },
      { "setAutohinted", &SkPaintWrapper::setAutohinted },
      { "setStrokeWidth", &SkPaintWrapper::setStrokeWidth },
      { "getStyle", &SkPaintWrapper::getStyle },
      { "setStyle", &SkPaintWrapper::setStyle },
      { "setFill", &SkPaintWrapper::setFill },
      { "setStroke", &SkPaintWrapper::setStroke },
      { "setFillAndStroke", &SkPaintWrapper::setFillAndStroke },
      { "getStrokeCap", &SkPaintWrapper::getStrokeCap },
      { "setStrokeCap", &SkPaintWrapper::setStrokeCap },
      { "getStrokeJoin", &SkPaintWrapper::getStrokeJoin },
      { "setStrokeJoin", &SkPaintWrapper::setStrokeJoin },
      { "getStrokeMiter", &SkPaintWrapper::getStrokeMiter },
      { "setStrokeMiter", &SkPaintWrapper::setStrokeMiter },
      { "getFillPath", &SkPaintWrapper::getFillPath },
      { "setColor", &SkPaintWrapper::setColor },
      { "setColorHSV", &SkPaintWrapper::setColorHSV },
      { "setTextSize", &SkPaintWrapper::setTextSize },
      { "setTextAlign", &SkPaintWrapper::setTextAlign },
      { "setXfermodeMode", &SkPaintWrapper::setXfermodeMode },
      { "setFontFamily", &SkPaintWrapper::setFontFamily },
      { "setFontFamilyPostScript", &SkPaintWrapper::setFontFamilyPostScript },
      { "setLinearGradientShader", &SkPaintWrapper::setLinearGradientShader },
      { "setRadialGradientShader", &SkPaintWrapper::setRadialGradientShader },
      { "clearShader", &SkPaintWrapper::clearShader },
      { "setDashPathEffect", &SkPaintWrapper::setDashPathEffect },
      { "clearPathEffect", &SkPaintWrapper::clearPathEffect },
      { "measureText", &SkPaintWrapper::measureText },
      { "measureTextBounds", &SkPaintWrapper::measureTextBounds },
      { "getFontMetrics", &SkPaintWrapper::getFontMetrics },
      { "getTextPath", &SkPaintWrapper::getTextPath },
      { "setBitmapShader", &SkPaintWrapper::setBitmapShader }
    };

    for (size_t i = 0; i < arraysize(constants); ++i) {
      ft_cache->Set(v8::String::New(constants[i].name),
                    v8::Uint32::New(constants[i].val), v8::ReadOnly);
      instance->Set(v8::String::New(constants[i].name),
                    v8::Uint32::New(constants[i].val), v8::ReadOnly);
    }

    for (size_t i = 0; i < arraysize(methods); ++i) {
      instance->Set(v8::String::New(methods[i].name),
                    v8::FunctionTemplate::New(methods[i].func,
                                              v8::Handle<v8::Value>(),
                                              default_signature));
    }

    return ft_cache;
  }

  static SkPaint* ExtractPointer(v8::Handle<v8::Object> obj) {
    return v8_utils::UnwrapCPointer<SkPaint>(obj->GetInternalField(0));
  }

  static bool HasInstance(v8::Handle<v8::Value> value) {
    return GetTemplate()->HasInstance(value);
  }

 private:
    static void WeakCallback(v8::Persistent<v8::Value> value, void* data) {
        v8::Handle<v8::Object> obj = v8::Handle<v8::Object>::Cast(value);
        SkPaint* paint = ExtractPointer(obj);
        
        value.ClearWeak();
        value.Dispose();
        
        // Delete the backing SkCanvas object.  Skia reference counting should
        // handle cleaning up deeper resources (for example the backing pixels).
        delete paint;
    }
    
  static v8::Handle<v8::Value> reset(const v8::Arguments& args) {
    SkPaint* paint = ExtractPointer(args.Holder());
    paint->reset();
    return v8::Undefined();
  }

  static v8::Handle<v8::Value> getFlags(const v8::Arguments& args) {
    SkPaint* paint = ExtractPointer(args.Holder());
    return v8::Uint32::New(paint->getFlags());
  }

  static v8::Handle<v8::Value> setFlags(const v8::Arguments& args) {
    SkPaint* paint = ExtractPointer(args.Holder());
    paint->setFlags(v8_utils::ToInt32(args[0]));
    return v8::Undefined();
  }

  static v8::Handle<v8::Value> setAntiAlias(const v8::Arguments& args) {
    SkPaint* paint = ExtractPointer(args.Holder());
    paint->setAntiAlias(args[0]->BooleanValue());
    return v8::Undefined();
  }

  static v8::Handle<v8::Value> setFilterLevel(const v8::Arguments& args) {
    SkPaint* paint = ExtractPointer(args.Holder());
    paint->setFilterLevel(static_cast<SkPaint::FilterLevel>(v8_utils::ToInt32(args[0])));
    return v8::Undefined();
  }

  static v8::Handle<v8::Value> setDither(const v8::Arguments& args) {
    SkPaint* paint = ExtractPointer(args.Holder());
    paint->setDither(args[0]->BooleanValue());
    return v8::Undefined();
  }

  static v8::Handle<v8::Value> setUnderlineText(const v8::Arguments& args) {
    SkPaint* paint = ExtractPointer(args.Holder());
    paint->setUnderlineText(args[0]->BooleanValue());
    return v8::Undefined();
  }

  static v8::Handle<v8::Value> setStrikeThruText(const v8::Arguments& args) {
    SkPaint* paint = ExtractPointer(args.Holder());
    paint->setStrikeThruText(args[0]->BooleanValue());
    return v8::Undefined();
  }

  static v8::Handle<v8::Value> setFakeBoldText(const v8::Arguments& args) {
    SkPaint* paint = ExtractPointer(args.Holder());
    paint->setFakeBoldText(args[0]->BooleanValue());
    return v8::Undefined();
  }

  static v8::Handle<v8::Value> setSubpixelText(const v8::Arguments& args) {
    SkPaint* paint = ExtractPointer(args.Holder());
    paint->setSubpixelText(args[0]->BooleanValue());
    return v8::Undefined();
  }

  static v8::Handle<v8::Value> setDevKernText(const v8::Arguments& args) {
    SkPaint* paint = ExtractPointer(args.Holder());
    paint->setDevKernText(args[0]->BooleanValue());
    return v8::Undefined();
  }

  static v8::Handle<v8::Value> setLCDRenderText(const v8::Arguments& args) {
    SkPaint* paint = ExtractPointer(args.Holder());
    paint->setLCDRenderText(args[0]->BooleanValue());
    return v8::Undefined();
  }

  static v8::Handle<v8::Value> setAutohinted(const v8::Arguments& args) {
    SkPaint* paint = ExtractPointer(args.Holder());
    paint->setAutohinted(args[0]->BooleanValue());
    return v8::Undefined();
  }

  static v8::Handle<v8::Value> getStrokeWidth(const v8::Arguments& args) {
    SkPaint* paint = ExtractPointer(args.Holder());
    return v8::Number::New(SkScalarToDouble(paint->getStrokeWidth()));
  }

  static v8::Handle<v8::Value> setStrokeWidth(const v8::Arguments& args) {
    SkPaint* paint = ExtractPointer(args.Holder());
    paint->setStrokeWidth(SkDoubleToScalar(args[0]->NumberValue()));
    return v8::Undefined();
  }

  static v8::Handle<v8::Value> getStyle(const v8::Arguments& args) {
    SkPaint* paint = ExtractPointer(args.Holder());
    return v8::Uint32::New(paint->getStyle());
  }

  static v8::Handle<v8::Value> setStyle(const v8::Arguments& args) {
    SkPaint* paint = ExtractPointer(args.Holder());
    paint->setStyle(static_cast<SkPaint::Style>(v8_utils::ToInt32(args[0])));
    return v8::Undefined();
  }

  static v8::Handle<v8::Value> setFill(const v8::Arguments& args) {
    SkPaint* paint = ExtractPointer(args.Holder());
    paint->setStyle(SkPaint::kFill_Style);
    return v8::Undefined();
  }

  static v8::Handle<v8::Value> setStroke(const v8::Arguments& args) {
    SkPaint* paint = ExtractPointer(args.Holder());
    paint->setStyle(SkPaint::kStroke_Style);
    return v8::Undefined();
  }

  static v8::Handle<v8::Value> setFillAndStroke(const v8::Arguments& args) {
    SkPaint* paint = ExtractPointer(args.Holder());
    // We flip the name around because it makes more sense, generally you think
    // of the stroke happening after the fill.
    paint->setStyle(SkPaint::kStrokeAndFill_Style);
    return v8::Undefined();
  }

  static v8::Handle<v8::Value> getStrokeCap(const v8::Arguments& args) {
    SkPaint* paint = ExtractPointer(args.Holder());
    return v8::Uint32::New(paint->getStrokeCap());
  }

  static v8::Handle<v8::Value> setStrokeCap(const v8::Arguments& args) {
    SkPaint* paint = ExtractPointer(args.Holder());
    paint->setStrokeCap(static_cast<SkPaint::Cap>(v8_utils::ToInt32(args[0])));
    return v8::Undefined();
  }

  static v8::Handle<v8::Value> getStrokeJoin(const v8::Arguments& args) {
    SkPaint* paint = ExtractPointer(args.Holder());
    return v8::Uint32::New(paint->getStrokeJoin());
  }

  static v8::Handle<v8::Value> setStrokeJoin(const v8::Arguments& args) {
    SkPaint* paint = ExtractPointer(args.Holder());
    paint->setStrokeJoin(static_cast<SkPaint::Join>(v8_utils::ToInt32(args[0])));
    return v8::Undefined();
  }

  static v8::Handle<v8::Value> getStrokeMiter(const v8::Arguments& args) {
    SkPaint* paint = ExtractPointer(args.Holder());
    return v8::Number::New(SkScalarToDouble(paint->getStrokeMiter()));
  }

  static v8::Handle<v8::Value> setStrokeMiter(const v8::Arguments& args) {
    SkPaint* paint = ExtractPointer(args.Holder());
    paint->setStrokeMiter(SkDoubleToScalar(args[0]->NumberValue()));
    return v8::Undefined();
  }

  static v8::Handle<v8::Value> getFillPath(const v8::Arguments& args) {
    SkPaint* paint = ExtractPointer(args.Holder());

    if (!SkPathWrapper::HasInstance(args[0]))
      return v8::Undefined();

    if (!SkPathWrapper::HasInstance(args[1]))
      return v8::Undefined();

    SkPath* src = SkPathWrapper::ExtractPointer(
        v8::Handle<v8::Object>::Cast(args[0]));
    SkPath* dst = SkPathWrapper::ExtractPointer(
        v8::Handle<v8::Object>::Cast(args[1]));

    return v8::Boolean::New(paint->getFillPath(*src, dst));
  }

  // We wrap it as 4 params instead of 1 to try to keep things as SMIs.
  static v8::Handle<v8::Value> setColor(const v8::Arguments& args) {
    SkPaint* paint = ExtractPointer(args.Holder());

    int r = Clamp(v8_utils::ToInt32WithDefault(args[0], 0), 0, 255);
    int g = Clamp(v8_utils::ToInt32WithDefault(args[1], 0), 0, 255);
    int b = Clamp(v8_utils::ToInt32WithDefault(args[2], 0), 0, 255);
    int a = Clamp(v8_utils::ToInt32WithDefault(args[3], 255), 0, 255);

    paint->setColor(SkColorSetARGB(a, r, g, b));
    return v8::Undefined();
  }

  static v8::Handle<v8::Value> setColorHSV(const v8::Arguments& args) {
    SkPaint* paint = ExtractPointer(args.Holder());

    // TODO(deanm): Clamp.
    SkScalar hsv[] = { SkDoubleToScalar(args[0]->NumberValue()),
                       SkDoubleToScalar(args[1]->NumberValue()),
                       SkDoubleToScalar(args[2]->NumberValue()) };
    int a = Clamp(v8_utils::ToInt32WithDefault(args[3], 255), 0, 255);

    paint->setColor(SkHSVToColor(a, hsv));
    return v8::Undefined();
  }

  static v8::Handle<v8::Value> setTextSize(const v8::Arguments& args) {
    SkPaint* paint = ExtractPointer(args.Holder());

    paint->setTextSize(SkDoubleToScalar(args[0]->NumberValue()));
    return v8::Undefined();
  }
    
    static v8::Handle<v8::Value> setTextAlign(const v8::Arguments& args) {
        SkPaint* paint = ExtractPointer(args.Holder());
        
        paint->setTextAlign(static_cast<SkPaint::Align>(v8_utils::ToInt32(args[0])));
        return v8::Undefined();
    }

  static v8::Handle<v8::Value> setXfermodeMode(const v8::Arguments& args) {
    SkPaint* paint = ExtractPointer(args.Holder());

    paint->setXfermodeMode(
          static_cast<SkXfermode::Mode>(v8_utils::ToInt32(args[0])))->unref();
    return v8::Undefined();
  }

  static v8::Handle<v8::Value> setFontFamily(const v8::Arguments& args) {
    if (args.Length() < 1)
      return v8_utils::ThrowError("Wrong number of arguments.");

    SkPaint* paint = ExtractPointer(args.Holder());
    v8::String::Utf8Value family_name(args[0]);
    paint->setTypeface(SkTypeface::CreateFromName(
        *family_name, static_cast<SkTypeface::Style>(args[1]->Uint32Value())))->unref();
    return v8::Undefined();
  }

   static v8::Handle<v8::Value> setFontFamilyPostScript(
      const v8::Arguments& args) {
    if (args.Length() < 1)
      return v8_utils::ThrowError("Wrong number of arguments.");

    SkPaint* paint = ExtractPointer(args.Holder());
    v8::String::Utf8Value postscript_name(args[0]);

    CFStringRef cfFontName = CFStringCreateWithCString(
        NULL, *postscript_name, kCFStringEncodingUTF8);
    if (cfFontName == NULL)
      return v8_utils::ThrowError("Unable to create font CFString.");

    CTFontRef ctNamed = CTFontCreateWithName(cfFontName, 1, NULL);
    CFRelease(cfFontName);
    if (ctNamed == NULL)
      return v8_utils::ThrowError("Unable to create CTFont.");

    SkTypeface* typeface = SkCreateTypefaceFromCTFont(ctNamed);
    paint->setTypeface(typeface);
    typeface->unref();  // setTypeface will have held a ref.
    CFRelease(ctNamed);  // SkCreateTypefaceFromCTFont will have held a ref.
    return v8::Undefined();
  }

  static v8::Handle<v8::Value> setLinearGradientShader(
      const v8::Arguments& args) {
    if (args.Length() != 5)
      return v8_utils::ThrowError("Wrong number of arguments.");

    SkPaint* paint = ExtractPointer(args.Holder());

    SkPoint points[2] = {{SkDoubleToScalar(args[0]->NumberValue()),
                          SkDoubleToScalar(args[1]->NumberValue())},
                         {SkDoubleToScalar(args[2]->NumberValue()),
                          SkDoubleToScalar(args[3]->NumberValue())}};

    SkColor* colors = NULL;
    SkScalar* positions = NULL;
    uint32_t num = 0;

    if (args[4]->IsArray()) {
      v8::Handle<v8::Array> data = v8::Handle<v8::Array>::Cast(args[4]);
      uint32_t data_len = data->Length();
      num = data_len / 5;

      colors = new SkColor[num];
      positions = new SkScalar[num];

      for (uint32_t i = 0, j = 0; i < data_len; i += 5, ++j) {
        positions[j] = SkDoubleToScalar(data->Get(i)->NumberValue());
        colors[j] = SkColorSetARGB(data->Get(i+4)->Uint32Value() & 0xff,
                                   data->Get(i+1)->Uint32Value() & 0xff,
                                   data->Get(i+2)->Uint32Value() & 0xff,
                                   data->Get(i+3)->Uint32Value() & 0xff);
      }
    }

    // TODO(deanm): Tile mode.
    SkShader* s = SkGradientShader::CreateLinear(points, colors, positions, num,
                                                 SkShader::kClamp_TileMode);
    paint->setShader(s);

    delete[] colors;
    delete[] positions;

    return v8::Undefined();
  }

  static v8::Handle<v8::Value> setRadialGradientShader(
      const v8::Arguments& args) {
    if (args.Length() != 4)
      return v8_utils::ThrowError("Wrong number of arguments.");

    SkPaint* paint = ExtractPointer(args.Holder());

    SkPoint center = {SkDoubleToScalar(args[0]->NumberValue()),
                      SkDoubleToScalar(args[1]->NumberValue())};
    SkScalar radius = SkDoubleToScalar(args[2]->NumberValue());

    SkColor* colors = NULL;
    SkScalar* positions = NULL;
    uint32_t num = 0;

    if (args[3]->IsArray()) {
      v8::Handle<v8::Array> data = v8::Handle<v8::Array>::Cast(args[3]);
      uint32_t data_len = data->Length();
      num = data_len / 5;

      colors = new SkColor[num];
      positions = new SkScalar[num];

      for (uint32_t i = 0, j = 0; i < data_len; i += 5, ++j) {
        positions[j] = SkDoubleToScalar(data->Get(i)->NumberValue());
        colors[j] = SkColorSetARGB(data->Get(i+4)->Uint32Value() & 0xff,
                                   data->Get(i+1)->Uint32Value() & 0xff,
                                   data->Get(i+2)->Uint32Value() & 0xff,
                                   data->Get(i+3)->Uint32Value() & 0xff);
      }
    }

    // TODO(deanm): Tile mode.
    SkShader* s = SkGradientShader::CreateRadial(center, radius,
                                                 colors, positions, num,
                                                 SkShader::kClamp_TileMode);
    paint->setShader(s);

    delete[] colors;
    delete[] positions;

    return v8::Undefined();
  }
    
    static v8::Handle<v8::Value> setBitmapShader(const v8::Arguments& args) {
        if (args.Length() != 1)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        SkPaint* paint = ExtractPointer(args.Holder());
        
        if (!SkBitmapWrapper::HasInstance(args[0]))
            return v8_utils::ThrowError("Bad arguments.");
        
        SkBitmap* bitmap = SkBitmapWrapper::ExtractPointer(v8::Handle<v8::Object>::Cast(args[0]));
        
        // TODO(deanm): Tile mode.
        SkMatrix localM;
        localM.setTranslate(-bitmap->width()/2, -bitmap->height()/2);
//        localM.postScale(dstSize.fWidth / srcRectPtr->width(),
//                         dstSize.fHeight / srcRectPtr->height());
        SkShader* s = SkShader::CreateBitmapShader(*bitmap, SkShader::kRepeat_TileMode, SkShader::kRepeat_TileMode, &localM);
        paint->setShader(s);
        SkSafeUnref(s);
        
        return v8::Undefined();
    }

  static v8::Handle<v8::Value> clearShader(const v8::Arguments& args) {
    SkPaint* paint = ExtractPointer(args.Holder());
    paint->setShader(NULL);
    return v8::Undefined();
  }


  static v8::Handle<v8::Value> setDashPathEffect(const v8::Arguments& args) {
    if (!args[0]->IsArray())
      return v8_utils::ThrowError("Sequence must be an Array.");

    v8::Local<v8::Array> arr = v8::Local<v8::Array>::Cast(args[0]);
    uint32_t length = arr->Length();

    if (length & 1)
      return v8_utils::ThrowError("Sequence must be even.");

    SkScalar* intervals = new SkScalar[length];
    if (!intervals)
      return v8_utils::ThrowError("Unable to allocate intervals.");
    
    for (uint32_t i = 0; i < length; ++i) {
      intervals[i] = SkDoubleToScalar(arr->Get(i)->NumberValue());
    }

    SkPaint* paint = ExtractPointer(args.Holder());
      paint->setPathEffect(SkDashPathEffect::Create(intervals, length, SkDoubleToScalar(args[1]->IsUndefined() ? 0.0 : args[1]->NumberValue())));
    delete[] intervals;
    return v8::Undefined();
  }

  static v8::Handle<v8::Value> clearPathEffect(const v8::Arguments& args) {
    SkPaint* paint = ExtractPointer(args.Holder());
    paint->setPathEffect(NULL);
    return v8::Undefined();
  }

  static v8::Handle<v8::Value> measureText(const v8::Arguments& args) {
    SkPaint* paint = ExtractPointer(args.Holder());

    v8::String::Utf8Value utf8(args[0]);
    SkScalar width = paint->measureText(*utf8, utf8.length());
    return v8::Number::New(width);
  }

  static v8::Handle<v8::Value> measureTextBounds(const v8::Arguments& args) {
    SkPaint* paint = ExtractPointer(args.Holder());

    v8::String::Utf8Value utf8(args[0]);

    SkRect bounds = SkRect::MakeEmpty();
    paint->measureText(*utf8, utf8.length(), &bounds);

    v8::Local<v8::Array> res = v8::Array::New(4);
    res->Set(v8::Integer::New(0), v8::Number::New(bounds.fLeft));
    res->Set(v8::Integer::New(1), v8::Number::New(bounds.fTop));
    res->Set(v8::Integer::New(2), v8::Number::New(bounds.fRight));
    res->Set(v8::Integer::New(3), v8::Number::New(bounds.fBottom));

    return res;
  }

  static v8::Handle<v8::Value> getFontMetrics(const v8::Arguments& args) {
    SkPaint* paint = ExtractPointer(args.Holder());

    SkPaint::FontMetrics metrics;

    paint->getFontMetrics(&metrics);

    v8::Local<v8::Object> res = v8::Object::New();

    //!< The greatest distance above the baseline for any glyph (will be <= 0)
    res->Set(v8::String::New("top"), v8::Number::New(metrics.fTop));
    //!< The recommended distance above the baseline (will be <= 0)
    res->Set(v8::String::New("ascent"), v8::Number::New(metrics.fAscent));
    //!< The recommended distance below the baseline (will be >= 0)
    res->Set(v8::String::New("descent"), v8::Number::New(metrics.fDescent));
    //!< The greatest distance below the baseline for any glyph (will be >= 0)
    res->Set(v8::String::New("bottom"), v8::Number::New(metrics.fBottom));
    //!< The recommended distance to add between lines of text (will be >= 0)
    res->Set(v8::String::New("leading"), v8::Number::New(metrics.fLeading));
    //!< the average charactor width (>= 0)
    res->Set(v8::String::New("avgcharwidth"),
             v8::Number::New(metrics.fAvgCharWidth));
    //!< The minimum bounding box x value for all glyphs
    res->Set(v8::String::New("xmin"), v8::Number::New(metrics.fXMin));
    //!< The maximum bounding box x value for all glyphs
    res->Set(v8::String::New("xmax"), v8::Number::New(metrics.fXMax));
    //!< the height of an 'x' in px, or 0 if no 'x' in face
    res->Set(v8::String::New("xheight"), v8::Number::New(metrics.fXHeight));

    return res;
  }

  static v8::Handle<v8::Value> getTextPath(const v8::Arguments& args) {
    SkPaint* paint = ExtractPointer(args.Holder());

    if (!SkPathWrapper::HasInstance(args[3]))
      return v8_utils::ThrowTypeError("4th argument must be an SkPath.");

    SkPath* path = SkPathWrapper::ExtractPointer(
        v8::Handle<v8::Object>::Cast(args[3]));

    v8::String::Utf8Value utf8(args[0]);

    double x = SkDoubleToScalar(args[1]->NumberValue());
    double y = SkDoubleToScalar(args[2]->NumberValue());

    paint->getTextPath(*utf8, utf8.length(), x, y, path);
    return v8::Undefined();
  }

  static v8::Handle<v8::Value> V8New(const v8::Arguments& args) {
    if (!args.IsConstructCall())
      return v8_utils::ThrowTypeError(kMsgNonConstructCall);

    SkPaint* paint = NULL;
    if (SkPaintWrapper::HasInstance(args[0])) {
      paint = new SkPaint(*SkPaintWrapper::ExtractPointer(
          v8::Handle<v8::Object>::Cast(args[0])));
    } else {
      paint = new SkPaint;
      paint->setTypeface(SkTypeface::CreateFromName("Arial",
                                                    SkTypeface::kNormal));
      // Skia defaults to a stroke width of 0, which is a Skia specific
      // hair-line implementation.  It is most familiar to default to 1.
      paint->setStrokeWidth(1);
    }
    
    args.This()->SetInternalField(0, v8_utils::WrapCPointer(paint));
      
    v8::Persistent<v8::Object> persistent =
    v8::Persistent<v8::Object>::New(args.This());
    persistent.MakeWeak(NULL, &SkPaintWrapper::WeakCallback);
    return args.This();
  }
};


class SkCanvasWrapper {
 public:
  static v8::Persistent<v8::FunctionTemplate> GetTemplate() {
    static v8::Persistent<v8::FunctionTemplate> ft_cache;
    if (!ft_cache.IsEmpty())
      return ft_cache;

    v8::HandleScope scope;
    ft_cache = v8::Persistent<v8::FunctionTemplate>::New(
        v8::FunctionTemplate::New(&SkCanvasWrapper::V8New));
    v8::Local<v8::ObjectTemplate> instance = ft_cache->InstanceTemplate();
    instance->SetInternalFieldCount(2);  // SkCanvas pointers.

    v8::Local<v8::Signature> default_signature = v8::Signature::New(ft_cache);

    // Configure the template...
    static BatchedConstants constants[] = {
      // PointMode.
      { "kPointsPointMode", SkCanvas::kPoints_PointMode },
      { "kLinesPointMode", SkCanvas::kLines_PointMode },
      { "kPolygonPointMode", SkCanvas::kPolygon_PointMode },
    };

    static BatchedMethods methods[] = {
      { "clipRect", &SkCanvasWrapper::clipRect },
      { "clipCircle", &SkCanvasWrapper::clipCircle },
      { "clipPath", &SkCanvasWrapper::clipPath },
      { "drawCircle", &SkCanvasWrapper::drawCircle },
      { "drawLine", &SkCanvasWrapper::drawLine },
      { "drawPaint", &SkCanvasWrapper::drawPaint },
      { "drawCanvas", &SkCanvasWrapper::drawCanvas },
      { "drawBitmap", &SkCanvasWrapper::drawBitmap },
      { "drawColor", &SkCanvasWrapper::drawColor },
      { "eraseColor", &SkCanvasWrapper::eraseColor },
      { "clear", &SkCanvasWrapper::eraseColor },
      { "drawPath", &SkCanvasWrapper::drawPath },
      { "drawPoints", &SkCanvasWrapper::drawPoints },
      { "drawRect", &SkCanvasWrapper::drawRect },
      { "drawRoundRect", &SkCanvasWrapper::drawRoundRect },
      { "drawText", &SkCanvasWrapper::drawText },
      { "drawTextOnPathHV", &SkCanvasWrapper::drawTextOnPathHV },
      { "concatMatrix", &SkCanvasWrapper::concatMatrix },
      { "setMatrix", &SkCanvasWrapper::setMatrix },
      { "resetMatrix", &SkCanvasWrapper::resetMatrix },
      { "translate", &SkCanvasWrapper::translate },
      { "scale", &SkCanvasWrapper::scale },
      { "rotate", &SkCanvasWrapper::rotate },
      { "skew", &SkCanvasWrapper::skew },
      { "save", &SkCanvasWrapper::save },
      { "saveMatrix", &SkCanvasWrapper::saveMatrix },
      { "saveLayer", &SkCanvasWrapper::saveLayer },
      { "restore", &SkCanvasWrapper::restore },
      { "writeImage", &SkCanvasWrapper::writeImage },
      { "imageSnapshot", &SkCanvasWrapper::imageSnapshot },
    };

    for (size_t i = 0; i < arraysize(constants); ++i) {
      ft_cache->Set(v8::String::New(constants[i].name),
                    v8::Uint32::New(constants[i].val), v8::ReadOnly);
      instance->Set(v8::String::New(constants[i].name),
                    v8::Uint32::New(constants[i].val), v8::ReadOnly);
    }

    for (size_t i = 0; i < arraysize(methods); ++i) {
      instance->Set(v8::String::New(methods[i].name),
                    v8::FunctionTemplate::New(methods[i].func,
                                              v8::Handle<v8::Value>(),
                                              default_signature));
    }

    return ft_cache;
  }

  static SkCanvas* ExtractPointer(v8::Handle<v8::Object> obj) {
    return reinterpret_cast<SkCanvas*>(obj->GetPointerFromInternalField(0));
  }

  static bool HasInstance(v8::Handle<v8::Value> value) {
    return GetTemplate()->HasInstance(value);
  }

 private:
  static void WeakCallback(v8::Persistent<v8::Value> value, void* data) {
    v8::Handle<v8::Object> obj = v8::Handle<v8::Object>::Cast(value);
    SkSurface* surface = reinterpret_cast<SkSurface*>(obj->GetPointerFromInternalField(1));

    int size_bytes = surface->width() *
                     surface->height() * 4;
    v8::V8::AdjustAmountOfExternalAllocatedMemory(-size_bytes);

    value.ClearWeak();
    value.Dispose();

    // Delete the backing SkCanvas object.  Skia reference counting should
    // handle cleaning up deeper resources (for example the backing pixels).
    delete surface;
  }

  static v8::Handle<v8::Value> V8New(const v8::Arguments& args) {
    if (!args.IsConstructCall())
      return v8_utils::ThrowTypeError(kMsgNonConstructCall);

    SkSurface* surface;
    if (args.Length() == 2) {  // width / height gpu constructor
        GrContext* ctx = GrContext::Create(kOpenGL_GrBackend, 0);
        GrBackendRenderTargetDesc desc;
        desc.fWidth = args[0]->Uint32Value();
        desc.fHeight = args[1]->Uint32Value();
        desc.fConfig = kSkia8888_GrPixelConfig;
        desc.fOrigin = kBottomLeft_GrSurfaceOrigin;
        desc.fSampleCnt = 0;
        desc.fStencilBits = 8;
        
        GLint buffer;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &buffer);
        desc.fRenderTargetHandle = buffer;
        
        surface = SkSurface::NewRenderTargetDirect(ctx->wrapBackendRenderTarget(desc));
        
    } else if (args.Length() == 3) {
        auto parentCanvas = ExtractPointer(v8::Handle<v8::Object>::Cast(args[0]));
        surface = SkSurface::NewScratchRenderTarget(parentCanvas->getGrContext(),
                                                   SkImageInfo::Make(args[1]->Uint32Value(),
                                                                     args[2]->Uint32Value(),
                                                                     kRGBA_8888_SkColorType,
                                                                     kPremul_SkAlphaType));
    } else {
      return v8_utils::ThrowError("Improper SkCanvas constructor arguments.");
    }

    // TODO: get rid of canvas redudancy, store surface only (since the canvas is cached on the surface anyway)
    SkCanvas* canvas = surface->getCanvas();
    args.This()->SetPointerInInternalField(0, canvas);
    args.This()->SetPointerInInternalField(1, surface);

    // Notify the GC that we have a possibly large amount of data allocated
    // behind this object.  This is sometimes a bit of a lie, for example for
    // a PDF surface or an NSWindow surface.  Anyway, it's just a heuristic.
    int size_bytes = surface->width() * surface->height() * 4;
    v8::V8::AdjustAmountOfExternalAllocatedMemory(size_bytes);

    v8::Persistent<v8::Object> persistent =
        v8::Persistent<v8::Object>::New(args.This());
    persistent.MakeWeak(NULL, &SkCanvasWrapper::WeakCallback);

    return args.This();
  }

  static v8::Handle<v8::Value> concatMatrix(const v8::Arguments& args) {
    SkCanvas* canvas = ExtractPointer(args.Holder());
    SkMatrix matrix;
    matrix.setAll(SkDoubleToScalar(args[0]->NumberValue()),
                  SkDoubleToScalar(args[1]->NumberValue()),
                  SkDoubleToScalar(args[2]->NumberValue()),
                  SkDoubleToScalar(args[3]->NumberValue()),
                  SkDoubleToScalar(args[4]->NumberValue()),
                  SkDoubleToScalar(args[5]->NumberValue()),
                  SkDoubleToScalar(args[6]->NumberValue()),
                  SkDoubleToScalar(args[7]->NumberValue()),
                  SkDoubleToScalar(args[8]->NumberValue()));
      canvas->concat(matrix);
    return v8::Undefined();
  }

  static v8::Handle<v8::Value> setMatrix(const v8::Arguments& args) {
    SkCanvas* canvas = ExtractPointer(args.Holder());
    SkMatrix matrix;
    matrix.setAll(SkDoubleToScalar(args[0]->NumberValue()),
                  SkDoubleToScalar(args[1]->NumberValue()),
                  SkDoubleToScalar(args[2]->NumberValue()),
                  SkDoubleToScalar(args[3]->NumberValue()),
                  SkDoubleToScalar(args[4]->NumberValue()),
                  SkDoubleToScalar(args[5]->NumberValue()),
                  SkDoubleToScalar(args[6]->NumberValue()),
                  SkDoubleToScalar(args[7]->NumberValue()),
                  SkDoubleToScalar(args[8]->NumberValue()));
    canvas->setMatrix(matrix);
    return v8::Undefined();
  }

  static v8::Handle<v8::Value> resetMatrix(const v8::Arguments& args) {
    SkCanvas* canvas = ExtractPointer(args.Holder());
    canvas->resetMatrix();
    return v8::Undefined();
  }

  static v8::Handle<v8::Value> clipRect(const v8::Arguments& args) {
    SkCanvas* canvas = ExtractPointer(args.Holder());

    SkRect rect = { SkDoubleToScalar(args[0]->NumberValue()),
                    SkDoubleToScalar(args[1]->NumberValue()),
                    SkDoubleToScalar(args[2]->NumberValue()),
                    SkDoubleToScalar(args[3]->NumberValue()) };
    canvas->clipRect(rect, SkRegion::kIntersect_Op, true);
    return v8::Undefined();
  }

    static v8::Handle<v8::Value> clipCircle(const v8::Arguments& args) {
        SkCanvas* canvas = ExtractPointer(args.Holder());
        
        auto x = SkDoubleToScalar(args[0]->NumberValue());
        auto y = SkDoubleToScalar(args[1]->NumberValue());
        auto r = SkDoubleToScalar(args[2]->NumberValue());

        SkRRect rrect;
        rrect.setOval(SkRect::MakeLTRB(x-r, y-r, x+r, y+r));
        canvas->clipRRect(rrect, SkRegion::kIntersect_Op, true);
        return v8::Undefined();
    }
    
  static v8::Handle<v8::Value> clipPath(const v8::Arguments& args) {
    SkCanvas* canvas = ExtractPointer(args.Holder());

    if (!SkPathWrapper::HasInstance(args[0]))
      return v8::Undefined();

    SkPath* path = SkPathWrapper::ExtractPointer(
        v8::Handle<v8::Object>::Cast(args[0]));

    canvas->clipPath(*path);  // TODO(deanm): Handle the optional argument.
    return v8::Undefined();
  }

  static v8::Handle<v8::Value> drawCircle(const v8::Arguments& args) {
    SkCanvas* canvas = ExtractPointer(args.Holder());
    // TODO(deanm): Should we use the Signature to enforce this instead?
    if (!SkPaintWrapper::HasInstance(args[0]))
      return v8::Undefined();

    SkPaint* paint = SkPaintWrapper::ExtractPointer(
        v8::Handle<v8::Object>::Cast(args[0]));

    canvas->drawCircle(SkDoubleToScalar(args[1]->NumberValue()),
                       SkDoubleToScalar(args[2]->NumberValue()),
                       SkDoubleToScalar(args[3]->NumberValue()),
                       *paint);
    return v8::Undefined();
  }

  static v8::Handle<v8::Value> drawLine(const v8::Arguments& args) {
    SkCanvas* canvas = ExtractPointer(args.Holder());
    // TODO(deanm): Should we use the Signature to enforce this instead?
    if (!SkPaintWrapper::HasInstance(args[0]))
      return v8::Undefined();

    SkPaint* paint = SkPaintWrapper::ExtractPointer(
        v8::Handle<v8::Object>::Cast(args[0]));

    canvas->drawLine(SkDoubleToScalar(args[1]->NumberValue()),
                     SkDoubleToScalar(args[2]->NumberValue()),
                     SkDoubleToScalar(args[3]->NumberValue()),
                     SkDoubleToScalar(args[4]->NumberValue()),
                     *paint);
    return v8::Undefined();
  }

  static v8::Handle<v8::Value> drawPaint(const v8::Arguments& args) {
    SkCanvas* canvas = ExtractPointer(args.Holder());
    // TODO(deanm): Should we use the Signature to enforce this instead?
    if (!SkPaintWrapper::HasInstance(args[0]))
      return v8::Undefined();

    SkPaint* paint = SkPaintWrapper::ExtractPointer(
        v8::Handle<v8::Object>::Cast(args[0]));

    canvas->drawPaint(*paint);
    return v8::Undefined();
  }

  static v8::Handle<v8::Value> drawCanvas(const v8::Arguments& args) {
    if (args.Length() < 6)
      return v8_utils::ThrowError("Wrong number of arguments.");

    if (!SkCanvasWrapper::HasInstance(args[1]))
      return v8_utils::ThrowError("Bad arguments.");

    SkCanvas* canvas = ExtractPointer(args.Holder());
    SkPaint* paint = NULL;
    if (SkPaintWrapper::HasInstance(args[0])) {
      paint = SkPaintWrapper::ExtractPointer(
          v8::Handle<v8::Object>::Cast(args[0]));
    }

    SkCanvas* src_canvas = SkCanvasWrapper::ExtractPointer(
          v8::Handle<v8::Object>::Cast(args[1]));
    auto src_device = src_canvas->getDevice();

    SkRect dst_rect = { SkDoubleToScalar(args[2]->NumberValue()),
                        SkDoubleToScalar(args[3]->NumberValue()),
                        SkDoubleToScalar(args[4]->NumberValue()),
                        SkDoubleToScalar(args[5]->NumberValue()) };

    int srcx1 = v8_utils::ToInt32WithDefault(args[6], 0);
    int srcy1 = v8_utils::ToInt32WithDefault(args[7], 0);
    int srcx2 = v8_utils::ToInt32WithDefault(args[8],
                                             srcx1 + src_device->width());
    int srcy2 = v8_utils::ToInt32WithDefault(args[9],
                                             srcy1 + src_device->height());
    SkIRect src_rect = { srcx1, srcy1, srcx2, srcy2 };

    canvas->drawBitmapRect(src_device->accessBitmap(false),
                           &src_rect, dst_rect, paint);
    return v8::Undefined();
  }
    
    static v8::Handle<v8::Value> drawBitmap(const v8::Arguments& args) {
        if (args.Length() < 6)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        if (!SkBitmapWrapper::HasInstance(args[1]))
            return v8_utils::ThrowError("Bad arguments.");
        
        SkCanvas* canvas = ExtractPointer(args.Holder());
        SkPaint* paint = NULL;
        if (SkPaintWrapper::HasInstance(args[0])) {
            paint = SkPaintWrapper::ExtractPointer(
                                                   v8::Handle<v8::Object>::Cast(args[0]));
        }
        
        SkBitmap* bitmap = SkBitmapWrapper::ExtractPointer(v8::Handle<v8::Object>::Cast(args[1]));
        
        SkRect dst_rect = { SkDoubleToScalar(args[2]->NumberValue()),
            SkDoubleToScalar(args[3]->NumberValue()),
            SkDoubleToScalar(args[4]->NumberValue()),
            SkDoubleToScalar(args[5]->NumberValue()) };

        canvas->drawBitmapRect(*bitmap, nullptr, dst_rect, paint);
        return v8::Undefined();
    }

  static v8::Handle<v8::Value> drawColor(const v8::Arguments& args) {
    SkCanvas* canvas = ExtractPointer(args.Holder());

    int r = Clamp(v8_utils::ToInt32WithDefault(args[0], 0), 0, 255);
    int g = Clamp(v8_utils::ToInt32WithDefault(args[1], 0), 0, 255);
    int b = Clamp(v8_utils::ToInt32WithDefault(args[2], 0), 0, 255);
    int a = Clamp(v8_utils::ToInt32WithDefault(args[3], 255), 0, 255);
    int m = v8_utils::ToInt32WithDefault(args[4], SkXfermode::kSrcOver_Mode);

    canvas->drawARGB(a, r, g, b, static_cast<SkXfermode::Mode>(m));
    return v8::Undefined();
  }

  static v8::Handle<v8::Value> eraseColor(const v8::Arguments& args) {
    SkCanvas* canvas = ExtractPointer(args.Holder());

    int r = Clamp(v8_utils::ToInt32WithDefault(args[0], 0), 0, 255);
    int g = Clamp(v8_utils::ToInt32WithDefault(args[1], 0), 0, 255);
    int b = Clamp(v8_utils::ToInt32WithDefault(args[2], 0), 0, 255);
    int a = Clamp(v8_utils::ToInt32WithDefault(args[3], 255), 0, 255);

    canvas->clear(SkColorSetARGB(a, r, g, b));
    return v8::Undefined();
  }

  static v8::Handle<v8::Value> drawPath(const v8::Arguments& args) {
    SkCanvas* canvas = ExtractPointer(args.Holder());
    // TODO(deanm): Should we use the Signature to enforce this instead?
    if (!SkPaintWrapper::HasInstance(args[0]))
      return v8::Undefined();

    if (!SkPathWrapper::HasInstance(args[1]))
      return v8::Undefined();

    SkPaint* paint = SkPaintWrapper::ExtractPointer(
        v8::Handle<v8::Object>::Cast(args[0]));

    SkPath* path = SkPathWrapper::ExtractPointer(
        v8::Handle<v8::Object>::Cast(args[1]));

    canvas->drawPath(*path, *paint);
    return v8::Undefined();
  }

  static v8::Handle<v8::Value> drawPoints(const v8::Arguments& args) {
    SkCanvas* canvas = ExtractPointer(args.Holder());
    // TODO(deanm): Should we use the Signature to enforce this instead?
    if (!SkPaintWrapper::HasInstance(args[0]))
      return v8::Undefined();

    if (!args[2]->IsArray())
      return v8::Undefined();

    SkPaint* paint = SkPaintWrapper::ExtractPointer(
        v8::Handle<v8::Object>::Cast(args[0]));

    v8::Handle<v8::Array> data = v8::Handle<v8::Array>::Cast(args[2]);
    uint32_t data_len = data->Length();
    uint32_t points_len = data_len / 2;

    SkPoint* points = new SkPoint[points_len];

    for (uint32_t i = 0; i < points_len; ++i) {
      double x = data->Get(v8::Integer::New(i * 2))->NumberValue();
      double y = data->Get(v8::Integer::New(i * 2 + 1))->NumberValue();
      points[i].set(SkDoubleToScalar(x), SkDoubleToScalar(y));
    }

    canvas->drawPoints(
        static_cast<SkCanvas::PointMode>(v8_utils::ToInt32(args[1])),
        points_len, points, *paint);

    delete[] points;

    return v8::Undefined();
  }

  static v8::Handle<v8::Value> drawRect(const v8::Arguments& args) {
    SkCanvas* canvas = ExtractPointer(args.Holder());
    // TODO(deanm): Should we use the Signature to enforce this instead?
    if (!SkPaintWrapper::HasInstance(args[0]))
      return v8::Undefined();

    SkPaint* paint = SkPaintWrapper::ExtractPointer(
        v8::Handle<v8::Object>::Cast(args[0]));

    SkRect rect = { SkDoubleToScalar(args[1]->NumberValue()),
                    SkDoubleToScalar(args[2]->NumberValue()),
                    SkDoubleToScalar(args[3]->NumberValue()),
                    SkDoubleToScalar(args[4]->NumberValue()) };
    canvas->drawRect(rect, *paint);
    return v8::Undefined();
  }

  static v8::Handle<v8::Value> drawRoundRect(const v8::Arguments& args) {
    SkCanvas* canvas = ExtractPointer(args.Holder());
    // TODO(deanm): Should we use the Signature to enforce this instead?
    if (!SkPaintWrapper::HasInstance(args[0]))
      return v8::Undefined();

    SkPaint* paint = SkPaintWrapper::ExtractPointer(
        v8::Handle<v8::Object>::Cast(args[0]));

    SkRect rect = { SkDoubleToScalar(args[1]->NumberValue()),
                    SkDoubleToScalar(args[2]->NumberValue()),
                    SkDoubleToScalar(args[3]->NumberValue()),
                    SkDoubleToScalar(args[4]->NumberValue()) };
    canvas->drawRoundRect(rect,
                          SkDoubleToScalar(args[5]->NumberValue()),
                          SkDoubleToScalar(args[6]->NumberValue()),
                          *paint);
    return v8::Undefined();
  }

  static v8::Handle<v8::Value> drawText(const v8::Arguments& args) {
    SkCanvas* canvas = ExtractPointer(args.Holder());
    // TODO(deanm): Should we use the Signature to enforce this instead?
    if (!SkPaintWrapper::HasInstance(args[0]))
      return v8::Undefined();

    SkPaint* paint = SkPaintWrapper::ExtractPointer(
        v8::Handle<v8::Object>::Cast(args[0]));

    v8::String::Utf8Value utf8(args[1]);
    canvas->drawText(*utf8, utf8.length(),
                     SkDoubleToScalar(args[2]->NumberValue()),
                     SkDoubleToScalar(args[3]->NumberValue()),
                     *paint);
    return v8::Undefined();
  }

  static v8::Handle<v8::Value> drawTextOnPathHV(const v8::Arguments& args) {
    SkCanvas* canvas = ExtractPointer(args.Holder());
    // TODO(deanm): Should we use the Signature to enforce this instead?
    if (!SkPaintWrapper::HasInstance(args[0]))
      return v8::Undefined();

    if (!SkPathWrapper::HasInstance(args[1]))
      return v8::Undefined();

    SkPaint* paint = SkPaintWrapper::ExtractPointer(
        v8::Handle<v8::Object>::Cast(args[0]));

    SkPath* path = SkPathWrapper::ExtractPointer(
        v8::Handle<v8::Object>::Cast(args[1]));

    v8::String::Utf8Value utf8(args[2]);
    canvas->drawTextOnPathHV(*utf8, utf8.length(), *path,
                             SkDoubleToScalar(args[3]->NumberValue()),
                             SkDoubleToScalar(args[4]->NumberValue()),
                             *paint);
    return v8::Undefined();
  }

  static v8::Handle<v8::Value> translate(const v8::Arguments& args) {
    SkCanvas* canvas = ExtractPointer(args.Holder());
    canvas->translate(SkDoubleToScalar(args[0]->NumberValue()),
                      SkDoubleToScalar(args[1]->NumberValue()));
    return v8::Undefined();
  }

  static v8::Handle<v8::Value> scale(const v8::Arguments& args) {
    SkCanvas* canvas = ExtractPointer(args.Holder());
    canvas->scale(SkDoubleToScalar(args[0]->NumberValue()),
                  SkDoubleToScalar(args[1]->NumberValue()));
    return v8::Undefined();
  }

  static v8::Handle<v8::Value> rotate(const v8::Arguments& args) {
    SkCanvas* canvas = ExtractPointer(args.Holder());
    canvas->rotate(SkDoubleToScalar(args[0]->NumberValue()));
    return v8::Undefined();
  }

  static v8::Handle<v8::Value> skew(const v8::Arguments& args) {
    SkCanvas* canvas = ExtractPointer(args.Holder());
    canvas->skew(SkDoubleToScalar(args[0]->NumberValue()),
                 SkDoubleToScalar(args[1]->NumberValue()));
    return v8::Undefined();
  }

  static v8::Handle<v8::Value> save(const v8::Arguments& args) {
    SkCanvas* canvas = ExtractPointer(args.Holder());
    canvas->save();
    return v8::Undefined();
  }
    
    static v8::Handle<v8::Value> saveMatrix(const v8::Arguments& args) {
        SkCanvas* canvas = ExtractPointer(args.Holder());
        canvas->save(SkCanvas::kMatrix_SaveFlag);   // much cheaper
        return v8::Undefined();
    }

    static v8::Handle<v8::Value> saveLayer(const v8::Arguments& args) {
        SkCanvas* canvas = ExtractPointer(args.Holder());
        
        if (args.Length() == 0) {
            canvas->saveLayer(nullptr, nullptr);
            return v8::Undefined();
        }
        
        SkPaint* paint = nullptr;
        if (SkPaintWrapper::HasInstance(args[0])) {
            paint = SkPaintWrapper::ExtractPointer(v8::Handle<v8::Object>::Cast(args[0]));
        }
    
        if (args.Length() == 5) {
            SkRect bounds = { SkDoubleToScalar(args[1]->NumberValue()),
                SkDoubleToScalar(args[2]->NumberValue()),
                SkDoubleToScalar(args[3]->NumberValue()),
                SkDoubleToScalar(args[4]->NumberValue()) };
        
            canvas->saveLayer(&bounds, paint);
        } else canvas->saveLayer(nullptr, paint);
        
        return v8::Undefined();
    }

    
  static v8::Handle<v8::Value> restore(const v8::Arguments& args) {
    SkCanvas* canvas = ExtractPointer(args.Holder());
    canvas->restore();
    return v8::Undefined();
  }

  static v8::Handle<v8::Value> writeImage(const v8::Arguments& args) {
    const uint32_t rmask = SK_R32_MASK << SK_R32_SHIFT;
    const uint32_t gmask = SK_G32_MASK << SK_G32_SHIFT;
    const uint32_t bmask = SK_B32_MASK << SK_B32_SHIFT;

    if (args.Length() < 2)
      return v8_utils::ThrowError("Wrong number of arguments.");

    SkCanvas* canvas = ExtractPointer(args.Holder());
    const SkBitmap& bitmap = canvas->getDevice()->accessBitmap(false);

    v8::String::Utf8Value type(args[0]);
    if (strcmp(*type, "png") != 0)
      return v8_utils::ThrowError("writeImage can only write PNG types.");

    v8::String::Utf8Value filename(args[1]);

    FIBITMAP* fb = FreeImage_ConvertFromRawBits(
        reinterpret_cast<BYTE*>(bitmap.getPixels()),
        bitmap.width(), bitmap.height(), bitmap.rowBytes(), 32,
        rmask, gmask, bmask, TRUE);

    if (!fb)
      return v8_utils::ThrowError("Couldn't allocate output FreeImage bitmap.");

    // Let's hope that ConvertFromRawBits made a copy.
    for (int y = 0; y < bitmap.height(); ++y) {
      uint32_t* scanline =
          reinterpret_cast<uint32_t*>(FreeImage_GetScanLine(fb, y));
      for (int x = 0; x < bitmap.width(); ++x) {
        scanline[x] = SkUnPreMultiply::PMColorToColor(scanline[x]);
      }
    }

    if (args.Length() >= 3 && args[2]->IsObject()) {
      v8::Handle<v8::Object> opts = v8::Handle<v8::Object>::Cast(args[2]);
      if (opts->Has(v8::String::New("dotsPerMeterX"))) {
        FreeImage_SetDotsPerMeterX(fb,
            opts->Get(v8::String::New("dotsPerMeterX"))->Uint32Value());
      }
      if (opts->Has(v8::String::New("dotsPerMeterY"))) {
        FreeImage_SetDotsPerMeterY(fb,
            opts->Get(v8::String::New("dotsPerMeterY"))->Uint32Value());
      }
    }

    bool saved = FreeImage_Save(FIF_PNG, fb, *filename, 0);
    FreeImage_Unload(fb);

    if (!saved)
      return v8_utils::ThrowError("Failed to save png.");

    return v8::Undefined();
  }
    
  static v8::Handle<v8::Value> imageSnapshot(const v8::Arguments& args);
};


class SkImageWrapper {
public:
    static v8::Persistent<v8::FunctionTemplate> GetTemplate() {
        static v8::Persistent<v8::FunctionTemplate> ft_cache;
        if (!ft_cache.IsEmpty())
            return ft_cache;
        
        v8::HandleScope scope;
        ft_cache = v8::Persistent<v8::FunctionTemplate>::New(v8::FunctionTemplate::New(&SkImageWrapper::V8New));
        v8::Local<v8::ObjectTemplate> instance = ft_cache->InstanceTemplate();
        instance->SetInternalFieldCount(1);  // SkImageWrapper pointer.
        
        v8::Local<v8::Signature> default_signature = v8::Signature::New(ft_cache);
        
        static BatchedMethods methods[] = {
            { "width", &SkImageWrapper::width },
            { "height", &SkImageWrapper::height },
            { "draw", &SkImageWrapper::draw },
        };
        
        for (size_t i = 0; i < arraysize(methods); ++i) {
            instance->Set(v8::String::New(methods[i].name),
                          v8::FunctionTemplate::New(methods[i].func,
                                                    v8::Handle<v8::Value>(),
                                                    default_signature));
        }
        
        return ft_cache;
    }
    
    static SkImage* ExtractPointer(v8::Handle<v8::Object> obj) {
        return v8_utils::UnwrapCPointer<SkImage>(obj->GetInternalField(0));
    }
    
    static bool HasInstance(v8::Handle<v8::Value> value) {
        return GetTemplate()->HasInstance(value);
    }
    
private:
    static void WeakCallback(v8::Persistent<v8::Value> value, void* data) {
        v8::Handle<v8::Object> obj = v8::Handle<v8::Object>::Cast(value);
        auto p = ExtractPointer(obj);
        
        value.ClearWeak();
        value.Dispose();
        
        delete p;
    }
    
    static v8::Handle<v8::Value> width(const v8::Arguments& args) {
        SkImage* image = ExtractPointer(args.Holder());
        return v8::Integer::New(image->width());
    }
    
    static v8::Handle<v8::Value> height(const v8::Arguments& args) {
        SkImage* image = ExtractPointer(args.Holder());
        return v8::Integer::New(image->height());
    }
    
    static v8::Handle<v8::Value> draw(const v8::Arguments& args) {
        SkImage* image = ExtractPointer(args.Holder());
        if (args.Length() != 4
            || !SkCanvasWrapper::HasInstance(args[0])
            || !SkPaintWrapper::HasInstance(args[3])) {
            return v8_utils::ThrowError("Bad arguments.");
        }
        
        SkCanvas* canvas = SkCanvasWrapper::ExtractPointer(v8::Handle<v8::Object>::Cast(args[0]));
        SkPaint* paint = SkPaintWrapper::ExtractPointer(v8::Handle<v8::Object>::Cast(args[3]));
        image->draw(canvas,
                    SkDoubleToScalar(args[1]->NumberValue()),
                    SkDoubleToScalar(args[2]->NumberValue()),
                    paint);
        return v8::Undefined();
    }
    
    static v8::Handle<v8::Value> V8New(const v8::Arguments& args) {
        if (!args.IsConstructCall())
            return v8_utils::ThrowTypeError(kMsgNonConstructCall);
        
        if (args.Length() == 1
            && SkBitmapWrapper::HasInstance(args[0]))
        {
            SkBitmap* bitmap = SkBitmapWrapper::ExtractPointer(v8::Handle<v8::Object>::Cast(args[0]));
            
            SkImage* image = SkImage::NewTexture(*bitmap);
            
            args.This()->SetInternalField(0, v8_utils::WrapCPointer(image));
        }
        
        // leave default constructor accessible so imageSnapshot() can get an empty instance (there's probably a better way)
        
        v8::Persistent<v8::Object> persistent =
        v8::Persistent<v8::Object>::New(args.This());
        persistent.MakeWeak(NULL, &SkImageWrapper::WeakCallback);
        
        return args.This();
    }
};


v8::Handle<v8::Value> SkCanvasWrapper::imageSnapshot(const v8::Arguments& args) {
    auto obj = v8::Handle<v8::Object>::Cast(args.Holder());
    SkSurface* surface = reinterpret_cast<SkSurface*>(obj->GetPointerFromInternalField(1));

    v8::Local<v8::Object> res = SkImageWrapper::GetTemplate()->InstanceTemplate()->NewInstance();
    res->SetInternalField(0, v8_utils::WrapCPointer(surface->newImageSnapshot()));
    
    return res;
}

v8::Handle<v8::Value> NSOpenGLContextWrapper::texImage2DSkCanvasB(
    const v8::Arguments& args) {
  if (args.Length() != 3)
    return v8_utils::ThrowError("Wrong number of arguments.");

  if (!args[2]->IsObject() && !SkCanvasWrapper::HasInstance(args[2]))
    return v8_utils::ThrowError("Expected image to be an SkCanvas instance.");

  SkCanvas* canvas = SkCanvasWrapper::ExtractPointer(
      v8::Handle<v8::Object>::Cast(args[2]));
  const SkBitmap& bitmap = canvas->getDevice()->accessBitmap(false);
  glTexImage2D(args[0]->Uint32Value(),
               args[1]->Int32Value(),
               GL_RGBA8,
               bitmap.width(),
               bitmap.height(),
               0,
               GL_BGRA,  // We have to swizzle, so this technically isn't ES.
               GL_UNSIGNED_INT_8_8_8_8_REV,
               bitmap.getPixels());
  return v8::Undefined();
}

v8::Handle<v8::Value> NSOpenGLContextWrapper::drawSkCanvas(
    const v8::Arguments& args) {
    
    SkCanvas* canvas = SkCanvasWrapper::ExtractPointer(v8::Handle<v8::Object>::Cast(args[0]));
    canvas->getGrContext()->flush();
    
  return v8::Undefined();
}

class SBApplicationWrapper {
 public:
  static v8::Persistent<v8::FunctionTemplate> GetTemplate() {
    static v8::Persistent<v8::FunctionTemplate> ft_cache;
    if (!ft_cache.IsEmpty())
      return ft_cache;

    v8::HandleScope scope;
    ft_cache = v8::Persistent<v8::FunctionTemplate>::New(
        v8::FunctionTemplate::New(&SBApplicationWrapper::V8New));
    v8::Local<v8::ObjectTemplate> instance = ft_cache->InstanceTemplate();
    instance->SetInternalFieldCount(1);  // id.

    v8::Local<v8::Signature> default_signature = v8::Signature::New(ft_cache);

    // Configure the template...
    static BatchedConstants constants[] = {
      { "kDummy", 1 },
    };

    static BatchedMethods methods[] = {
      { "objcMethods", &SBApplicationWrapper::objcMethods },
      { "invokeVoid0", &SBApplicationWrapper::invokeVoid0 },
      { "invokeVoid1s", &SBApplicationWrapper::invokeVoid1s },
    };

    for (size_t i = 0; i < arraysize(constants); ++i) {
      instance->Set(v8::String::New(constants[i].name),
                    v8::Uint32::New(constants[i].val), v8::ReadOnly);
    }

    for (size_t i = 0; i < arraysize(methods); ++i) {
      instance->Set(v8::String::New(methods[i].name),
                    v8::FunctionTemplate::New(methods[i].func,
                                              v8::Handle<v8::Value>(),
                                              default_signature));
    }

    return ft_cache;
  }

  static id ExtractID(v8::Handle<v8::Object> obj) {
    return reinterpret_cast<id>(obj->GetPointerFromInternalField(0));
  }

  static bool HasInstance(v8::Handle<v8::Value> value) {
    return GetTemplate()->HasInstance(value);
  }

 private:
  static v8::Handle<v8::Value> V8New(const v8::Arguments& args) {
    if (!args.IsConstructCall())
      return v8_utils::ThrowTypeError(kMsgNonConstructCall);

    if (args.Length() != 1)
      return v8_utils::ThrowError("Wrong number of arguments.");

    v8::String::Utf8Value bundleid(args[0]);
    id obj = [SBApplication applicationWithBundleIdentifier:
        [NSString stringWithUTF8String:*bundleid]];
    [obj retain];

    if (obj == nil)
      return v8_utils::ThrowError("Unable to create SBApplication.");

    args.This()->SetPointerInInternalField(0, obj);

    return args.This();
  }

  static v8::Handle<v8::Value> objcMethods(const v8::Arguments& args) {
    id obj = ExtractID(args.Holder());
    unsigned int num_methods;
    Method* methods = class_copyMethodList(obj->isa, &num_methods);
    v8::Local<v8::Array> res = v8::Array::New(num_methods);

    for (unsigned int i = 0; i < num_methods; ++i) {
      unsigned num_args = method_getNumberOfArguments(methods[i]);
      v8::Local<v8::Array> sig = v8::Array::New(num_args + 1);
      char rettype[256];
      method_getReturnType(methods[i], rettype, sizeof(rettype));
      sig->Set(v8::Integer::NewFromUnsigned(0),
               v8::String::New(sel_getName(method_getName(methods[i]))));
      sig->Set(v8::Integer::NewFromUnsigned(1),
               v8::String::New(rettype));
      for (unsigned j = 0; j < num_args; ++j) {
        char argtype[256];
        method_getArgumentType(methods[i], j, argtype, sizeof(argtype));
        sig->Set(v8::Integer::NewFromUnsigned(j + 2),
                 v8::String::New(argtype));
      }
      res->Set(v8::Integer::NewFromUnsigned(i), sig);
    }

    return res;
  }

  static v8::Handle<v8::Value> invokeVoid0(const v8::Arguments& args) {
    if (args.Length() != 1)
      return v8_utils::ThrowError("Wrong number of arguments.");

    id obj = ExtractID(args.Holder());
    v8::String::Utf8Value method_name(args[0]);
    [obj performSelector:sel_getUid(*method_name)];
    return v8::Undefined();
  }

  static v8::Handle<v8::Value> invokeVoid1s(const v8::Arguments& args) {
    if (args.Length() != 2)
      return v8_utils::ThrowError("Wrong number of arguments.");

    id obj = ExtractID(args.Holder());
    v8::String::Utf8Value method_name(args[0]);
    v8::String::Utf8Value arg(args[1]);
    [obj performSelector:sel_getUid(*method_name) withObject:
        [NSString stringWithUTF8String:*arg]];
    return v8::Undefined();
  }
};


class NSAppleScriptWrapper {
 public:
  static v8::Persistent<v8::FunctionTemplate> GetTemplate() {
    static v8::Persistent<v8::FunctionTemplate> ft_cache;
    if (!ft_cache.IsEmpty())
      return ft_cache;

    v8::HandleScope scope;
    ft_cache = v8::Persistent<v8::FunctionTemplate>::New(
        v8::FunctionTemplate::New(&NSAppleScriptWrapper::V8New));
    v8::Local<v8::ObjectTemplate> instance = ft_cache->InstanceTemplate();
    instance->SetInternalFieldCount(1);  // NSAppleScript*.

    v8::Local<v8::Signature> default_signature = v8::Signature::New(ft_cache);

    // Configure the template...
    static BatchedConstants constants[] = {
      { "kDummy", 1 },
    };

    static BatchedMethods methods[] = {
      { "execute", &NSAppleScriptWrapper::execute },
    };

    for (size_t i = 0; i < arraysize(constants); ++i) {
      instance->Set(v8::String::New(constants[i].name),
                    v8::Uint32::New(constants[i].val), v8::ReadOnly);
    }

    for (size_t i = 0; i < arraysize(methods); ++i) {
      instance->Set(v8::String::New(methods[i].name),
                    v8::FunctionTemplate::New(methods[i].func,
                                              v8::Handle<v8::Value>(),
                                              default_signature));
    }

    return ft_cache;
  }

  static NSAppleScript* ExtractNSAppleScript(v8::Handle<v8::Object> obj) {
    return reinterpret_cast<NSAppleScript*>(
        obj->GetPointerFromInternalField(0));
  }

  static bool HasInstance(v8::Handle<v8::Value> value) {
    return GetTemplate()->HasInstance(value);
  }

 private:
  static v8::Handle<v8::Value> V8New(const v8::Arguments& args) {
    if (!args.IsConstructCall())
      return v8_utils::ThrowTypeError(kMsgNonConstructCall);

    if (args.Length() != 1)
      return v8_utils::ThrowError("Wrong number of arguments.");

    v8::String::Utf8Value src(args[0]);
    NSAppleScript* ascript = [[NSAppleScript alloc] initWithSource:
        [NSString stringWithUTF8String:*src]];

    if (ascript == nil)
      return v8_utils::ThrowError("Unable to create NSAppleScript.");

    args.This()->SetPointerInInternalField(0, ascript);

    return args.This();
  }

  static v8::Handle<v8::Value> execute(const v8::Arguments& args) {
    NSAppleScript* ascript = ExtractNSAppleScript(args.Holder());
    if ([ascript executeAndReturnError:nil] == nil)
      return v8_utils::ThrowError("Error executing AppleScript.");
    return v8::Undefined();
  }
};

@implementation WrappedNSWindow

-(void)setEventCallbackWithHandle:(v8::Handle<v8::Function>)func {
  event_callback_ = v8::Persistent<v8::Function>::New(func);
}

-(void)setFrameUpdateCallbackWithHandle:(v8::Handle<v8::Function>)func frameRate:(float)fps {
    if (animationTimer && [animationTimer isValid])
    {
        update_callback_.Dispose();
        [animationTimer invalidate];
    }
    
    animationTimer = [NSTimer timerWithTimeInterval:(1.0f / fps)
                                              target:self
                                            selector:@selector(timerFired:)
                                            userInfo:nil
                                             repeats:YES];
    [[NSRunLoop currentRunLoop] addTimer:animationTimer forMode:NSDefaultRunLoopMode];
    [[NSRunLoop currentRunLoop] addTimer:animationTimer forMode:NSEventTrackingRunLoopMode];

    update_callback_ = v8::Persistent<v8::Function>::New(func);
}
//#include <mach/mach_time.h>
//double MachTimeToSecs(uint64_t time)
//{
//    mach_timebase_info_data_t timebase;
//    mach_timebase_info(&timebase);
//    return (double)time * (double)timebase.numer /
//    (double)timebase.denom / 1e9;
//}
//
//double ft = 0;
//int fcount = 0;
- (void)timerFired:(NSTimer *)t
{
//    uint64_t begin = mach_absolute_time();
  if (*update_callback_) {
      // v8::HandleScope scope; // needed?
      v8::TryCatch try_catch;
      update_callback_->Call(v8::Context::GetCurrent()->Global(), 0, NULL);
      // Hopefully plask.js will have caught any exceptions already.
      if (try_catch.HasCaught()) {
          printf("Exception in event callback, TODO(deanm): print something.\n");
      }
  }
//    uint64_t end = mach_absolute_time();
//    ft = (ft + MachTimeToSecs(end - begin)) / 2;
//    if ((fcount++ % 30) == 0) printf("%g\n", 1/ft);
}

-(void)processEvent:(NSEvent *)event {
  if (*event_callback_) {
    v8::HandleScope scope;
    [event retain];  // Released by NSEventWrapper.
    v8::Local<v8::Object> res =
        NSEventWrapper::GetTemplate()->InstanceTemplate()->NewInstance();
    res->SetInternalField(0, v8_utils::WrapCPointer(event));
    v8::Local<v8::Value> argv[] = { v8::Number::New(0), res };
    v8::TryCatch try_catch;
    event_callback_->Call(v8::Context::GetCurrent()->Global(), 2, argv);
    // Hopefully plask.js will have caught any exceptions already.
    if (try_catch.HasCaught()) {
      printf("Exception in event callback, TODO(deanm): print something.\n");
    }
  }
}

// In order to receive keyboard events, we need to be able to be the key window.
// By default this would be YES, except if we don't have a title bar, for
// example in fullscreen mode.  We want to always be able to become the key
// window and the main window.
-(BOOL)canBecomeMainWindow {
  return YES;
}

-(BOOL)canBecomeKeyWindow {
  return YES;
}

-(void)sendEvent:(NSEvent *)event {
  [super sendEvent:event];
  [self processEvent:event];
}

-(void)noResponderFor:(SEL)event_selector {
  // Overridden since the default implementation beeps for keyDown.
}

- (NSDragOperation)draggingEntered:(id <NSDraggingInfo>)sender {
  return NSDragOperationCopy;
}

- (BOOL)performDragOperation:(id <NSDraggingInfo>)sender {
  NSPasteboard* board = [sender draggingPasteboard];
  NSArray* paths = [board propertyListForType:NSFilenamesPboardType];
  v8::Local<v8::Array> jspaths = v8::Array::New([paths count]);
  for (int i = 0; i < [paths count]; ++i) {
    jspaths->Set(v8::Integer::New(i), v8::String::New(
        [[paths objectAtIndex:i] UTF8String]));
  }

  NSPoint location = [sender draggingLocation];

  v8::Local<v8::Object> res = v8::Object::New();
  res->Set(v8::String::New("paths"), jspaths);
  res->Set(v8::String::New("x"), v8::Number::New(location.x));
  res->Set(v8::String::New("y"), v8::Number::New(location.y));

  v8::Handle<v8::Value> argv[] = { v8::Number::New(1), res };
  v8::TryCatch try_catch;
  event_callback_->Call(v8::Context::GetCurrent()->Global(), 2, argv);
  // Hopefully plask.js will have caught any exceptions already.
  if (try_catch.HasCaught()) {
    printf("Exception in event callback, TODO(deanm): print something.\n");
  }

  return YES;
}


@end

@implementation WindowDelegate

-(void)windowDidMove:(NSNotification *)notification {
}

@end

@implementation BlitImageView

-(id)initWithSkBitmap:(SkBitmap*)bitmap {
  bitmap_ = bitmap;
  [self initWithFrame:NSMakeRect(0.0, 0.0,
                                 bitmap->width(), bitmap->height())];
  return self;
}

// TODO(deanm): There is too much copying going on here.  Can a CGImage back
// directly to my pixels without going through a decoder?
-(void)drawRect:(NSRect)dirty {
  int width = bitmap_->width(), height = bitmap_->height();
  void* pixels = bitmap_->getPixels();

  CFDataRef cfdata = CFDataCreateWithBytesNoCopy(
      NULL, (UInt8*)pixels, width * height * 4, kCFAllocatorNull);
  CGDataProviderRef cgdata_provider = CGDataProviderCreateWithCFData(cfdata);
  //CGDataProviderRef cgdata_provider =
  //    CGDataProviderCreateWithFilename("/tmp/maxmsp.png");
  CGColorSpaceRef colorspace = CGColorSpaceCreateDeviceRGB();
  CGImageRef cgimage = CGImageCreate(
      width, height, 8, 32, width * 4, colorspace,
      kCGBitmapByteOrder32Little | kCGImageAlphaPremultipliedFirst,
      cgdata_provider, NULL, false, kCGRenderingIntentDefault);
  //CGImageRef cgimage = CGImageCreateWithPNGDataProvider(
  //  cgdata_provider, NULL, false, kCGRenderingIntentDefault);
  CGColorSpaceRelease(colorspace);

  CGRect image_rect = CGRectMake(0.0, 0.0, width, height);
  CGContextRef context =
      (CGContextRef)[[NSGraphicsContext currentContext] graphicsPort];
  // TODO(deanm): Deal with subimages when dirty rect isn't full frame.
  CGContextDrawImage(context, image_rect, cgimage);

  CGImageRelease(cgimage);
  CGDataProviderRelease(cgdata_provider);
  CFRelease(cfdata);
}

@end


void plask_setup_bindings(v8::Handle<v8::ObjectTemplate> obj) {
  obj->Set(v8::String::New("NSWindow"), NSWindowWrapper::GetTemplate());
  obj->Set(v8::String::New("NSEvent"), NSEventWrapper::GetTemplate());
  obj->Set(v8::String::New("SkPath"), SkPathWrapper::GetTemplate());
  obj->Set(v8::String::New("SkPaint"), SkPaintWrapper::GetTemplate());
  obj->Set(v8::String::New("SkBitmap"), SkBitmapWrapper::GetTemplate());
  obj->Set(v8::String::New("SkCanvas"), SkCanvasWrapper::GetTemplate());
  obj->Set(v8::String::New("NVG"), NVGWrapper::GetTemplate());
  obj->Set(v8::String::New("NSOpenGLContext"),
           NSOpenGLContextWrapper::GetTemplate());
  obj->Set(v8::String::New("NSSound"), NSSoundWrapper::GetTemplate());
  obj->Set(v8::String::New("CAMIDISource"), CAMIDISourceWrapper::GetTemplate());
  obj->Set(v8::String::New("CAMIDIDestination"),
           CAMIDIDestinationWrapper::GetTemplate());
  obj->Set(v8::String::New("SBApplication"),
           SBApplicationWrapper::GetTemplate());
  obj->Set(v8::String::New("NSAppleScript"),
           NSAppleScriptWrapper::GetTemplate());
}
