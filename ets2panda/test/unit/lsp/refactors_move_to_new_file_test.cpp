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

#include <gtest/gtest.h>
#include <cstddef>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>
#include "ir/astNode.h"
#include "lsp/include/internal_api.h"
#include "lsp_api_test.h"
#include "lsp/include/applicable_refactors.h"
#include "lsp/include/refactors/refactor_types.h"
#include "lsp/include/refactors/move_to_new_file.h"
#include "lsp/include/organize_imports.h"
#include "compiler/lowering/util.h"
#include "public/es2panda_lib.h"
#include <fstream>

#if __has_include(<filesystem>)
#include <filesystem>
namespace fs = std::filesystem;
#elif __has_include(<experimental/filesystem>)
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif

namespace {
using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::MoveToNewFileRefactor;
using ark::es2panda::lsp::RefactorContext;

class LSPMoveToNewFileTests : public LSPAPITests {
public:
    static constexpr std::string_view K_KIND = "refactor.move.newFile";
    static constexpr std::string_view K_ACTION_NAME = "Move to a new file";
    static constexpr std::string_view K_START_MARKER = "/*1*/";
    static constexpr std::string_view K_END_MARKER = "/*2*/";

protected:
    static RefactorContext MakeCtx(es2panda_Context *ctx, TextChangesContext *textCtx, size_t pos, size_t end)
    {
        RefactorContext rc;
        rc.context = ctx;
        rc.kind = std::string(K_KIND);
        rc.span.pos = pos;
        rc.span.end = end;
        rc.textChangesContext = textCtx;
        return rc;
    }
    bool NodeIsMissing(ark::es2panda::ir::AstNode *node)
    {
        if (node == nullptr) {
            return true;
        }
        size_t pos = node->Range().start.index;
        size_t end = node->Range().end.index;
        return pos == end;
    }

    std::string GetSourceTextOfNodeFromSourceFile(ark::es2panda::util::StringView sourceCode,
                                                  ark::es2panda::ir::AstNode *node)
    {
        if (NodeIsMissing(node)) {
            return "";
        }
        size_t pos = node->Range().start.index;
        size_t end = node->Range().end.index;
        auto text = std::string(sourceCode.Substr(pos, end));
        return text;
    }

    void DeleteFile(const std::string &fullPath);
    std::string ReadFile(const std::string &fullPath);
};

void LSPMoveToNewFileTests::DeleteFile(const std::string &fullPath)
{
    fs::remove(fullPath);
}

std::string LSPMoveToNewFileTests::ReadFile(const std::string &fullPath)
{
    std::ifstream ifs(fullPath);
    if (!ifs.is_open()) {
        return "";
    }
    std::stringstream buffer;
    buffer << ifs.rdbuf();
    return buffer.str();
}

const std::string TEMP_DIR = "/tmp/lsp_api_test_refactors_move_to_new_file/";

TEST_F(LSPMoveToNewFileTests, ExportCase1)
{
    std::string strNewFilePath = TEMP_DIR + "MoveToNewFile_sum.ets";
    std::vector<std::string> files = {"MoveToNewFileTests_export_case1.ets"};
    std::vector<std::string> texts = {R"(
function MoveToNewFile_sum(a: number, b: number): number {
    return a + b;
}
let result = sum(1, 2);
)"};
    auto filePaths = CreateTempFile(files, texts);
    DeleteFile(strNewFilePath);

    Initializer init;
    es2panda_Context *ctx = init.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_PARSED);

    ASSERT_NE(ctx, nullptr);
    const auto funcNode = ark::es2panda::lsp::GetTouchingToken(ctx, 10, true);
    const size_t pos = funcNode->Start().index;
    const size_t end = funcNode->End().index;
    MoveToNewFileRefactor ref;
    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    TextChangesContext changeText {{}, formatContext, {}};
    auto refContext = MakeCtx(ctx, &changeText, pos, end);
    auto avail = ref.GetAvailableActions(refContext);
    ASSERT_FALSE(avail.empty());
    EXPECT_EQ(avail[0].action.name, std::string(K_ACTION_NAME));
    EXPECT_EQ(avail[0].action.kind, std::string(K_KIND));
    std::unique_ptr<ark::es2panda::lsp::RefactorEditInfo> editInfo =
        ref.GetEditsForAction(refContext, std::string(K_ACTION_NAME));
    ASSERT_NE(editInfo, nullptr);
    std::vector<FileTextChanges> changes = editInfo->GetFileTextChanges();
    ASSERT_FALSE(changes.empty());
    ASSERT_TRUE(changes.size() == 1);
    std::string newFileContent = ReadFile(strNewFilePath);
    const auto content = R"(export function MoveToNewFile_sum(a: number, b: number): number {
    return a + b;
}
)";
    ASSERT_EQ(newFileContent, content);

    init.DestroyContext(ctx);
}

TEST_F(LSPMoveToNewFileTests, ExportCase2)
{
    std::string strNewFilePath = TEMP_DIR + "MoveToNewFile_PI.ets";
    std::vector<std::string> files = {"MoveToNewFileTests_export_case2.ets"};
    std::vector<std::string> texts = {R"(const MoveToNewFile_PI = 3.14;
let version = "1.0";
console.log(MoveToNewFile_PI, version, debug);
)"};
    DeleteFile(strNewFilePath);

    auto filePaths = CreateTempFile(files, texts);

    Initializer init;
    es2panda_Context *ctx = init.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);
    const size_t pos = 0;
    const size_t end = 49;
    MoveToNewFileRefactor ref;
    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    TextChangesContext changeText {{}, formatContext, {}};
    auto refContext = MakeCtx(ctx, &changeText, pos, end);
    auto avail = ref.GetAvailableActions(refContext);
    ASSERT_FALSE(avail.empty());
    EXPECT_EQ(avail[0].action.name, std::string(K_ACTION_NAME));
    EXPECT_EQ(avail[0].action.kind, std::string(K_KIND));
    std::unique_ptr<ark::es2panda::lsp::RefactorEditInfo> editInfo =
        ref.GetEditsForAction(refContext, std::string(K_ACTION_NAME));
    ASSERT_NE(editInfo, nullptr);
    std::vector<FileTextChanges> changes = editInfo->GetFileTextChanges();
    EXPECT_FALSE(changes.empty());
    EXPECT_EQ(changes.size(), 1);

    std::string newFileContent = ReadFile(strNewFilePath);
    const auto content = R"(export const MoveToNewFile_PI = 3.14;
export let version = "1.0";
)";
    ASSERT_EQ(newFileContent, content);
    init.DestroyContext(ctx);
}

TEST_F(LSPMoveToNewFileTests, ImportCase5)
{
    std::vector<std::string> files = {"MoveToNewFileTests_import_case5.ets"};
    std::vector<std::string> texts = {
        R"(
            export function foo() {}
            foo();
        )"};
    auto filePaths = CreateTempFile(files, texts);

    Initializer init;
    es2panda_Context *ctx = init.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_PARSED);

    ASSERT_NE(ctx, nullptr);
    const auto context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    const auto ast = context->parserProgram->Ast();
    const auto funcNode = ark::es2panda::lsp::GetTouchingToken(ctx, 30, true);
    auto deneme = ark::es2panda::lsp::GetIsNodeHasExport(ctx, funcNode);
    (void)deneme;
    (void)ast;
    init.DestroyContext(ctx);
}

constexpr size_t MOVE_TO_NEW_FILE_1_TEXT_CHANGES_SIZE = 4;
constexpr size_t INTERFACE_BASE_START = 1;
constexpr size_t INTERFACE_BASE_LENGTH = 87;
constexpr size_t INTERFACE_DERIVED_START = 89;
constexpr size_t INTERFACE_DERIVED_LENGTH = 53;

TEST_F(LSPMoveToNewFileTests, MoveToNewFile1)
{
    std::string strNewFilePath = TEMP_DIR + "MoveToNewFile_Base.ets";
    DeleteFile(strNewFilePath);
    const std::string src = R"(
interface MoveToNewFile_Base {
param(p: Derived): /*1*/void
ret(): MoveToNewFile_Base
}
interface Derived extends MoveToNewFile_Base {/*2*/
}
)";
    auto files = CreateTempFile({"MoveToNewFileTests_case1.ets"}, {src});
    ASSERT_FALSE(files.empty());
    Initializer init;
    es2panda_Context *ctx = init.CreateContext(files[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);
    const auto context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    const auto ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);
    const size_t pos = src.find(std::string(K_START_MARKER));
    const size_t end = src.find(std::string(K_END_MARKER));
    MoveToNewFileRefactor ref;
    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    TextChangesContext changeText {{}, formatContext, {}};
    auto refContext = MakeCtx(ctx, &changeText, pos, end);
    auto avail = ref.GetAvailableActions(refContext);
    ASSERT_FALSE(avail.empty());
    EXPECT_EQ(avail[0].action.name, std::string(K_ACTION_NAME));
    EXPECT_EQ(avail[0].action.kind, std::string(K_KIND));
    std::unique_ptr<ark::es2panda::lsp::RefactorEditInfo> editInfo =
        ref.GetEditsForAction(refContext, std::string(K_ACTION_NAME));
    ASSERT_NE(editInfo, nullptr);
    std::vector<FileTextChanges> changes = editInfo->GetFileTextChanges();
    ASSERT_FALSE(changes.empty());
    fs::path filePath = fs::path(strNewFilePath);
    EXPECT_TRUE(fs::exists(filePath));
    EXPECT_TRUE(changes.size() == 1);
    EXPECT_TRUE(changes[0].textChanges.size() == MOVE_TO_NEW_FILE_1_TEXT_CHANGES_SIZE);
    EXPECT_TRUE(changes[0].textChanges[0].newText.empty() && changes[0].textChanges[1].newText.empty());
    EXPECT_TRUE(changes[0].textChanges[0].span.start == INTERFACE_BASE_START);
    EXPECT_TRUE(changes[0].textChanges[0].span.length == INTERFACE_BASE_LENGTH);
    EXPECT_TRUE(changes[0].textChanges[1].span.start == INTERFACE_DERIVED_START);
    EXPECT_TRUE(changes[0].textChanges[1].span.length == INTERFACE_DERIVED_LENGTH);
    EXPECT_TRUE("\n" + ReadFile(strNewFilePath) == src);
    init.DestroyContext(ctx);
}

TEST_F(LSPMoveToNewFileTests, MoveToNewFile2)
{
    std::string strNewFilePath = TEMP_DIR + "MoveToNewFile_Derived.ets";
    DeleteFile(strNewFilePath);
    const std::string src = R"(
interface Base {
param(p: MoveToNewFile_Derived): void
ret(): Base
}
interface MoveToNewFile_Derived /*1*/extends Base {/*2*/
}
)";
    const size_t interfaceDerivedStartIndex = 70;
    const size_t interfaceDerivedSize = 59;
    auto files = CreateTempFile({"MoveToNewFileTests_case2.ets"}, {src});
    ASSERT_FALSE(files.empty());

    Initializer init;
    es2panda_Context *ctx = init.CreateContext(files[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);
    const auto context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    const auto ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);
    const size_t pos = src.find(std::string(K_START_MARKER));
    const size_t end = src.find(std::string(K_END_MARKER));
    MoveToNewFileRefactor ref;
    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    TextChangesContext changeText {{}, formatContext, {}};
    auto refContext = MakeCtx(ctx, &changeText, pos, end);
    auto avail = ref.GetAvailableActions(refContext);
    ASSERT_FALSE(avail.empty());
    EXPECT_EQ(avail[0].action.name, std::string(K_ACTION_NAME));
    EXPECT_EQ(avail[0].action.kind, std::string(K_KIND));
    std::unique_ptr<ark::es2panda::lsp::RefactorEditInfo> editInfo =
        ref.GetEditsForAction(refContext, std::string(K_ACTION_NAME));
    ASSERT_NE(editInfo, nullptr);
    std::vector<FileTextChanges> changes = editInfo->GetFileTextChanges();
    ASSERT_FALSE(changes.empty());
    ASSERT_TRUE(changes.size() == 1);
    std::string newFileContent = ReadFile(strNewFilePath);
    ASSERT_TRUE(newFileContent == src.substr(interfaceDerivedStartIndex, interfaceDerivedSize));
    init.DestroyContext(ctx);
}

TEST_F(LSPMoveToNewFileTests, ImportCase1)
{
    DeleteFile(TEMP_DIR + "MoveToNewFile_Derived.ets");
    std::vector<std::string> files = {"MoveToNewFileTests_import_case1.ets", "MoveToNewFileTests_import_11.ets",
                                      "MoveToNewFileTests_import_12.ets"};
    std::vector<std::string> texts = {
        R"(
import {B, C, A} from "./MoveToNewFileTests_import_11";
import { X } from "./MoveToNewFileTests_import_12";
interface Base {
param(p: MoveToNewFile_Derived): void
ret(): Base
}
interface MoveToNewFile_Derived extends Base {
}
)",
        R"(export const A = 1; export const B = 2; export const C = 3;)", R"(export const X = 1;)"};
    auto filePaths = CreateTempFile(files, texts);

    Initializer init;
    es2panda_Context *ctx = init.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    ASSERT_NE(ctx, nullptr);
    const auto context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    const auto ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);
    const size_t pos = 186;
    const size_t end = 220;
    MoveToNewFileRefactor ref;
    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    TextChangesContext changeText {{}, formatContext, {}};
    auto refContext = MakeCtx(ctx, &changeText, pos, end);
    auto avail = ref.GetAvailableActions(refContext);
    ASSERT_FALSE(avail.empty());
    EXPECT_EQ(avail[0].action.name, std::string(K_ACTION_NAME));
    EXPECT_EQ(avail[0].action.kind, std::string(K_KIND));

    std::unique_ptr<ark::es2panda::lsp::RefactorEditInfo> editInfo =
        ref.GetEditsForAction(refContext, std::string(K_ACTION_NAME));
    ASSERT_NE(editInfo, nullptr);
    std::vector<FileTextChanges> changes = editInfo->GetFileTextChanges();

    init.DestroyContext(ctx);
}

TEST_F(LSPMoveToNewFileTests, ImportCase2)
{
    DeleteFile(TEMP_DIR + "MoveToNewFileDerived.ets");
    std::vector<std::string> texts = {R"(
import {B, C, A, Test1} from "./MoveToNewFileTests_import_21";
import { Base, X } from "./MoveToNewFileTests_import_22";
interface MoveToNewFileDerived extends Base {
}
interface Test2 extends Test1 {
}
)",
                                      R"(export const A = 1; export const B = 2; export const C = 3;
export interface Test1 { }
)",
                                      R"(export interface Base {
param(): void
ret(): Base }
export const X = 1;
)"};
    std::vector<std::string> files = {"MoveToNewFileTests_import_case2.ets", "MoveToNewFileTests_import_21.ets",
                                      "MoveToNewFileTests_import_22"};
    auto filePaths = CreateTempFile(files, texts);
    Initializer init;
    es2panda_Context *ctx = init.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);
    const auto context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    const auto ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);
    auto statements = ast->Statements();
    std::vector<ark::es2panda::ir::Statement *> list;
    for (ark::es2panda::ir::Statement *node : statements) {
        list.push_back(node);
    }
    const size_t pos = 96;
    const size_t end = 128;
    MoveToNewFileRefactor ref;
    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    TextChangesContext changeText {{}, formatContext, {}};
    auto refContext = MakeCtx(ctx, &changeText, pos, end);
    auto avail = ref.GetAvailableActions(refContext);
    ASSERT_FALSE(avail.empty());
    EXPECT_EQ(avail[0].action.name, std::string(K_ACTION_NAME));
    EXPECT_EQ(avail[0].action.kind, std::string(K_KIND));
    std::unique_ptr<ark::es2panda::lsp::RefactorEditInfo> editInfo =
        ref.GetEditsForAction(refContext, std::string(K_ACTION_NAME));
    ASSERT_NE(editInfo, nullptr);
    std::vector<FileTextChanges> changes = editInfo->GetFileTextChanges();
    ASSERT_TRUE(changes.empty() == false && changes.size() == 1);
    init.DestroyContext(ctx);
}

}  // namespace