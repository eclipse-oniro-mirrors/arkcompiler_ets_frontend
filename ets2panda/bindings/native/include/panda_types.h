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

#ifndef TS_TYPES_H
#define TS_TYPES_H

#include <cstdint>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <variant>
#include "securec.h"

const unsigned int ARK_TAG_INT32 = 102U;
const unsigned int ARK_TAG_FLOAT32 = 103U;

struct KStringPtrImpl {
    explicit KStringPtrImpl(const char *str) : value_(nullptr), lengthStr_(0), owned_(true)
    {
        int len = str != nullptr ? strlen(str) : 0;
        Assign(str, len);
    }
    KStringPtrImpl(const char *str, int len, bool isowned) : value_(nullptr), lengthStr_(0), owned_(isowned)
    {
        Assign(str, len);
    }
    KStringPtrImpl() : value_(nullptr), lengthStr_(0), owned_(true) {}

    KStringPtrImpl(const KStringPtrImpl &other) = delete;
    KStringPtrImpl &operator=(const KStringPtrImpl &other) = delete;

    KStringPtrImpl(KStringPtrImpl &&other)
    {
        this->value_ = other.Release();
        this->owned_ = other.owned_;
        other.owned_ = false;
        this->lengthStr_ = other.lengthStr_;
    }
    KStringPtrImpl &operator=(KStringPtrImpl &&other)
    {
        this->value_ = other.Release();
        this->owned_ = other.owned_;
        other.owned_ = false;
        this->lengthStr_ = other.lengthStr_;
        return *this;
    }

    ~KStringPtrImpl()
    {
        if (value_ != nullptr && owned_) {
            delete[] value_;
        }
    }

    bool IsNull() const
    {
        return value_ == nullptr;
    }
    const char *CStr() const
    {
        return value_;
    }
    char *Data() const
    {
        return value_;
    }
    int Length() const
    {
        return lengthStr_;
    }

    void Resize(unsigned int size)
    {
        lengthStr_ = size;
        if (!owned_) {
            return;
        }
        // Ignore old content.
        if (value_ != nullptr && owned_) {
            delete[] value_;
        }
        value_ = new char[size + 1] {};
        if (value_ == nullptr) {
            // NOTE(khil): should be refactored to proper malloc
            return;
        }
    }

    void Assign(const char *data)
    {
        Assign(data, data != nullptr ? strlen(data) : 0);
    }

    void Assign(const char *data, unsigned int len)
    {
        if (value_ != nullptr && owned_) {
            delete[] value_;
        }
        if (data != nullptr) {
            if (owned_) {
                value_ = new char[len + 1] {};
                if (value_ == nullptr) {
                    return;
                }
                memcpy_s(value_, len, data, len);
            } else {
                value_ = const_cast<char *>(data);
            }
        } else {
            value_ = nullptr;
        }
        lengthStr_ = len;
    }

protected:
    char *Release()
    {
        char *result = this->value_;
        this->value_ = nullptr;
        return result;
    }

private:
    char *value_;
    // CC-OFFNXT(G.NAM.01) project code style
    int lengthStr_;
    bool owned_;
};

struct KInteropNumber {
    KInteropNumber() : value_(0) {}
    static inline KInteropNumber FromDouble(double value)
    {
        KInteropNumber result {};
        // NOTE(khil): boundary check
        if (value == std::floor(value)) {
            result.SetTag(ARK_TAG_INT32);
            result.SetValue<int32_t>(static_cast<int32_t>(value));
        } else {
            result.SetTag(ARK_TAG_FLOAT32);
            result.SetValue<float>(static_cast<float>(value));
        }
        return result;
    }
    inline double AsDouble()
    {
        if (tag_ == ARK_TAG_INT32) {
            return static_cast<double>(std::get<int32_t>(value_));
        }
        return static_cast<double>(std::get<float>(value_));
    }
    inline void SetTag(int8_t tag)
    {
        tag_ = tag;
    }
    template <typename T>
    void SetValue(T value)
    {
        value_ = value;
    }

private:
    std::variant<int32_t, float> value_;
    // CC-OFFNXT(G.NAM.01) project code style
    int8_t tag_ {0};
};

using KBoolean = int8_t;
using KByte = uint8_t;
using KChar = int16_t;
using KShort = int16_t;
using KUShort = uint16_t;
using KInt = int32_t;
using KUInt = uint32_t;
using KFloat = float;
using KLong = int64_t;
using KDouble = double;
using KNativePointer = void *;
using KStringPtr = KStringPtrImpl;
using KFloatArray = float *;
using KStringArray = const uint8_t *;
using KNativePointerArray = void **;

struct KInteropBuffer {
    KLong length;
    KNativePointer data;

    KInt resourceId;
    void (*dispose)(KInt /* resourceId for now */);
};

struct KInteropReturnBuffer {
    KInt length;
    KNativePointer data;
    void (*dispose)(KNativePointer data, KInt length);
};

struct KLength {
    KByte type;
    KFloat value;
    KInt unit;
    KInt resource;
};

// CC-OFFNXT(G.FUD.06) switch-case, ODR
inline void ParseKLength(const KStringPtrImpl &string, KLength *result)
{
    char *suffixPtr = nullptr;

    float value = std::strtof(string.CStr(), &suffixPtr);

    if (suffixPtr == nullptr || suffixPtr == string.CStr()) {
        // not a numeric value
        result->unit = -1;
        return;
    }
    result->value = value;
    const size_t cmpOneByte = 1;
    const size_t cmpTwoByte = 2;
    const size_t cmpThreeByte = 3;
    if (std::strncmp(suffixPtr, "\0", 1) == 0 || std::strncmp(suffixPtr, "vp", cmpTwoByte) == 0) {
        result->unit = 1;
    } else if (std::strncmp(suffixPtr, "%", cmpOneByte) == 0) {
        result->unit = 3U;
    } else if (std::strncmp(suffixPtr, "px", cmpTwoByte) == 0) {
        result->unit = 0;
    } else if (std::strncmp(suffixPtr, "lpx", cmpThreeByte) == 0) {
        result->unit = 4U;
    } else if (std::strncmp(suffixPtr, "fp", cmpTwoByte) == 0) {
        result->unit = 2U;
    } else {
        result->unit = -1;
    }
}

struct KVMContextRaw;
using KVMContext = KVMContextRaw *;

// BEWARE: this MUST never be used in user code, only in very rare service code.
struct KVMObjectRaw;
using KVMObjectHandle = KVMObjectRaw *;

struct KVMDeferred {
    void *handler;
    void *context;
    void (*resolve)(KVMDeferred *thiz, uint8_t *data, int32_t length);
    void (*reject)(KVMDeferred *thiz, const char *message);
};

template <class T>
T *Ptr(KNativePointer ptr)
{
    return reinterpret_cast<T *>(ptr);
}

template <class T>
T &Ref(KNativePointer ptr)
{
    return *reinterpret_cast<T *>(ptr);
}

inline KNativePointer NativePtr(void *pointer)
{
    return reinterpret_cast<KNativePointer>(pointer);
}

template <class T>
KNativePointer FnPtr(void (*pointer)(T *))
{
    return reinterpret_cast<KNativePointer>(pointer);
}

#endif /* TS_TYPES_H */
