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

#include <iostream>
#include <map>
#include <ostream>
#include <string>
#include "public/es2panda_lib.h"
#include "util.h"

// NOLINTBEGIN
static std::string source = R"(
export declare interface XX {}
/**
* test line
* test line
*
* @syscap_test test.test.test
* @unpublished_test
* @since 00 test
*/
@Retention({policy: "SOURCE"})
export declare @interface State {}

/**
* test line
* test line
*
* @syscap_test test.test.test
* @unpublished_test
* @since 00 test
*/
@Retention({policy: "SOURCE"})
export declare @interface State2 {}
)";

constexpr size_t STATE_START_LINE = 10;
constexpr size_t STATE_START_COL = 1;
constexpr size_t STATE_END_LINE = 11;
constexpr size_t STATE_END_COL = 35;
constexpr size_t STATE2_START_LINE = 21;
constexpr size_t STATE2_START_COL = 1;
constexpr size_t STATE2_END_LINE = 22;
constexpr size_t STATE2_END_COL = 36;

static std::map<std::string, size_t> startLineMap = {{"State", STATE_START_LINE}, {"State2", STATE2_START_LINE}};
static std::map<std::string, size_t> startColMap = {{"State", STATE_START_COL}, {"State2", STATE2_START_COL}};
static std::map<std::string, size_t> endLineMap = {{"State", STATE_END_LINE}, {"State2", STATE2_END_LINE}};
static std::map<std::string, size_t> endColMap = {{"State", STATE_END_COL}, {"State2", STATE2_END_COL}};

static es2panda_Impl *impl = nullptr;
static es2panda_Context *context = nullptr;

static std::map<std::string, es2panda_AstNode *> annoDeclMap = {{"State", nullptr}, {"State2", nullptr}};
static void FindAnnotationDecl(es2panda_AstNode *ast, [[maybe_unused]] void *ctx)
{
    if (!impl->IsAnnotationDeclaration(ast)) {
        return;
    }
    auto *expr = impl->AnnotationDeclarationExpr(context, ast);
    if (expr == nullptr) {
        return;
    }
    auto name = std::string(impl->IdentifierNameConst(context, expr));
    if (annoDeclMap.find(name) != annoDeclMap.end()) {
        annoDeclMap[name] = ast;
    }
}

static void FindTargetAst(es2panda_AstNode *ast, [[maybe_unused]] void *ctx)
{
    impl->AstNodeForEach(ast, FindAnnotationDecl, context);
}

static bool CheckLineAndCol(es2panda_AstNode *ast, std::string name)
{
    auto start = impl->AstNodeStartConst(context, ast);
    auto end = impl->AstNodeEndConst(context, ast);
    auto res = startLineMap[name] == impl->SourcePositionLine(context, start);
    ASSERT(startLineMap[name] == impl->SourcePositionLine(context, start));

    res &= startColMap[name] == impl->SourcePositionCol(context, start);
    ASSERT(startColMap[name] == impl->SourcePositionCol(context, start));

    res &= endLineMap[name] == impl->SourcePositionLine(context, end);
    ASSERT(endLineMap[name] == impl->SourcePositionLine(context, end));

    res &= endColMap[name] == impl->SourcePositionCol(context, end);
    ASSERT(endColMap[name] == impl->SourcePositionCol(context, end));
    return res;
}

static bool CheckAllNode()
{
    bool res = CheckLineAndCol(annoDeclMap["State"], "State");
    res &= CheckLineAndCol(annoDeclMap["State2"], "State2");
    return res;
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

    const char **args = const_cast<const char **>(&(argv[1]));
    auto config = impl->CreateConfig(argc - 1, args);
    context = impl->CreateContextFromString(config, source.data(), argv[argc - 1]);
    if (context == nullptr) {
        return NULLPTR_CONTEXT_ERROR_CODE;
    }
    impl->ProceedToState(context, ES2PANDA_STATE_PARSED);
    CheckForErrors("PARSED", context);

    auto *program = impl->ContextProgram(context);
    es2panda_AstNode *programNode = impl->ProgramAst(context, program);
    FindTargetAst(programNode, context);
    if (!CheckAllNode()) {
        impl->DestroyConfig(config);
        return TEST_ERROR_CODE;
    }
    impl->DestroyConfig(config);
    return 0;
}
// NOLINTEND