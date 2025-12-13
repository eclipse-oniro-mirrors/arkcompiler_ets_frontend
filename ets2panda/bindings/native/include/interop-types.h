/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef INTEROP_TYPES_H_
#define INTEROP_TYPES_H_

#include <cstdint>
#include <array>

enum InteropTag {
    INTEROP_TAG_UNDEFINED = 101,
    INTEROP_TAG_INT32 = 102,
    INTEROP_TAG_FLOAT32 = 103,
    INTEROP_TAG_STRING = 104,
    INTEROP_TAG_LENGTH = 105,
    INTEROP_TAG_RESOURCE = 106,
    INTEROP_TAG_OBJECT = 107,
};

enum InteropRuntimeType {
    INTEROP_RUNTIME_UNEXPECTED = -1,
    INTEROP_RUNTIME_NUMBER = 1,
    INTEROP_RUNTIME_STRING = 2,
    INTEROP_RUNTIME_OBJECT = 3,
    INTEROP_RUNTIME_BOOLEAN = 4,
    INTEROP_RUNTIME_UNDEFINED = 5,
    INTEROP_RUNTIME_BIGINT = 6,
    INTEROP_RUNTIME_FUNCTION = 7,
    INTEROP_RUNTIME_SYMBOL = 8,
    INTEROP_RUNTIME_MATERIALIZED = 9,
};

using InteropFloat32 = float;
using InteropFloat64 = double;
using InteropInt32 = int32_t;
using InteropUInt32 = unsigned int;
using InteropInt64 = int64_t;
using InteropInt8 = int8_t;
using InteropUInt8 = uint8_t;
using InteropDate = int64_t;
using InteropBoolean = int8_t;
using InteropCharPtr = const char *;
using InteropNativePointer = void *;

struct InteropVMContextRaw;
using InteropVMContext = InteropVMContextRaw *;
struct InteropPipelineContextRaw;
using InteropPipelineContext = InteropPipelineContextRaw *;
struct InteropVMObjectRaw;
using InteropVMObject = InteropVMObjectRaw *;
struct InteropNode;
using InteropNodeHandle = InteropNode *;
struct InteropDeferred {
    void *handler;
    void *context;
    void (*resolve)(struct InteropDeferred *thiz, uint8_t *data, int32_t length);
    void (*reject)(struct InteropDeferred *thiz, const char *message);
};

// Binary layout of InteropString must match that of KStringPtrImpl.
struct InteropString {
    const char *chars;
    InteropInt32 length;
};

struct InteropEmpty {
    InteropInt32 dummy;  // Empty structs are forbidden in C.
};

struct InteropNumber {
    InteropInt8 tag;
    union {
        InteropFloat32 f32;
        InteropInt32 i32;
    };
};

// Binary layout of InteropLength must match that of KLength.
struct InteropLength {
    InteropInt8 type;
    InteropFloat32 value;
    InteropInt32 unit;
    InteropInt32 resource;
};

const int G_INTEROP_CUSTOM_OBJECT_KIND_SIZE = 20;
const int G_UNION_CAPACITY32 = 4;
struct InteropCustomObject {
    std::array<char, G_INTEROP_CUSTOM_OBJECT_KIND_SIZE> kind;
    InteropInt32 id;
    // Data of custom object.
    union {
        std::array<InteropInt32, G_UNION_CAPACITY32> ints;
        std::array<InteropFloat32, G_UNION_CAPACITY32> floats;
        std::array<void *, G_UNION_CAPACITY32> pointers;
        InteropString string;
    };
};

struct InteropUndefined {
    InteropInt32 dummy;  // Empty structs are forbidden in C.
};

struct InteropVoid {
    InteropInt32 dummy;  // Empty structs are forbidden in C.
};

struct InteropFunction {
    InteropInt32 id;
};
using InteropCallback = InteropFunction;
using InteropErrorCallback = InteropFunction;

struct InteropMaterialized {
    InteropNativePointer ptr;
};

struct InteropCallbackResource {
    InteropInt32 resourceId;
    void (*hold)(InteropInt32 resourceId);
    void (*release)(InteropInt32 resourceId);
};

struct InteropBuffer {
    InteropCallbackResource resource;
    InteropNativePointer data;
    InteropInt64 length;
};

#endif  // INTEROP_TYPES_H_
