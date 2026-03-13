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

#define ETS_INTEROP_MODULE GeneratedEs2pandaNativeModule

#include "common.h"
#include "converters-ani.h"
#include "ets-types.h"

#include "public/es2panda_lib.h"

// CC-OFFNXT(G.NAM.03-CPP) project code style
EtsCString impl_ProgramDump(EtsNativePointer contextPtr, EtsNativePointer programPtr)
{
    auto context = reinterpret_cast<es2panda_Context *>(contextPtr);
    auto program = reinterpret_cast<es2panda_Program *>(programPtr);
    return GetPublicImpl()->ProgramDumpConst(context, program);
}
ETS_INTEROP_2(ProgramDump, EtsCString, EtsNativePointer, EtsNativePointer)
