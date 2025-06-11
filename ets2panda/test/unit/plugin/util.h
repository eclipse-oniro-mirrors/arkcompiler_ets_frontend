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

#ifndef ES2PANDA_TEST_UNIT_PLUGIN_UTIL_H
#define ES2PANDA_TEST_UNIT_PLUGIN_UTIL_H

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

struct ProccedToStatePluginTestData {
    int argc;
    char **argv;
    es2panda_Impl **impl;
    std::map<es2panda_ContextState, std::vector<std::function<bool(es2panda_Context *)>>> testFunctions;
    bool fromSource;
    std::string source;
    es2panda_ContextState exitAfterState = ES2PANDA_STATE_ERROR;
};

void CheckForErrors(const std::string &stateName, es2panda_Context *context);
bool IsAssertCall(es2panda_AstNode *ast);
es2panda_AstNode *CreateAssertStatement(es2panda_Context *context, es2panda_AstNode *test, es2panda_AstNode *second);
es2panda_AstNode *AssertStatementTest(es2panda_Context *context, es2panda_AstNode *classInstance);
int RunAllStagesWithTestFunction(ProccedToStatePluginTestData &data);

es2panda_AstNode *CreateIdentifierFromString(es2panda_Context *context, const std::string_view &name);

void AppendStatementToProgram(es2panda_Context *context, es2panda_AstNode *program, es2panda_AstNode *newStatement);

void PrependStatementToProgram(es2panda_Context *context, es2panda_AstNode *program, es2panda_AstNode *newStatement);

int Test(es2panda_Context *context, es2panda_Impl *impl, int stage,
         const std::function<bool(es2panda_Context *, es2panda_AstNode *)> &handle);

#endif
