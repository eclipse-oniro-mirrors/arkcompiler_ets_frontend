/**
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

#include "public/es2panda_lib.h"
#include "util.h"

// NOLINTBEGIN

/*
 * Covered C API List:
 *   size_t(*SourcePositionIndex)(es2panda_Context *context, es2panda_SourcePosition *position);
 *   size_t(*SourcePositionLine)(es2panda_Context *context, es2panda_SourcePosition *position);
 *   es2panda_Config *(*CreateConfig)(int argc, char const * const *argv);
 *   es2panda_Context *(*CreateContextFromString)(es2panda_Config *config, const char *source, char const *file_name);
 *   es2panda_SourcePosition *(*CreateSourcePosition)(es2panda_Context *context, size_t index, size_t line);
 *   es2panda_SourceRange *(*CreateSourceRange)(es2panda_Context *context, es2panda_SourcePosition *start,
 *                                              es2panda_SourcePosition *end);
 *   es2panda_SourcePosition *(*SourceRangeStart)(es2panda_Context *context, es2panda_SourceRange *range);
 *   es2panda_SourcePosition *(*SourceRangeEnd)(es2panda_Context *context, es2panda_SourceRange *range);
 *   es2panda_AstNode *(*CreateNumberLiteral)(es2panda_Context *ctx, int32_t value);
 *   es2panda_AstNode *(*CreateNumberLiteral1)(es2panda_Context *ctx, int64_t value);
 *   es2panda_AstNode *(*CreateNumberLiteral2)(es2panda_Context *ctx, double value);
 *   es2panda_AstNode *(*CreateNumberLiteral3)(es2panda_Context *ctx, float value);
 *   es2panda_AstNode *(*UpdateNumberLiteral)(es2panda_Context *ctx, es2panda_AstNode *original, int32_t value);
 *   es2panda_AstNode *(*UpdateNumberLiteral1)(es2panda_Context *ctx, es2panda_AstNode *original, int64_t value);
 *   es2panda_AstNode *(*UpdateNumberLiteral2)(es2panda_Context *ctx, es2panda_AstNode *original, double value);
 *   es2panda_AstNode *(*UpdateNumberLiteral3)(es2panda_Context *ctx, es2panda_AstNode *original, float value);
 *   bool(*NumberLiteralSetInt)(es2panda_AstNode *node, int32_t new_value);
 *   bool(*NumberLiteralSetLong)(es2panda_AstNode *node, int64_t new_value);
 *   bool(*NumberLiteralSetDouble)(es2panda_AstNode *node, double new_value);
 *   bool(*NumberLiteralSetFloat)(es2panda_AstNode *node, float new_value);
 *   char *(*)(es2panda_Context *, Es2pandaEnum);
 *   Es2pandaEnum (*)(es2panda_Context *, const char *);
 *   void(*DestroyContext)(es2panda_Context *context);
 *   void(*DestroyConfig)(es2panda_Config *config);
 *   es2panda_Program *(*ContextProgram)(es2panda_Context *context);
 *   es2panda_Context *(*ProceedToState)(es2panda_Context *context, es2panda_ContextState state);
 *   es2panda_ExternalSource **(*ProgramExternalSources)(es2panda_Program *program, size_t *len_p)
 *   char const *(*ExternalSourceName)(es2panda_ExternalSource *e_source)
 *   es2panda_Program **(*ExternalSourcePrograms)(es2panda_ExternalSource *e_source, size_t *len_p)
 */

namespace {
// these FAIL_CODE does not share with other files.
constexpr int SOURCE_POSITION_FAIL_CODE = 0x10;
constexpr int NUMBER_LITERAL_FAIL_CODE = 0x11;
constexpr int ENUM_STRING_CONVERSION_FAIL_CODE = 0x12;
constexpr int EXTERNAL_SOURCE_FAIL_CODE = 0x13;

es2panda_Impl *g_impl = nullptr;
}  // namespace

bool IsSourcePositionEqual(es2panda_Context *ctx, es2panda_SourcePosition *firstPos, es2panda_SourcePosition *secondPos)
{
    if (firstPos == nullptr || secondPos == nullptr) {
        return false;
    }
    if (g_impl->SourcePositionIndex(ctx, firstPos) != g_impl->SourcePositionIndex(ctx, secondPos)) {
        return false;
    }
    if (g_impl->SourcePositionLine(ctx, firstPos) != g_impl->SourcePositionLine(ctx, secondPos)) {
        return false;
    }

    return true;
}

bool CheckSourcePosition(es2panda_Context *ctx)
{
    es2panda_SourcePosition *pos = g_impl->CreateSourcePosition(ctx, 3, 5);
    size_t origPosIndex {3};
    size_t origPosLine {5};
    size_t posIndex = g_impl->SourcePositionIndex(ctx, pos);
    if (posIndex != origPosIndex) {
        return false;
    }
    size_t posLine = g_impl->SourcePositionLine(ctx, pos);
    if (posLine != origPosLine) {
        return false;
    }

    es2panda_SourcePosition *leftPos = g_impl->CreateSourcePosition(ctx, 4, 1);
    es2panda_SourcePosition *rightPos = g_impl->CreateSourcePosition(ctx, 4, 5);
    es2panda_SourceRange *posRange = g_impl->CreateSourceRange(ctx, leftPos, rightPos);
    if (!IsSourcePositionEqual(ctx, leftPos, g_impl->SourceRangeStart(ctx, posRange))) {
        return false;
    }
    if (!IsSourcePositionEqual(ctx, rightPos, g_impl->SourceRangeEnd(ctx, posRange))) {
        return false;
    }
    return true;
}

bool CheckNumberLiteral(es2panda_Context *ctx)
{
    auto *int32AstNode = g_impl->CreateNumberLiteral(ctx, 0);
    if (int32AstNode == nullptr) {
        return false;
    }
    auto *int64AstNode = g_impl->CreateNumberLiteral1(ctx, 1);
    if (int64AstNode == nullptr) {
        return false;
    }
    auto *doubleAstNode = g_impl->CreateNumberLiteral2(ctx, 2.0);
    if (doubleAstNode == nullptr) {
        return false;
    }
    auto *floatAstNode = g_impl->CreateNumberLiteral3(ctx, 3.0);
    if (floatAstNode == nullptr) {
        return false;
    }

    auto *updatedInt32AstNode = g_impl->UpdateNumberLiteral(ctx, int32AstNode, 4);
    if (updatedInt32AstNode == nullptr) {
        return false;
    }
    auto *updatedInt64AstNode = g_impl->UpdateNumberLiteral1(ctx, int64AstNode, 5);
    if (updatedInt64AstNode == nullptr) {
        return false;
    }
    auto *updatedDoubleAstNode = g_impl->UpdateNumberLiteral2(ctx, doubleAstNode, 6.0);
    if (updatedDoubleAstNode == nullptr) {
        return false;
    }
    auto *updatedFloatAstNode = g_impl->UpdateNumberLiteral3(ctx, floatAstNode, 7.0);
    if (updatedFloatAstNode == nullptr) {
        return false;
    }

    bool setInt32Result = g_impl->NumberLiteralSetInt(updatedInt32AstNode, 8);
    bool setInt64Result = g_impl->NumberLiteralSetLong(updatedInt64AstNode, 9);
    bool setDoubleResult = g_impl->NumberLiteralSetDouble(updatedDoubleAstNode, 10.0);
    bool setFloatResult = g_impl->NumberLiteralSetFloat(updatedFloatAstNode, 11.0);
    return setInt32Result && setInt64Result && setDoubleResult && setFloatResult;
}

bool CheckEnumStringConversion(es2panda_Context *ctx)
{
    auto enumNodeHasParent =
        g_impl->Es2pandaEnumFromString(ctx, g_impl->Es2pandaEnumToString(ctx, Es2pandaEnum::ENUM_NODE_HAS_PARENT));
    if (enumNodeHasParent != Es2pandaEnum::ENUM_NODE_HAS_PARENT) {
        return false;
    }

    auto enumFirst = g_impl->Es2pandaEnumFromString(ctx, g_impl->Es2pandaEnumToString(ctx, Es2pandaEnum::ENUM_FIRST));
    if (enumFirst != Es2pandaEnum::ENUM_FIRST) {
        return false;
    }

    return true;
}

bool CheckExternalSource(es2panda_Context *ctx)
{
    g_impl->ProceedToState(ctx, ES2PANDA_STATE_BOUND);
    auto *program = g_impl->ContextProgram(ctx);

    size_t externalSourceCnt {0};
    auto **externalSourceList = g_impl->ProgramExternalSources(ctx, program, &externalSourceCnt);
    if (externalSourceList == nullptr) {
        return false;
    }

    auto externalSourceName = g_impl->ExternalSourceName(externalSourceList[0]);
    if (externalSourceName == nullptr) {
        return false;
    }

    size_t externalProgramCount {0};
    auto **programList = g_impl->ExternalSourcePrograms(externalSourceList[0], &externalProgramCount);
    if (programList == nullptr) {
        return false;
    }

    return true;
}

int main(int argc, char **argv)
{
    if (argc < MIN_ARGC) {
        return INVALID_ARGC_ERROR_CODE;
    }
    g_impl = GetImpl();
    if (g_impl == nullptr) {
        return NULLPTR_IMPL_ERROR_CODE;
    }
    auto config = g_impl->CreateConfig(argc - 1, argv + 1);
    auto src = std::string("function foo(builder: () => void) {}\nfoo(() => {})");
    auto context = g_impl->CreateContextFromString(config, src.c_str(), argv[argc - 1]);
    if (context == nullptr) {
        std::cerr << "FAILED TO CREATE CONTEXT" << std::endl;
        return NULLPTR_CONTEXT_ERROR_CODE;
    }

    if (!CheckSourcePosition(context)) {
        return SOURCE_POSITION_FAIL_CODE;
    }

    if (!CheckNumberLiteral(context)) {
        return NUMBER_LITERAL_FAIL_CODE;
    }

    if (!CheckEnumStringConversion(context)) {
        return ENUM_STRING_CONVERSION_FAIL_CODE;
    }

    if (!CheckExternalSource(context)) {
        return EXTERNAL_SOURCE_FAIL_CODE;
    }

    g_impl->DestroyContext(context);
    g_impl->DestroyConfig(config);

    return 0;
}

// NOLINTEND