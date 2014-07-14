#include "bindings_common.h"
#include "TuioClient.h"

class TuioClientWrapper {
public:
    static v8::Persistent<v8::FunctionTemplate> GetTemplate() {
        static v8::Persistent<v8::FunctionTemplate> ft_cache;
        if (!ft_cache.IsEmpty())
            return ft_cache;
        
        v8::HandleScope scope;
        ft_cache = v8::Persistent<v8::FunctionTemplate>::New(v8::FunctionTemplate::New(&TuioClientWrapper::V8New));
        v8::Local<v8::ObjectTemplate> instance = ft_cache->InstanceTemplate();
        instance->SetInternalFieldCount(1);  // TuioClientWrapper pointer.
        
        v8::Local<v8::Signature> default_signature = v8::Signature::New(ft_cache);
        
        static BatchedMethods methods[] = {
            { "width", &TuioClientWrapper::width },
            { "height", &TuioClientWrapper::height },
            { "draw", &TuioClientWrapper::draw },
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
        persistent.MakeWeak(NULL, &TuioClientWrapper::WeakCallback);
        
        return args.This();
    }
};