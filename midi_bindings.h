#include "bindings_common.h"
#include <CoreAudio/CoreAudio.h>
#include <CoreMIDI/CoreMIDI.h>

// MIDI notes (pun pun):
// Like UTF-8, there is tagging to identify the begin of a message.
// All first bytes are in the range of 0x80 - 0xff, the MSB is set.
// 8 = Note Off
// 9 = Note On
// A = AfterTouch (ie, key pressure)
// B = Control Change
// C = Program (patch) change
// D = Channel Pressure
// E = Pitch Wheel

// TODO(deanm): Global MIDIClientCreate, lazily initialized, ever torn down?
MIDIClientRef g_midi_client = NULL;

// This function was submitted by Douglas Casey Tucker and apparently
// derived largely from PortMidi.  (From RtMidi).
static CFStringRef EndpointName( MIDIEndpointRef endpoint, bool isExternal ) CF_RETURNS_RETAINED;
static CFStringRef EndpointName( MIDIEndpointRef endpoint, bool isExternal )
{
    CFMutableStringRef result = CFStringCreateMutable( NULL, 0 );
    CFStringRef str;
    
    // Begin with the endpoint's name.
    str = NULL;
    MIDIObjectGetStringProperty( endpoint, kMIDIPropertyName, &str );
    if ( str != NULL ) {
        CFStringAppend( result, str );
        CFRelease( str );
    }
    
    MIDIEntityRef entity = 0;
    MIDIEndpointGetEntity( endpoint, &entity );
    if ( entity == 0 )
        // probably virtual
        return result;
    
    if ( CFStringGetLength( result ) == 0 ) {
        // endpoint name has zero length -- try the entity
        str = NULL;
        MIDIObjectGetStringProperty( entity, kMIDIPropertyName, &str );
        if ( str != NULL ) {
            CFStringAppend( result, str );
            CFRelease( str );
        }
    }
    // now consider the device's name
    MIDIDeviceRef device = 0;
    MIDIEntityGetDevice( entity, &device );
    if ( device == 0 )
        return result;
    
    str = NULL;
    MIDIObjectGetStringProperty( device, kMIDIPropertyName, &str );
    if ( CFStringGetLength( result ) == 0 ) {
        CFRelease( result );
        return str;
    }
    if ( str != NULL ) {
        // if an external device has only one entity, throw away
        // the endpoint name and just use the device name
        if ( isExternal && MIDIDeviceGetNumberOfEntities( device ) < 2 ) {
            CFRelease( result );
            return str;
        } else {
            if ( CFStringGetLength( str ) == 0 ) {
                CFRelease( str );
                return result;
            }
            // does the entity name already start with the device name?
            // (some drivers do this though they shouldn't)
            // if so, do not prepend
            if ( CFStringCompareWithOptions( result, /* endpoint name */
                                            str /* device name */,
                                            CFRangeMake(0, CFStringGetLength( str ) ), 0 ) != kCFCompareEqualTo ) {
                // prepend the device name to the entity name
                if ( CFStringGetLength( result ) > 0 )
                    CFStringInsert( result, 0, CFSTR(" ") );
                CFStringInsert( result, 0, str );
            }
            CFRelease( str );
        }
    }
    return result;
}

// This function was submitted by Douglas Casey Tucker and apparently
// derived largely from PortMidi.  (From RtMidi).
static CFStringRef ConnectedEndpointName( MIDIEndpointRef endpoint ) CF_RETURNS_RETAINED;
static CFStringRef ConnectedEndpointName( MIDIEndpointRef endpoint )
{
    CFMutableStringRef result = CFStringCreateMutable( NULL, 0 );
    CFStringRef str;
    OSStatus err;
    int i;
    
    // Does the endpoint have connections?
    CFDataRef connections = NULL;
    int nConnected = 0;
    bool anyStrings = false;
    err = MIDIObjectGetDataProperty( endpoint, kMIDIPropertyConnectionUniqueID, &connections );
    if ( err == noErr && connections != NULL ) {
        // It has connections, follow them
        // Concatenate the names of all connected devices
        nConnected = CFDataGetLength( connections ) / sizeof(MIDIUniqueID);
        if ( nConnected ) {
            const SInt32 *pid = (const SInt32 *)(CFDataGetBytePtr(connections));
            for ( i=0; i<nConnected; ++i, ++pid ) {
                MIDIUniqueID id = EndianS32_BtoN( *pid );
                MIDIObjectRef connObject;
                MIDIObjectType connObjectType;
                err = MIDIObjectFindByUniqueID( id, &connObject, &connObjectType );
                if ( err == noErr ) {
                    if ( connObjectType == kMIDIObjectType_ExternalSource  ||
                        connObjectType == kMIDIObjectType_ExternalDestination ) {
                        // Connected to an external device's endpoint (10.3 and later).
                        str = EndpointName( (MIDIEndpointRef)(connObject), true );
                    } else {
                        // Connected to an external device (10.2) (or something else, catch-
                        str = NULL;
                        MIDIObjectGetStringProperty( connObject, kMIDIPropertyName, &str );
                    }
                    if ( str != NULL ) {
                        if ( anyStrings )
                            CFStringAppend( result, CFSTR(", ") );
                        else anyStrings = true;
                        CFStringAppend( result, str );
                        CFRelease( str );
                    }
                }
            }
        }
        CFRelease( connections );
    }
    if ( anyStrings )
        return result;
    
    if ( result )
        CFRelease( result );
    
    // Here, either the endpoint had no connections, or we failed to obtain names
    return EndpointName( endpoint, false );
}


class CAMIDISourceWrapper {
public:
    static v8::Persistent<v8::FunctionTemplate> GetTemplate() {
        static v8::Persistent<v8::FunctionTemplate> ft_cache;
        if (!ft_cache.IsEmpty())
            return ft_cache;
        
        v8::HandleScope scope;
        ft_cache = v8::Persistent<v8::FunctionTemplate>::New(
                                                             v8::FunctionTemplate::New(&CAMIDISourceWrapper::V8New));
        v8::Local<v8::ObjectTemplate> instance = ft_cache->InstanceTemplate();
        instance->SetInternalFieldCount(2);  // MIDIEndpointRef and MIDIPortRef.
        
        v8::Local<v8::Signature> default_signature = v8::Signature::New(ft_cache);
        
        // Configure the template...
        static BatchedConstants constants[] = {
            { "blah", 1 },
        };
        
        static BatchedMethods methods[] = {
            { "destinations", &CAMIDISourceWrapper::destinations },
            { "openDestination", &CAMIDISourceWrapper::openDestination },
            { "createVirtual", &CAMIDISourceWrapper::createVirtual },
            { "sendData", &CAMIDISourceWrapper::sendData },
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
    
    static MIDIEndpointRef ExtractEndpoint(v8::Handle<v8::Object> obj) {
        // NOTE(deanm): MIDIEndpointRef (MIDIObjectRef) is UInt32 on 64-bit.
        return (MIDIEndpointRef)(intptr_t)obj->GetPointerFromInternalField(0);
    }
    
    static MIDIPortRef ExtractPort(v8::Handle<v8::Object> obj) {
        return (MIDIPortRef)(intptr_t)obj->GetPointerFromInternalField(1);
    }
    
    static bool HasInstance(v8::Handle<v8::Value> value) {
        return GetTemplate()->HasInstance(value);
    }
    
private:
    static MIDIPacketList* AllocateMIDIPacketList(size_t num_packets,
                                                  ByteCount* out_cnt) {
        ByteCount cnt = num_packets * sizeof(MIDIPacket) +
        (sizeof(MIDIPacketList) - sizeof(MIDIPacket));
        char* buf = new char[cnt];
        if (out_cnt)
            *out_cnt = cnt;
        return reinterpret_cast<MIDIPacketList*>(buf);
    }
    
    static void FreeMIDIPacketList(MIDIPacketList* pl) {
        delete[] reinterpret_cast<char*>(pl);
    }
    
    static v8::Handle<v8::Value> sendData(const v8::Arguments& args) {
        if (!args[0]->IsArray())
            return v8::Undefined();
        
        MIDIEndpointRef endpoint = ExtractEndpoint(args.Holder());
        
        if (!endpoint) {
            return v8_utils::ThrowError("Can't send on midi without an endpoint.");
        }
        
        MIDIPortRef port = ExtractPort(args.Holder());
        MIDITimeStamp timestamp = AudioGetCurrentHostTime();
        ByteCount pl_count;
        MIDIPacketList* pl = AllocateMIDIPacketList(1, &pl_count);
        MIDIPacket* cur_packet = MIDIPacketListInit(pl);
        
        v8::Handle<v8::Array> data = v8::Handle<v8::Array>::Cast(args[0]);
        uint32_t data_len = data->Length();
        
        Byte* data_buf = new Byte[data_len];
        
        for (uint32_t i = 0; i < data_len; ++i) {
            // Convert to an integer and truncate to 8 bits.
            data_buf[i] = data->Get(v8::Integer::New(i))->Uint32Value();
        }
        
        cur_packet = MIDIPacketListAdd(pl, pl_count, cur_packet,
                                       timestamp, data_len, data_buf);
        // Depending whether we are virtual we need to send differently.
        OSStatus result = port ? MIDISend(port, endpoint, pl) :
        MIDIReceived(endpoint, pl);
        delete[] data_buf;
        FreeMIDIPacketList(pl);
        
        if (result != noErr) {
            return v8_utils::ThrowError("Couldn't send midi data.");
        }
        
        return v8::Undefined();
    }
    
    static v8::Handle<v8::Value> V8New(const v8::Arguments& args) {
        if (!args.IsConstructCall())
            return v8_utils::ThrowTypeError(kMsgNonConstructCall);
        
        OSStatus result;
        
        if (!g_midi_client) {
            result = MIDIClientCreate(CFSTR("Plask"), NULL, NULL, &g_midi_client);
            if (result != noErr) {
                return v8_utils::ThrowError("Couldn't create midi client object.");
            }
        }
        
        args.This()->SetPointerInInternalField(0, NULL);
        args.This()->SetPointerInInternalField(1, NULL);
        return args.This();
    }
    
    static v8::Handle<v8::Value> createVirtual(const v8::Arguments& args) {
        if (args.Length() != 1)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        OSStatus result;
        
        v8::String::Utf8Value name_val(args[0]);
        CFStringRef name =
        CFStringCreateWithCString(NULL, *name_val, kCFStringEncodingUTF8);
        
        MIDIEndpointRef endpoint;
        result = MIDISourceCreate(g_midi_client, name, &endpoint);
        CFRelease(name);
        if (result != noErr) {
            return v8_utils::ThrowError("Couldn't create midi source object.");
        }
        
        // NOTE(deanm): MIDIEndpointRef (MIDIObjectRef) is UInt32 on 64-bit.
        args.This()->SetPointerInInternalField(0, (void*)(intptr_t)endpoint);
        args.This()->SetPointerInInternalField(1, NULL);
        return v8::Undefined();
    }
    
    static v8::Handle<v8::Value> openDestination(const v8::Arguments& args) {
        if (args.Length() != 1)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        OSStatus result;
        
        ItemCount num_destinations = MIDIGetNumberOfDestinations();
        ItemCount index = args[0]->Uint32Value();
        if (index >= num_destinations)
            return v8_utils::ThrowError("Invalid MIDI destination index.");
        
        MIDIEndpointRef destination = MIDIGetDestination(index);
        
        MIDIPortRef port;
        result = MIDIOutputPortCreate(
                                      g_midi_client, CFSTR("Plask"), &port);
        if (result != noErr)
            return v8_utils::ThrowError("Couldn't create midi output port.");
        
        args.This()->SetPointerInInternalField(0, (void*)(intptr_t)destination);
        args.This()->SetPointerInInternalField(1, (void*)(intptr_t)port);
        
        return v8::Undefined();
        return v8::Undefined();
    }
    
    // NOTE(deanm): See API notes about sources(), same comments apply here.
    static v8::Handle<v8::Value> destinations(const v8::Arguments& args) {
        ItemCount num_destinations = MIDIGetNumberOfDestinations();
        v8::Local<v8::Array> arr = v8::Array::New(num_destinations);
        for (ItemCount i = 0; i < num_destinations; ++i) {
            MIDIEndpointRef point = MIDIGetDestination(i);
            CFStringRef name = ConnectedEndpointName(point);
            arr->Set(i, v8::String::New([(NSString*)name UTF8String]));
            CFRelease(name);
        }
        return arr;
    }
    
};

class CAMIDIDestinationWrapper {
private:
    struct State {
        MIDIEndpointRef endpoint;
        int64_t clocks;
        int pipe_fds[2];
    };
    
public:
    static v8::Persistent<v8::FunctionTemplate> GetTemplate() {
        static v8::Persistent<v8::FunctionTemplate> ft_cache;
        if (!ft_cache.IsEmpty())
            return ft_cache;
        
        v8::HandleScope scope;
        ft_cache = v8::Persistent<v8::FunctionTemplate>::New(
                                                             v8::FunctionTemplate::New(&CAMIDIDestinationWrapper::V8New));
        v8::Local<v8::ObjectTemplate> instance = ft_cache->InstanceTemplate();
        instance->SetInternalFieldCount(1);  // MIDIEndpointRef.
        
        v8::Local<v8::Signature> default_signature = v8::Signature::New(ft_cache);
        
        // Configure the template...
        static BatchedConstants constants[] = {
            { "kDummy", 1 },
        };
        
        static BatchedMethods methods[] = {
            { "createVirtual", &CAMIDIDestinationWrapper::createVirtual },
            { "sources", &CAMIDIDestinationWrapper::sources },
            { "openSource", &CAMIDIDestinationWrapper::openSource },
            { "syncClocks", &CAMIDIDestinationWrapper::syncClocks },
            { "getPipeDescriptor", &CAMIDIDestinationWrapper::getPipeDescriptor },
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
    
    // Can't use v8_utils::UnrwapCPointer because of LSB clear expectations.
    static State* ExtractPointer(v8::Handle<v8::Object> obj) {
        return v8_utils::UnwrapCPointer<State>(obj->GetInternalField(0));
    }
    
    static bool HasInstance(v8::Handle<v8::Value> value) {
        return GetTemplate()->HasInstance(value);
    }
    
private:
    // TODO(deanm): Access to state isn't thread safe.  As long as we're 64-bit
    // I'm not particularly concerned.
    static void ReadCallback(const MIDIPacketList* pktlist,
                             void* state_raw,
                             void* src) {
        State* state = reinterpret_cast<State*>(state_raw);
        const MIDIPacket* packet = &pktlist->packet[0];
        for (int i = 0; i < pktlist->numPackets; ++i) {
            //printf("Packet: ");
            //for (int j = 0; j < packet->length; ++j) {
            //printf("%02x ", packet->data[j]);
            //}
            //printf("\n");
            if (packet->data[0] == 0xf2) {
                // NOTE(deanm): Wraps around bar 1024.
                int beat = packet->data[2] << 7 | packet->data[1];
                state->clocks = beat * 6;
            } else if (packet->data[0] == 0xf8) {
                ++state->clocks;
                //printf("Clock position: %lld\n", state->clocks);
            } else {
                // TODO(deanm): Message framing.
                ssize_t res = write(state->pipe_fds[1], packet->data, packet->length);
                if (res != packet->length) {
                    printf("Error sending midi -> pipe (%zd)\n", res);
                }
            }
            packet = MIDIPacketNext(packet);
        }
    }
    
    static v8::Handle<v8::Value> syncClocks(const v8::Arguments& args) {
        State* state = ExtractPointer(args.Holder());
        return v8::Integer::New(state->clocks);
    }
    
    static v8::Handle<v8::Value> getPipeDescriptor(const v8::Arguments& args) {
        State* state = ExtractPointer(args.Holder());
        return v8::Integer::NewFromUnsigned(state->pipe_fds[0]);
    }
    
    static v8::Handle<v8::Value> V8New(const v8::Arguments& args) {
        if (!args.IsConstructCall())
            return v8_utils::ThrowTypeError(kMsgNonConstructCall);
        
        OSStatus result;
        
        if (!g_midi_client) {
            result = MIDIClientCreate(CFSTR("Plask"), NULL, NULL, &g_midi_client);
            if (result != noErr) {
                return v8_utils::ThrowError("Couldn't create midi client object.");
            }
        }
        
        State* state = new State;
        state->endpoint = NULL;
        state->clocks = 0;
        int res = pipe(state->pipe_fds);
        if (res != 0)
            return v8_utils::ThrowError("Couldn't create internal MIDI pipe.");
        args.This()->SetInternalField(0, v8::External::Wrap(state));
        return args.This();
    }
    
    
    static v8::Handle<v8::Value> createVirtual(const v8::Arguments& args) {
        if (args.Length() != 1)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        OSStatus result;
        v8::String::Utf8Value name_val(args[0]);
        CFStringRef name =
        CFStringCreateWithCString(NULL, *name_val, kCFStringEncodingUTF8);
        
        State* state = ExtractPointer(args.Holder());
        
        MIDIEndpointRef endpoint;
        result = MIDIDestinationCreate(
                                       g_midi_client, name, &ReadCallback, state, &endpoint);
        CFRelease(name);
        if (result != noErr)
            return v8_utils::ThrowError("Couldn't create midi source object.");
        
        state->endpoint = endpoint;
        return v8::Undefined();
    }
    
    // NOTE(deanm): Could make sense for the API to be numSources() and then
    // you query for sourceName(index), but really, do you ever want the index
    // without the name?  This could be a little extra work if you don't, but
    // really it seems to make sense in most of the use cases.
    static v8::Handle<v8::Value> sources(const v8::Arguments& args) {
        ItemCount num_sources = MIDIGetNumberOfSources();
        v8::Local<v8::Array> arr = v8::Array::New(num_sources);
        for (ItemCount i = 0; i < num_sources; ++i) {
            MIDIEndpointRef point = MIDIGetSource(i);
            CFStringRef name = ConnectedEndpointName(point);
            arr->Set(i, v8::String::New([(NSString*)name UTF8String]));
            CFRelease(name);
        }
        return arr;
    }
    
    static v8::Handle<v8::Value> openSource(const v8::Arguments& args) {
        if (args.Length() != 1)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        OSStatus result;
        State* state = ExtractPointer(args.Holder());
        
        ItemCount num_sources = MIDIGetNumberOfSources();
        ItemCount index = args[0]->Uint32Value();
        if (index >= num_sources)
            return v8_utils::ThrowError("Invalid MIDI source index.");
        
        MIDIEndpointRef source = MIDIGetSource(index);
        
        MIDIPortRef port;
        result = MIDIInputPortCreate(
                                     g_midi_client, CFSTR("Plask"), &ReadCallback, state, &port);
        if (result != noErr)
            return v8_utils::ThrowError("Couldn't create midi source object.");
        
        result = MIDIPortConnectSource(port, source, NULL);
        if (result != noErr)
            return v8_utils::ThrowError("Couldn't create midi source object.");
        
        state->endpoint = source;
        
        return v8::Undefined();
    }
};

class NSSoundWrapper {
public:
    static v8::Persistent<v8::FunctionTemplate> GetTemplate() {
        static v8::Persistent<v8::FunctionTemplate> ft_cache;
        if (!ft_cache.IsEmpty())
            return ft_cache;
        
        v8::HandleScope scope;
        ft_cache = v8::Persistent<v8::FunctionTemplate>::New(
                                                             v8::FunctionTemplate::New(&NSSoundWrapper::V8New));
        v8::Local<v8::ObjectTemplate> instance = ft_cache->InstanceTemplate();
        instance->SetInternalFieldCount(1);
        
        v8::Local<v8::Signature> default_signature = v8::Signature::New(ft_cache);
        
        // Configure the template...
        static BatchedConstants constants[] = {
            { "kConstant", 1 },
        };
        
        static BatchedMethods methods[] = {
            { "isPlaying", &NSSoundWrapper::isPlaying },
            { "pause", &NSSoundWrapper::pause },
            { "play", &NSSoundWrapper::play },
            { "resume", &NSSoundWrapper::resume },
            { "stop", &NSSoundWrapper::stop },
            { "volume", &NSSoundWrapper::volume },
            { "setVolume", &NSSoundWrapper::setVolume },
            { "currentTime", &NSSoundWrapper::currentTime },
            { "setCurrentTime", &NSSoundWrapper::setCurrentTime },
            { "loops", &NSSoundWrapper::loops },
            { "setLoops", &NSSoundWrapper::setLoops },
            { "duration", &NSSoundWrapper::duration },
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
    
    static NSSound* ExtractNSSoundPointer(v8::Handle<v8::Object> obj) {
        return v8_utils::UnwrapCPointer<NSSound>(obj->GetInternalField(0));
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
        
        v8::String::Utf8Value filename(args[0]);
        NSSound* sound = [[NSSound alloc] initWithContentsOfFile:
                          [NSString stringWithUTF8String:*filename] byReference:YES];
        
        args.This()->SetInternalField(0, v8_utils::WrapCPointer(sound));
        return args.This();
    }
    
    static v8::Handle<v8::Value> isPlaying(const v8::Arguments& args) {
        NSSound* sound = ExtractNSSoundPointer(args.Holder());
        return v8::Boolean::New([sound isPlaying]);
    }
    
    static v8::Handle<v8::Value> pause(const v8::Arguments& args) {
        NSSound* sound = ExtractNSSoundPointer(args.Holder());
        return v8::Boolean::New([sound pause]);
    }
    
    static v8::Handle<v8::Value> play(const v8::Arguments& args) {
        NSSound* sound = ExtractNSSoundPointer(args.Holder());
        return v8::Boolean::New([sound play]);
    }
    
    static v8::Handle<v8::Value> resume(const v8::Arguments& args) {
        NSSound* sound = ExtractNSSoundPointer(args.Holder());
        return v8::Boolean::New([sound resume]);
    }
    
    static v8::Handle<v8::Value> stop(const v8::Arguments& args) {
        NSSound* sound = ExtractNSSoundPointer(args.Holder());
        return v8::Boolean::New([sound stop]);
    }
    
    static v8::Handle<v8::Value> volume(const v8::Arguments& args) {
        NSSound* sound = ExtractNSSoundPointer(args.Holder());
        return v8::Number::New([sound volume]);
    }
    
    static v8::Handle<v8::Value> setVolume(const v8::Arguments& args) {
        if (args.Length() != 1)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        NSSound* sound = ExtractNSSoundPointer(args.Holder());
        [sound setVolume:args[0]->NumberValue()];
        return v8::Undefined();
    }
    
    static v8::Handle<v8::Value> currentTime(const v8::Arguments& args) {
        NSSound* sound = ExtractNSSoundPointer(args.Holder());
        return v8::Number::New([sound currentTime]);
    }
    
    static v8::Handle<v8::Value> setCurrentTime(const v8::Arguments& args) {
        if (args.Length() != 1)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        NSSound* sound = ExtractNSSoundPointer(args.Holder());
        [sound setCurrentTime:args[0]->NumberValue()];
        return v8::Undefined();
    }
    
    static v8::Handle<v8::Value> loops(const v8::Arguments& args) {
        NSSound* sound = ExtractNSSoundPointer(args.Holder());
        return v8::Boolean::New([sound loops]);
    }
    
    static v8::Handle<v8::Value> setLoops(const v8::Arguments& args) {
        if (args.Length() != 1)
            return v8_utils::ThrowError("Wrong number of arguments.");
        
        NSSound* sound = ExtractNSSoundPointer(args.Holder());
        [sound setLoops:args[0]->BooleanValue()];
        return v8::Undefined();
    }
    
    static v8::Handle<v8::Value> duration(const v8::Arguments& args) {
        NSSound* sound = ExtractNSSoundPointer(args.Holder());
        return v8::Number::New([sound duration]);
    }
};
