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

#include <iostream>
#include <ostream>
#include <string>
#include <unordered_map>
#include "libarkbase/os/library_loader.h"
#include "public/es2panda_lib.h"
#include "util.h"

// NOLINTBEGIN

static es2panda_Impl *impl = nullptr;
static es2panda_Context *ctx = nullptr;

static std::string source = R"(
function returnUndefined(): undefined {
    return undefined;
}

function anotherUndefined(): undefined {
    return undefined;
}

function varWithUndefined(): void {
    let x: undefined = undefined;
    let y: undefined = undefined;
    console.log(x);
    console.log(y);
}

function main(): void {
    // Repeated use of the undefined type and value
    let result1: undefined = returnUndefined();
    let result2: undefined = anotherUndefined();
    varWithUndefined();
}
)";

static std::unordered_map<std::string, int> stringLiteralCount;
static std::unordered_map<std::string, es2panda_AstNode *> stringLiteralNodes;
static std::unordered_map<std::string, es2panda_AstNode *> undefinedLiteralNodes;
static std::unordered_map<std::string, es2panda_AstNode *> voidLiteralNodes;

static void CollectLiterals(es2panda_Context *context, es2panda_AstNode *ast)
{
    char *countName = const_cast<char *>("LiteralCount");
    impl->AstNodeTransformChildrenRecursively(
        context, ast,
        [](es2panda_AstNode *node) {
            if (node == nullptr) {
                return node;
            }

            if (impl->IsStringLiteral(node)) {
                const char *strValue = impl->StringLiteralStrConst(ctx, node);
                stringLiteralCount[strValue]++;
                if (stringLiteralCount[strValue] == 1) {
                    stringLiteralNodes[strValue] = node;
                }
            }

            if (impl->IsTSUndefinedKeyword(node) || impl->IsUndefinedLiteral(node)) {
                const char *key = "undefined";
                if (undefinedLiteralNodes.find(key) == undefinedLiteralNodes.end()) {
                    undefinedLiteralNodes[key] = node;
                }
            }

            if (impl->IsTSVoidKeyword(node)) {
                const char *key = "void";
                if (voidLiteralNodes.find(key) == voidLiteralNodes.end()) {
                    voidLiteralNodes[key] = node;
                }
            }

            return node;
        },
        countName);
}

static void ReplaceDuplicatedLiterals(es2panda_Context *context, es2panda_AstNode *ast)
{
    char *transformationName = const_cast<char *>("LiteralOptimize");
    impl->AstNodeTransformChildrenRecursively(
        context, ast,
        [](es2panda_AstNode *node) {
            if (node == nullptr) {
                return node;
            }

            if (impl->IsStringLiteral(node)) {
                const char *strValue = impl->StringLiteralStrConst(ctx, node);
                if (stringLiteralCount[strValue] > 1 && stringLiteralNodes[strValue] != node) {
                    return stringLiteralNodes[strValue];
                }
            }

            if (impl->IsTSUndefinedKeyword(node) || impl->IsUndefinedLiteral(node)) {
                const char *key = "undefined";
                if (undefinedLiteralNodes.find(key) != undefinedLiteralNodes.end() &&
                    undefinedLiteralNodes[key] != node) {
                    return undefinedLiteralNodes[key];
                }
            }

            if (impl->IsTSVoidKeyword(node)) {
                const char *key = "void";
                if (voidLiteralNodes.find(key) != voidLiteralNodes.end() && voidLiteralNodes[key] != node) {
                    return voidLiteralNodes[key];
                }
            }
            return node;
        },
        transformationName);
}

static bool TestStringOptimize(es2panda_Context *context, es2panda_AstNode *ast)
{
    stringLiteralCount.clear();
    stringLiteralNodes.clear();
    undefinedLiteralNodes.clear();
    voidLiteralNodes.clear();

    CollectLiterals(context, ast);
    ReplaceDuplicatedLiterals(context, ast);

    return true;
}

int main(int argc, char **argv)
{
    if (argc < MIN_ARGC) {
        return INVALID_ARGC_ERROR_CODE;
    }

    if (GetImpl() == nullptr) {
        return NULLPTR_IMPL_ERROR_CODE;
    }
    impl = GetImpl();
    std::cout << "LOAD SUCCESS" << std::endl;

    const char **args = const_cast<const char **>(&(argv[1]));
    auto config = impl->CreateConfig(argc - 1, args);

    auto context = impl->CreateContextFromString(config, source.data(), argv[argc - 1]);
    ctx = context;
    if (ctx == nullptr) {
        std::cerr << "FAILED TO CREATE CONTEXT" << std::endl;
        impl->DestroyConfig(config);
        return 1;
    }

    impl->ProceedToState(ctx, ES2PANDA_STATE_PARSED);
    CheckForErrors("PARSE", ctx);

    auto *program = impl->ContextProgram(context);
    auto *ast = impl->ProgramAst(context, program);

    if (!TestStringOptimize(ctx, ast)) {
        // Report error
        std::cout << "String optimization failed." << std::endl;
        impl->DestroyContext(ctx);
        impl->DestroyConfig(config);
        return 1;
    }

    impl->ProceedToState(ctx, ES2PANDA_STATE_CHECKED);
    CheckForErrors("BIN", ctx);

    impl->AstNodeRecheck(context, ast);

    impl->ProceedToState(ctx, ES2PANDA_STATE_BIN_GENERATED);
    CheckForErrors("BIN", ctx);

    impl->DestroyContext(ctx);
    impl->DestroyConfig(config);
    return 0;
}

// NOLINTEND
