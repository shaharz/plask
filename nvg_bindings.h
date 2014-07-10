#include "bindings_common.h"
#include "nanovg.h"
#define NANOVG_GL2_IMPLEMENTATION
#include "nanovg_gl.h"
#include "nanovg_gl_utils.h"

class NVGWrapper {
public:
    static v8::Persistent<v8::FunctionTemplate> GetTemplate() {
        static v8::Persistent<v8::FunctionTemplate> ft_cache;
        if (!ft_cache.IsEmpty())
            return ft_cache;
        
        v8::HandleScope scope;
        ft_cache = v8::Persistent<v8::FunctionTemplate>::New(
                                                             v8::FunctionTemplate::New(&NVGWrapper::V8New));
        v8::Local<v8::ObjectTemplate> instance = ft_cache->InstanceTemplate();
        instance->SetInternalFieldCount(1);
        
        v8::Local<v8::Signature> default_signature = v8::Signature::New(ft_cache);
        
        // Configure the template...
        static BatchedConstants constants[] = {
            { "NVG_STENCIL_STROKES", NVG_STENCIL_STROKES },
            { "NVG_ANTIALIAS", NVG_ANTIALIAS },
            { "NVG_ALIGN_LEFT", NVG_ALIGN_LEFT },
            { "NVG_ALIGN_CENTER", NVG_ALIGN_CENTER },
            { "NVG_ALIGN_RIGHT", NVG_ALIGN_RIGHT },
            { "NVG_ALIGN_TOP", NVG_ALIGN_TOP },
            { "NVG_ALIGN_MIDDLE", NVG_ALIGN_MIDDLE },
            { "NVG_ALIGN_BOTTOM", NVG_ALIGN_BOTTOM },
            { "NVG_ALIGN_BASELINE", NVG_ALIGN_BASELINE },
            { "NVG_BUTT", NVG_BUTT },
            { "NVG_ROUND", NVG_ROUND },
            { "NVG_SQUARE", NVG_SQUARE },
            { "NVG_BEVEL", NVG_BEVEL },
            { "NVG_MITER", NVG_MITER },
            { "NVG_SOLID", NVG_SOLID },
            { "NVG_HOLE", NVG_HOLE },
            { "NVG_CCW", NVG_CCW },
            { "NVG_CW", NVG_CW },
        };
        
        static BatchedMethods methods[] = {
            { "beginFrame", &NVGWrapper::beginFrame },
            { "endFrame", &NVGWrapper::endFrame },
            { "beginPath", &NVGWrapper::beginPath },
            { "rect", &NVGWrapper::rect },
            { "ellipse", &NVGWrapper::ellipse },
            { "circle", &NVGWrapper::circle },
            { "moveTo", &NVGWrapper::moveTo },
            { "lineTo", &NVGWrapper::lineTo },
            { "bezierTo", &NVGWrapper::bezierTo },
            { "arc", &NVGWrapper::arc },
            { "arcTo", &NVGWrapper::arcTo },
            { "roundedRect", &NVGWrapper::roundedRect },
            { "closePath", &NVGWrapper::closePath },
            
            { "strokeColor", &NVGWrapper::strokeColor },
            { "strokeWidth", &NVGWrapper::strokeWidth },
            { "stroke", &NVGWrapper::stroke },
            { "fillColor", &NVGWrapper::fillColor },
            { "fillColorHSL", &NVGWrapper::fillColorHSL },
            { "fill", &NVGWrapper::fill },
            { "miterLimit", &NVGWrapper::miterLimit },
            { "lineCap", &NVGWrapper::lineCap },
            { "lineJoin", &NVGWrapper::lineJoin },
            { "globalAlpha", &NVGWrapper::globalAlpha },
            
            { "resetTransform", &NVGWrapper::resetTransform },
            { "transform", &NVGWrapper::transform },
            { "translate", &NVGWrapper::translate },
            { "rotate", &NVGWrapper::rotate },
            { "skewX", &NVGWrapper::skewX },
            { "skewY", &NVGWrapper::skewY },
            { "scale", &NVGWrapper::scale },
            
            { "save", &NVGWrapper::save },
            { "restore", &NVGWrapper::restore },
            { "reset", &NVGWrapper::reset },
            
            { "createFont", &NVGWrapper::createFont },
            { "findFont", &NVGWrapper::findFont },
            { "fontSize", &NVGWrapper::fontSize },
            { "fontBlur", &NVGWrapper::fontBlur },
            { "textLetterSpacing", &NVGWrapper::textLetterSpacing },
            { "textLineHeight", &NVGWrapper::textLineHeight },
            { "textAlign", &NVGWrapper::textAlign },
            { "fontFaceId", &NVGWrapper::fontFaceId },
            { "fontFace", &NVGWrapper::fontFace },
            { "text", &NVGWrapper::text },
            { "textBox", &NVGWrapper::textBox },
            { "textBounds", &NVGWrapper::textBounds },
            { "textBoxBounds", &NVGWrapper::textBoxBounds },
            
            // TODO: missing functions
            // - image functions
            // - scissor clip
            // - proper NVGpaint (gradients, images)
            // - proper NVGcolor (solid colors)
            // - text metrics/glyph positions
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
    
    static NVGcontext* ExtractPointer(v8::Handle<v8::Object> obj) {
        return reinterpret_cast<NVGcontext*>(obj->GetPointerFromInternalField(0));
    }
    
    static bool HasInstance(v8::Handle<v8::Value> value) {
        return GetTemplate()->HasInstance(value);
    }
    
private:
    static void WeakCallback(v8::Persistent<v8::Value> value, void* data) {
        v8::Handle<v8::Object> obj = v8::Handle<v8::Object>::Cast(value);
        NVGcontext* context = ExtractPointer(obj);
        
        value.ClearWeak();
        value.Dispose();
        
        // Delete the backing SkCanvas object.  Skia reference counting should
        // handle cleaning up deeper resources (for example the backing pixels).
        nvgDeleteGL2(context);
    }
    
    static v8::Handle<v8::Value> V8New(const v8::Arguments& args) {
        if (!args.IsConstructCall())
            return v8_utils::ThrowTypeError(kMsgNonConstructCall);
        
        NVGcontext* context;
        //        if (args.Length() == 2) {  // width / height offscreen constructor.
        //            unsigned int width = args[0]->Uint32Value();
        //            unsigned int height = args[1]->Uint32Value();
        context = nvgCreateGL2(NVG_ANTIALIAS);
        //        } else {
        //            return v8_utils::ThrowError("Improper NVG constructor arguments.");
        //        }
        
        args.This()->SetPointerInInternalField(0, context);
        
        v8::Persistent<v8::Object> persistent =
        v8::Persistent<v8::Object>::New(args.This());
        persistent.MakeWeak(NULL, &NVGWrapper::WeakCallback);
        
        return args.This();
    }
    
    static v8::Handle<v8::Value> beginFrame(const v8::Arguments& args) {
        NVGcontext* context = ExtractPointer(args.Holder());
        nvgBeginFrame(context,
                      args[0]->NumberValue(),
                      args[1]->NumberValue(),
                      args[2]->NumberValue());
        return v8::Undefined();
    }
    
    static v8::Handle<v8::Value> endFrame(const v8::Arguments& args) {
        NVGcontext* context = ExtractPointer(args.Holder());
        nvgEndFrame(context);
        return v8::Undefined();
    }
    
    static v8::Handle<v8::Value> beginPath(const v8::Arguments& args) {
        NVGcontext* context = ExtractPointer(args.Holder());
        nvgBeginPath(context);
        return v8::Undefined();
    }
    
    static v8::Handle<v8::Value> save(const v8::Arguments& args) {
        NVGcontext* context = ExtractPointer(args.Holder());
        nvgSave(context);
        return v8::Undefined();
    }
    
    static v8::Handle<v8::Value> restore(const v8::Arguments& args) {
        NVGcontext* context = ExtractPointer(args.Holder());
        nvgRestore(context);
        return v8::Undefined();
    }
    
    static v8::Handle<v8::Value> reset(const v8::Arguments& args) {
        NVGcontext* context = ExtractPointer(args.Holder());
        nvgReset(context);
        return v8::Undefined();
    }
    
    static v8::Handle<v8::Value> rect(const v8::Arguments& args) {
        NVGcontext* context = ExtractPointer(args.Holder());
        nvgRect(context,
                args[0]->NumberValue(),
                args[1]->NumberValue(),
                args[2]->NumberValue(),
                args[3]->NumberValue());
        return v8::Undefined();
    }
    
    static v8::Handle<v8::Value> ellipse(const v8::Arguments& args) {
        NVGcontext* context = ExtractPointer(args.Holder());
        nvgEllipse(context,
                   args[0]->NumberValue(),
                   args[1]->NumberValue(),
                   args[2]->NumberValue(),
                   args[3]->NumberValue());
        return v8::Undefined();
    }
    
    static v8::Handle<v8::Value> circle(const v8::Arguments& args) {
        NVGcontext* context = ExtractPointer(args.Holder());
        nvgCircle(context,
                  args[0]->NumberValue(),
                  args[1]->NumberValue(),
                  args[2]->NumberValue());
        return v8::Undefined();
    }
    
    static v8::Handle<v8::Value> moveTo(const v8::Arguments& args) {
        NVGcontext* context = ExtractPointer(args.Holder());
        nvgMoveTo(context,
                  args[0]->NumberValue(),
                  args[1]->NumberValue());
        return v8::Undefined();
    }
    
    static v8::Handle<v8::Value> lineTo(const v8::Arguments& args) {
        NVGcontext* context = ExtractPointer(args.Holder());
        nvgLineTo(context,
                  args[0]->NumberValue(),
                  args[1]->NumberValue());
        return v8::Undefined();
    }
    
    static v8::Handle<v8::Value> fill(const v8::Arguments& args) {
        NVGcontext* context = ExtractPointer(args.Holder());
        nvgFill(context);
        return v8::Undefined();
    }
    
    static v8::Handle<v8::Value> fillColor(const v8::Arguments& args) {
        NVGcontext* context = ExtractPointer(args.Holder());
        nvgFillColor(context, nvgRGBAf(args[0]->NumberValue(),
                                       args[1]->NumberValue(),
                                       args[2]->NumberValue(),
                                       args[3]->NumberValue()));
        return v8::Undefined();
    }
    
    static v8::Handle<v8::Value> fillColorHSL(const v8::Arguments& args) {
        NVGcontext* context = ExtractPointer(args.Holder());
        nvgFillColor(context, nvgHSLA(args[0]->NumberValue(),
                                      args[1]->NumberValue(),
                                      args[2]->NumberValue(),
                                      args[3]->NumberValue()));
        return v8::Undefined();
    }
    
    static v8::Handle<v8::Value> stroke(const v8::Arguments& args) {
        NVGcontext* context = ExtractPointer(args.Holder());
        nvgStroke(context);
        return v8::Undefined();
    }
    
    static v8::Handle<v8::Value> strokeWidth(const v8::Arguments& args) {
        NVGcontext* context = ExtractPointer(args.Holder());
        nvgStrokeWidth(context, args[0]->NumberValue());
        return v8::Undefined();
    }
    
    static v8::Handle<v8::Value> strokeColor(const v8::Arguments& args) {
        NVGcontext* context = ExtractPointer(args.Holder());
        nvgStrokeColor(context, nvgRGBAf(args[0]->NumberValue(),
                                         args[1]->NumberValue(),
                                         args[2]->NumberValue(),
                                         args[3]->NumberValue()));
        return v8::Undefined();
    }
    
    static v8::Handle<v8::Value> miterLimit(const v8::Arguments& args) {
        NVGcontext* context = ExtractPointer(args.Holder());
        nvgMiterLimit(context, args[0]->NumberValue());
        return v8::Undefined();
    }
    
    static v8::Handle<v8::Value> lineCap(const v8::Arguments& args) {
        NVGcontext* context = ExtractPointer(args.Holder());
        nvgLineCap(context, args[0]->Int32Value());
        return v8::Undefined();
    }
    
    static v8::Handle<v8::Value> lineJoin(const v8::Arguments& args) {
        NVGcontext* context = ExtractPointer(args.Holder());
        nvgLineJoin(context, args[0]->Int32Value());
        return v8::Undefined();
    }
    
    static v8::Handle<v8::Value> globalAlpha(const v8::Arguments& args) {
        NVGcontext* context = ExtractPointer(args.Holder());
        nvgGlobalAlpha(context, args[0]->NumberValue());
        return v8::Undefined();
    }
    
    
    static v8::Handle<v8::Value> resetTransform(const v8::Arguments& args) {
        NVGcontext* context = ExtractPointer(args.Holder());
        nvgResetTransform(context);
        return v8::Undefined();
    }
    
    static v8::Handle<v8::Value> transform(const v8::Arguments& args) {
        NVGcontext* context = ExtractPointer(args.Holder());
        nvgTransform(context,
                     args[0]->NumberValue(),
                     args[1]->NumberValue(),
                     args[2]->NumberValue(),
                     args[3]->NumberValue(),
                     args[4]->NumberValue(),
                     args[5]->NumberValue());
        
        return v8::Undefined();
    }
    
    static v8::Handle<v8::Value> translate(const v8::Arguments& args) {
        NVGcontext* context = ExtractPointer(args.Holder());
        nvgTranslate(context, args[0]->NumberValue(), args[1]->NumberValue());
        return v8::Undefined();
    }
    
    static v8::Handle<v8::Value> rotate(const v8::Arguments& args) {
        NVGcontext* context = ExtractPointer(args.Holder());
        nvgRotate(context, args[0]->NumberValue());
        return v8::Undefined();
    }
    
    static v8::Handle<v8::Value> skewX(const v8::Arguments& args) {
        NVGcontext* context = ExtractPointer(args.Holder());
        nvgSkewX(context, args[0]->NumberValue());
        return v8::Undefined();
    }
    
    static v8::Handle<v8::Value> skewY(const v8::Arguments& args) {
        NVGcontext* context = ExtractPointer(args.Holder());
        nvgSkewY(context, args[0]->NumberValue());
        return v8::Undefined();
    }
    
    static v8::Handle<v8::Value> scale(const v8::Arguments& args) {
        NVGcontext* context = ExtractPointer(args.Holder());
        nvgScale(context, args[0]->NumberValue(), args[1]->NumberValue());
        return v8::Undefined();
    }
    
    static v8::Handle<v8::Value> bezierTo(const v8::Arguments& args) {
        NVGcontext* context = ExtractPointer(args.Holder());
        nvgBezierTo(context,
                    args[0]->NumberValue(),
                    args[1]->NumberValue(),
                    args[2]->NumberValue(),
                    args[3]->NumberValue(),
                    args[4]->NumberValue(),
                    args[5]->NumberValue());
        
        return v8::Undefined();
    }
    
    static v8::Handle<v8::Value> arcTo(const v8::Arguments& args) {
        NVGcontext* context = ExtractPointer(args.Holder());
        nvgArcTo(context,
                 args[0]->NumberValue(),
                 args[1]->NumberValue(),
                 args[2]->NumberValue(),
                 args[3]->NumberValue(),
                 args[4]->NumberValue());
        
        return v8::Undefined();
    }
    
    static v8::Handle<v8::Value> arc(const v8::Arguments& args) {
        NVGcontext* context = ExtractPointer(args.Holder());
        nvgArc(context,
               args[0]->NumberValue(),
               args[1]->NumberValue(),
               args[2]->NumberValue(),
               args[3]->NumberValue(),
               args[4]->NumberValue(),
               args[5]->NumberValue());
        
        return v8::Undefined();
    }
    
    static v8::Handle<v8::Value> roundedRect(const v8::Arguments& args) {
        NVGcontext* context = ExtractPointer(args.Holder());
        nvgRoundedRect(context,
                       args[0]->NumberValue(),
                       args[1]->NumberValue(),
                       args[2]->NumberValue(),
                       args[3]->NumberValue(),
                       args[4]->NumberValue());
        
        return v8::Undefined();
    }
    
    static v8::Handle<v8::Value> closePath(const v8::Arguments& args) {
        NVGcontext* context = ExtractPointer(args.Holder());
        nvgClosePath(context);
        
        return v8::Undefined();
    }
    
    static v8::Handle<v8::Value> createFont(const v8::Arguments& args) {
        NVGcontext* context = ExtractPointer(args.Holder());
        v8::String::Utf8Value a1(args[0]);
        v8::String::Utf8Value a2(args[1]);
        int ret = nvgCreateFont(context, *a1, *a2);
        
        return v8::Integer::New(ret);
    }
    
    static v8::Handle<v8::Value> findFont(const v8::Arguments& args) {
        NVGcontext* context = ExtractPointer(args.Holder());
        v8::String::Utf8Value a1(args[0]);
        int ret = nvgFindFont(context, *a1);
        
        return v8::Integer::New(ret);
    }
    
    static v8::Handle<v8::Value> fontSize(const v8::Arguments& args) {
        NVGcontext* context = ExtractPointer(args.Holder());
        nvgFontSize(context, args[0]->NumberValue());
        return v8::Undefined();
    }
    
    static v8::Handle<v8::Value> fontBlur(const v8::Arguments& args) {
        NVGcontext* context = ExtractPointer(args.Holder());
        nvgFontBlur(context, args[0]->NumberValue());
        return v8::Undefined();
    }
    
    static v8::Handle<v8::Value> textLetterSpacing(const v8::Arguments& args) {
        NVGcontext* context = ExtractPointer(args.Holder());
        nvgTextLetterSpacing(context, args[0]->NumberValue());
        return v8::Undefined();
    }
    
    static v8::Handle<v8::Value> textLineHeight(const v8::Arguments& args) {
        NVGcontext* context = ExtractPointer(args.Holder());
        nvgTextLineHeight(context, args[0]->NumberValue());
        return v8::Undefined();
    }
    
    static v8::Handle<v8::Value> textAlign(const v8::Arguments& args) {
        NVGcontext* context = ExtractPointer(args.Holder());
        nvgTextAlign(context, args[0]->Int32Value());
        return v8::Undefined();
    }
    
    static v8::Handle<v8::Value> fontFaceId(const v8::Arguments& args) {
        NVGcontext* context = ExtractPointer(args.Holder());
        nvgTextLetterSpacing(context, args[0]->Int32Value());
        return v8::Undefined();
    }
    
    static v8::Handle<v8::Value> fontFace(const v8::Arguments& args) {
        NVGcontext* context = ExtractPointer(args.Holder());
        v8::String::Utf8Value a1(args[0]);
        nvgFontFace(context, *a1);
        
        return v8::Undefined();
    }
    
    static v8::Handle<v8::Value> text(const v8::Arguments& args) {
        NVGcontext* context = ExtractPointer(args.Holder());
        v8::String::Utf8Value a2(args[2]);
        float ret = nvgText(context,
                            args[0]->NumberValue(),
                            args[1]->NumberValue(),
                            *a2, NULL);
        
        return v8::Number::New(ret);
    }
    
    static v8::Handle<v8::Value> textBox(const v8::Arguments& args) {
        NVGcontext* context = ExtractPointer(args.Holder());
        v8::String::Utf8Value a3(args[3]);
        nvgTextBox(context,
                   args[0]->NumberValue(),
                   args[1]->NumberValue(),
                   args[2]->NumberValue(),
                   *a3, NULL);
        
        return v8::Undefined();
    }
    
    static v8::Handle<v8::Value> textBounds(const v8::Arguments& args) {
        NVGcontext* context = ExtractPointer(args.Holder());
        v8::String::Utf8Value a2(args[2]);
        auto v8bounds = v8::Array::New(4);
        float bounds[4];
        
        float ret = nvgTextBounds(context,
                                  args[0]->NumberValue(),
                                  args[1]->NumberValue(),
                                  *a2,
                                  NULL,
                                  bounds);
        
        for (int i = 0; i < 4; i++) {
            v8bounds->Set(i, v8::Number::New(bounds[i]));
        }
        
        // TODO: handle return value
        
        return v8bounds;
    }
    
    static v8::Handle<v8::Value> textBoxBounds(const v8::Arguments& args) {
        NVGcontext* context = ExtractPointer(args.Holder());
        v8::String::Utf8Value a3(args[3]);
        auto v8bounds = v8::Array::New(4);
        float bounds[4];
        
        nvgTextBoxBounds(context,
                         args[0]->NumberValue(),
                         args[1]->NumberValue(),
                         args[2]->NumberValue(),
                         *a3,
                         NULL,
                         bounds);
        
        for (int i = 0; i < 4; i++) {
            v8bounds->Set(i, v8::Number::New(bounds[i]));
        }
        
        return v8bounds;
    }
    
    //    // Calculates the glyph x positions of the specified text. If end is specified only the sub-string will be used.
    //    // Measured values are returned in local coordinate space.
    //    int nvgTextGlyphPositions(struct NVGcontext* ctx, float x, float y, const char* string, const char* end, struct NVGglyphPosition* positions, int maxPositions);
    //
    //    // Returns the vertical metrics based on the current text style.
    //    // Measured values are returned in local coordinate space.
    //    void nvgTextMetrics(struct NVGcontext* ctx, float* ascender, float* descender, float* lineh);
    //
    //    // Breaks the specified text into lines. If end is specified only the sub-string will be used.
    //    // White space is stripped at the beginning of the rows, the text is split at word boundaries or when new-line characters are encountered.
    //    // Words longer than the max width are slit at nearest character (i.e. no hyphenation).
    //    int nvgTextBreakLines(struct NVGcontext* ctx, const char* string, const char* end, float breakRowWidth, struct NVGtextRow* rows, int maxRows);
};
