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
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifdef ETS_INTEROP_MODULE
#undef ETS_INTEROP_MODULE
#endif

#define ETS_INTEROP_MODULE InteropNativeModule

#include "common.h"
#include "converters-ani.h"
#include "ets-types.h"

#include <filesystem>
#include <fstream>
#include <sstream>

// CC-OFFNXT(G.NAM.03-CPP) project code style
EtsStringPtr impl_ReadFile(EtsStringPtr &filePath)
{
    std::ifstream inputStream;
    inputStream.open(filePath.Data());
    if (!inputStream.is_open()) {
        ThrowEtsError(std::string("Failed to open file: ").append(filePath.Data()));
        return EtsStringPtr("");
    }
    std::stringstream ss;
    ss << inputStream.rdbuf();
    if (inputStream.fail()) {
        ThrowEtsError(std::string("Failed to read file: ").append(filePath.Data()));
        return EtsStringPtr("");
    }
    return EtsStringPtr(ss.str().c_str());
}
ETS_INTEROP_1(ReadFile, EtsStringPtr, EtsStringPtr)

// CC-OFFNXT(G.NAM.03-CPP) project code style
EtsBoolean impl_FileExists(EtsStringPtr &path)
{
    return std::filesystem::exists(path.Data());
}
ETS_INTEROP_1(FileExists, EtsBoolean, EtsStringPtr)
