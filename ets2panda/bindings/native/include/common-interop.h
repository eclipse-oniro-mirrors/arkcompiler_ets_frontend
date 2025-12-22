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

#ifndef COMMON_INTEROP_BASE_H
#define COMMON_INTEROP_BASE_H

#include <vector>

#include "panda_types.h"

// NOLINTBEGIN(cppcoreguidelines-macro-usage)

// CC-OFFNXT(G.PRE.02) code gen
#define TS_INTEROP_PROFILER 0
// CC-OFFNXT(G.PRE.02) code gen
#define TS_INTEROP_TRACER 0

// NOLINTEND(cppcoreguidelines-macro-usage)

#if TS_INTEROP_PROFILER
#include "profiler.h"
// CC-OFFNXT(G.PRE.09) code gen
#define TS_INTEROP_LOGGER(name) InteropMethodCall logger(#name);
#endif

#if TS_INTEROP_TRACER
#include "tracer.h"
// CC-OFFNXT(G.PRE.09) code gen
#define TS_INTEROP_LOGGER(name) InteropMethodCall logger(#name);
#endif

#ifdef TS_INTEROP_LOGGER
// CC-OFFNXT(G.PRE.09) code gen
#define TS_MAYBE_LOG(name) TS_INTEROP_LOGGER(name);
#else
// CC-OFFNXT(G.PRE.02) code gen
#define TS_MAYBE_LOG(name)
#endif

using CallbackCallert = void (*)(KInt callbackKind, KByte *argsData, KInt argsLength);
using CallbackCallerSynct = void (*)(KVMContext vmContext, KInt callbackKind, KByte *argsData, KInt argsLength);
void SetCallbackCaller(CallbackCallert caller);
void SetCallbackCallerSync(CallbackCallerSynct callerSync);

KVMDeferred *CreateDeferred(KVMContext context, KVMObjectHandle *promise);

// CC-OFFNXT(G.NAM.01) false positive
std::vector<KStringPtr> MakeStringVector(KStringArray strArray);
// CC-OFFNXT(G.NAM.01) false positive
std::vector<KStringPtr> MakeStringVector(KNativePointerArray arr, KInt length);

#include "convertors-napi.h"

#endif  // COMMON_INTEROP_BASE_H
