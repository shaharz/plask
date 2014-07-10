#include "bindings_common.h"

class WebGLActiveInfo {
public:
    static v8::Persistent<v8::FunctionTemplate> GetTemplate() {
        static v8::Persistent<v8::FunctionTemplate> ft_cache;
        if (!ft_cache.IsEmpty())
            return ft_cache;
        
        v8::HandleScope scope;
        ft_cache = v8::Persistent<v8::FunctionTemplate>::New(
                                                             v8::FunctionTemplate::New(&WebGLActiveInfo::V8New));
        ft_cache->SetClassName(v8::String::New("WebGLActiveInfo"));
        v8::Local<v8::ObjectTemplate> instance = ft_cache->InstanceTemplate();
        instance->SetInternalFieldCount(0);
        
        v8::Local<v8::Signature> default_signature = v8::Signature::New(ft_cache);
        
        return ft_cache;
    }
    
    static bool HasInstance(v8::Handle<v8::Value> value) {
        return GetTemplate()->HasInstance(value);
    }
    
    static v8::Handle<v8::Value> NewFromSizeTypeName(GLint size,
                                                     GLenum type,
                                                     const char* name) {
        v8::Local<v8::Object> obj = WebGLActiveInfo::GetTemplate()->
        InstanceTemplate()->NewInstance();
        obj->Set(v8::String::New("size"), v8::Integer::New(size), v8::ReadOnly);
        obj->Set(v8::String::New("type"),
                 v8::Integer::NewFromUnsigned(type),
                 v8::ReadOnly);
        obj->Set(v8::String::New("name"), v8::String::New(name), v8::ReadOnly);
        return obj;
    }
    
private:
    static v8::Handle<v8::Value> V8New(const v8::Arguments& args) {
        if (!args.IsConstructCall())
            return v8_utils::ThrowTypeError(kMsgNonConstructCall);
        
        return args.This();
    }
};

class WebGLProgram {
public:
    static v8::Persistent<v8::FunctionTemplate> GetTemplate() {
        static v8::Persistent<v8::FunctionTemplate> ft_cache;
        if (!ft_cache.IsEmpty())
            return ft_cache;
        
        v8::HandleScope scope;
        ft_cache = v8::Persistent<v8::FunctionTemplate>::New(
                                                             v8::FunctionTemplate::New(&WebGLProgram::V8New));
        ft_cache->SetClassName(v8::String::New("WebGLProgram"));
        v8::Local<v8::ObjectTemplate> instance = ft_cache->InstanceTemplate();
        instance->SetInternalFieldCount(1);  // GLuint name.
        
        return ft_cache;
    }
    
    static bool HasInstance(v8::Handle<v8::Value> value) {
        return GetTemplate()->HasInstance(value);
    }
    
    static v8::Handle<v8::Value> NewFromProgramName(
                                                    GLuint program) {
        v8::Local<v8::Object> obj = WebGLProgram::GetTemplate()->
        InstanceTemplate()->NewInstance();
        obj->SetInternalField(0, v8::Integer::NewFromUnsigned(program));
        program_map.insert(std::pair<GLuint, v8::Persistent<v8::Value> >(
                                                                         program, v8::Persistent<v8::Value>::New(obj)));
        return obj;
    }
    
    static v8::Handle<v8::Value> LookupFromProgramName(
                                                       GLuint program) {
        if (program != 0 && program_map.count(program) == 1)
            return program_map[program];
        return v8::Null();
    }
    
    // Use to set the name to 0, when it is deleted, for example.
    static void ClearProgramName(v8::Handle<v8::Value> value) {
        GLuint program = ExtractProgramNameFromValue(value);
        if (program != 0) {
            if (program_map.count(program) == 1) {
                program_map[program].Dispose();
                if (program_map.erase(program) != 1) {
                    printf("Warning: Should have erased program map entry.\n");
                }
            } else {
                printf("Warning: Should have disposed program map handle.\n");
            }
        }
        return v8::Handle<v8::Object>::Cast(value)->
        SetInternalField(0, v8::Integer::NewFromUnsigned(0));
    }
    
    static GLuint ExtractProgramNameFromValue(
                                              v8::Handle<v8::Value> value) {
        if (value->IsNull()) return 0;
        return v8::Handle<v8::Object>::Cast(value)->
        GetInternalField(0)->Uint32Value();
    }
    
private:
    static std::map<GLuint, v8::Persistent<v8::Value> > program_map;
    
    static v8::Handle<v8::Value> V8New(const v8::Arguments& args) {
        if (!args.IsConstructCall())
            return v8_utils::ThrowTypeError(kMsgNonConstructCall);
        
        // TODO(deanm): How to throw an exception when called from JavaScript?
        // For now we don't expose the object directly, so maybe it's okay
        // (although I suppose you can still get to it from an instance)...
        //return v8_utils::ThrowTypeError("Type error.");
        
        // Initially set to 0.
        args.This()->SetInternalField(0, v8::Integer::NewFromUnsigned(0));
        
        return args.This();
    }
};

std::map<GLuint, v8::Persistent<v8::Value> > WebGLProgram::program_map;


class WebGLShader {
public:
    static v8::Persistent<v8::FunctionTemplate> GetTemplate() {
        static v8::Persistent<v8::FunctionTemplate> ft_cache;
        if (!ft_cache.IsEmpty())
            return ft_cache;
        
        v8::HandleScope scope;
        ft_cache = v8::Persistent<v8::FunctionTemplate>::New(
                                                             v8::FunctionTemplate::New(&WebGLShader::V8New));
        ft_cache->SetClassName(v8::String::New("WebGLShader"));
        v8::Local<v8::ObjectTemplate> instance = ft_cache->InstanceTemplate();
        instance->SetInternalFieldCount(1);  // GLuint name.
        
        return ft_cache;
    }
    
    static bool HasInstance(v8::Handle<v8::Value> value) {
        return GetTemplate()->HasInstance(value);
    }
    
    static v8::Handle<v8::Value> NewFromShaderName(
                                                   GLuint shader) {
        v8::Local<v8::Object> obj = WebGLShader::GetTemplate()->
        InstanceTemplate()->NewInstance();
        obj->SetInternalField(0, v8::Integer::NewFromUnsigned(shader));
        shader_map.insert(std::pair<GLuint, v8::Persistent<v8::Value> >(
                                                                        shader, v8::Persistent<v8::Value>::New(obj)));
        return obj;
    }
    
    static v8::Handle<v8::Value> LookupFromShaderName(
                                                      GLuint shader) {
        if (shader != 0 && shader_map.count(shader) == 1)
            return shader_map[shader];
        return v8::Null();
    }
    
    // Use to set the name to 0, when it is deleted, for example.
    static void ClearShaderName(v8::Handle<v8::Value> value) {
        GLuint shader = ExtractShaderNameFromValue(value);
        if (shader != 0) {
            if (shader_map.count(shader) == 1) {
                shader_map[shader].Dispose();
                if (shader_map.erase(shader) != 1) {
                    printf("Warning: Should have erased shader map entry.\n");
                }
            } else {
                printf("Warning: Should have disposed shader map handle.\n");
            }
        }
        return v8::Handle<v8::Object>::Cast(value)->
        SetInternalField(0, v8::Integer::NewFromUnsigned(0));
    }
    
    static GLuint ExtractShaderNameFromValue(
                                             v8::Handle<v8::Value> value) {
        if (value->IsNull()) return 0;
        return v8::Handle<v8::Object>::Cast(value)->
        GetInternalField(0)->Uint32Value();
    }
    
private:
    static std::map<GLuint, v8::Persistent<v8::Value> > shader_map;
    
    static v8::Handle<v8::Value> V8New(const v8::Arguments& args) {
        if (!args.IsConstructCall())
            return v8_utils::ThrowTypeError(kMsgNonConstructCall);
        
        // TODO(deanm): How to throw an exception when called from JavaScript?
        // For now we don't expose the object directly, so maybe it's okay
        // (although I suppose you can still get to it from an instance)...
        //return v8_utils::ThrowTypeError("Type error.");
        
        // Initially set to 0.
        args.This()->SetInternalField(0, v8::Integer::NewFromUnsigned(0));
        
        return args.This();
    }
};

std::map<GLuint, v8::Persistent<v8::Value> > WebGLShader::shader_map;


class WebGLUniformLocation {
public:
    static v8::Persistent<v8::FunctionTemplate> GetTemplate() {
        static v8::Persistent<v8::FunctionTemplate> ft_cache;
        if (!ft_cache.IsEmpty())
            return ft_cache;
        
        v8::HandleScope scope;
        ft_cache = v8::Persistent<v8::FunctionTemplate>::New(
                                                             v8::FunctionTemplate::New(&WebGLUniformLocation::V8New));
        ft_cache->SetClassName(v8::String::New("WebGLUniformLocation"));
        v8::Local<v8::ObjectTemplate> instance = ft_cache->InstanceTemplate();
        instance->SetInternalFieldCount(1);  // GLint location.
        
        v8::Local<v8::Signature> default_signature = v8::Signature::New(ft_cache);
        
        return ft_cache;
    }
    
    static bool HasInstance(v8::Handle<v8::Value> value) {
        return GetTemplate()->HasInstance(value);
    }
    
    static v8::Handle<v8::Value> NewFromLocation(GLint location) {
        v8::Local<v8::Object> obj = WebGLUniformLocation::GetTemplate()->
        InstanceTemplate()->NewInstance();
        obj->SetInternalField(0, v8::Integer::New(location));
        return obj;
    }
    
    static GLint ExtractLocationFromValue(v8::Handle<v8::Value> value) {
        return v8::Handle<v8::Object>::Cast(value)->
        GetInternalField(0)->Int32Value();
    }
    
private:
    static v8::Handle<v8::Value> V8New(const v8::Arguments& args) {
        if (!args.IsConstructCall())
            return v8_utils::ThrowTypeError(kMsgNonConstructCall);
        
        // TODO(deanm): How to throw an exception when called from JavaScript but
        // not from NewFromLocation?
        //return v8_utils::ThrowTypeError("Type error.");
        return args.This();
    }
};


class WebGLFramebuffer {
public:
    static v8::Persistent<v8::FunctionTemplate> GetTemplate() {
        static v8::Persistent<v8::FunctionTemplate> ft_cache;
        if (!ft_cache.IsEmpty())
            return ft_cache;
        
        v8::HandleScope scope;
        ft_cache = v8::Persistent<v8::FunctionTemplate>::New(
                                                             v8::FunctionTemplate::New(&WebGLFramebuffer::V8New));
        ft_cache->SetClassName(v8::String::New("WebGLFramebuffer"));
        v8::Local<v8::ObjectTemplate> instance = ft_cache->InstanceTemplate();
        instance->SetInternalFieldCount(1);  // GLuint name.
        
        return ft_cache;
    }
    
    static bool HasInstance(v8::Handle<v8::Value> value) {
        return GetTemplate()->HasInstance(value);
    }
    
    static v8::Handle<v8::Value> NewFromFramebufferObjectName(
                                                              GLuint framebuffer) {
        v8::Local<v8::Object> obj = WebGLFramebuffer::GetTemplate()->
        InstanceTemplate()->NewInstance();
        obj->SetInternalField(0, v8::Integer::NewFromUnsigned(framebuffer));
        framebuffer_map.insert(std::pair<GLuint, v8::Persistent<v8::Value> >(
                                                                             framebuffer, v8::Persistent<v8::Value>::New(obj)));
        return obj;
    }
    
    static v8::Handle<v8::Value> LookupFromFramebufferObjectName(
                                                                 GLuint framebuffer) {
        if (framebuffer != 0 && framebuffer_map.count(framebuffer) == 1)
            return framebuffer_map[framebuffer];
        return v8::Null();
    }
    
    // Use to set the object name to 0, when it is deleted, for example.
    static void ClearFramebufferObjectName(v8::Handle<v8::Value> value) {
        GLuint framebuffer = ExtractFramebufferObjectNameFromValue(value);
        if (framebuffer != 0) {
            if (framebuffer_map.count(framebuffer) == 1) {
                framebuffer_map[framebuffer].Dispose();
                if (framebuffer_map.erase(framebuffer) != 1) {
                    printf("Warning: Should have erased framebuffer map entry.\n");
                }
            } else {
                printf("Warning: Should have disposed framebuffer map handle.\n");
            }
        }
        return v8::Handle<v8::Object>::Cast(value)->
        SetInternalField(0, v8::Integer::NewFromUnsigned(0));
    }
    
    static GLuint ExtractFramebufferObjectNameFromValue(
                                                        v8::Handle<v8::Value> value) {
        if (value->IsNull()) return 0;
        return v8::Handle<v8::Object>::Cast(value)->
        GetInternalField(0)->Uint32Value();
    }
    
private:
    // If we call getParameter(FRAMEBUFFER_BINDING) twice, for example, we need
    // to get the same wrapper object (not a newly created one) as the one we
    // got from the call to frameFramebuffer().  (This is the WebGL spec).  So,
    // we must track a mapping between OpenGL GLuint "framebuffer object name"
    // and the wrapper objects.
    static std::map<GLuint, v8::Persistent<v8::Value> > framebuffer_map;
    
    static v8::Handle<v8::Value> V8New(const v8::Arguments& args) {
        if (!args.IsConstructCall())
            return v8_utils::ThrowTypeError(kMsgNonConstructCall);
        
        // TODO(deanm): How to throw an exception when called from JavaScript?
        // For now we don't expose the object directly, so maybe it's okay
        // (although I suppose you can still get to it from an instance)...
        //return v8_utils::ThrowTypeError("Type error.");
        
        // Initially set to 0.
        args.This()->SetInternalField(0, v8::Integer::NewFromUnsigned(0));
        
        return args.This();
    }
};

std::map<GLuint, v8::Persistent<v8::Value> > WebGLFramebuffer::framebuffer_map;


class WebGLTexture {
public:
    static v8::Persistent<v8::FunctionTemplate> GetTemplate() {
        static v8::Persistent<v8::FunctionTemplate> ft_cache;
        if (!ft_cache.IsEmpty())
            return ft_cache;
        
        v8::HandleScope scope;
        ft_cache = v8::Persistent<v8::FunctionTemplate>::New(
                                                             v8::FunctionTemplate::New(&WebGLTexture::V8New));
        ft_cache->SetClassName(v8::String::New("WebGLTexture"));
        v8::Local<v8::ObjectTemplate> instance = ft_cache->InstanceTemplate();
        instance->SetInternalFieldCount(1);  // GLuint name.
        
        return ft_cache;
    }
    
    static bool HasInstance(v8::Handle<v8::Value> value) {
        return GetTemplate()->HasInstance(value);
    }
    
    static v8::Handle<v8::Value> NewFromTextureName(
                                                    GLuint texture) {
        v8::Local<v8::Object> obj = WebGLTexture::GetTemplate()->
        InstanceTemplate()->NewInstance();
        obj->SetInternalField(0, v8::Integer::NewFromUnsigned(texture));
        texture_map.insert(std::pair<GLuint, v8::Persistent<v8::Value> >(
                                                                         texture, v8::Persistent<v8::Value>::New(obj)));
        return obj;
    }
    
    static v8::Handle<v8::Value> LookupFromTextureName(
                                                       GLuint texture) {
        if (texture != 0 && texture_map.count(texture) == 1)
            return texture_map[texture];
        return v8::Null();
    }
    
    // Use to set the name to 0, when it is deleted, for example.
    static void ClearTextureName(v8::Handle<v8::Value> value) {
        GLuint texture = ExtractTextureNameFromValue(value);
        if (texture != 0) {
            if (texture_map.count(texture) == 1) {
                texture_map[texture].Dispose();
                if (texture_map.erase(texture) != 1) {
                    printf("Warning: Should have erased texture map entry.\n");
                }
            } else {
                printf("Warning: Should have disposed texture map handle.\n");
            }
        }
        return v8::Handle<v8::Object>::Cast(value)->
        SetInternalField(0, v8::Integer::NewFromUnsigned(0));
    }
    
    static GLuint ExtractTextureNameFromValue(
                                              v8::Handle<v8::Value> value) {
        if (value->IsNull()) return 0;
        return v8::Handle<v8::Object>::Cast(value)->
        GetInternalField(0)->Uint32Value();
    }
    
private:
    static std::map<GLuint, v8::Persistent<v8::Value> > texture_map;
    
    static v8::Handle<v8::Value> V8New(const v8::Arguments& args) {
        if (!args.IsConstructCall())
            return v8_utils::ThrowTypeError(kMsgNonConstructCall);
        
        // TODO(deanm): How to throw an exception when called from JavaScript?
        // For now we don't expose the object directly, so maybe it's okay
        // (although I suppose you can still get to it from an instance)...
        //return v8_utils::ThrowTypeError("Type error.");
        
        // Initially set to 0.
        args.This()->SetInternalField(0, v8::Integer::NewFromUnsigned(0));
        
        return args.This();
    }
};

std::map<GLuint, v8::Persistent<v8::Value> > WebGLTexture::texture_map;


class WebGLRenderbuffer {
public:
    static v8::Persistent<v8::FunctionTemplate> GetTemplate() {
        static v8::Persistent<v8::FunctionTemplate> ft_cache;
        if (!ft_cache.IsEmpty())
            return ft_cache;
        
        v8::HandleScope scope;
        ft_cache = v8::Persistent<v8::FunctionTemplate>::New(
                                                             v8::FunctionTemplate::New(&WebGLRenderbuffer::V8New));
        ft_cache->SetClassName(v8::String::New("WebGLRenderbuffer"));
        v8::Local<v8::ObjectTemplate> instance = ft_cache->InstanceTemplate();
        instance->SetInternalFieldCount(1);  // GLuint name.
        
        return ft_cache;
    }
    
    static bool HasInstance(v8::Handle<v8::Value> value) {
        return GetTemplate()->HasInstance(value);
    }
    
    static v8::Handle<v8::Value> NewFromRenderbufferObjectName(
                                                               GLuint renderbuffer) {
        v8::Local<v8::Object> obj = WebGLRenderbuffer::GetTemplate()->
        InstanceTemplate()->NewInstance();
        obj->SetInternalField(0, v8::Integer::NewFromUnsigned(renderbuffer));
        renderbuffer_map.insert(std::pair<GLuint, v8::Persistent<v8::Value> >(
                                                                              renderbuffer, v8::Persistent<v8::Value>::New(obj)));
        return obj;
    }
    
    static v8::Handle<v8::Value> LookupFromRenderbufferObjectName(
                                                                  GLuint renderbuffer) {
        if (renderbuffer != 0 && renderbuffer_map.count(renderbuffer) == 1)
            return renderbuffer_map[renderbuffer];
        return v8::Null();
    }
    
    // Use to set the object name to 0, when it is deleted, for example.
    static void ClearRenderbufferObjectName(v8::Handle<v8::Value> value) {
        GLuint renderbuffer = ExtractRenderbufferObjectNameFromValue(value);
        if (renderbuffer != 0) {
            if (renderbuffer_map.count(renderbuffer) == 1) {
                renderbuffer_map[renderbuffer].Dispose();
                if (renderbuffer_map.erase(renderbuffer) != 1) {
                    printf("Warning: Should have erased renderbuffer map entry.\n");
                }
            } else {
                printf("Warning: Should have disposed renderbuffer map handle.\n");
            }
        }
        return v8::Handle<v8::Object>::Cast(value)->
        SetInternalField(0, v8::Integer::NewFromUnsigned(0));
    }
    
    static GLuint ExtractRenderbufferObjectNameFromValue(
                                                         v8::Handle<v8::Value> value) {
        if (value->IsNull()) return 0;
        return v8::Handle<v8::Object>::Cast(value)->
        GetInternalField(0)->Uint32Value();
    }
    
private:
    static std::map<GLuint, v8::Persistent<v8::Value> > renderbuffer_map;
    
    static v8::Handle<v8::Value> V8New(const v8::Arguments& args) {
        if (!args.IsConstructCall())
            return v8_utils::ThrowTypeError(kMsgNonConstructCall);
        
        // TODO(deanm): How to throw an exception when called from JavaScript?
        // For now we don't expose the object directly, so maybe it's okay
        // (although I suppose you can still get to it from an instance)...
        //return v8_utils::ThrowTypeError("Type error.");
        
        // Initially set to 0.
        args.This()->SetInternalField(0, v8::Integer::NewFromUnsigned(0));
        
        return args.This();
    }
};

std::map<GLuint, v8::Persistent<v8::Value> >
WebGLRenderbuffer::renderbuffer_map;


class WebGLBuffer {
public:
    static v8::Persistent<v8::FunctionTemplate> GetTemplate() {
        static v8::Persistent<v8::FunctionTemplate> ft_cache;
        if (!ft_cache.IsEmpty())
            return ft_cache;
        
        v8::HandleScope scope;
        ft_cache = v8::Persistent<v8::FunctionTemplate>::New(
                                                             v8::FunctionTemplate::New(&WebGLBuffer::V8New));
        ft_cache->SetClassName(v8::String::New("WebGLBuffer"));
        v8::Local<v8::ObjectTemplate> instance = ft_cache->InstanceTemplate();
        instance->SetInternalFieldCount(1);  // GLuint name.
        
        return ft_cache;
    }
    
    static bool HasInstance(v8::Handle<v8::Value> value) {
        return GetTemplate()->HasInstance(value);
    }
    
    static v8::Handle<v8::Value> NewFromBufferName(
                                                   GLuint buffer) {
        v8::Local<v8::Object> obj = WebGLBuffer::GetTemplate()->
        InstanceTemplate()->NewInstance();
        obj->SetInternalField(0, v8::Integer::NewFromUnsigned(buffer));
        buffer_map.insert(std::pair<GLuint, v8::Persistent<v8::Value> >(
                                                                        buffer, v8::Persistent<v8::Value>::New(obj)));
        return obj;
    }
    
    static v8::Handle<v8::Value> LookupFromBufferName(
                                                      GLuint buffer) {
        if (buffer != 0 && buffer_map.count(buffer) == 1)
            return buffer_map[buffer];
        return v8::Null();
    }
    
    // Use to set the name to 0, when it is deleted, for example.
    static void ClearBufferName(v8::Handle<v8::Value> value) {
        GLuint buffer = ExtractBufferNameFromValue(value);
        if (buffer != 0) {
            if (buffer_map.count(buffer) == 1) {
                buffer_map[buffer].Dispose();
                if (buffer_map.erase(buffer) != 1) {
                    printf("Warning: Should have erased buffer map entry.\n");
                }
            } else {
                printf("Warning: Should have disposed buffer map handle.\n");
            }
        }
        return v8::Handle<v8::Object>::Cast(value)->
        SetInternalField(0, v8::Integer::NewFromUnsigned(0));
    }
    
    static GLuint ExtractBufferNameFromValue(
                                             v8::Handle<v8::Value> value) {
        if (value->IsNull()) return 0;
        return v8::Handle<v8::Object>::Cast(value)->
        GetInternalField(0)->Uint32Value();
    }
    
private:
    static std::map<GLuint, v8::Persistent<v8::Value> > buffer_map;
    
    static v8::Handle<v8::Value> V8New(const v8::Arguments& args) {
        if (!args.IsConstructCall())
            return v8_utils::ThrowTypeError(kMsgNonConstructCall);
        
        // TODO(deanm): How to throw an exception when called from JavaScript?
        // For now we don't expose the object directly, so maybe it's okay
        // (although I suppose you can still get to it from an instance)...
        //return v8_utils::ThrowTypeError("Type error.");
        
        // Initially set to 0.
        args.This()->SetInternalField(0, v8::Integer::NewFromUnsigned(0));
        
        return args.This();
    }
};

std::map<GLuint, v8::Persistent<v8::Value> > WebGLBuffer::buffer_map;


// TODO
// 5.12 WebGLShaderPrecisionFormat


class SyphonServerWrapper {
public:
    static v8::Persistent<v8::FunctionTemplate> GetTemplate() {
        static v8::Persistent<v8::FunctionTemplate> ft_cache;
        if (!ft_cache.IsEmpty())
            return ft_cache;
        
        v8::HandleScope scope;
        ft_cache = v8::Persistent<v8::FunctionTemplate>::New(
                                                             v8::FunctionTemplate::New(&SyphonServerWrapper::V8New));
        ft_cache->SetClassName(v8::String::New("SyphonServer"));
        v8::Local<v8::ObjectTemplate> instance = ft_cache->InstanceTemplate();
        instance->SetInternalFieldCount(1);  // SyphonServer
        
        v8::Local<v8::Signature> default_signature = v8::Signature::New(ft_cache);
        
        // Configure the template...
        static BatchedConstants constants[] = {
            { "kValue", 12 },
        };
        
        static BatchedMethods methods[] = {
            { "publishFrameTexture", &SyphonServerWrapper::publishFrameTexture },
            { "bindToDrawFrameOfSize", &SyphonServerWrapper::bindToDrawFrameOfSize },
            { "unbindAndPublish", &SyphonServerWrapper::unbindAndPublish },
            { "hasClients", &SyphonServerWrapper::hasClients },
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
    
    static bool HasInstance(v8::Handle<v8::Value> value) {
        return GetTemplate()->HasInstance(value);
    }
    
    static SyphonServer* ExtractSyphonServerPointer(v8::Handle<v8::Object> obj) {
        return reinterpret_cast<SyphonServer*>(obj->GetPointerFromInternalField(0));
    }
    
    static v8::Handle<v8::Value> NewFromSyphonServer(SyphonServer* server) {
        v8::Local<v8::Object> obj = SyphonServerWrapper::GetTemplate()->
        InstanceTemplate()->NewInstance();
        obj->SetPointerInInternalField(0, server);
        return obj;
    }
    
private:
    static v8::Handle<v8::Value> V8New(const v8::Arguments& args) {
        if (!args.IsConstructCall())
            return v8_utils::ThrowTypeError(kMsgNonConstructCall);
        
        return args.This();
    }
    
    static v8::Handle<v8::Value> publishFrameTexture(const v8::Arguments& args) {
        if (args.Length() != 9)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        SyphonServer* server = ExtractSyphonServerPointer(args.Holder());
        [server publishFrameTexture:args[0]->Uint32Value()
                      textureTarget:args[1]->Uint32Value()
                        imageRegion:NSMakeRect(args[2]->Int32Value(),
                                               args[3]->Int32Value(),
                                               args[4]->Int32Value(),
                                               args[5]->Int32Value())
                  textureDimensions:NSMakeSize(args[6]->Int32Value(),
                                               args[7]->Int32Value())
                            flipped:args[8]->BooleanValue()];
        return v8::Undefined();
    }
    
    static v8::Handle<v8::Value> bindToDrawFrameOfSize(
                                                       const v8::Arguments& args) {
        if (args.Length() != 2)
            return v8_utils::ThrowError("Wrong number of arguments.");
        SyphonServer* server = ExtractSyphonServerPointer(args.Holder());
        BOOL res = [server bindToDrawFrameOfSize:NSMakeSize(args[0]->Int32Value(),
                                                            args[1]->Int32Value())];
        return v8::Boolean::New(res);
    }
    
    static v8::Handle<v8::Value> unbindAndPublish(const v8::Arguments& args) {
        SyphonServer* server = ExtractSyphonServerPointer(args.Holder());
        [server unbindAndPublish];
        return v8::Undefined();
    }
    
    static v8::Handle<v8::Value> hasClients(const v8::Arguments& args) {
        SyphonServer* server = ExtractSyphonServerPointer(args.Holder());
        return v8::Boolean::New([server hasClients]);
    }
};

class SyphonClientWrapper {
public:
    static v8::Persistent<v8::FunctionTemplate> GetTemplate() {
        static v8::Persistent<v8::FunctionTemplate> ft_cache;
        if (!ft_cache.IsEmpty())
            return ft_cache;
        
        v8::HandleScope scope;
        ft_cache = v8::Persistent<v8::FunctionTemplate>::New(
                                                             v8::FunctionTemplate::New(&SyphonClientWrapper::V8New));
        ft_cache->SetClassName(v8::String::New("SyphonClient"));
        v8::Local<v8::ObjectTemplate> instance = ft_cache->InstanceTemplate();
        instance->SetInternalFieldCount(2);  // SyphonClient, CGLContextObj
        
        v8::Local<v8::Signature> default_signature = v8::Signature::New(ft_cache);
        
        // Configure the template...
        static BatchedConstants constants[] = {
            { "kValue", 12 },
        };
        
        static BatchedMethods methods[] = {
            { "newFrameImage", &SyphonClientWrapper::newFrameImage },
            { "isValid", &SyphonClientWrapper::isValid },
            { "hasNewFrame", &SyphonClientWrapper::hasNewFrame },
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
    
    static bool HasInstance(v8::Handle<v8::Value> value) {
        return GetTemplate()->HasInstance(value);
    }
    
    static SyphonClient* ExtractSyphonClientPointer(v8::Handle<v8::Object> obj) {
        return reinterpret_cast<SyphonClient*>(obj->GetPointerFromInternalField(0));
    }
    
    static CGLContextObj ExtractContextObj(v8::Handle<v8::Object> obj) {
        return reinterpret_cast<CGLContextObj>(obj->GetPointerFromInternalField(1));
    }
    
    static v8::Handle<v8::Value> NewFromSyphonClient(SyphonClient* client,
                                                     CGLContextObj context) {
        v8::Local<v8::Object> obj = SyphonClientWrapper::GetTemplate()->
        InstanceTemplate()->NewInstance();
        obj->SetPointerInInternalField(0, client);
        obj->SetPointerInInternalField(1, context);
        return obj;
    }
    
private:
    static v8::Handle<v8::Value> V8New(const v8::Arguments& args) {
        if (!args.IsConstructCall())
            return v8_utils::ThrowTypeError(kMsgNonConstructCall);
        
        return args.This();
    }
    
    static v8::Handle<v8::Value> newFrameImage(const v8::Arguments& args) {
        if (args.Length() != 0)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        SyphonClient* client = ExtractSyphonClientPointer(args.Holder());
        CGLContextObj context = ExtractContextObj(args.Holder());
        SyphonImage* image = [client newFrameImageForContext:context];
        
        if (!image) return v8::Null();
        
        v8::Local<v8::Object> res = v8::Object::New();
        res->Set(v8::String::New("name"),
                 v8::Integer::NewFromUnsigned([image textureName]));
        res->Set(v8::String::New("width"),
                 v8::Number::New([image textureSize].width));
        res->Set(v8::String::New("height"),
                 v8::Number::New([image textureSize].height));
        
        // The SyphonImage is just a container of the data.  The lifetime of it has
        // no relationship with the lifetime of the texture.
        [image release];
        
        return res;
    }
    
    static v8::Handle<v8::Value> isValid(const v8::Arguments& args) {
        SyphonClient* client = ExtractSyphonClientPointer(args.Holder());
        return v8::Boolean::New([client isValid]);
    }
    
    static v8::Handle<v8::Value> hasNewFrame(const v8::Arguments& args) {
        SyphonClient* client = ExtractSyphonClientPointer(args.Holder());
        return v8::Boolean::New([client hasNewFrame]);
    }
};


class NSOpenGLContextWrapper {
public:
    static v8::Persistent<v8::FunctionTemplate> GetTemplate() {
        static v8::Persistent<v8::FunctionTemplate> ft_cache;
        if (!ft_cache.IsEmpty())
            return ft_cache;
        
        v8::HandleScope scope;
        ft_cache = v8::Persistent<v8::FunctionTemplate>::New(
                                                             v8::FunctionTemplate::New(&NSOpenGLContextWrapper::V8New));
        v8::Local<v8::ObjectTemplate> instance = ft_cache->InstanceTemplate();
        instance->SetInternalFieldCount(1);
        
        v8::Local<v8::Signature> default_signature = v8::Signature::New(ft_cache);
        
        // Configure the template...
        static BatchedConstants constants[] = {
            /* ClearBufferMask */
            { "DEPTH_BUFFER_BIT", 0x00000100 },
            { "STENCIL_BUFFER_BIT", 0x00000400 },
            { "COLOR_BUFFER_BIT", 0x00004000 },
            
            /* Boolean */
            { "FALSE", 0 },
            { "TRUE", 1 },
            
            /* BeginMode */
            { "POINTS", 0x0000 },
            { "LINES", 0x0001 },
            { "LINE_LOOP", 0x0002 },
            { "LINE_STRIP", 0x0003 },
            { "TRIANGLES", 0x0004 },
            { "TRIANGLE_STRIP", 0x0005 },
            { "TRIANGLE_FAN", 0x0006 },
            
            /* AlphaFunction (not supported in ES20) */
            /*      NEVER */
            /*      LESS */
            /*      EQUAL */
            /*      LEQUAL */
            /*      GREATER */
            /*      NOTEQUAL */
            /*      GEQUAL */
            /*      ALWAYS */
            
            /* BlendingFactorDest */
            { "ZERO", 0 },
            { "ONE", 1 },
            { "SRC_COLOR", 0x0300 },
            { "ONE_MINUS_SRC_COLOR", 0x0301 },
            { "SRC_ALPHA", 0x0302 },
            { "ONE_MINUS_SRC_ALPHA", 0x0303 },
            { "DST_ALPHA", 0x0304 },
            { "ONE_MINUS_DST_ALPHA", 0x0305 },
            
            /* BlendingFactorSrc */
            /*      ZERO */
            /*      ONE */
            { "DST_COLOR", 0x0306 },
            { "ONE_MINUS_DST_COLOR", 0x0307 },
            { "SRC_ALPHA_SATURATE", 0x0308 },
            /*      SRC_ALPHA */
            /*      ONE_MINUS_SRC_ALPHA */
            /*      DST_ALPHA */
            /*      ONE_MINUS_DST_ALPHA */
            
            /* BlendEquationSeparate */
            { "FUNC_ADD", 0x8006 },
            { "BLEND_EQUATION", 0x8009 },
            { "BLEND_EQUATION_RGB", 0x8009 },   /* same as BLEND_EQUATION */
            { "BLEND_EQUATION_ALPHA", 0x883D },
            
            /* BlendSubtract */
            { "FUNC_SUBTRACT", 0x800A },
            { "FUNC_REVERSE_SUBTRACT", 0x800B },
            
            /* Separate Blend Functions */
            { "BLEND_DST_RGB", 0x80C8 },
            { "BLEND_SRC_RGB", 0x80C9 },
            { "BLEND_DST_ALPHA", 0x80CA },
            { "BLEND_SRC_ALPHA", 0x80CB },
            { "CONSTANT_COLOR", 0x8001 },
            { "ONE_MINUS_CONSTANT_COLOR", 0x8002 },
            { "CONSTANT_ALPHA", 0x8003 },
            { "ONE_MINUS_CONSTANT_ALPHA", 0x8004 },
            { "BLEND_COLOR", 0x8005 },
            
            /* Buffer Objects */
            { "ARRAY_BUFFER", 0x8892 },
            { "ELEMENT_ARRAY_BUFFER", 0x8893 },
            { "ARRAY_BUFFER_BINDING", 0x8894 },
            { "ELEMENT_ARRAY_BUFFER_BINDING", 0x8895 },
            
            { "STREAM_DRAW", 0x88E0 },
            { "STATIC_DRAW", 0x88E4 },
            { "DYNAMIC_DRAW", 0x88E8 },
            
            { "BUFFER_SIZE", 0x8764 },
            { "BUFFER_USAGE", 0x8765 },
            
            { "CURRENT_VERTEX_ATTRIB", 0x8626 },
            
            /* CullFaceMode */
            { "FRONT", 0x0404 },
            { "BACK", 0x0405 },
            { "FRONT_AND_BACK", 0x0408 },
            
            /* DepthFunction */
            /*      NEVER */
            /*      LESS */
            /*      EQUAL */
            /*      LEQUAL */
            /*      GREATER */
            /*      NOTEQUAL */
            /*      GEQUAL */
            /*      ALWAYS */
            
            /* EnableCap */
            { "TEXTURE_2D", 0x0DE1 },
            { "CULL_FACE", 0x0B44 },
            { "BLEND", 0x0BE2 },
            { "DITHER", 0x0BD0 },
            { "STENCIL_TEST", 0x0B90 },
            { "DEPTH_TEST", 0x0B71 },
            { "SCISSOR_TEST", 0x0C11 },
            { "POLYGON_OFFSET_FILL", 0x8037 },
            { "SAMPLE_ALPHA_TO_COVERAGE", 0x809E },
            { "SAMPLE_COVERAGE", 0x80A0 },
            
            /* ErrorCode */
            { "NO_ERROR", 0 },
            { "INVALID_ENUM", 0x0500 },
            { "INVALID_VALUE", 0x0501 },
            { "INVALID_OPERATION", 0x0502 },
            { "OUT_OF_MEMORY", 0x0505 },
            
            /* FrontFaceDirection */
            { "CW", 0x0900 },
            { "CCW", 0x0901 },
            
            /* GetPName */
            { "LINE_WIDTH", 0x0B21 },
            { "ALIASED_POINT_SIZE_RANGE", 0x846D },
            { "ALIASED_LINE_WIDTH_RANGE", 0x846E },
            { "CULL_FACE_MODE", 0x0B45 },
            { "FRONT_FACE", 0x0B46 },
            { "DEPTH_RANGE", 0x0B70 },
            { "DEPTH_WRITEMASK", 0x0B72 },
            { "DEPTH_CLEAR_VALUE", 0x0B73 },
            { "DEPTH_FUNC", 0x0B74 },
            { "STENCIL_CLEAR_VALUE", 0x0B91 },
            { "STENCIL_FUNC", 0x0B92 },
            { "STENCIL_FAIL", 0x0B94 },
            { "STENCIL_PASS_DEPTH_FAIL", 0x0B95 },
            { "STENCIL_PASS_DEPTH_PASS", 0x0B96 },
            { "STENCIL_REF", 0x0B97 },
            { "STENCIL_VALUE_MASK", 0x0B93 },
            { "STENCIL_WRITEMASK", 0x0B98 },
            { "STENCIL_BACK_FUNC", 0x8800 },
            { "STENCIL_BACK_FAIL", 0x8801 },
            { "STENCIL_BACK_PASS_DEPTH_FAIL", 0x8802 },
            { "STENCIL_BACK_PASS_DEPTH_PASS", 0x8803 },
            { "STENCIL_BACK_REF", 0x8CA3 },
            { "STENCIL_BACK_VALUE_MASK", 0x8CA4 },
            { "STENCIL_BACK_WRITEMASK", 0x8CA5 },
            { "VIEWPORT", 0x0BA2 },
            { "SCISSOR_BOX", 0x0C10 },
            /*      SCISSOR_TEST */
            { "COLOR_CLEAR_VALUE", 0x0C22 },
            { "COLOR_WRITEMASK", 0x0C23 },
            { "UNPACK_ALIGNMENT", 0x0CF5 },
            { "PACK_ALIGNMENT", 0x0D05 },
            { "MAX_TEXTURE_SIZE", 0x0D33 },
            { "MAX_VIEWPORT_DIMS", 0x0D3A },
            { "SUBPIXEL_BITS", 0x0D50 },
            { "RED_BITS", 0x0D52 },
            { "GREEN_BITS", 0x0D53 },
            { "BLUE_BITS", 0x0D54 },
            { "ALPHA_BITS", 0x0D55 },
            { "DEPTH_BITS", 0x0D56 },
            { "STENCIL_BITS", 0x0D57 },
            { "POLYGON_OFFSET_UNITS", 0x2A00 },
            /*      POLYGON_OFFSET_FILL */
            { "POLYGON_OFFSET_FACTOR", 0x8038 },
            { "TEXTURE_BINDING_2D", 0x8069 },
            { "SAMPLE_BUFFERS", 0x80A8 },
            { "SAMPLES", 0x80A9 },
            { "SAMPLE_COVERAGE_VALUE", 0x80AA },
            { "SAMPLE_COVERAGE_INVERT", 0x80AB },
            
            /* GetTextureParameter */
            /*      TEXTURE_MAG_FILTER */
            /*      TEXTURE_MIN_FILTER */
            /*      TEXTURE_WRAP_S */
            /*      TEXTURE_WRAP_T */
            
            { "NUM_COMPRESSED_TEXTURE_FORMATS", 0x86A2 },
            { "COMPRESSED_TEXTURE_FORMATS", 0x86A3 },
            
            /* HintMode */
            { "DONT_CARE", 0x1100 },
            { "FASTEST", 0x1101 },
            { "NICEST", 0x1102 },
            
            /* HintTarget */
            { "GENERATE_MIPMAP_HINT", 0x8192 },
            
            /* DataType */
            { "BYTE", 0x1400 },
            { "UNSIGNED_BYTE", 0x1401 },
            { "SHORT", 0x1402 },
            { "UNSIGNED_SHORT", 0x1403 },
            { "INT", 0x1404 },
            { "UNSIGNED_INT", 0x1405 },
            { "FLOAT", 0x1406 },
            { "FIXED", 0x140C },
            
            /* PixelFormat */
            { "DEPTH_COMPONENT", 0x1902 },
            { "ALPHA", 0x1906 },
            { "RGB", 0x1907 },
            { "RGBA", 0x1908 },
            { "LUMINANCE", 0x1909 },
            { "LUMINANCE_ALPHA", 0x190A },
            
            /* PixelType */
            /*      UNSIGNED_BYTE */
            { "UNSIGNED_SHORT_4_4_4_4", 0x8033 },
            { "UNSIGNED_SHORT_5_5_5_1", 0x8034 },
            { "UNSIGNED_SHORT_5_6_5", 0x8363 },
            
            /* Shaders */
            { "FRAGMENT_SHADER", 0x8B30 },
            { "VERTEX_SHADER", 0x8B31 },
            { "MAX_VERTEX_ATTRIBS", 0x8869 },
            { "MAX_VERTEX_UNIFORM_VECTORS", 0x8DFB },
            { "MAX_VARYING_VECTORS", 0x8DFC },
            { "MAX_COMBINED_TEXTURE_IMAGE_UNITS", 0x8B4D },
            { "MAX_VERTEX_TEXTURE_IMAGE_UNITS", 0x8B4C },
            { "MAX_TEXTURE_IMAGE_UNITS", 0x8872 },
            { "MAX_FRAGMENT_UNIFORM_VECTORS", 0x8DFD },
            { "SHADER_TYPE", 0x8B4F },
            { "DELETE_STATUS", 0x8B80 },
            { "LINK_STATUS", 0x8B82 },
            { "VALIDATE_STATUS", 0x8B83 },
            { "ATTACHED_SHADERS", 0x8B85 },
            { "ACTIVE_UNIFORMS", 0x8B86 },
            { "ACTIVE_UNIFORM_MAX_LENGTH", 0x8B87 },
            { "ACTIVE_ATTRIBUTES", 0x8B89 },
            { "ACTIVE_ATTRIBUTE_MAX_LENGTH", 0x8B8A },
            { "SHADING_LANGUAGE_VERSION", 0x8B8C },
            { "CURRENT_PROGRAM", 0x8B8D },
            
            /* StencilFunction */
            { "NEVER", 0x0200 },
            { "LESS", 0x0201 },
            { "EQUAL", 0x0202 },
            { "LEQUAL", 0x0203 },
            { "GREATER", 0x0204 },
            { "NOTEQUAL", 0x0205 },
            { "GEQUAL", 0x0206 },
            { "ALWAYS", 0x0207 },
            
            /* StencilOp */
            /*      ZERO */
            { "KEEP", 0x1E00 },
            { "REPLACE", 0x1E01 },
            { "INCR", 0x1E02 },
            { "DECR", 0x1E03 },
            { "INVERT", 0x150A },
            { "INCR_WRAP", 0x8507 },
            { "DECR_WRAP", 0x8508 },
            
            /* StringName */
            { "VENDOR", 0x1F00 },
            { "RENDERER", 0x1F01 },
            { "VERSION", 0x1F02 },
            { "EXTENSIONS", 0x1F03 },
            
            /* TextureMagFilter */
            { "NEAREST", 0x2600 },
            { "LINEAR", 0x2601 },
            
            /* TextureMinFilter */
            /*      NEAREST */
            /*      LINEAR */
            { "NEAREST_MIPMAP_NEAREST", 0x2700 },
            { "LINEAR_MIPMAP_NEAREST", 0x2701 },
            { "NEAREST_MIPMAP_LINEAR", 0x2702 },
            { "LINEAR_MIPMAP_LINEAR", 0x2703 },
            
            /* TextureParameterName */
            { "TEXTURE_MAG_FILTER", 0x2800 },
            { "TEXTURE_MIN_FILTER", 0x2801 },
            { "TEXTURE_WRAP_S", 0x2802 },
            { "TEXTURE_WRAP_T", 0x2803 },
            
            /* TextureTarget */
            /*      TEXTURE_2D */
            { "TEXTURE", 0x1702 },
            
            { "TEXTURE_CUBE_MAP", 0x8513 },
            { "TEXTURE_BINDING_CUBE_MAP", 0x8514 },
            { "TEXTURE_CUBE_MAP_POSITIVE_X", 0x8515 },
            { "TEXTURE_CUBE_MAP_NEGATIVE_X", 0x8516 },
            { "TEXTURE_CUBE_MAP_POSITIVE_Y", 0x8517 },
            { "TEXTURE_CUBE_MAP_NEGATIVE_Y", 0x8518 },
            { "TEXTURE_CUBE_MAP_POSITIVE_Z", 0x8519 },
            { "TEXTURE_CUBE_MAP_NEGATIVE_Z", 0x851A },
            { "MAX_CUBE_MAP_TEXTURE_SIZE", 0x851C },
            
            /* TextureUnit */
            { "TEXTURE0", 0x84C0 },
            { "TEXTURE1", 0x84C1 },
            { "TEXTURE2", 0x84C2 },
            { "TEXTURE3", 0x84C3 },
            { "TEXTURE4", 0x84C4 },
            { "TEXTURE5", 0x84C5 },
            { "TEXTURE6", 0x84C6 },
            { "TEXTURE7", 0x84C7 },
            { "TEXTURE8", 0x84C8 },
            { "TEXTURE9", 0x84C9 },
            { "TEXTURE10", 0x84CA },
            { "TEXTURE11", 0x84CB },
            { "TEXTURE12", 0x84CC },
            { "TEXTURE13", 0x84CD },
            { "TEXTURE14", 0x84CE },
            { "TEXTURE15", 0x84CF },
            { "TEXTURE16", 0x84D0 },
            { "TEXTURE17", 0x84D1 },
            { "TEXTURE18", 0x84D2 },
            { "TEXTURE19", 0x84D3 },
            { "TEXTURE20", 0x84D4 },
            { "TEXTURE21", 0x84D5 },
            { "TEXTURE22", 0x84D6 },
            { "TEXTURE23", 0x84D7 },
            { "TEXTURE24", 0x84D8 },
            { "TEXTURE25", 0x84D9 },
            { "TEXTURE26", 0x84DA },
            { "TEXTURE27", 0x84DB },
            { "TEXTURE28", 0x84DC },
            { "TEXTURE29", 0x84DD },
            { "TEXTURE30", 0x84DE },
            { "TEXTURE31", 0x84DF },
            { "ACTIVE_TEXTURE", 0x84E0 },
            
            /* TextureWrapMode */
            { "REPEAT", 0x2901 },
            { "CLAMP_TO_EDGE", 0x812F },
            { "MIRRORED_REPEAT", 0x8370 },
            
            /* Uniform Types */
            { "FLOAT_VEC2", 0x8B50 },
            { "FLOAT_VEC3", 0x8B51 },
            { "FLOAT_VEC4", 0x8B52 },
            { "INT_VEC2", 0x8B53 },
            { "INT_VEC3", 0x8B54 },
            { "INT_VEC4", 0x8B55 },
            { "BOOL", 0x8B56 },
            { "BOOL_VEC2", 0x8B57 },
            { "BOOL_VEC3", 0x8B58 },
            { "BOOL_VEC4", 0x8B59 },
            { "FLOAT_MAT2", 0x8B5A },
            { "FLOAT_MAT3", 0x8B5B },
            { "FLOAT_MAT4", 0x8B5C },
            { "SAMPLER_2D", 0x8B5E },
            { "SAMPLER_CUBE", 0x8B60 },
            
            /* Vertex Arrays */
            { "VERTEX_ATTRIB_ARRAY_ENABLED", 0x8622 },
            { "VERTEX_ATTRIB_ARRAY_SIZE", 0x8623 },
            { "VERTEX_ATTRIB_ARRAY_STRIDE", 0x8624 },
            { "VERTEX_ATTRIB_ARRAY_TYPE", 0x8625 },
            { "VERTEX_ATTRIB_ARRAY_NORMALIZED", 0x886A },
            { "VERTEX_ATTRIB_ARRAY_POINTER", 0x8645 },
            { "VERTEX_ATTRIB_ARRAY_BUFFER_BINDING", 0x889F },
            
            /* Read Format */
            { "IMPLEMENTATION_COLOR_READ_TYPE", 0x8B9A },
            { "IMPLEMENTATION_COLOR_READ_FORMAT", 0x8B9B },
            
            /* Shader Source */
            { "COMPILE_STATUS", 0x8B81 },
            { "INFO_LOG_LENGTH", 0x8B84 },
            { "SHADER_SOURCE_LENGTH", 0x8B88 },
            { "SHADER_COMPILER", 0x8DFA },
            
            /* Shader Binary */
            { "SHADER_BINARY_FORMATS", 0x8DF8 },
            { "NUM_SHADER_BINARY_FORMATS", 0x8DF9 },
            
            /* Shader Precision-Specified Types */
            { "LOW_FLOAT", 0x8DF0 },
            { "MEDIUM_FLOAT", 0x8DF1 },
            { "HIGH_FLOAT", 0x8DF2 },
            { "LOW_INT", 0x8DF3 },
            { "MEDIUM_INT", 0x8DF4 },
            { "HIGH_INT", 0x8DF5 },
            
            /* Framebuffer Object. */
            { "FRAMEBUFFER", 0x8D40 },
            { "RENDERBUFFER", 0x8D41 },
            
            { "RGBA4", 0x8056 },
            { "RGB5_A1", 0x8057 },
            { "RGB565", 0x8D62 },
            { "DEPTH_COMPONENT16", 0x81A5 },
            { "STENCIL_INDEX", 0x1901 },
            { "STENCIL_INDEX8", 0x8D48 },
            { "DEPTH_STENCIL", 0x84F9 },
            
            { "RENDERBUFFER_WIDTH", 0x8D42 },
            { "RENDERBUFFER_HEIGHT", 0x8D43 },
            { "RENDERBUFFER_INTERNAL_FORMAT", 0x8D44 },
            { "RENDERBUFFER_RED_SIZE", 0x8D50 },
            { "RENDERBUFFER_GREEN_SIZE", 0x8D51 },
            { "RENDERBUFFER_BLUE_SIZE", 0x8D52 },
            { "RENDERBUFFER_ALPHA_SIZE", 0x8D53 },
            { "RENDERBUFFER_DEPTH_SIZE", 0x8D54 },
            { "RENDERBUFFER_STENCIL_SIZE", 0x8D55 },
            
            { "FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE", 0x8CD0 },
            { "FRAMEBUFFER_ATTACHMENT_OBJECT_NAME", 0x8CD1 },
            { "FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL", 0x8CD2 },
            { "FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE", 0x8CD3 },
            
            { "COLOR_ATTACHMENT0", 0x8CE0 },
            { "DEPTH_ATTACHMENT", 0x8D00 },
            { "STENCIL_ATTACHMENT", 0x8D20 },
            { "DEPTH_STENCIL_ATTACHMENT", 0x821A },
            
            { "NONE", 0 },
            
            { "FRAMEBUFFER_COMPLETE", 0x8CD5 },
            { "FRAMEBUFFER_INCOMPLETE_ATTACHMENT", 0x8CD6 },
            { "FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT", 0x8CD7 },
            { "FRAMEBUFFER_INCOMPLETE_DIMENSIONS", 0x8CD9 },
            { "FRAMEBUFFER_UNSUPPORTED", 0x8CDD },
            
            { "FRAMEBUFFER_BINDING", 0x8CA6 },
            { "RENDERBUFFER_BINDING", 0x8CA7 },
            { "MAX_RENDERBUFFER_SIZE", 0x84E8 },
            
            { "INVALID_FRAMEBUFFER_OPERATION", 0x0506 },
            
            /* WebGL-specific enums */
            { "UNPACK_FLIP_Y_WEBGL", 0x9240 },
            { "UNPACK_PREMULTIPLY_ALPHA_WEBGL", 0x9241 },
            
            // Some Plask non-WebGL enums, some of which are likely a bad idea.
            { "UNPACK_CLIENT_STORAGE_APPLE", GL_UNPACK_CLIENT_STORAGE_APPLE },
            { "RGBA16F", 0x881A },
            { "RGBA32F", 0x8814 },
            { "COLOR_ATTACHMENT1", 0x8CE1 },
            { "COLOR_ATTACHMENT2", 0x8CE2 },
            { "COLOR_ATTACHMENT3", 0x8CE3 },
            { "COLOR_ATTACHMENT4", 0x8CE4 },
            { "COLOR_ATTACHMENT5", 0x8CE5 },
            { "COLOR_ATTACHMENT6", 0x8CE6 },
            { "COLOR_ATTACHMENT7", 0x8CE7 },
            { "DEPTH_COMPONENT24", 0x81A6 },
            { "DEPTH_COMPONENT32", 0x81A7 },
            { "DRAW_FRAMEBUFFER", GL_DRAW_FRAMEBUFFER },
            { "READ_FRAMEBUFFER", GL_READ_FRAMEBUFFER },
            // Rectangle textures (used by Syphon, for example).
            { "TEXTURE_RECTANGLE", 0x84F5 },
            { "TEXTURE_BINDING_RECTANGLE", 0x84F6 },
            { "MAX_RECTANGLE_TEXTURE_SIZE", 0x84F8 },
            { "SAMPLER_2D_RECT", 0x8B63 },
        };
        
        static BatchedMethods methods[] = {
            { "makeCurrentContext", &NSOpenGLContextWrapper::makeCurrentContext },
            { "setSwapInterval", &NSOpenGLContextWrapper::setSwapInterval },
            { "writeImage", &NSOpenGLContextWrapper::writeImage },
            
            { "activeTexture", &NSOpenGLContextWrapper::activeTexture },
            { "attachShader", &NSOpenGLContextWrapper::attachShader },
            { "bindAttribLocation", &NSOpenGLContextWrapper::bindAttribLocation },
            { "bindBuffer", &NSOpenGLContextWrapper::bindBuffer },
            { "bindFramebuffer", &NSOpenGLContextWrapper::bindFramebuffer },
            { "bindRenderbuffer", &NSOpenGLContextWrapper::bindRenderbuffer },
            { "bindTexture", &NSOpenGLContextWrapper::bindTexture },
            { "blendColor", &NSOpenGLContextWrapper::blendColor },
            { "blendEquation", &NSOpenGLContextWrapper::blendEquation },
            { "blendEquationSeparate",
                &NSOpenGLContextWrapper::blendEquationSeparate },
            { "blendFunc", &NSOpenGLContextWrapper::blendFunc },
            { "blendFuncSeparate", &NSOpenGLContextWrapper::blendFuncSeparate },
            { "bufferData", &NSOpenGLContextWrapper::bufferData },
            { "bufferSubData", &NSOpenGLContextWrapper::bufferSubData },
            { "checkFramebufferStatus",
                &NSOpenGLContextWrapper::checkFramebufferStatus },
            { "clear", &NSOpenGLContextWrapper::clear },
            { "clearColor", &NSOpenGLContextWrapper::clearColor },
            { "clearDepth", &NSOpenGLContextWrapper::clearDepth },
            { "clearStencil", &NSOpenGLContextWrapper::clearStencil },
            { "colorMask", &NSOpenGLContextWrapper::colorMask },
            { "compileShader", &NSOpenGLContextWrapper::compileShader },
            // { "copyTexImage2D", &NSOpenGLContextWrapper::copyTexImage2D },
            // { "copyTexSubImage2D", &NSOpenGLContextWrapper::copyTexSubImage2D },
            { "createBuffer", &NSOpenGLContextWrapper::createBuffer },
            { "createFramebuffer", &NSOpenGLContextWrapper::createFramebuffer },
            { "createProgram", &NSOpenGLContextWrapper::createProgram },
            { "createRenderbuffer", &NSOpenGLContextWrapper::createRenderbuffer },
            { "createShader", &NSOpenGLContextWrapper::createShader },
            { "createTexture", &NSOpenGLContextWrapper::createTexture },
            { "cullFace", &NSOpenGLContextWrapper::cullFace },
            { "deleteBuffer", &NSOpenGLContextWrapper::deleteBuffer },
            { "deleteFramebuffer", &NSOpenGLContextWrapper::deleteFramebuffer },
            { "deleteProgram", &NSOpenGLContextWrapper::deleteProgram },
            { "deleteRenderbuffer", &NSOpenGLContextWrapper::deleteRenderbuffer },
            { "deleteShader", &NSOpenGLContextWrapper::deleteShader },
            { "deleteTexture", &NSOpenGLContextWrapper::deleteTexture },
            { "depthFunc", &NSOpenGLContextWrapper::depthFunc },
            { "depthMask", &NSOpenGLContextWrapper::depthMask },
            { "depthRange", &NSOpenGLContextWrapper::depthRange },
            { "detachShader", &NSOpenGLContextWrapper::detachShader },
            { "disable", &NSOpenGLContextWrapper::disable },
            { "disableVertexAttribArray",
                &NSOpenGLContextWrapper::disableVertexAttribArray },
            { "drawArrays", &NSOpenGLContextWrapper::drawArrays },
            { "drawElements", &NSOpenGLContextWrapper::drawElements },
            { "enable", &NSOpenGLContextWrapper::enable },
            { "enableVertexAttribArray",
                &NSOpenGLContextWrapper::enableVertexAttribArray },
            { "finish", &NSOpenGLContextWrapper::finish },
            { "flush", &NSOpenGLContextWrapper::flush },
            { "framebufferRenderbuffer",
                &NSOpenGLContextWrapper::framebufferRenderbuffer },
            { "framebufferTexture2D", &NSOpenGLContextWrapper::framebufferTexture2D },
            { "frontFace", &NSOpenGLContextWrapper::frontFace },
            { "generateMipmap", &NSOpenGLContextWrapper::generateMipmap },
            { "getActiveAttrib", &NSOpenGLContextWrapper::getActiveAttrib },
            { "getActiveUniform", &NSOpenGLContextWrapper::getActiveUniform },
            { "getAttachedShaders", &NSOpenGLContextWrapper::getAttachedShaders },
            { "getAttribLocation", &NSOpenGLContextWrapper::getAttribLocation },
            { "getParameter", &NSOpenGLContextWrapper::getParameter },
            { "getBufferParameter", &NSOpenGLContextWrapper::getBufferParameter },
            { "getError", &NSOpenGLContextWrapper::getError },
            { "getFramebufferAttachmentParameter",
                &NSOpenGLContextWrapper::getFramebufferAttachmentParameter },
            { "getProgramParameter", &NSOpenGLContextWrapper::getProgramParameter },
            { "getProgramInfoLog", &NSOpenGLContextWrapper::getProgramInfoLog },
            { "getRenderbufferParameter",
                &NSOpenGLContextWrapper::getRenderbufferParameter },
            { "getShaderParameter", &NSOpenGLContextWrapper::getShaderParameter },
            { "getShaderInfoLog", &NSOpenGLContextWrapper::getShaderInfoLog },
            { "getShaderSource", &NSOpenGLContextWrapper::getShaderSource },
            { "getTexParameter", &NSOpenGLContextWrapper::getTexParameter },
            { "getUniform", &NSOpenGLContextWrapper::getUniform },
            { "getUniformLocation", &NSOpenGLContextWrapper::getUniformLocation },
            { "getVertexAttrib", &NSOpenGLContextWrapper::getVertexAttrib },
            { "getVertexAttribOffset",
                &NSOpenGLContextWrapper::getVertexAttribOffset },
            { "hint", &NSOpenGLContextWrapper::hint },
            { "isBuffer", &NSOpenGLContextWrapper::isBuffer },
            { "isEnabled", &NSOpenGLContextWrapper::isEnabled },
            { "isFramebuffer", &NSOpenGLContextWrapper::isFramebuffer },
            { "isProgram", &NSOpenGLContextWrapper::isProgram },
            { "isRenderbuffer", &NSOpenGLContextWrapper::isRenderbuffer },
            { "isShader", &NSOpenGLContextWrapper::isShader },
            { "isTexture", &NSOpenGLContextWrapper::isTexture },
            { "lineWidth", &NSOpenGLContextWrapper::lineWidth },
            { "linkProgram", &NSOpenGLContextWrapper::linkProgram },
            { "pixelStorei", &NSOpenGLContextWrapper::pixelStorei },
            { "polygonOffset", &NSOpenGLContextWrapper::polygonOffset },
            { "readPixels", &NSOpenGLContextWrapper::readPixels },
            { "renderbufferStorage", &NSOpenGLContextWrapper::renderbufferStorage },
            { "sampleCoverage", &NSOpenGLContextWrapper::sampleCoverage },
            { "scissor", &NSOpenGLContextWrapper::scissor },
            { "shaderSource", &NSOpenGLContextWrapper::shaderSource },
            { "stencilFunc", &NSOpenGLContextWrapper::stencilFunc },
            { "stencilFuncSeparate", &NSOpenGLContextWrapper::stencilFuncSeparate },
            { "stencilMask", &NSOpenGLContextWrapper::stencilMask },
            { "stencilMaskSeparate", &NSOpenGLContextWrapper::stencilMaskSeparate },
            { "stencilOp", &NSOpenGLContextWrapper::stencilOp },
            { "stencilOpSeparate", &NSOpenGLContextWrapper::stencilOpSeparate },
            { "texImage2D", &NSOpenGLContextWrapper::texImage2D },
            { "texImage2DSkCanvasB", &NSOpenGLContextWrapper::texImage2DSkCanvasB },
            { "texParameterf", &NSOpenGLContextWrapper::texParameterf },
            { "texParameteri", &NSOpenGLContextWrapper::texParameteri },
            { "texSubImage2D", &NSOpenGLContextWrapper::texSubImage2D },
            { "uniform1f", &NSOpenGLContextWrapper::uniform1f },
            { "uniform1fv", &NSOpenGLContextWrapper::uniform1fv },
            { "uniform1i", &NSOpenGLContextWrapper::uniform1i },
            { "uniform1iv", &NSOpenGLContextWrapper::uniform1iv },
            { "uniform2f", &NSOpenGLContextWrapper::uniform2f },
            { "uniform2fv", &NSOpenGLContextWrapper::uniform2fv },
            { "uniform2i", &NSOpenGLContextWrapper::uniform2i },
            { "uniform2iv", &NSOpenGLContextWrapper::uniform2iv },
            { "uniform3f", &NSOpenGLContextWrapper::uniform3f },
            { "uniform3fv", &NSOpenGLContextWrapper::uniform3fv },
            { "uniform3i", &NSOpenGLContextWrapper::uniform3i },
            { "uniform3iv", &NSOpenGLContextWrapper::uniform3iv },
            { "uniform4f", &NSOpenGLContextWrapper::uniform4f },
            { "uniform4fv", &NSOpenGLContextWrapper::uniform4fv },
            { "uniform4i", &NSOpenGLContextWrapper::uniform4i },
            { "uniform4iv", &NSOpenGLContextWrapper::uniform4iv },
            { "uniformMatrix2fv", &NSOpenGLContextWrapper::uniformMatrix2fv },
            { "uniformMatrix3fv", &NSOpenGLContextWrapper::uniformMatrix3fv },
            { "uniformMatrix4fv", &NSOpenGLContextWrapper::uniformMatrix4fv },
            { "useProgram", &NSOpenGLContextWrapper::useProgram },
            { "validateProgram", &NSOpenGLContextWrapper::validateProgram },
            { "vertexAttrib1f", &NSOpenGLContextWrapper::vertexAttrib1f },
            //{ "vertexAttrib1fv", &NSOpenGLContextWrapper::vertexAttrib1fv },
            { "vertexAttrib2f", &NSOpenGLContextWrapper::vertexAttrib2f },
            //{ "vertexAttrib2fv", &NSOpenGLContextWrapper::vertexAttrib2fv },
            { "vertexAttrib3f", &NSOpenGLContextWrapper::vertexAttrib3f },
            //{ "vertexAttrib3fv", &NSOpenGLContextWrapper::vertexAttrib3fv },
            { "vertexAttrib4f", &NSOpenGLContextWrapper::vertexAttrib4f },
            //{ "vertexAttrib4fv", &NSOpenGLContextWrapper::vertexAttrib4fv },
            { "vertexAttribPointer", &NSOpenGLContextWrapper::vertexAttribPointer },
            { "viewport", &NSOpenGLContextWrapper::viewport },
            // Plask-specific, not in WebGL.  From ARB_draw_buffers.
            { "drawBuffers", &NSOpenGLContextWrapper::drawBuffers },
            { "blitFramebuffer", &NSOpenGLContextWrapper::blitFramebuffer },
            { "drawSkCanvas", &NSOpenGLContextWrapper::drawSkCanvas },
            // Syphon.
            { "createSyphonServer", &NSOpenGLContextWrapper::createSyphonServer },
            { "createSyphonClient", &NSOpenGLContextWrapper::createSyphonClient },
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
    
    static NSOpenGLContext* ExtractContextPointer(v8::Handle<v8::Object> obj) {
        return v8_utils::UnwrapCPointer<NSOpenGLContext>(obj->GetInternalField(0));
    }
    
private:
    static v8::Handle<v8::Value> V8New(const v8::Arguments& args) {
        if (!args.IsConstructCall())
            return v8_utils::ThrowTypeError(kMsgNonConstructCall);
        
        args.This()->SetInternalField(0, v8_utils::WrapCPointer(NULL));
        return args.This();
    }
    
    static v8::Handle<v8::Value> makeCurrentContext(const v8::Arguments& args) {
        NSOpenGLContext* context = ExtractContextPointer(args.Holder());
        [context makeCurrentContext];
        return v8::Undefined();
    }
    
    static v8::Handle<v8::Value> createSyphonServer(const v8::Arguments& args) {
        if (args.Length() != 1)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        NSOpenGLContext* context = ExtractContextPointer(args.Holder());
        v8::String::Utf8Value name(args[0]);
        SyphonServer* server = [[SyphonServer alloc]
                                initWithName:[NSString stringWithUTF8String:*name]
                                context:reinterpret_cast<CGLContextObj>([context CGLContextObj])
                                options:nil];
        return SyphonServerWrapper::NewFromSyphonServer(server);
    }
    
    static v8::Handle<v8::Value> createSyphonClient(const v8::Arguments& args) {
        if (args.Length() != 1)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        NSOpenGLContext* context = ExtractContextPointer(args.This());
        //v8::String::Utf8Value uuid(args[0]);
        v8::String::Utf8Value name(args[0]);
        
        NSArray* servers = [[SyphonServerDirectory sharedDirectory] servers];
        NSLog(@"Servers: %@", servers);
        
        NSDictionary* found_server = NULL;
        for (NSUInteger i = 0, il = [servers count]; i < il; ++i) {
            NSDictionary* server = reinterpret_cast<NSDictionary*>(
                                                                   [servers objectAtIndex:i]);
            //NSString* suuid = [server objectForKey:SyphonServerDescriptionUUIDKey];
            //NSLog(@"UUID: %@", suuid);
            NSString* sname = [server objectForKey:SyphonServerDescriptionNameKey];
            NSLog(@"Name: %@", sname);
            if (strcmp([sname UTF8String], *name) == 0) {
                found_server = server;
                break;
            }
        }
        
        if (!found_server)
            return v8_utils::ThrowError("No server found matching given name.");
        
        SyphonClient* client = [[SyphonClient alloc]
                                initWithServerDescription:found_server
                                options:nil
                                newFrameHandler:nil];
        return SyphonClientWrapper::NewFromSyphonClient(
                                                        client, reinterpret_cast<CGLContextObj>([context CGLContextObj]));
    }
    
    // aka vsync.
    static v8::Handle<v8::Value> setSwapInterval(const v8::Arguments& args) {
        NSOpenGLContext* context = ExtractContextPointer(args.Holder());
        GLint interval = args[0]->Int32Value();
        [context setValues:&interval forParameter:NSOpenGLCPSwapInterval];
        return v8::Undefined();
    }
    
    // TODO(deanm): Share more code with SkCanvas#writeImage.
    static v8::Handle<v8::Value> writeImage(const v8::Arguments& args) {
        const uint32_t rmask = SK_R32_MASK << SK_R32_SHIFT;
        const uint32_t gmask = SK_G32_MASK << SK_G32_SHIFT;
        const uint32_t bmask = SK_B32_MASK << SK_B32_SHIFT;
        
        if (args.Length() < 2)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        NSOpenGLContext* context = ExtractContextPointer(args.Holder());
        // TODO(deanm): There should be a better way to get the width and height.
        NSRect frame = [[context view] frame];
        int width = frame.size.width;
        int height = frame.size.height;
        
        int buffer_type = args[3]->Int32Value();
        
        // Handle width / height in the optional options object.  This allows you
        // to override the width and height, for example if there is a framebuffer
        // object that is a different size than the window.
        // TODO(deanm): Also allow passing the x/y ?
        if (args.Length() >= 3 && args[2]->IsObject()) {
            v8::Handle<v8::Object> opts = v8::Handle<v8::Object>::Cast(args[2]);
            if (opts->Has(v8::String::New("width")))
                width = opts->Get(v8::String::New("width"))->Int32Value();
            if (opts->Has(v8::String::New("height")))
                height = opts->Get(v8::String::New("height"))->Int32Value();
        }
        
        FREE_IMAGE_FORMAT format;
        
        v8::String::Utf8Value type(args[0]);
        if (strcmp(*type, "png") == 0) {
            format = FIF_PNG;
        } else if (strcmp(*type, "tiff") == 0) {
            format = FIF_TIFF;
        } else if (strcmp(*type, "targa") == 0) {
            format = FIF_TARGA;
        } else {
            return v8_utils::ThrowError("writeImage unsupported output type.");
        }
        
        v8::String::Utf8Value filename(args[1]);
        
        FIBITMAP* fb;
        
        if (buffer_type == 0) {  // RGBA color buffer.
            void* pixels = malloc(width * height * 4);
            glReadPixels(0, 0, width, height, GL_BGRA, GL_UNSIGNED_BYTE, pixels);
            
            fb = FreeImage_ConvertFromRawBits(
                                              reinterpret_cast<BYTE*>(pixels),
                                              width, height, width * 4, 32,
                                              rmask, gmask, bmask, FALSE);
            free(pixels);
            if (!fb)
                return v8_utils::ThrowError("Couldn't allocate FreeImage bitmap.");
        } else {  // Floating point depth buffer
            fb = FreeImage_AllocateT(FIT_FLOAT, width, height);
            if (!fb)
                return v8_utils::ThrowError("Couldn't allocate FreeImage bitmap.");
            glReadPixels(0, 0, width, height,
                         GL_DEPTH_COMPONENT, GL_FLOAT, FreeImage_GetBits(fb));
        }
        
        int save_flags = 0;
        
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
            if (format == FIF_TIFF && opts->Has(v8::String::New("tiffCompression"))) {
                if (!opts->Get(v8::String::New("tiffCompression"))->BooleanValue())
                    save_flags = TIFF_NONE;
            }
        }
        
        bool saved = FreeImage_Save(format, fb, *filename, save_flags);
        FreeImage_Unload(fb);
        
        if (!saved)
            return v8_utils::ThrowError("Failed to save png.");
        
        return v8::Undefined();
    }
    
    // void activeTexture(GLenum texture)
    static v8::Handle<v8::Value> activeTexture(const v8::Arguments& args) {
        if (args.Length() != 1)
            return v8_utils::ThrowError("Wrong number of arguments.");
        glActiveTexture(args[0]->Uint32Value());
        return v8::Undefined();
    }
    
    // void attachShader(WebGLProgram program, WebGLShader shader)
    static v8::Handle<v8::Value> attachShader(const v8::Arguments& args) {
        if (args.Length() != 2)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        if (!WebGLProgram::HasInstance(args[0]))
            return v8_utils::ThrowTypeError("Type error");
        
        if (!WebGLShader::HasInstance(args[1]))
            return v8_utils::ThrowTypeError("Type error");
        
        GLuint program = WebGLProgram::ExtractProgramNameFromValue(args[0]);
        GLuint shader = WebGLShader::ExtractShaderNameFromValue(args[1]);
        
        glAttachShader(program, shader);
        return v8::Undefined();
    }
    
    // void bindAttribLocation(WebGLProgram program, GLuint index, DOMString name)
    static v8::Handle<v8::Value> bindAttribLocation(const v8::Arguments& args) {
        if (args.Length() != 3)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        if (!WebGLProgram::HasInstance(args[0]))
            return v8_utils::ThrowTypeError("Type error");
        
        GLuint program = WebGLProgram::ExtractProgramNameFromValue(args[0]);
        
        v8::String::Utf8Value name(args[2]);
        glBindAttribLocation(program, args[1]->Uint32Value(), *name);
        return v8::Undefined();
    }
    
    // void bindBuffer(GLenum target, WebGLBuffer buffer)
    static v8::Handle<v8::Value> bindBuffer(const v8::Arguments& args) {
        if (args.Length() != 2)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        if (!args[1]->IsNull() && !WebGLBuffer::HasInstance(args[1]))
            return v8_utils::ThrowTypeError("Type error");
        
        GLuint buffer = WebGLBuffer::ExtractBufferNameFromValue(args[1]);
        
        glBindBuffer(args[0]->Uint32Value(), buffer);
        return v8::Undefined();
    }
    
    // void bindFramebuffer(GLenum target, WebGLFramebuffer framebuffer)
    static v8::Handle<v8::Value> bindFramebuffer(const v8::Arguments& args) {
        if (args.Length() != 2)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        // NOTE: ExtractFramebufferObjectNameFromValue handles null.
        if (!args[1]->IsNull() && !WebGLFramebuffer::HasInstance(args[1]))
            return v8_utils::ThrowTypeError("Type error");
        
        glBindFramebuffer(
                          args[0]->Uint32Value(),
                          WebGLFramebuffer::ExtractFramebufferObjectNameFromValue(args[1]));
        return v8::Undefined();
    }
    
    // void bindRenderbuffer(GLenum target, WebGLRenderbuffer renderbuffer)
    static v8::Handle<v8::Value> bindRenderbuffer(const v8::Arguments& args) {
        if (args.Length() != 2)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        // NOTE: ExtractRenderbufferObjectNameFromValue handles null.
        if (!args[1]->IsNull() && !WebGLRenderbuffer::HasInstance(args[1]))
            return v8_utils::ThrowTypeError("Type error");
        
        glBindRenderbuffer(
                           args[0]->Uint32Value(),
                           WebGLRenderbuffer::ExtractRenderbufferObjectNameFromValue(args[1]));
        return v8::Undefined();
    }
    
    // void bindTexture(GLenum target, WebGLTexture texture)
    static v8::Handle<v8::Value> bindTexture(const v8::Arguments& args) {
        if (args.Length() != 2)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        // NOTE: ExtractTextureNameFromValue handles null.
        if (!args[1]->IsNull() && !WebGLTexture::HasInstance(args[1]))
            return v8_utils::ThrowTypeError("Type error");
        
        glBindTexture(args[0]->Uint32Value(),
                      WebGLTexture::ExtractTextureNameFromValue(args[1]));
        return v8::Undefined();
    }
    
    // void blendColor(GLclampf red, GLclampf green,
    //                 GLclampf blue, GLclampf alpha)
    static v8::Handle<v8::Value> blendColor(const v8::Arguments& args) {
        if (args.Length() != 4)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        glBlendColor(args[0]->NumberValue(),
                     args[1]->NumberValue(),
                     args[2]->NumberValue(),
                     args[3]->NumberValue());
        return v8::Undefined();
    }
    
    // void blendEquation(GLenum mode)
    static v8::Handle<v8::Value> blendEquation(const v8::Arguments& args) {
        if (args.Length() != 1)
            return v8_utils::ThrowError("Wrong number of arguments.");
        glBlendEquation(args[0]->Uint32Value());
        return v8::Undefined();
    }
    
    // void blendEquationSeparate(GLenum modeRGB, GLenum modeAlpha)
    static v8::Handle<v8::Value> blendEquationSeparate(
                                                       const v8::Arguments& args) {
        if (args.Length() != 2)
            return v8_utils::ThrowError("Wrong number of arguments.");
        glBlendEquationSeparate(args[0]->Uint32Value(), args[1]->Uint32Value());
        return v8::Undefined();
    }
    
    
    // void blendFunc(GLenum sfactor, GLenum dfactor)
    static v8::Handle<v8::Value> blendFunc(
                                           const v8::Arguments& args) {
        if (args.Length() != 2)
            return v8_utils::ThrowError("Wrong number of arguments.");
        glBlendFunc(args[0]->Uint32Value(), args[1]->Uint32Value());
        return v8::Undefined();
    }
    
    // void blendFuncSeparate(GLenum srcRGB, GLenum dstRGB,
    //                        GLenum srcAlpha, GLenum dstAlpha)
    static v8::Handle<v8::Value> blendFuncSeparate(
                                                   const v8::Arguments& args) {
        if (args.Length() != 4)
            return v8_utils::ThrowError("Wrong number of arguments.");
        glBlendFuncSeparate(args[0]->Uint32Value(), args[1]->Uint32Value(),
                            args[2]->Uint32Value(), args[3]->Uint32Value());
        return v8::Undefined();
    }
    
    // void bufferData(GLenum target, GLsizei size, GLenum usage)
    // void bufferData(GLenum target, ArrayBufferView data, GLenum usage)
    // void bufferData(GLenum target, ArrayBuffer data, GLenum usage)
    static v8::Handle<v8::Value> bufferData(
                                            const v8::Arguments& args) {
        if (args.Length() != 3)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        GLsizeiptr size = 0;
        GLvoid* data = NULL;
        
        if (args[1]->IsObject()) {
            v8::Local<v8::Object> obj = v8::Local<v8::Object>::Cast(args[1]);
            if (!obj->HasIndexedPropertiesInExternalArrayData())
                return v8_utils::ThrowError("Data must be an ArrayBuffer.");
            int element_size = v8_typed_array::SizeOfArrayElementForType(
                                                                         obj->GetIndexedPropertiesExternalArrayDataType());
            size = obj->GetIndexedPropertiesExternalArrayDataLength() * element_size;
            data = obj->GetIndexedPropertiesExternalArrayData();
        } else {
            size = args[1]->Uint32Value();
        }
        
        glBufferData(args[0]->Uint32Value(), size, data, args[2]->Uint32Value());
        return v8::Undefined();
    }
    
    // void bufferSubData(GLenum target, GLsizeiptr offset, ArrayBufferView data)
    // void bufferSubData(GLenum target, GLsizeiptr offset, ArrayBuffer data)
    static v8::Handle<v8::Value> bufferSubData(
                                               const v8::Arguments& args) {
        if (args.Length() != 3)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        GLsizeiptr size = 0;
        GLintptr offset = args[1]->Int32Value();
        GLvoid* data = NULL;
        
        if (args[2]->IsObject()) {
            v8::Local<v8::Object> obj = v8::Local<v8::Object>::Cast(args[2]);
            if (!obj->HasIndexedPropertiesInExternalArrayData())
                return v8_utils::ThrowError("Data must be an ArrayBuffer.");
            int element_size = v8_typed_array::SizeOfArrayElementForType(
                                                                         obj->GetIndexedPropertiesExternalArrayDataType());
            size = obj->GetIndexedPropertiesExternalArrayDataLength() * element_size;
            data = obj->GetIndexedPropertiesExternalArrayData();
        } else {
            size = args[1]->Uint32Value();
        }
        
        glBufferSubData(args[0]->Uint32Value(), offset, size, data);
        return v8::Undefined();
    }
    
    // GLenum checkFramebufferStatus(GLenum target)
    static v8::Handle<v8::Value> checkFramebufferStatus(
                                                        const v8::Arguments& args) {
        if (args.Length() != 1)
            return v8_utils::ThrowError("Wrong number of arguments.");
        return v8::Integer::NewFromUnsigned(
                                            glCheckFramebufferStatus(args[0]->Uint32Value()));
    }
    
    // void clear(GLbitfield mask)
    static v8::Handle<v8::Value> clear(const v8::Arguments& args) {
        if (args.Length() != 1)
            return v8_utils::ThrowError("Wrong number of arguments.");
        glClear(args[0]->Uint32Value());
        return v8::Undefined();
    }
    
    // void clearColor(GLclampf red, GLclampf green,
    //                 GLclampf blue, GLclampf alpha)
    static v8::Handle<v8::Value> clearColor(const v8::Arguments& args) {
        if (args.Length() != 4)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        glClearColor(args[0]->NumberValue(),
                     args[1]->NumberValue(),
                     args[2]->NumberValue(),
                     args[3]->NumberValue());
        return v8::Undefined();
    }
    
    // void clearDepth(GLclampf depth)
    static v8::Handle<v8::Value> clearDepth(const v8::Arguments& args) {
        if (args.Length() != 1)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        glClearDepth(args[0]->NumberValue());
        return v8::Undefined();
    }
    
    // void clearStencil(GLint s)
    static v8::Handle<v8::Value> clearStencil(const v8::Arguments& args) {
        if (args.Length() != 1)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        glClearStencil(args[0]->Int32Value());
        return v8::Undefined();
    }
    
    // void colorMask(GLboolean red, GLboolean green,
    //                GLboolean blue, GLboolean alpha)
    static v8::Handle<v8::Value> colorMask(const v8::Arguments& args) {
        if (args.Length() != 4)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        glColorMask(args[0]->BooleanValue(),
                    args[1]->BooleanValue(),
                    args[2]->BooleanValue(),
                    args[3]->BooleanValue());
        return v8::Undefined();
    }
    
    // void compileShader(WebGLShader shader)
    static v8::Handle<v8::Value> compileShader(const v8::Arguments& args) {
        if (args.Length() != 1)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        if (!WebGLShader::HasInstance(args[0]))
            return v8_utils::ThrowTypeError("Type error");
        
        GLuint shader = WebGLShader::ExtractShaderNameFromValue(args[0]);
        
        glCompileShader(shader);
        return v8::Undefined();
    }
    
    // WebGLBuffer createBuffer()
    static v8::Handle<v8::Value> createBuffer(const v8::Arguments& args) {
        GLuint buffer;
        glGenBuffers(1, &buffer);
        return WebGLBuffer::NewFromBufferName(buffer);
    }
    
    // WebGLFramebuffer createFramebuffer()
    static v8::Handle<v8::Value> createFramebuffer(const v8::Arguments& args) {
        GLuint framebuffer;
        glGenFramebuffers(1, &framebuffer);
        return WebGLFramebuffer::NewFromFramebufferObjectName(framebuffer);
    }
    
    // WebGLProgram createProgram()
    static v8::Handle<v8::Value> createProgram(const v8::Arguments& args) {
        return WebGLProgram::NewFromProgramName(glCreateProgram());
    }
    
    // WebGLRenderbuffer createRenderbuffer()
    static v8::Handle<v8::Value> createRenderbuffer(const v8::Arguments& args) {
        GLuint renderbuffer;
        glGenRenderbuffers(1, &renderbuffer);
        return WebGLRenderbuffer::NewFromRenderbufferObjectName(renderbuffer);
    }
    
    // WebGLShader createShader(GLenum type)
    static v8::Handle<v8::Value> createShader(const v8::Arguments& args) {
        if (args.Length() != 1)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        return WebGLShader::NewFromShaderName(
                                              glCreateShader(args[0]->Uint32Value()));
    }
    
    // WebGLTexture createTexture()
    static v8::Handle<v8::Value> createTexture(const v8::Arguments& args) {
        GLuint texture;
        glGenTextures(1, &texture);
        return WebGLTexture::NewFromTextureName(texture);
    }
    
    // void cullFace(GLenum mode)
    static v8::Handle<v8::Value> cullFace(const v8::Arguments& args) {
        if (args.Length() != 1)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        glCullFace(args[0]->Uint32Value());
        return v8::Undefined();
    }
    
    // void deleteBuffer(WebGLBuffer buffer)
    static v8::Handle<v8::Value> deleteBuffer(const v8::Arguments& args) {
        if (args.Length() != 1)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        // Seems that Chrome does this...
        if (args[0]->IsNull() || args[0]->IsUndefined())
            return v8::Undefined();
        
        if (!WebGLBuffer::HasInstance(args[0]))
            return v8_utils::ThrowTypeError("Type error");
        
        GLuint buffer = WebGLBuffer::ExtractBufferNameFromValue(args[0]);
        if (buffer != 0) {
            glDeleteBuffers(1, &buffer);
            WebGLBuffer::ClearBufferName(args[0]);
        }
        return v8::Undefined();
    }
    
    // void deleteFramebuffer(WebGLFramebuffer framebuffer)
    static v8::Handle<v8::Value> deleteFramebuffer(const v8::Arguments& args) {
        if (args.Length() != 1)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        // Seems that Chrome does this...
        if (args[0]->IsNull() || args[0]->IsUndefined())
            return v8::Undefined();
        
        if (!WebGLFramebuffer::HasInstance(args[0]))
            return v8_utils::ThrowTypeError("Type error");
        
        GLuint framebuffer =
        WebGLFramebuffer::ExtractFramebufferObjectNameFromValue(args[0]);
        if (framebuffer != 0) {
            glDeleteFramebuffers(1, &framebuffer);
            WebGLFramebuffer::ClearFramebufferObjectName(args[0]);
        }
        return v8::Undefined();
    }
    
    // void deleteProgram(WebGLProgram program)
    static v8::Handle<v8::Value> deleteProgram(const v8::Arguments& args) {
        if (args.Length() != 1)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        // Seems that Chrome does this...
        if (args[0]->IsNull() || args[0]->IsUndefined())
            return v8::Undefined();
        
        if (!WebGLProgram::HasInstance(args[0]))
            return v8_utils::ThrowTypeError("Type error");
        
        GLuint program = WebGLProgram::ExtractProgramNameFromValue(args[0]);
        if (program != 0) {
            glDeleteProgram(program);
            WebGLProgram::ClearProgramName(args[0]);
        }
        return v8::Undefined();
    }
    
    // void deleteRenderbuffer(WebGLRenderbuffer renderbuffer)
    static v8::Handle<v8::Value> deleteRenderbuffer(const v8::Arguments& args) {
        if (args.Length() != 1)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        // Seems that Chrome does this...
        if (args[0]->IsNull() || args[0]->IsUndefined())
            return v8::Undefined();
        
        if (!WebGLRenderbuffer::HasInstance(args[0]))
            return v8_utils::ThrowTypeError("Type error");
        
        GLuint renderbuffer =
        WebGLRenderbuffer::ExtractRenderbufferObjectNameFromValue(args[0]);
        if (renderbuffer != 0) {
            glDeleteRenderbuffers(1, &renderbuffer);
            WebGLRenderbuffer::ClearRenderbufferObjectName(args[0]);
        }
        return v8::Undefined();
    }
    
    // void deleteShader(WebGLShader shader)
    static v8::Handle<v8::Value> deleteShader(const v8::Arguments& args) {
        if (args.Length() != 1)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        // Seems that Chrome does this...
        if (args[0]->IsNull() || args[0]->IsUndefined())
            return v8::Undefined();
        
        if (!WebGLShader::HasInstance(args[0]))
            return v8_utils::ThrowTypeError("Type error");
        
        GLuint shader = WebGLShader::ExtractShaderNameFromValue(args[0]);
        if (shader != 0) {
            glDeleteShader(shader);
            WebGLShader::ClearShaderName(args[0]);
        }
        return v8::Undefined();
    }
    
    // void deleteTexture(WebGLTexture texture)
    static v8::Handle<v8::Value> deleteTexture(const v8::Arguments& args) {
        if (args.Length() != 1)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        // Seems that Chrome does this...
        if (args[0]->IsNull() || args[0]->IsUndefined())
            return v8::Undefined();
        
        if (!WebGLTexture::HasInstance(args[0]))
            return v8_utils::ThrowTypeError("Type error");
        
        GLuint texture =
        WebGLTexture::ExtractTextureNameFromValue(args[0]);
        if (texture != 0) {
            glDeleteTextures(1, &texture);
            WebGLTexture::ClearTextureName(args[0]);
        }
        return v8::Undefined();
    }
    
    // void depthFunc(GLenum func)
    static v8::Handle<v8::Value> depthFunc(const v8::Arguments& args) {
        if (args.Length() != 1)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        glDepthFunc(args[0]->Uint32Value());
        return v8::Undefined();
    }
    
    // void depthMask(GLboolean flag)
    static v8::Handle<v8::Value> depthMask(const v8::Arguments& args) {
        if (args.Length() != 1)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        glDepthMask(args[0]->BooleanValue());
        return v8::Undefined();
    }
    
    // void depthRange(GLclampf zNear, GLclampf zFar)
    static v8::Handle<v8::Value> depthRange(const v8::Arguments& args) {
        if (args.Length() != 2)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        glDepthRange(args[0]->NumberValue(), args[1]->NumberValue());
        return v8::Undefined();
    }
    
    // void detachShader(WebGLProgram program, WebGLShader shader)
    static v8::Handle<v8::Value> detachShader(const v8::Arguments& args) {
        if (args.Length() != 2)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        if (!WebGLProgram::HasInstance(args[0]))
            return v8_utils::ThrowTypeError("Type error");
        
        if (!WebGLShader::HasInstance(args[1]))
            return v8_utils::ThrowTypeError("Type error");
        
        GLuint program = WebGLProgram::ExtractProgramNameFromValue(args[0]);
        GLuint shader = WebGLShader::ExtractShaderNameFromValue(args[1]);
        
        glDetachShader(program, shader);
        return v8::Undefined();
    }
    
    // void disable(GLenum cap)
    static v8::Handle<v8::Value> disable(const v8::Arguments& args) {
        if (args.Length() != 1)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        glDisable(args[0]->Uint32Value());
        return v8::Undefined();
    }
    
    // void disableVertexAttribArray(GLuint index)
    static v8::Handle<v8::Value> disableVertexAttribArray(
                                                          const v8::Arguments& args) {
        if (args.Length() != 1)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        glDisableVertexAttribArray(args[0]->Uint32Value());
        return v8::Undefined();
    }
    
    // void drawArrays(GLenum mode, GLint first, GLsizei count)
    static v8::Handle<v8::Value> drawArrays(const v8::Arguments& args) {
        if (args.Length() != 3)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        glDrawArrays(args[0]->Uint32Value(),
                     args[1]->Int32Value(), args[2]->Int32Value());
        return v8::Undefined();
    }
    
    // void drawElements(GLenum mode, GLsizei count,
    //                   GLenum type, GLsizeiptr offset)
    static v8::Handle<v8::Value> drawElements(const v8::Arguments& args) {
        if (args.Length() != 4)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        glDrawElements(args[0]->Uint32Value(),
                       args[1]->Int32Value(),
                       args[2]->Uint32Value(),
                       reinterpret_cast<GLvoid*>(args[3]->Int32Value()));
        return v8::Undefined();
    }
    
    // void enable(GLenum cap)
    static v8::Handle<v8::Value> enable(const v8::Arguments& args) {
        if (args.Length() != 1)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        glEnable(args[0]->Uint32Value());
        return v8::Undefined();
    }
    
    // void enableVertexAttribArray(GLuint index)
    static v8::Handle<v8::Value> enableVertexAttribArray(
                                                         const v8::Arguments& args) {
        if (args.Length() != 1)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        glEnableVertexAttribArray(args[0]->Uint32Value());
        return v8::Undefined();
    }
    
    // void finish()
    static v8::Handle<v8::Value> finish(const v8::Arguments& args) {
        glFinish();
        return v8::Undefined();
    }
    
    // void flush()
    static v8::Handle<v8::Value> flush(const v8::Arguments& args) {
        glFlush();
        return v8::Undefined();
    }
    
    // void framebufferRenderbuffer(GLenum target, GLenum attachment,
    //                              GLenum renderbuffertarget,
    //                              WebGLRenderbuffer renderbuffer)
    static v8::Handle<v8::Value> framebufferRenderbuffer(
                                                         const v8::Arguments& args) {
        if (args.Length() != 4)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        // NOTE: ExtractRenderbufferObjectNameFromValue will handle null.
        if (!args[3]->IsNull() && !WebGLRenderbuffer::HasInstance(args[3]))
            return v8_utils::ThrowTypeError("Type error");
        
        glFramebufferRenderbuffer(
                                  args[0]->Uint32Value(),
                                  args[1]->Uint32Value(),
                                  args[2]->Uint32Value(),
                                  WebGLRenderbuffer::ExtractRenderbufferObjectNameFromValue(args[3]));
        return v8::Undefined();
    }
    
    // void framebufferTexture2D(GLenum target, GLenum attachment,
    //                           GLenum textarget, WebGLTexture texture,
    //                           GLint level)
    static v8::Handle<v8::Value> framebufferTexture2D(
                                                      const v8::Arguments& args) {
        if (args.Length() != 5)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        // NOTE: ExtractTextureNameFromValue will handle null.
        if (!args[3]->IsNull() && !WebGLTexture::HasInstance(args[3]))
            return v8_utils::ThrowTypeError("Type error");
        
        glFramebufferTexture2D(args[0]->Uint32Value(),
                               args[1]->Uint32Value(),
                               args[2]->Uint32Value(),
                               WebGLTexture::ExtractTextureNameFromValue(args[3]),
                               args[4]->Int32Value());
        return v8::Undefined();
    }
    
    // void frontFace(GLenum mode)
    static v8::Handle<v8::Value> frontFace(
                                           const v8::Arguments& args) {
        if (args.Length() != 1)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        glFrontFace(args[0]->Uint32Value());
        return v8::Undefined();
    }
    
    // void generateMipmap(GLenum target)
    static v8::Handle<v8::Value> generateMipmap(
                                                const v8::Arguments& args) {
        if (args.Length() != 1)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        glGenerateMipmap(args[0]->Uint32Value());
        return v8::Undefined();
    }
    
    // WebGLActiveInfo getActiveAttrib(WebGLProgram program, GLuint index)
    static v8::Handle<v8::Value> getActiveAttrib(const v8::Arguments& args) {
        if (args.Length() != 2)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        if (!WebGLProgram::HasInstance(args[0]))
            return v8_utils::ThrowTypeError("Type error");
        
        GLuint program = WebGLProgram::ExtractProgramNameFromValue(args[0]);
        
        char namebuf[1024];
        GLint size;
        GLenum type;
        
        glGetActiveAttrib(program, args[1]->Uint32Value(),
                          sizeof(namebuf), NULL, &size, &type, namebuf);
        
        return WebGLActiveInfo::NewFromSizeTypeName(size, type, namebuf);
    }
    
    // WebGLActiveInfo getActiveUniform(WebGLProgram program, GLuint index)
    static v8::Handle<v8::Value> getActiveUniform(const v8::Arguments& args) {
        if (args.Length() != 2)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        if (!WebGLProgram::HasInstance(args[0]))
            return v8_utils::ThrowTypeError("Type error");
        
        GLuint program = WebGLProgram::ExtractProgramNameFromValue(args[0]);
        
        char namebuf[1024];
        GLint size;
        GLenum type;
        
        glGetActiveUniform(program, args[1]->Uint32Value(),
                           sizeof(namebuf), NULL, &size, &type, namebuf);
        
        return WebGLActiveInfo::NewFromSizeTypeName(size, type, namebuf);
    }
    
    // WebGLShader[ ] getAttachedShaders(WebGLProgram program)
    static v8::Handle<v8::Value> getAttachedShaders(const v8::Arguments& args) {
        if (args.Length() != 1)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        if (!WebGLProgram::HasInstance(args[0]))
            return v8_utils::ThrowTypeError("Type error");
        
        GLuint program = WebGLProgram::ExtractProgramNameFromValue(args[0]);
        
        GLuint shaders[10];
        GLsizei count;
        glGetAttachedShaders(program, 10, &count, shaders);
        
        v8::Local<v8::Array> res = v8::Array::New(count);
        for (int i = 0; i < count; ++i) {
            res->Set(v8::Integer::New(i),
                     WebGLShader::LookupFromShaderName(shaders[i]));
        }
        
        return res;
    }
    
    // GLint getAttribLocation(WebGLProgram program, DOMString name)
    static v8::Handle<v8::Value> getAttribLocation(const v8::Arguments& args) {
        if (args.Length() != 2)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        if (!WebGLProgram::HasInstance(args[0]))
            return v8_utils::ThrowTypeError("Type error");
        
        GLuint program = WebGLProgram::ExtractProgramNameFromValue(args[0]);
        
        v8::String::Utf8Value name(args[1]);
        return v8::Integer::New(glGetAttribLocation(program, *name));
    }
    
    // Helper for getParameter, based on
    //   WebCore/html/canvas/WebGLRenderingContext.cpp
    static v8::Handle<v8::Value> getBooleanParameter(unsigned long pname) {
        unsigned char value;
        glGetBooleanv(pname, &value);
        return v8::Boolean::New(static_cast<bool>(value));
    }
    
    static v8::Handle<v8::Value> getBooleanArrayParameter(unsigned long pname) {
        return v8_utils::ThrowError("Unimplemented.");
        //    if (pname != GL_COLOR_WRITEMASK) {
        //      notImplemented();
        //      return static v8::Handle<v8::Value>(0, 0);
        //    }
        //    unsigned char value[4] = {0};
        //    m_context->getBooleanv(pname, value);
        //    bool boolValue[4];
        //    for (int ii = 0; ii < 4; ++ii)
        //        boolValue[ii] = static_cast<bool>(value[ii]);
        //    return static v8::Handle<v8::Value>(boolValue, 4);
    }
    
    static v8::Handle<v8::Value> getFloatParameter(unsigned long pname) {
        float value;
        glGetFloatv(pname, &value);
        return v8::Number::New(value);
    }
    
    static v8::Handle<v8::Value> getIntParameter(unsigned long pname) {
        return getLongParameter(pname);
    }
    
    static v8::Handle<v8::Value> getLongParameter(unsigned long pname) {
        int value;
        glGetIntegerv(pname, &value);
        return v8::Integer::New(static_cast<long>(value));
    }
    
    static v8::Handle<v8::Value> getUnsignedLongParameter(unsigned long pname) {
        int value;
        glGetIntegerv(pname, &value);
        unsigned int uValue = static_cast<unsigned int>(value);
        return v8::Integer::NewFromUnsigned(static_cast<unsigned long>(uValue));
    }
    
    static v8::Handle<v8::Value> getWebGLFloatArrayParameter(unsigned long pname) {
        return v8_utils::ThrowError("Unimplemented.");
        //    float value[4] = {0};
        //    m_context->getFloatv(pname, value);
        //    unsigned length = 0;
        //    switch (pname) {
        //      case GL_ALIASED_POINT_SIZE_RANGE:
        //      case GL_ALIASED_LINE_WIDTH_RANGE:
        //      case GL_DEPTH_RANGE:
        //        length = 2;
        //        break;
        //      case GL_BLEND_COLOR:
        //      case GL_COLOR_CLEAR_VALUE:
        //        length = 4;
        //        break;
        //      default:
        //        notImplemented();
        //    }
        //    return static v8::Handle<v8::Value>(Float32Array::create(value, length));
    }
    
    static v8::Handle<v8::Value> getWebGLIntArrayParameter(unsigned long pname) {
        int value[4] = {0};
        glGetIntegerv(pname, value);
        int length = 0;
        switch (pname) {
            case GL_MAX_VIEWPORT_DIMS:
                length = 2;
                break;
            case GL_SCISSOR_BOX:
            case GL_VIEWPORT:
                length = 4;
                break;
            default:
                return v8_utils::ThrowError("Unimplemented.");
        }
        v8::Handle<v8::Value> ta_args[1] = {v8::Integer::New(length)};
        // TODO(deanm): A better way of getting the TypedArray constructors.
        v8::Handle<v8::Object> ta = v8::Handle<v8::Function>::Cast(
                                                                   v8::Context::GetCurrent()->Global()->
                                                                   Get(v8::String::New("Int32Array")))->NewInstance(1, ta_args);
        for (int i = 0; i < length; ++i) {
            ta->Set(i, v8::Integer::New(value[i]));
        }
        
        return ta;
    }
    
    // any getParameter(GLenum pname)
    static v8::Handle<v8::Value> getParameter(const v8::Arguments& args) {
        if (args.Length() != 1)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        unsigned long pname = args[0]->Uint32Value();
        
        switch (pname) {
            case GL_ACTIVE_TEXTURE:
                return getUnsignedLongParameter(pname);
            case GL_ALIASED_LINE_WIDTH_RANGE:
                return getWebGLFloatArrayParameter(pname);
            case GL_ALIASED_POINT_SIZE_RANGE:
                return getWebGLFloatArrayParameter(pname);
            case GL_ALPHA_BITS:
                return getLongParameter(pname);
            case GL_ARRAY_BUFFER_BINDING:
            case GL_ELEMENT_ARRAY_BUFFER_BINDING:
            {
                int value;
                glGetIntegerv(pname, &value);
                GLuint buffer = static_cast<unsigned int>(value);
                return WebGLBuffer::LookupFromBufferName(buffer);
            }
            case GL_BLEND:
                return getBooleanParameter(pname);
            case GL_BLEND_COLOR:
                return getWebGLFloatArrayParameter(pname);
            case GL_BLEND_DST_ALPHA:
                return getUnsignedLongParameter(pname);
            case GL_BLEND_DST_RGB:
                return getUnsignedLongParameter(pname);
            case GL_BLEND_EQUATION_ALPHA:
                return getUnsignedLongParameter(pname);
            case GL_BLEND_EQUATION_RGB:
                return getUnsignedLongParameter(pname);
            case GL_BLEND_SRC_ALPHA:
                return getUnsignedLongParameter(pname);
            case GL_BLEND_SRC_RGB:
                return getUnsignedLongParameter(pname);
            case GL_BLUE_BITS:
                return getLongParameter(pname);
            case GL_COLOR_CLEAR_VALUE:
                return getWebGLFloatArrayParameter(pname);
            case GL_COLOR_WRITEMASK:
                return getBooleanArrayParameter(pname);
            case GL_COMPRESSED_TEXTURE_FORMATS:
                // Defined as null in the spec
                return v8::Undefined();
            case GL_CULL_FACE:
                return getBooleanParameter(pname);
            case GL_CULL_FACE_MODE:
                return getUnsignedLongParameter(pname);
            case GL_CURRENT_PROGRAM:
            {
                int value;
                glGetIntegerv(pname, &value);
                GLuint program = static_cast<unsigned int>(value);
                return WebGLProgram::LookupFromProgramName(program);
            }
            case GL_DEPTH_BITS:
                return getLongParameter(pname);
            case GL_DEPTH_CLEAR_VALUE:
                return getFloatParameter(pname);
            case GL_DEPTH_FUNC:
                return getUnsignedLongParameter(pname);
            case GL_DEPTH_RANGE:
                return getWebGLFloatArrayParameter(pname);
            case GL_DEPTH_TEST:
                return getBooleanParameter(pname);
            case GL_DEPTH_WRITEMASK:
                return getBooleanParameter(pname);
            case GL_DITHER:
                return getBooleanParameter(pname);
            case GL_FRAMEBUFFER_BINDING:
            {
                int value;
                glGetIntegerv(pname, &value);
                GLuint framebuffer = static_cast<unsigned int>(value);
                return WebGLFramebuffer::LookupFromFramebufferObjectName(framebuffer);
            }
            case GL_FRONT_FACE:
                return getUnsignedLongParameter(pname);
            case GL_GENERATE_MIPMAP_HINT:
                return getUnsignedLongParameter(pname);
            case GL_GREEN_BITS:
                return getLongParameter(pname);
                //case GL_IMPLEMENTATION_COLOR_READ_FORMAT:
                //  return getLongParameter(pname);
                //case GL_IMPLEMENTATION_COLOR_READ_TYPE:
                //  return getLongParameter(pname);
            case GL_LINE_WIDTH:
                return getFloatParameter(pname);
            case GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS:
                return getLongParameter(pname);
            case GL_MAX_CUBE_MAP_TEXTURE_SIZE:
                return getLongParameter(pname);
                //case GL_MAX_FRAGMENT_UNIFORM_VECTORS:
                //  return getLongParameter(pname);
            case GL_MAX_RENDERBUFFER_SIZE:
                return getLongParameter(pname);
            case GL_MAX_TEXTURE_IMAGE_UNITS:
                return getLongParameter(pname);
            case GL_MAX_TEXTURE_SIZE:
                return getLongParameter(pname);
                //case GL_MAX_VARYING_VECTORS:
                //  return getLongParameter(pname);
            case GL_MAX_VERTEX_ATTRIBS:
                return getLongParameter(pname);
            case GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS:
                return getLongParameter(pname);
                //case GL_MAX_VERTEX_UNIFORM_VECTORS:
                //  return getLongParameter(pname);
            case GL_MAX_VIEWPORT_DIMS:
                return getWebGLIntArrayParameter(pname);
            case GL_NUM_COMPRESSED_TEXTURE_FORMATS:
                // WebGL 1.0 specifies that there are no compressed texture formats.
                return v8::Integer::New(0);
                //case GL_NUM_SHADER_BINARY_FORMATS:
                //  // FIXME: should we always return 0 for this?
                //  return getLongParameter(pname);
            case GL_PACK_ALIGNMENT:
                return getLongParameter(pname);
            case GL_POLYGON_OFFSET_FACTOR:
                return getFloatParameter(pname);
            case GL_POLYGON_OFFSET_FILL:
                return getBooleanParameter(pname);
            case GL_POLYGON_OFFSET_UNITS:
                return getFloatParameter(pname);
            case GL_RED_BITS:
                return getLongParameter(pname);
            case GL_RENDERBUFFER_BINDING:
            {
                int value;
                glGetIntegerv(pname, &value);
                GLuint renderbuffer = static_cast<unsigned int>(value);
                return WebGLRenderbuffer::LookupFromRenderbufferObjectName(
                                                                           renderbuffer);
            }
            case GL_RENDERER:
                return v8::String::New(
                                       reinterpret_cast<const char*>(glGetString(pname)));
            case GL_SAMPLE_BUFFERS:
                return getLongParameter(pname);
            case GL_SAMPLE_COVERAGE_INVERT:
                return getBooleanParameter(pname);
            case GL_SAMPLE_COVERAGE_VALUE:
                return getFloatParameter(pname);
            case GL_SAMPLES:
                return getLongParameter(pname);
            case GL_SCISSOR_BOX:
                return getWebGLIntArrayParameter(pname);
            case GL_SCISSOR_TEST:
                return getBooleanParameter(pname);
            case GL_SHADING_LANGUAGE_VERSION:
            {
                std::string str = "WebGL GLSL ES 1.0 (";
                str.append(reinterpret_cast<const char*>(glGetString(pname)));
                str.push_back(')');
                return v8::String::New(str.c_str());
            }
            case GL_STENCIL_BACK_FAIL:
                return getUnsignedLongParameter(pname);
            case GL_STENCIL_BACK_FUNC:
                return getUnsignedLongParameter(pname);
            case GL_STENCIL_BACK_PASS_DEPTH_FAIL:
                return getUnsignedLongParameter(pname);
            case GL_STENCIL_BACK_PASS_DEPTH_PASS:
                return getUnsignedLongParameter(pname);
            case GL_STENCIL_BACK_REF:
                return getLongParameter(pname);
            case GL_STENCIL_BACK_VALUE_MASK:
                return getUnsignedLongParameter(pname);
            case GL_STENCIL_BACK_WRITEMASK:
                return getUnsignedLongParameter(pname);
            case GL_STENCIL_BITS:
                return getLongParameter(pname);
            case GL_STENCIL_CLEAR_VALUE:
                return getLongParameter(pname);
            case GL_STENCIL_FAIL:
                return getUnsignedLongParameter(pname);
            case GL_STENCIL_FUNC:
                return getUnsignedLongParameter(pname);
            case GL_STENCIL_PASS_DEPTH_FAIL:
                return getUnsignedLongParameter(pname);
            case GL_STENCIL_PASS_DEPTH_PASS:
                return getUnsignedLongParameter(pname);
            case GL_STENCIL_REF:
                return getLongParameter(pname);
            case GL_STENCIL_TEST:
                return getBooleanParameter(pname);
            case GL_STENCIL_VALUE_MASK:
                return getUnsignedLongParameter(pname);
            case GL_STENCIL_WRITEMASK:
                return getUnsignedLongParameter(pname);
            case GL_SUBPIXEL_BITS:
                return getLongParameter(pname);
            case GL_TEXTURE_BINDING_2D:
            case GL_TEXTURE_BINDING_CUBE_MAP:
            {
                int value;
                glGetIntegerv(pname, &value);
                GLuint texture = static_cast<unsigned int>(value);
                return WebGLTexture::LookupFromTextureName(texture);
            }
            case GL_UNPACK_ALIGNMENT:
                // FIXME: should this be "long" in the spec?
                return getIntParameter(pname);
                //case GL_UNPACK_FLIP_Y_WEBGL:
                //  return v8_utils::ThrowError("Unimplemented.");
                //case GL_UNPACK_PREMULTIPLY_ALPHA_WEBGL:
                //  return v8_utils::ThrowError("Unimplemented.");
            case GL_VENDOR:
            {
                std::string str = "Plask (";
                str.append(reinterpret_cast<const char*>(glGetString(pname)));
                str.push_back(')');
                return v8::String::New(str.c_str());
            }
            case GL_VERSION:
            {
                std::string str = "WebGL 1.0 (";
                str.append(reinterpret_cast<const char*>(glGetString(pname)));
                str.push_back(')');
                return v8::String::New(str.c_str());
            }
            case GL_VIEWPORT:
                return getWebGLIntArrayParameter(pname);
            default:
                return v8::Undefined();
        }
    }
    
    // any getBufferParameter(GLenum target, GLenum pname)
    static v8::Handle<v8::Value> getBufferParameter(const v8::Arguments& args) {
        if (args.Length() != 1)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        unsigned long pname = args[0]->Uint32Value();
        if (pname == GL_BUFFER_SIZE)
            return getLongParameter(pname);
        else
            return getUnsignedLongParameter(pname);
    }
    
    // GLenum getError()
    static v8::Handle<v8::Value> getError(const v8::Arguments& args) {
        return v8::Integer::NewFromUnsigned(glGetError());
    }
    
    // any getFramebufferAttachmentParameter(GLenum target, GLenum attachment,
    //                                       GLenum pname)
    static v8::Handle<v8::Value> getFramebufferAttachmentParameter(
                                                                   const v8::Arguments& args) {
        return v8_utils::ThrowError("Unimplemented.");
    }
    
    // any getProgramParameter(WebGLProgram program, GLenum pname)
    static v8::Handle<v8::Value> getProgramParameter(const v8::Arguments& args) {
        if (args.Length() != 2)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        if (!WebGLProgram::HasInstance(args[0]))
            return v8_utils::ThrowTypeError("Type error");
        
        GLuint program = WebGLProgram::ExtractProgramNameFromValue(args[0]);
        
        unsigned long pname = args[1]->Uint32Value();
        GLint value = 0;
        switch (pname) {
            case GL_DELETE_STATUS:
            case GL_VALIDATE_STATUS:
            case GL_LINK_STATUS:
                glGetProgramiv(program, pname, &value);
                return v8::Boolean::New(value);
            case GL_INFO_LOG_LENGTH:
            case GL_ATTACHED_SHADERS:
            case GL_ACTIVE_ATTRIBUTES:
            case GL_ACTIVE_ATTRIBUTE_MAX_LENGTH:
            case GL_ACTIVE_UNIFORMS:
            case GL_ACTIVE_UNIFORM_MAX_LENGTH:
                glGetProgramiv(program, pname, &value);
                return v8::Integer::New(value);
            default:
                return v8::Undefined();
        }
    }
    
    // DOMString getProgramInfoLog(WebGLProgram program)
    static v8::Handle<v8::Value> getProgramInfoLog(const v8::Arguments& args) {
        if (args.Length() != 1)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        if (!WebGLProgram::HasInstance(args[0]))
            return v8_utils::ThrowTypeError("Type error");
        
        GLuint program = WebGLProgram::ExtractProgramNameFromValue(args[0]);
        
        GLint length = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        GLchar* buf = new GLchar[length + 1];
        glGetProgramInfoLog(program, length + 1, NULL, buf);
        v8::Handle<v8::Value> res = v8::String::New(buf, length);
        delete[] buf;
        return res;
    }
    
    // any getRenderbufferParameter(GLenum target, GLenum pname)
    static v8::Handle<v8::Value> getRenderbufferParameter(
                                                          const v8::Arguments& args) {
        return v8_utils::ThrowError("Unimplemented.");
    }
    
    // any getShaderParameter(WebGLShader shader, GLenum pname)
    static v8::Handle<v8::Value> getShaderParameter(const v8::Arguments& args) {
        if (args.Length() != 2)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        if (!WebGLShader::HasInstance(args[0]))
            return v8_utils::ThrowTypeError("Type error");
        
        GLuint shader = WebGLShader::ExtractShaderNameFromValue(args[0]);
        unsigned long pname = args[1]->Uint32Value();
        GLint value = 0;
        switch (pname) {
            case GL_DELETE_STATUS:
            case GL_COMPILE_STATUS:
                glGetShaderiv(shader, pname, &value);
                return v8::Boolean::New(value);
            case GL_SHADER_TYPE:
                glGetShaderiv(shader, pname, &value);
                return v8::Integer::NewFromUnsigned(value);
            case GL_INFO_LOG_LENGTH:
            case GL_SHADER_SOURCE_LENGTH:
                glGetShaderiv(shader, pname, &value);
                return v8::Integer::New(value);
            default:
                return v8::Undefined();
        }
    }
    
    // DOMString getShaderInfoLog(WebGLShader shader)
    static v8::Handle<v8::Value> getShaderInfoLog(const v8::Arguments& args) {
        if (args.Length() != 1)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        if (!WebGLShader::HasInstance(args[0]))
            return v8_utils::ThrowTypeError("Type error");
        
        GLuint shader = WebGLShader::ExtractShaderNameFromValue(args[0]);
        GLint length = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        GLchar* buf = new GLchar[length + 1];
        glGetShaderInfoLog(shader, length + 1, NULL, buf);
        v8::Handle<v8::Value> res = v8::String::New(buf, length);
        delete[] buf;
        return res;
    }
    
    // DOMString getShaderSource(WebGLShader shader)
    static v8::Handle<v8::Value> getShaderSource(const v8::Arguments& args) {
        if (args.Length() != 1)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        if (!WebGLShader::HasInstance(args[0]))
            return v8_utils::ThrowTypeError("Type error");
        
        GLuint shader = WebGLShader::ExtractShaderNameFromValue(args[0]);
        GLint length = 0;
        glGetShaderiv(shader, GL_SHADER_SOURCE_LENGTH, &length);
        GLchar* buf = new GLchar[length + 1];
        glGetShaderSource(shader, length + 1, NULL, buf);
        v8::Handle<v8::Value> res = v8::String::New(buf, length);
        delete[] buf;
        return res;
    }
    
    // any getTexParameter(GLenum target, GLenum pname)
    static v8::Handle<v8::Value> getTexParameter(const v8::Arguments& args) {
        return v8_utils::ThrowError("Unimplemented.");
    }
    
    // any getUniform(WebGLProgram program, WebGLUniformLocation location)
    static v8::Handle<v8::Value> getUniform(const v8::Arguments& args) {
        return v8_utils::ThrowError("Unimplemented.");
    }
    
    // WebGLUniformLocation getUniformLocation(WebGLProgram program,
    //                                         DOMString name)
    static v8::Handle<v8::Value> getUniformLocation(const v8::Arguments& args) {
        if (args.Length() != 2)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        if (!WebGLProgram::HasInstance(args[0]))
            return v8_utils::ThrowTypeError("Type error");
        
        GLuint program = WebGLProgram::ExtractProgramNameFromValue(args[0]);
        
        v8::String::Utf8Value name(args[1]);
        GLint location = glGetUniformLocation(program, *name);
        if (location == -1)
            return v8::Null();
        return WebGLUniformLocation::NewFromLocation(location);
    }
    
    // any getVertexAttrib(GLuint index, GLenum pname)
    static v8::Handle<v8::Value> getVertexAttrib(
                                                 const v8::Arguments& args) {
        return v8_utils::ThrowError("Unimplemented.");
    }
    
    // GLsizeiptr getVertexAttribOffset(GLuint index, GLenum pname)
    static v8::Handle<v8::Value> getVertexAttribOffset(
                                                       const v8::Arguments& args) {
        return v8_utils::ThrowError("Unimplemented.");
    }
    
    // void hint(GLenum target, GLenum mode)
    static v8::Handle<v8::Value> hint(const v8::Arguments& args) {
        if (args.Length() != 2)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        glHint(args[0]->Uint32Value(), args[1]->Uint32Value());
        return v8::Undefined();
    }
    
    // GLboolean isBuffer(WebGLBuffer buffer)
    static v8::Handle<v8::Value> isBuffer(const v8::Arguments& args) {
        if (args.Length() != 1)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        // Seems that Chrome does this...
        if (args[0]->IsNull() || args[0]->IsUndefined())
            return v8::False();
        
        if (!WebGLBuffer::HasInstance(args[0]))
            return v8_utils::ThrowTypeError("Type error");
        
        return v8::Boolean::New(glIsBuffer(
                                           WebGLBuffer::ExtractBufferNameFromValue(args[0])));
    }
    
    // GLboolean isEnabled(GLenum cap)
    static v8::Handle<v8::Value> isEnabled(const v8::Arguments& args) {
        if (args.Length() != 1)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        return v8::Boolean::New(glIsEnabled(args[0]->Uint32Value()));
    }
    
    // GLboolean isFramebuffer(WebGLFramebuffer framebuffer)
    static v8::Handle<v8::Value> isFramebuffer(const v8::Arguments& args) {
        if (args.Length() != 1)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        // Seems that Chrome does this...
        if (args[0]->IsNull() || args[0]->IsUndefined())
            return v8::False();
        
        if (!WebGLFramebuffer::HasInstance(args[0]))
            return v8_utils::ThrowTypeError("Type error");
        
        return v8::Boolean::New(glIsFramebuffer(
                                                WebGLFramebuffer::ExtractFramebufferObjectNameFromValue(args[0])));
    }
    
    // GLboolean isProgram(WebGLProgram program)
    static v8::Handle<v8::Value> isProgram(const v8::Arguments& args) {
        if (args.Length() != 1)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        // Seems that Chrome does this...
        if (args[0]->IsNull() || args[0]->IsUndefined())
            return v8::False();
        
        if (!WebGLProgram::HasInstance(args[0]))
            return v8_utils::ThrowTypeError("Type error");
        
        return v8::Boolean::New(glIsProgram(
                                            WebGLProgram::ExtractProgramNameFromValue(args[0])));
    }
    
    // GLboolean isRenderbuffer(WebGLRenderbuffer renderbuffer)
    static v8::Handle<v8::Value> isRenderbuffer(const v8::Arguments& args) {
        if (args.Length() != 1)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        // Seems that Chrome does this...
        if (args[0]->IsNull() || args[0]->IsUndefined())
            return v8::False();
        
        if (!WebGLRenderbuffer::HasInstance(args[0]))
            return v8_utils::ThrowTypeError("Type error");
        
        return v8::Boolean::New(glIsRenderbuffer(
                                                 WebGLRenderbuffer::ExtractRenderbufferObjectNameFromValue(args[0])));
    }
    
    // GLboolean isShader(WebGLShader shader)
    static v8::Handle<v8::Value> isShader(const v8::Arguments& args) {
        if (args.Length() != 1)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        // Seems that Chrome does this...
        if (args[0]->IsNull() || args[0]->IsUndefined())
            return v8::False();
        
        if (!WebGLShader::HasInstance(args[0]))
            return v8_utils::ThrowTypeError("Type error");
        
        return v8::Boolean::New(glIsShader(
                                           WebGLShader::ExtractShaderNameFromValue(args[0])));
    }
    
    // GLboolean isTexture(WebGLTexture texture)
    static v8::Handle<v8::Value> isTexture(const v8::Arguments& args) {
        if (args.Length() != 1)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        // Seems that Chrome does this...
        if (args[0]->IsNull() || args[0]->IsUndefined())
            return v8::False();
        
        if (!WebGLTexture::HasInstance(args[0]))
            return v8_utils::ThrowTypeError("Type error");
        
        return v8::Boolean::New(glIsTexture(
                                            WebGLTexture::ExtractTextureNameFromValue(args[0])));
    }
    
    // void lineWidth(GLfloat width)
    static v8::Handle<v8::Value> lineWidth(const v8::Arguments& args) {
        if (args.Length() != 1)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        glLineWidth(args[0]->NumberValue());
        return v8::Undefined();
    }
    
    // void linkProgram(WebGLProgram program)
    static v8::Handle<v8::Value> linkProgram(const v8::Arguments& args) {
        if (args.Length() != 1)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        if (!WebGLProgram::HasInstance(args[0]))
            return v8_utils::ThrowTypeError("Type error");
        
        glLinkProgram(WebGLProgram::ExtractProgramNameFromValue(args[0]));
        return v8::Undefined();
    }
    
    // void pixelStorei(GLenum pname, GLint param)
    static v8::Handle<v8::Value> pixelStorei(const v8::Arguments& args) {
        if (args.Length() != 2)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        glPixelStorei(args[0]->Uint32Value(), args[1]->Int32Value());
        return v8::Undefined();
    }
    
    // void polygonOffset(GLfloat factor, GLfloat units)
    static v8::Handle<v8::Value> polygonOffset(const v8::Arguments& args) {
        if (args.Length() != 2)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        glPolygonOffset(args[0]->NumberValue(), args[1]->NumberValue());
        return v8::Undefined();
    }
    
    // void readPixels(GLint x, GLint y, GLsizei width, GLsizei height,
    //                 GLenum format, GLenum type, ArrayBufferView pixels)
    static v8::Handle<v8::Value> readPixels(
                                            const v8::Arguments& args) {
        if (args.Length() != 7)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        GLint x = args[0]->Int32Value();
        GLint y = args[1]->Int32Value();
        GLsizei width = args[2]->Int32Value();
        GLsizei height = args[3]->Int32Value();
        GLenum format = args[4]->Int32Value();
        GLenum type = args[5]->Int32Value();
        if (format != GL_RGBA)
            return v8_utils::ThrowError("readPixels only supports GL_RGBA.");
        //format = GL_BGRA;  // TODO(deanm): Fixme.
        
        if (type != GL_UNSIGNED_BYTE)
            return v8_utils::ThrowError("readPixels only supports GL_UNSIGNED_BYTE.");
        
        if (!args[6]->IsObject())
            return v8_utils::ThrowError("readPixels only supports Uint8Array.");
        
        v8::Handle<v8::Object> data = v8::Handle<v8::Object>::Cast(args[6]);
        
        if (data->GetIndexedPropertiesExternalArrayDataType() !=
            v8::kExternalUnsignedByteArray)
            return v8_utils::ThrowError("readPixels only supports Uint8Array.");
        
        // TODO(deanm):  From the spec (requires synthesizing gl errors):
        //   If pixels is non-null, but is not large enough to retrieve all of the
        //   pixels in the specified rectangle taking into account pixel store
        //   modes, an INVALID_OPERATION value is generated.
        if (data->GetIndexedPropertiesExternalArrayDataLength() < width*height*4)
            return v8_utils::ThrowError("Uint8Array buffer too small.");
        
        glReadPixels(x, y, width, height, format, type,
                     data->GetIndexedPropertiesExternalArrayData());
        return v8::Undefined();
    }
    
    // void renderbufferStorage(GLenum target, GLenum internalformat,
    //                          GLsizei width, GLsizei height)
    static v8::Handle<v8::Value> renderbufferStorage(const v8::Arguments& args) {
        if (args.Length() != 4)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        glRenderbufferStorage(args[0]->Uint32Value(),
                              args[1]->Uint32Value(),
                              args[2]->Int32Value(),
                              args[3]->Int32Value());
        return v8::Undefined();
    }
    
    // void sampleCoverage(GLclampf value, GLboolean invert)
    static v8::Handle<v8::Value> sampleCoverage(const v8::Arguments& args) {
        if (args.Length() != 2)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        glSampleCoverage(args[0]->NumberValue(),
                         args[1]->BooleanValue());
        return v8::Undefined();
    }
    
    // void scissor(GLint x, GLint y, GLsizei width, GLsizei height)
    static v8::Handle<v8::Value> scissor(const v8::Arguments& args) {
        if (args.Length() != 4)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        glScissor(args[0]->Int32Value(),
                  args[1]->Int32Value(),
                  args[2]->Int32Value(),
                  args[3]->Int32Value());
        return v8::Undefined();
    }
    
    // void shaderSource(WebGLShader shader, DOMString source)
    static v8::Handle<v8::Value> shaderSource(const v8::Arguments& args) {
        if (args.Length() != 2)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        if (!WebGLShader::HasInstance(args[0]))
            return v8_utils::ThrowTypeError("Type error");
        
        GLuint shader = WebGLShader::ExtractShaderNameFromValue(args[0]);
        
        v8::String::Utf8Value data(args[1]);
        // NOTE(deanm): We want GLSL version 1.20.  Is there a better way to do this
        // than sneaking in a #version at the beginning?
        const GLchar* strs[] = { "#version 120\n", *data };
        glShaderSource(shader, 2, strs, NULL);
        return v8::Undefined();
    }
    
    // void stencilFunc(GLenum func, GLint ref, GLuint mask)
    static v8::Handle<v8::Value> stencilFunc(const v8::Arguments& args) {
        if (args.Length() != 3)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        glStencilFunc(args[0]->Uint32Value(),
                      args[1]->Int32Value(),
                      args[2]->Uint32Value());
        return v8::Undefined();
    }
    
    // void stencilFuncSeparate(GLenum face, GLenum func, GLint ref, GLuint mask)
    static v8::Handle<v8::Value> stencilFuncSeparate(const v8::Arguments& args) {
        if (args.Length() != 4)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        glStencilFuncSeparate(args[0]->Uint32Value(),
                              args[1]->Uint32Value(),
                              args[2]->Int32Value(),
                              args[3]->Uint32Value());
        return v8::Undefined();
    }
    
    // void stencilMask(GLuint mask)
    static v8::Handle<v8::Value> stencilMask(const v8::Arguments& args) {
        if (args.Length() != 1)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        glStencilMask(args[0]->Uint32Value());
        return v8::Undefined();
    }
    
    // void stencilMaskSeparate(GLenum face, GLuint mask)
    static v8::Handle<v8::Value> stencilMaskSeparate(const v8::Arguments& args) {
        if (args.Length() != 2)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        glStencilMaskSeparate(args[0]->Uint32Value(), args[1]->Uint32Value());
        return v8::Undefined();
    }
    
    // void stencilOp(GLenum fail, GLenum zfail, GLenum zpass)
    static v8::Handle<v8::Value> stencilOp(const v8::Arguments& args) {
        if (args.Length() != 3)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        glStencilOp(args[0]->Uint32Value(),
                    args[1]->Uint32Value(),
                    args[2]->Uint32Value());
        return v8::Undefined();
    }
    
    // void stencilOpSeparate(GLenum face, GLenum fail,
    //                        GLenum zfail, GLenum zpass)
    static v8::Handle<v8::Value> stencilOpSeparate(const v8::Arguments& args) {
        if (args.Length() != 4)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        glStencilOpSeparate(args[0]->Uint32Value(),
                            args[1]->Uint32Value(),
                            args[2]->Uint32Value(),
                            args[3]->Uint32Value());
        return v8::Undefined();
    }
    
    // void texImage2D(GLenum target, GLint level, GLenum internalformat,
    //                 GLsizei width, GLsizei height, GLint border,
    //                 GLenum format, GLenum type, ArrayBufferView pixels)
    // void texImage2D(GLenum target, GLint level, GLenum internalformat,
    //                 GLenum format, GLenum type, ImageData pixels)
    // void texImage2D(GLenum target, GLint level, GLenum internalformat,
    //                 GLenum format, GLenum type, HTMLImageElement image)
    // void texImage2D(GLenum target, GLint level, GLenum internalformat,
    //                 GLenum format, GLenum type, HTMLCanvasElement canvas)
    // void texImage2D(GLenum target, GLint level, GLenum internalformat,
    //                 GLenum format, GLenum type, HTMLVideoElement video)
    static v8::Handle<v8::Value> texImage2D(const v8::Arguments& args) {
        if (args.Length() != 9)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        GLvoid* data = NULL;
        
        if (!args[8]->IsNull()) {
            if (!args[8]->IsObject())
                return v8_utils::ThrowError("Data must be an ArrayBuffer.");
            
            v8::Local<v8::Object> obj = v8::Local<v8::Object>::Cast(args[8]);
            if (!obj->HasIndexedPropertiesInExternalArrayData())
                return v8_utils::ThrowError("Data must be an ArrayBuffer.");
            
            // TODO(deanm): Check size / format.  For now just use it correctly.
            data = obj->GetIndexedPropertiesExternalArrayData();
        }
        
        // TODO(deanm): Support more than just the zero initialization case.
        glTexImage2D(args[0]->Uint32Value(),  // target
                     args[1]->Int32Value(),   // level
                     args[2]->Int32Value(),   // internalFormat
                     args[3]->Int32Value(),   // width
                     args[4]->Int32Value(),   // height
                     args[5]->Int32Value(),   // border
                     args[6]->Uint32Value(),  // format
                     args[7]->Uint32Value(),  // type
                     data);                   // data
        return v8::Undefined();
    }
    
    // NOTE: implemented outside of class definition (SkCanvasWrapper dependency).
    static v8::Handle<v8::Value> texImage2DSkCanvasB(const v8::Arguments& args);
    static v8::Handle<v8::Value> drawSkCanvas(const v8::Arguments& args);
    
    // void texParameterf(GLenum target, GLenum pname, GLfloat param)
    static v8::Handle<v8::Value> texParameterf(const v8::Arguments& args) {
        if (args.Length() != 3)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        glTexParameterf(args[0]->Uint32Value(),
                        args[1]->Uint32Value(),
                        args[2]->NumberValue());
        return v8::Undefined();
    }
    
    // void texParameteri(GLenum target, GLenum pname, GLint param)
    static v8::Handle<v8::Value> texParameteri(const v8::Arguments& args) {
        if (args.Length() != 3)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        glTexParameteri(args[0]->Uint32Value(),
                        args[1]->Uint32Value(),
                        args[2]->Int32Value());
        return v8::Undefined();
    }
    
    // void texSubImage2D(GLenum target, GLint level,
    //                    GLint xoffset, GLint yoffset,
    //                    GLsizei width, GLsizei height,
    //                    GLenum format, GLenum type, ArrayBufferView pixels)
    // void texSubImage2D(GLenum target, GLint level,
    //                    GLint xoffset, GLint yoffset,
    //                    GLenum format, GLenum type, ImageData pixels)
    // void texSubImage2D(GLenum target, GLint level,
    //                    GLint xoffset, GLint yoffset,
    //                    GLenum format, GLenum type, HTMLImageElement image)
    // void texSubImage2D(GLenum target, GLint level,
    //                    GLint xoffset, GLint yoffset,
    //                    GLenum format, GLenum type, HTMLCanvasElement canvas)
    // void texSubImage2D(GLenum target, GLint level,
    //                    GLint xoffset, GLint yoffset,
    //                    GLenum format, GLenum type, HTMLVideoElement video)
    
    static v8::Handle<v8::Value> texSubImage2D(const v8::Arguments& args) {
        if (args.Length() != 9)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        GLvoid* data = NULL;
        
        if (!args[8]->IsNull()) {
            if (!args[8]->IsObject())
                return v8_utils::ThrowError("Data must be an ArrayBuffer.");
            
            v8::Local<v8::Object> obj = v8::Local<v8::Object>::Cast(args[8]);
            if (!obj->HasIndexedPropertiesInExternalArrayData())
                return v8_utils::ThrowError("Data must be an ArrayBuffer.");
            
            // TODO(deanm): Check size / format.  For now just use it correctly.
            data = obj->GetIndexedPropertiesExternalArrayData();
        }
        
        glTexSubImage2D(args[0]->Uint32Value(),  // target
                        args[1]->Int32Value(),   // level
                        args[2]->Int32Value(),   // xoffset
                        args[3]->Int32Value(),   // yoffset
                        args[4]->Int32Value(),   // width
                        args[5]->Int32Value(),   // height
                        args[6]->Uint32Value(),  // format
                        args[7]->Uint32Value(),  // type
                        data);                   // data
        return v8::Undefined();
    }
    
    static v8::Handle<v8::Value> uniformfvHelper(
                                                 void (*uniformFunc)(GLint, GLsizei, const GLfloat*),
                                                 GLsizei numcomps,
                                                 const v8::Arguments& args) {
        if (args.Length() != 2)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        if (args[0]->IsNull())  // null location is silently ignored.
            return v8::Undefined();
        
        if (!WebGLUniformLocation::HasInstance(args[0]))
            return v8_utils::ThrowTypeError("Expected a WebGLUniformLocation.");
        GLuint location = WebGLUniformLocation::ExtractLocationFromValue(args[0]);
        
        int length = 0;
        if (!args[1]->IsObject())
            return v8_utils::ThrowError("value must be an Sequence.");
        
        v8::Handle<v8::Object> obj = v8::Handle<v8::Object>::Cast(args[1]);
        if (obj->HasIndexedPropertiesInExternalArrayData()) {
            length = obj->GetIndexedPropertiesExternalArrayDataLength();
        } else if (obj->IsArray()) {
            length = v8::Handle<v8::Array>::Cast(obj)->Length();
        } else {
            return v8_utils::ThrowError("value must be an Sequence.");
        }
        
        if (length % numcomps)
            return v8_utils::ThrowError("Sequence size not multiple of components.");
        
        float* buffer = new float[length];
        if (!buffer)
            return v8_utils::ThrowError("Unable to allocate memory for sequence.");
        
        for (int i = 0; i < length; ++i) {
            buffer[i] = obj->Get(i)->NumberValue();
        }
        uniformFunc(location, length / numcomps, buffer);
        delete[] buffer;
        return v8::Undefined();
    }
    
    static v8::Handle<v8::Value> uniformivHelper(
                                                 void (*uniformFunc)(GLint, GLsizei, const GLint*),
                                                 GLsizei numcomps,
                                                 const v8::Arguments& args) {
        if (args.Length() != 2)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        if (args[0]->IsNull())  // null location is silently ignored.
            return v8::Undefined();
        
        if (!WebGLUniformLocation::HasInstance(args[0]))
            return v8_utils::ThrowTypeError("Expected a WebGLUniformLocation.");
        GLuint location = WebGLUniformLocation::ExtractLocationFromValue(args[0]);
        
        int length = 0;
        if (!args[1]->IsObject())
            return v8_utils::ThrowError("value must be an Sequence.");
        
        v8::Handle<v8::Object> obj = v8::Handle<v8::Object>::Cast(args[1]);
        if (obj->HasIndexedPropertiesInExternalArrayData()) {
            length = obj->GetIndexedPropertiesExternalArrayDataLength();
        } else if (obj->IsArray()) {
            length = v8::Handle<v8::Array>::Cast(obj)->Length();
        } else {
            return v8_utils::ThrowError("value must be an Sequence.");
        }
        
        if (length % numcomps)
            return v8_utils::ThrowError("Sequence size not multiple of components.");
        
        GLint* buffer = new GLint[length];
        if (!buffer)
            return v8_utils::ThrowError("Unable to allocate memory for sequence.");
        
        for (int i = 0; i < length; ++i) {
            buffer[i] = obj->Get(i)->Int32Value();
        }
        uniformFunc(location, length / numcomps, buffer);
        delete[] buffer;
        return v8::Undefined();
    }
    
    // void uniform1f(WebGLUniformLocation location, GLfloat x)
    static v8::Handle<v8::Value> uniform1f(const v8::Arguments& args) {
        if (args.Length() != 2)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        if (args[0]->IsNull())  // null location is silently ignored.
            return v8::Undefined();
        
        if (!WebGLUniformLocation::HasInstance(args[0]))
            return v8_utils::ThrowTypeError("Expected a WebGLUniformLocation.");
        GLuint location = WebGLUniformLocation::ExtractLocationFromValue(args[0]);
        
        glUniform1f(location, args[1]->NumberValue());
        return v8::Undefined();
    }
    
    // void uniform1fv(WebGLUniformLocation location, Float32Array v)
    // void uniform1fv(WebGLUniformLocation location, sequence v)
    static v8::Handle<v8::Value> uniform1fv(const v8::Arguments& args) {
        return uniformfvHelper(glUniform1fv, 1, args);
    }
    
    // void uniform1i(WebGLUniformLocation location, GLint x)
    static v8::Handle<v8::Value> uniform1i(const v8::Arguments& args) {
        if (args.Length() != 2)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        if (args[0]->IsNull())  // null location is silently ignored.
            return v8::Undefined();
        
        if (!WebGLUniformLocation::HasInstance(args[0]))
            return v8_utils::ThrowTypeError("Expected a WebGLUniformLocation.");
        GLuint location = WebGLUniformLocation::ExtractLocationFromValue(args[0]);
        
        glUniform1i(location, args[1]->Int32Value());
        return v8::Undefined();
    }
    
    // void uniform1iv(WebGLUniformLocation location, Int32Array v)
    // void uniform1iv(WebGLUniformLocation location, sequence v)
    static v8::Handle<v8::Value> uniform1iv(const v8::Arguments& args) {
        return uniformivHelper(glUniform1iv, 1, args);
    }
    
    // void uniform2f(WebGLUniformLocation location, GLfloat x, GLfloat y)
    static v8::Handle<v8::Value> uniform2f(const v8::Arguments& args) {
        if (args.Length() != 3)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        if (args[0]->IsNull())  // null location is silently ignored.
            return v8::Undefined();
        
        if (!WebGLUniformLocation::HasInstance(args[0]))
            return v8_utils::ThrowTypeError("Expected a WebGLUniformLocation.");
        GLuint location = WebGLUniformLocation::ExtractLocationFromValue(args[0]);
        
        glUniform2f(location,
                    args[1]->NumberValue(),
                    args[2]->NumberValue());
        return v8::Undefined();
    }
    
    // void uniform2fv(WebGLUniformLocation location, Float32Array v)
    // void uniform2fv(WebGLUniformLocation location, sequence v)
    static v8::Handle<v8::Value> uniform2fv(const v8::Arguments& args) {
        return uniformfvHelper(glUniform2fv, 2, args);
    }
    
    // void uniform2i(WebGLUniformLocation location, GLint x, GLint y)
    static v8::Handle<v8::Value> uniform2i(const v8::Arguments& args) {
        if (args.Length() != 3)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        if (args[0]->IsNull())  // null location is silently ignored.
            return v8::Undefined();
        
        if (!WebGLUniformLocation::HasInstance(args[0]))
            return v8_utils::ThrowTypeError("Expected a WebGLUniformLocation.");
        GLuint location = WebGLUniformLocation::ExtractLocationFromValue(args[0]);
        
        glUniform2i(location,
                    args[1]->Int32Value(),
                    args[2]->Int32Value());
        return v8::Undefined();
    }
    
    // void uniform2iv(WebGLUniformLocation location, Int32Array v)
    // void uniform2iv(WebGLUniformLocation location, sequence v)
    static v8::Handle<v8::Value> uniform2iv(const v8::Arguments& args) {
        return uniformivHelper(glUniform2iv, 2, args);
    }
    
    // void uniform3f(WebGLUniformLocation location, GLfloat x, GLfloat y,
    //                GLfloat z)
    static v8::Handle<v8::Value> uniform3f(const v8::Arguments& args) {
        if (args.Length() != 4)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        if (args[0]->IsNull())  // null location is silently ignored.
            return v8::Undefined();
        
        if (!WebGLUniformLocation::HasInstance(args[0]))
            return v8_utils::ThrowTypeError("Expected a WebGLUniformLocation.");
        GLuint location = WebGLUniformLocation::ExtractLocationFromValue(args[0]);
        
        glUniform3f(location,
                    args[1]->NumberValue(),
                    args[2]->NumberValue(),
                    args[3]->NumberValue());
        return v8::Undefined();
    }
    
    // void uniform3fv(WebGLUniformLocation location, Float32Array v)
    // void uniform3fv(WebGLUniformLocation location, sequence v)
    static v8::Handle<v8::Value> uniform3fv(const v8::Arguments& args) {
        return uniformfvHelper(glUniform3fv, 3, args);
    }
    
    // void uniform3i(WebGLUniformLocation location, GLint x, GLint y, GLint z)
    static v8::Handle<v8::Value> uniform3i(const v8::Arguments& args) {
        if (args.Length() != 4)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        if (args[0]->IsNull())  // null location is silently ignored.
            return v8::Undefined();
        
        if (!WebGLUniformLocation::HasInstance(args[0]))
            return v8_utils::ThrowTypeError("Expected a WebGLUniformLocation.");
        GLuint location = WebGLUniformLocation::ExtractLocationFromValue(args[0]);
        
        glUniform3i(location,
                    args[1]->Int32Value(),
                    args[2]->Int32Value(),
                    args[3]->Int32Value());
        return v8::Undefined();
    }
    
    // void uniform3iv(WebGLUniformLocation location, Int32Array v)
    // void uniform3iv(WebGLUniformLocation location, sequence v)
    static v8::Handle<v8::Value> uniform3iv(const v8::Arguments& args) {
        return uniformivHelper(glUniform3iv, 3, args);
    }
    
    // void uniform4f(WebGLUniformLocation location, GLfloat x, GLfloat y,
    //                GLfloat z, GLfloat w)
    static v8::Handle<v8::Value> uniform4f(const v8::Arguments& args) {
        if (args.Length() != 5)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        if (args[0]->IsNull())  // null location is silently ignored.
            return v8::Undefined();
        
        if (!WebGLUniformLocation::HasInstance(args[0]))
            return v8_utils::ThrowTypeError("Expected a WebGLUniformLocation.");
        GLuint location = WebGLUniformLocation::ExtractLocationFromValue(args[0]);
        
        glUniform4f(location,
                    args[1]->NumberValue(),
                    args[2]->NumberValue(),
                    args[3]->NumberValue(),
                    args[4]->NumberValue());
        return v8::Undefined();
    }
    
    // void uniform4fv(WebGLUniformLocation location, Float32Array v)
    // void uniform4fv(WebGLUniformLocation location, sequence v)
    static v8::Handle<v8::Value> uniform4fv(const v8::Arguments& args) {
        return uniformfvHelper(glUniform4fv, 4, args);
    }
    
    // void uniform4i(WebGLUniformLocation location, GLint x, GLint y,
    //                GLint z, GLint w)
    static v8::Handle<v8::Value> uniform4i(const v8::Arguments& args) {
        if (args.Length() != 5)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        if (args[0]->IsNull())  // null location is silently ignored.
            return v8::Undefined();
        
        if (!WebGLUniformLocation::HasInstance(args[0]))
            return v8_utils::ThrowTypeError("Expected a WebGLUniformLocation.");
        GLuint location = WebGLUniformLocation::ExtractLocationFromValue(args[0]);
        
        glUniform4i(location,
                    args[1]->Int32Value(),
                    args[2]->Int32Value(),
                    args[3]->Int32Value(),
                    args[4]->Int32Value());
        return v8::Undefined();
    }
    
    // void uniform4iv(WebGLUniformLocation location, Int32Array v)
    // void uniform4iv(WebGLUniformLocation location, sequence v)
    static v8::Handle<v8::Value> uniform4iv(const v8::Arguments& args) {
        return uniformivHelper(glUniform4iv, 4, args);
    }
    
    static v8::Handle<v8::Value> uniformMatrixfvHelper(
                                                       void (*uniformFunc)(GLint, GLsizei, GLboolean, const GLfloat*),
                                                       GLsizei numcomps,
                                                       const v8::Arguments& args) {
        if (args.Length() != 3)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        if (args[0]->IsNull())  // null location is silently ignored.
            return v8::Undefined();
        
        if (!WebGLUniformLocation::HasInstance(args[0]))
            return v8_utils::ThrowTypeError("Expected a WebGLUniformLocation.");
        GLuint location = WebGLUniformLocation::ExtractLocationFromValue(args[0]);
        
        int length = 0;
        if (!args[2]->IsObject())
            return v8_utils::ThrowError("value must be an Sequence.");
        
        v8::Handle<v8::Object> obj = v8::Handle<v8::Object>::Cast(args[2]);
        if (obj->HasIndexedPropertiesInExternalArrayData()) {
            length = obj->GetIndexedPropertiesExternalArrayDataLength();
        } else if (obj->IsArray()) {
            length = v8::Handle<v8::Array>::Cast(obj)->Length();
        } else {
            return v8_utils::ThrowError("value must be an Sequence.");
        }
        
        if (length % numcomps)
            return v8_utils::ThrowError("Sequence size not multiple of components.");
        
        float* buffer = new float[length];
        if (!buffer)
            return v8_utils::ThrowError("Unable to allocate memory for sequence.");
        
        for (int i = 0; i < length; ++i) {
            buffer[i] = obj->Get(i)->NumberValue();
        }
        uniformFunc(location, length / numcomps, GL_FALSE, buffer);
        delete[] buffer;
        return v8::Undefined();
    }
    
    // void uniformMatrix2fv(WebGLUniformLocation location, GLboolean transpose,
    //                       Float32Array value)
    // void uniformMatrix2fv(WebGLUniformLocation location, GLboolean transpose,
    //                       sequence value)
    static v8::Handle<v8::Value> uniformMatrix2fv(const v8::Arguments& args) {
        return uniformMatrixfvHelper(glUniformMatrix2fv, 4, args);
    }
    
    // void uniformMatrix3fv(WebGLUniformLocation location, GLboolean transpose,
    //                       Float32Array value)
    // void uniformMatrix3fv(WebGLUniformLocation location, GLboolean transpose,
    //                       sequence value)
    static v8::Handle<v8::Value> uniformMatrix3fv(const v8::Arguments& args) {
        return uniformMatrixfvHelper(glUniformMatrix3fv, 9, args);
    }
    
    // void uniformMatrix4fv(WebGLUniformLocation location, GLboolean transpose,
    //                       Float32Array value)
    // void uniformMatrix4fv(WebGLUniformLocation location, GLboolean transpose,
    //                       sequence value)
    static v8::Handle<v8::Value> uniformMatrix4fv(const v8::Arguments& args) {
        return uniformMatrixfvHelper(glUniformMatrix4fv, 16, args);
    }
    
    // void useProgram(WebGLProgram program)
    static v8::Handle<v8::Value> useProgram(const v8::Arguments& args) {
        if (args.Length() != 1)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        // Break the WebGL spec by allowing you to pass 'null' to unbind
        // the shader, handy for drawSkCanvas, for example.
        // NOTE: ExtractProgramNameFromValue handles null.
        if (!args[0]->IsNull() && !WebGLProgram::HasInstance(args[0]))
            return v8_utils::ThrowTypeError("Type error");
        
        glUseProgram(WebGLProgram::ExtractProgramNameFromValue(args[0]));
        return v8::Undefined();
    }
    
    // void validateProgram(WebGLProgram program)
    static v8::Handle<v8::Value> validateProgram(const v8::Arguments& args) {
        if (args.Length() != 1)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        if (!WebGLProgram::HasInstance(args[0]))
            return v8_utils::ThrowTypeError("Type error");
        
        GLuint program = WebGLProgram::ExtractProgramNameFromValue(args[0]);
        
        glValidateProgram(program);
        return v8::Undefined();
    }
    
    // NOTE: The array forms (functions that end in v) are handled in plask.js.
    
    // void vertexAttrib1f(GLuint indx, GLfloat x)
    // void vertexAttrib1fv(GLuint indx, Float32Array values)
    // void vertexAttrib1fv(GLuint indx, sequence values)
    static v8::Handle<v8::Value> vertexAttrib1f(const v8::Arguments& args) {
        if (args.Length() != 2)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        glVertexAttrib1f(args[0]->Uint32Value(),
                         args[1]->NumberValue());
        return v8::Undefined();
    }
    
    // void vertexAttrib2f(GLuint indx, GLfloat x, GLfloat y)
    // void vertexAttrib2fv(GLuint indx, Float32Array values)
    // void vertexAttrib2fv(GLuint indx, sequence values)
    static v8::Handle<v8::Value> vertexAttrib2f(const v8::Arguments& args) {
        if (args.Length() != 3)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        glVertexAttrib2f(args[0]->Uint32Value(),
                         args[1]->NumberValue(),
                         args[2]->NumberValue());
        return v8::Undefined();
    }
    
    // void vertexAttrib3f(GLuint indx, GLfloat x, GLfloat y, GLfloat z)
    // void vertexAttrib3fv(GLuint indx, Float32Array values)
    // void vertexAttrib3fv(GLuint indx, sequence values)
    static v8::Handle<v8::Value> vertexAttrib3f(const v8::Arguments& args) {
        if (args.Length() != 4)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        glVertexAttrib3f(args[0]->Uint32Value(),
                         args[1]->NumberValue(),
                         args[2]->NumberValue(),
                         args[3]->NumberValue());
        return v8::Undefined();
    }
    
    // void vertexAttrib4f(GLuint indx, GLfloat x, GLfloat y,
    //                     GLfloat z, GLfloat w)
    // void vertexAttrib4fv(GLuint indx, Float32Array values)
    // void vertexAttrib4fv(GLuint indx, sequence values)
    static v8::Handle<v8::Value> vertexAttrib4f(const v8::Arguments& args) {
        if (args.Length() != 5)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        glVertexAttrib4f(args[0]->Uint32Value(),
                         args[1]->NumberValue(),
                         args[2]->NumberValue(),
                         args[3]->NumberValue(),
                         args[4]->NumberValue());
        return v8::Undefined();
    }
    
    // void vertexAttribPointer(GLuint indx, GLint size, GLenum type,
    //                          GLboolean normalized, GLsizei stride,
    //                          GLsizeiptr offset)
    static v8::Handle<v8::Value> vertexAttribPointer(const v8::Arguments& args) {
        if (args.Length() != 6)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        glVertexAttribPointer(args[0]->Uint32Value(),
                              args[1]->Int32Value(),
                              args[2]->Uint32Value(),
                              args[3]->BooleanValue(),
                              args[4]->Int32Value(),
                              reinterpret_cast<GLvoid*>(args[5]->Int32Value()));
        return v8::Undefined();
    }
    
    // void viewport(GLint x, GLint y, GLsizei width, GLsizei height)
    static v8::Handle<v8::Value> viewport(const v8::Arguments& args) {
        if (args.Length() != 4)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        glViewport(args[0]->Int32Value(),
                   args[1]->Int32Value(),
                   args[2]->Int32Value(),
                   args[3]->Int32Value());
        return v8::Undefined();
    }
    
    // void DrawBuffersARB(sizei n, const enum *bufs);
    static v8::Handle<v8::Value> drawBuffers(const v8::Arguments& args) {
        if (args.Length() != 1)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        if (!args[0]->IsArray())
            return v8_utils::ThrowError("Sequence must be an Array.");
        
        v8::Local<v8::Array> arr = v8::Local<v8::Array>::Cast(args[0]);
        
        uint32_t length = arr->Length();
        GLenum* attachments = new GLenum[length];
        for (uint32_t i = 0; i < length; ++i) {
            attachments[i] = arr->Get(i)->Uint32Value();
        }
        
        glDrawBuffers(length, attachments);
        delete[] attachments;
        return v8::Undefined();
    }
    
    // void glBlitFramebuffer(GLint srcX0,
    //                        GLint srcY0,
    //                        GLint srcX1,
    //                        GLint srcY1,
    //                        GLint dstX0,
    //                        GLint dstY0,
    //                        GLint dstX1,
    //                        GLint dstY1,
    //                        GLbitfield mask,
    //                        GLenum filter);
    static v8::Handle<v8::Value> blitFramebuffer(const v8::Arguments& args) {
        if (args.Length() != 10)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        glBlitFramebuffer(args[0]->Int32Value(),
                          args[1]->Int32Value(),
                          args[2]->Int32Value(),
                          args[3]->Int32Value(),
                          args[4]->Int32Value(),
                          args[5]->Int32Value(),
                          args[6]->Int32Value(),
                          args[7]->Int32Value(),
                          args[8]->Uint32Value(),
                          args[9]->Uint32Value());
        return v8::Undefined();
    }
};
