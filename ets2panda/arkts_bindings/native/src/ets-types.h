/**
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY EtsIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ETS_TYPES_H
#define ETS_TYPES_H

#include <cstdint>
#include <cstring>
#include "securec.h"

struct EtsStringPtrImpl {
    explicit EtsStringPtrImpl(const char *str) : value_(nullptr), lengthStr_(0), owned_(true)
    {
        int len = str != nullptr ? strlen(str) : 0;
        Assign(str, len);
    }
    EtsStringPtrImpl(const char *str, int len, bool isowned) : value_(nullptr), lengthStr_(0), owned_(isowned)
    {
        Assign(str, len);
    }
    EtsStringPtrImpl() : value_(nullptr), lengthStr_(0), owned_(true) {}

    EtsStringPtrImpl(const EtsStringPtrImpl &other) = delete;
    EtsStringPtrImpl &operator=(const EtsStringPtrImpl &other) = delete;

    EtsStringPtrImpl(EtsStringPtrImpl &&other)
    {
        this->value_ = other.Release();
        this->owned_ = other.owned_;
        other.owned_ = false;
        this->lengthStr_ = other.lengthStr_;
    }
    EtsStringPtrImpl &operator=(EtsStringPtrImpl &&other)
    {
        this->value_ = other.Release();
        this->owned_ = other.owned_;
        other.owned_ = false;
        this->lengthStr_ = other.lengthStr_;
        return *this;
    }

    ~EtsStringPtrImpl()
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
                if (memcpy_s(value_, len + 1, data, len) != 0) {
                    return;
                }
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
    // CC-OFFNXT(G.NAM.03-CPP) project code style
    char *value_;
    // CC-OFFNXT(G.NAM.03-CPP) project code style
    int lengthStr_;
    // CC-OFFNXT(G.NAM.03-CPP) project code style
    bool owned_;
};

using EtsBoolean = bool;
using EtsByte = int8_t;
using EtsChar = uint16_t;
using EtsShort = int16_t;
using EtsInt = int32_t;
using EtsFloat = float;
using EtsLong = int64_t;
using EtsDouble = double;
using EtsNativePointer = void *;
using EtsCString = const char *;
using EtsStringPtr = EtsStringPtrImpl;
using EtsStringArray = char **;
using EtsNativePointerArray = void **;

#endif  // ETS_TYPES_H
