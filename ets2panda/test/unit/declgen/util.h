/**
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

#ifndef ES2PANDA_TEST_UNIT_DECLGEN_PLUGIN_UTIL_H
#define ES2PANDA_TEST_UNIT_DECLGEN_PLUGIN_UTIL_H

#include <functional>
#include <iostream>
#include <string_view>
#include <map>
#include <vector>

#include "libarkbase/os/library_loader.h"

#include "public/es2panda_lib.h"

constexpr int MIN_ARGC = 3;

// error code number
constexpr int NULLPTR_IMPL_ERROR_CODE = 2;
constexpr int PROCEED_ERROR_CODE = 3;
constexpr int TEST_ERROR_CODE = 4;
constexpr int INVALID_ARGC_ERROR_CODE = 5;
constexpr int NULLPTR_CONTEXT_ERROR_CODE = 6;

es2panda_Impl *GetImpl();
void CheckForErrors(const std::string &stateName, es2panda_Context *context);

es2panda_AstNode *GetETSGlobalClass(es2panda_Context *ctx, es2panda_AstNode *rootNode);

// CC-OFFNXT(G.NAM.01) false positive
std::string GetDeclPrefix(const char *etsSrcName);

#endif