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

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <ostream>
#include <string>
#include <cstring>
#include "util.h"
#include "public/es2panda_lib.h"
#include "os/file.h"

// NOLINTBEGIN

static es2panda_Impl *impl = nullptr;
static auto source = std::string("function main() { 1 + 2 }");

static auto ResolveImportPath(int argc, char **argv, es2panda_Context *context, char *sourceLiteral)
{
    auto importPathLiteral = impl->CreateStringLiteral1(context, sourceLiteral);
    impl->AstNodeSetRange(context, importPathLiteral,
                          impl->CreateSourceRange(context, impl->CreateSourcePosition(context, 0, 0),
                                                  impl->CreateSourcePosition(context, 0, 0)));
    auto resolvedPath = impl->ImportPathManagerResolvePathAPIConst(
        context, impl->ETSParserGetImportPathManager(context), argv[argc - 1], importPathLiteral);
    return resolvedPath;
}

int main(int argc, char **argv)
{
    if (argc < MIN_ARGC) {
        return 1;
    }

    if (GetImpl() == nullptr) {
        return NULLPTR_IMPL_ERROR_CODE;
    }
    impl = GetImpl();
    const char **args = const_cast<const char **>(&(argv[1]));
    auto config = impl->CreateConfig(argc - 1, args);
    auto context = impl->CreateContextFromString(config, source.data(), argv[argc - 1]);
    if (context == nullptr) {
        std::cerr << "FAILED TO CREATE CONTEXT" << std::endl;
        return NULLPTR_CONTEXT_ERROR_CODE;
    }

    impl->ProceedToState(context, ES2PANDA_STATE_PARSED);
    CheckForErrors("PARSE", context);

    auto pathDelim = ark::os::file::File::GetPathDelim();
    auto resolvedPath = ResolveImportPath(argc, argv, context, const_cast<char *>("./export.ets"));
    if (strstr(resolvedPath, ("cache" + std::string(pathDelim) + "export.d.ets").c_str()) == nullptr) {
        return TEST_ERROR_CODE;
    }

    auto resolvedPath1 = ResolveImportPath(argc, argv, context, const_cast<char *>("./export"));
    if (strstr(resolvedPath1, ("cache" + std::string(pathDelim) + "export.d.ets").c_str()) == nullptr) {
        return TEST_ERROR_CODE;
    }

    auto resolvedPath2 = ResolveImportPath(argc, argv, context, const_cast<char *>("./export.d.ets"));
    if (strstr(resolvedPath2, "export.d.ets") == nullptr ||
        strstr(resolvedPath2, ("cache" + std::string(pathDelim) + "export.d.ets").c_str()) != nullptr) {
        return TEST_ERROR_CODE;
    }

    impl->ProceedToState(context, ES2PANDA_STATE_CHECKED);
    if (impl->ContextState(context) == ES2PANDA_STATE_ERROR) {
        return PROCEED_ERROR_CODE;
    }

    auto ast = impl->ProgramAst(context, impl->ContextProgram(context));
    impl->AstNodeRecheck(context, ast);

    impl->DestroyContext(context);
    impl->DestroyConfig(config);

    return 0;
}

// NOLINTEND
