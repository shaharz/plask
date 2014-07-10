#pragma once
#include "v8_utils.h"
#include "v8_typed_array.h"

template <typename T, size_t N>
char (&ArraySizeHelper(T (&array)[N]))[N];

// That gcc wants both of these prototypes seems mysterious. VC, for
// its part, can't decide which to use (another mystery). Matching of
// template overloads: the final frontier.
#ifndef _MSC_VER
template <typename T, size_t N>
char (&ArraySizeHelper(const T (&array)[N]))[N];
#endif

#define arraysize(array) (sizeof(ArraySizeHelper(array)))


namespace {
    
    const char kMsgNonConstructCall[] =
    "Constructor cannot be called as a function.";
    
#if 0
    // A CGDataProvider that just provides from a pointer.
    const void* PointerProviderGetBytePointer(void* info) {
        return info;
    }
    
    void PointerProviderReleaseData(void* info, const void* data, size_t size) {
    }
    
    size_t PointerProviderGetBytesAtPosition(char* info, void* buffer,
                                             off_t position, size_t count) {
        memcpy(buffer, info + position, count);
        return count;
    }
    
    void PointerProviderReleaseInfo(void* info) {
    }
    
    CGDataProviderDirectCallbacks PointerProviderCallbacks = {
        0, &PointerProviderGetBytePointer, &PointerProviderReleaseData,
        &PointerProviderGetBytesAtPosition, &PointerProviderReleaseInfo };
#endif
    
    struct BatchedConstants {
        const char* name;
        uint32_t val;
    };
    
    struct BatchedMethods {
        const char* name;
        v8::Handle<v8::Value> (*func)(const v8::Arguments& args);
    };
}