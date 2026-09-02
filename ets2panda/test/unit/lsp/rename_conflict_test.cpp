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

#include <gtest/gtest.h>
#include <string>
#include <variant>
#include <vector>
#include "lsp_api_test.h"
#include "lsp/include/rename.h"
#include "public/es2panda_lib.h"
#include "public/public.h"
#include "util/path.h"

namespace {

class LspRenameConflictTests : public LSPAPITests {
public:
    // Get the panda library path from the console declaration node (same pattern as lsp_rename_test.cpp).
    std::string GetPandalibPath(const ark::es2panda::ir::AstNode *consoleDecl)
    {
        auto consoleFilePath = std::string(consoleDecl->Range().start.Program()->SourceFile().GetAbsolutePath().Utf8());
        size_t lastDelimPos = consoleFilePath.find_last_of(ark::es2panda::util::PATH_DELIMITER);
        std::string consoleDir =
            (lastDelimPos != std::string::npos) ? consoleFilePath.substr(0, lastDelimPos) : consoleFilePath;
        std::string pandaLibPath = consoleDir;
        size_t pos = 0;
        const int threeLevelsUp = 3;
        for (int i = 0; i < threeLevelsUp; ++i) {
            pos = pandaLibPath.find_last_of(ark::es2panda::util::PATH_DELIMITER);
            if (pos != std::string::npos) {
                pandaLibPath = pandaLibPath.substr(0, pos);
            }
        }
        return pandaLibPath;
    }

    // Helper to get pandaLibPath from a context that uses console.
    std::string GetLibPathFromContext(es2panda_Context *ctx)
    {
        auto ast = GetAstFromContext<ark::es2panda::ir::AstNode>(ctx);
        auto consoleNode = ast->FindChild([](ark::es2panda::ir::AstNode *childNode) {
            return childNode->IsIdentifier() && childNode->AsIdentifier()->Name() == "console";
        });
        auto consoleDecl = consoleNode->Variable()->Declaration()->Node();
        return GetPandalibPath(consoleDecl);
    }
};

// Test: Rename on a number literal position returns failure (not eligible for rename)
TEST_F(LspRenameConflictTests, RenameOnNumberLiteralFails)
{
    const std::string fileContent = "let x = 42;\nconsole.log(x);\n";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("rename_num.ets", ES2PANDA_STATE_CHECKED, fileContent.c_str());
    ASSERT_NE(ctx, nullptr);

    std::string pandaLibPath = GetLibPathFromContext(ctx);
    LSPAPI const *lspApi = GetImpl();

    // Position at the number literal "42".
    auto numPos = fileContent.find("42");
    ASSERT_NE(numPos, std::string::npos);
    auto result = lspApi->getRenameInfo(ctx, numPos, const_cast<char *>(pandaLibPath.c_str()));

    ASSERT_TRUE(std::holds_alternative<ark::es2panda::lsp::RenameInfoFailure>(result));
    auto failure = std::get<ark::es2panda::lsp::RenameInfoFailure>(result);
    EXPECT_FALSE(failure.GetCanRenameFailure());
    EXPECT_EQ(failure.GetLocalizedErrorMessage(), "You cannot rename this element");

    initializer.DestroyContext(ctx);
}

// Test: Rename on a position with no valid token returns failure
TEST_F(LspRenameConflictTests, RenameOnEmptyPositionFails)
{
    const std::string fileContent = "let x = 1;\nconsole.log(x);\n";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("rename_empty.ets", ES2PANDA_STATE_CHECKED, fileContent.c_str());
    ASSERT_NE(ctx, nullptr);

    std::string pandaLibPath = GetLibPathFromContext(ctx);
    LSPAPI const *lspApi = GetImpl();

    // Position past the end of the file content.
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t pastEndMargin = 100;
    auto result =
        lspApi->getRenameInfo(ctx, fileContent.size() + pastEndMargin, const_cast<char *>(pandaLibPath.c_str()));

    ASSERT_TRUE(std::holds_alternative<ark::es2panda::lsp::RenameInfoFailure>(result));
    auto failure = std::get<ark::es2panda::lsp::RenameInfoFailure>(result);
    EXPECT_FALSE(failure.GetCanRenameFailure());

    initializer.DestroyContext(ctx);
}

// Test: Rename on a string literal that is not a module specifier or contextual type returns failure
TEST_F(LspRenameConflictTests, RenameOnPlainStringLiteralFails)
{
    const std::string fileContent = "let msg = \"hello world\";\nconsole.log(msg);\n";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("rename_str.ets", ES2PANDA_STATE_CHECKED, fileContent.c_str());
    ASSERT_NE(ctx, nullptr);

    std::string pandaLibPath = GetLibPathFromContext(ctx);
    LSPAPI const *lspApi = GetImpl();

    // Position at the string literal content "hello world".
    auto strPos = fileContent.find("hello world");
    ASSERT_NE(strPos, std::string::npos);
    auto result = lspApi->getRenameInfo(ctx, strPos, const_cast<char *>(pandaLibPath.c_str()));

    // String literals that are not contextual types or module specifiers should not be renameable.
    ASSERT_TRUE(std::holds_alternative<ark::es2panda::lsp::RenameInfoFailure>(result));
    auto failure = std::get<ark::es2panda::lsp::RenameInfoFailure>(result);
    EXPECT_FALSE(failure.GetCanRenameFailure());

    initializer.DestroyContext(ctx);
}

// Test: Rename on a valid local variable succeeds
TEST_F(LspRenameConflictTests, RenameOnLocalVariableSucceeds)
{
    const std::string fileContent =
        "let myVar = 1;\nfunction use(): number {\n  return myVar;\n}\nconsole.log(myVar);\n";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("rename_var.ets", ES2PANDA_STATE_CHECKED, fileContent.c_str());
    ASSERT_NE(ctx, nullptr);

    std::string pandaLibPath = GetLibPathFromContext(ctx);
    LSPAPI const *lspApi = GetImpl();

    // Position at "myVar" usage in the function body.
    auto usePos = fileContent.find("return myVar");
    ASSERT_NE(usePos, std::string::npos);
    auto varPos = usePos + std::string("return ").size();
    auto result = lspApi->getRenameInfo(ctx, varPos, const_cast<char *>(pandaLibPath.c_str()));

    ASSERT_TRUE(std::holds_alternative<ark::es2panda::lsp::RenameInfoSuccess>(result));
    auto success = std::get<ark::es2panda::lsp::RenameInfoSuccess>(result);
    EXPECT_TRUE(success.GetCanRenameSuccess());
    EXPECT_EQ(success.GetDisplayName(), "myVar");

    initializer.DestroyContext(ctx);
}

// Test: Rename on a library-defined symbol (console) returns failure
TEST_F(LspRenameConflictTests, RenameOnLibrarySymbolFails)
{
    const std::string fileContent = "let x = 1;\nconsole.log(x);\n";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("rename_lib.ets", ES2PANDA_STATE_CHECKED, fileContent.c_str());
    ASSERT_NE(ctx, nullptr);

    std::string pandaLibPath = GetLibPathFromContext(ctx);
    LSPAPI const *lspApi = GetImpl();

    // Position at "console" usage.
    auto consolePos = fileContent.find("console");
    ASSERT_NE(consolePos, std::string::npos);
    auto result = lspApi->getRenameInfo(ctx, consolePos, const_cast<char *>(pandaLibPath.c_str()));

    // console is defined in a library file, so rename should fail.
    ASSERT_TRUE(std::holds_alternative<ark::es2panda::lsp::RenameInfoFailure>(result));
    auto failure = std::get<ark::es2panda::lsp::RenameInfoFailure>(result);
    EXPECT_FALSE(failure.GetCanRenameFailure());
    EXPECT_EQ(failure.GetLocalizedErrorMessage(), "You cannot rename this element");

    initializer.DestroyContext(ctx);
}

// Declaration/generated files are read-only API surfaces and must not offer rename.
TEST_F(LspRenameConflictTests, DISABLED_RenameOnDeclarationFileSymbolFails)
{
    ark::es2panda::lsp::Initializer initializer;
    const std::string probeSource = "let probe = 1;\nconsole.log(probe);\n";
    auto *probeCtx =
        initializer.CreateContext("rename_decl_path_probe.ets", ES2PANDA_STATE_CHECKED, probeSource.c_str());
    ASSERT_NE(probeCtx, nullptr);
    std::string pandaLibPath = GetLibPathFromContext(probeCtx);
    initializer.DestroyContext(probeCtx);

    const std::string declarationSource = "export declare class DeclaredType {}\n";
    auto *declarationCtx =
        initializer.CreateContext("rename_generated.d.ets", ES2PANDA_STATE_CHECKED, declarationSource.c_str());
    ASSERT_NE(declarationCtx, nullptr);
    const auto position = declarationSource.find("DeclaredType");
    ASSERT_NE(position, std::string::npos);

    auto result = GetImpl()->getRenameInfo(declarationCtx, position, const_cast<char *>(pandaLibPath.c_str()));
    initializer.DestroyContext(declarationCtx);

    ASSERT_TRUE(std::holds_alternative<ark::es2panda::lsp::RenameInfoFailure>(result));
    const auto &failure = std::get<ark::es2panda::lsp::RenameInfoFailure>(result);
    EXPECT_FALSE(failure.GetCanRenameFailure());
    EXPECT_EQ(failure.GetLocalizedErrorMessage(), "You cannot rename this element");
}

// Test: Rename on a function parameter succeeds
TEST_F(LspRenameConflictTests, RenameOnFunctionParameterSucceeds)
{
    const std::string fileContent =
        "function add(a: number, b: number): number {\n  return a + b;\n}\nconsole.log(add(1, 2));\n";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("rename_param.ets", ES2PANDA_STATE_CHECKED, fileContent.c_str());
    ASSERT_NE(ctx, nullptr);

    std::string pandaLibPath = GetLibPathFromContext(ctx);
    LSPAPI const *lspApi = GetImpl();

    // Position at parameter "a" in the function body.
    auto bodyPos = fileContent.find("return a");
    ASSERT_NE(bodyPos, std::string::npos);
    auto paramPos = bodyPos + std::string("return ").size();
    auto result = lspApi->getRenameInfo(ctx, paramPos, const_cast<char *>(pandaLibPath.c_str()));

    ASSERT_TRUE(std::holds_alternative<ark::es2panda::lsp::RenameInfoSuccess>(result));
    auto success = std::get<ark::es2panda::lsp::RenameInfoSuccess>(result);
    EXPECT_TRUE(success.GetCanRenameSuccess());

    initializer.DestroyContext(ctx);
}

// Test: Rename on a class method succeeds
TEST_F(LspRenameConflictTests, RenameOnClassMethodSucceeds)
{
    const std::string fileContent = R"(
class Calculator {
    x: number = 0;
    add(value: number): void {
        this.x += value;
    }
    getX(): number {
        return this.x;
    }
}
const calc = new Calculator();
calc.add(5);
console.log(calc.getX());
)";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("rename_method.ets", ES2PANDA_STATE_CHECKED, fileContent.c_str());
    ASSERT_NE(ctx, nullptr);

    std::string pandaLibPath = GetLibPathFromContext(ctx);
    LSPAPI const *lspApi = GetImpl();

    // Position at "add" usage.
    auto addUsePos = fileContent.find("calc.add(5)");
    ASSERT_NE(addUsePos, std::string::npos);
    auto methodPos = addUsePos + std::string("calc.").size();
    auto result = lspApi->getRenameInfo(ctx, methodPos, const_cast<char *>(pandaLibPath.c_str()));

    ASSERT_TRUE(std::holds_alternative<ark::es2panda::lsp::RenameInfoSuccess>(result));
    auto success = std::get<ark::es2panda::lsp::RenameInfoSuccess>(result);
    EXPECT_TRUE(success.GetCanRenameSuccess());
    EXPECT_EQ(success.GetDisplayName(), "add");

    initializer.DestroyContext(ctx);
}

// Test: findRenameLocations returns empty for a position with no renameable symbol
TEST_F(LspRenameConflictTests, FindRenameLocationsOnInvalidPositionReturnsEmpty)
{
    const std::string fileContent = "let x = 1;\nconsole.log(x);\n";
    std::vector<std::string> files = {"rename_invalid_pos.ets"};
    std::vector<std::string> texts = {fileContent};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    ark::es2panda::lsp::Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    std::vector<es2panda_Context *> fileContexts = {context};

    LSPAPI const *lspApi = GetImpl();
    // Position at the number literal "1" (not a renameable symbol).
    auto numPos = fileContent.find("1;");
    ASSERT_NE(numPos, std::string::npos);
    auto result = lspApi->findRenameLocations(fileContexts, context, numPos);
    // No rename locations should be found for a non-renameable position.
    EXPECT_TRUE(result.empty());

    initializer.DestroyContext(context);
}

}  // namespace
