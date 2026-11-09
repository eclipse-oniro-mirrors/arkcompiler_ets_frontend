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

#include "lsp/include/api.h"
#include "lsp_api_test.h"
#include "public/es2panda_lib.h"
#include <gtest/gtest.h>

namespace {
using ark::es2panda::lsp::Initializer;

class LspGetDefinitionFromNodeTest : public LSPAPITests {};

TEST_F(LspGetDefinitionFromNodeTest, GetDefinitionFromNode1)
{
    std::vector<std::string> files = {"GetDefinitionFromNode1.ets"};
    std::vector<std::string> texts = {R"(class Foo {
    Foo = 1;
})"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    Initializer initializer = Initializer();
    auto context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_PARSED);
    LSPAPI const *lspApi = GetImpl();
    const std::string nodeName = "Foo";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {nodeName, ark::es2panda::ir::AstNodeType::CLASS_PROPERTY});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(context, nodeInfoPtrs);
    initializer.DestroyContext(context);
    size_t const expectedStart = 16;
    size_t const expectedLength = 3;
    ASSERT_EQ(res.start, expectedStart);
    ASSERT_EQ(res.length, expectedLength);
}

TEST_F(LspGetDefinitionFromNodeTest, GetDefinitionFromNode2)
{
    std::vector<std::string> files = {"GetDefinitionFromNode2.ets"};
    std::vector<std::string> texts = {R"(
    //中文测试
    class Foo {
        //中文测试
        Foo = 1;
})"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    Initializer initializer = Initializer();
    auto context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_PARSED);
    LSPAPI const *lspApi = GetImpl();
    const std::string nodeName = "Foo";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {nodeName, ark::es2panda::ir::AstNodeType::CLASS_PROPERTY});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(context, nodeInfoPtrs);
    initializer.DestroyContext(context);
    size_t const expectedStart = 51;
    size_t const expectedLength = 3;
    ASSERT_EQ(res.start, expectedStart);
    ASSERT_EQ(res.length, expectedLength);
}

// A plain `export { name }` statement carries no node in a cleanly compiled AST: the
// global-class lowering removes it (only a build with errors keeps a range-preserving empty
// statement). The reported position is the exported name inside the statement, recovered
// from the source text; both build shapes report the same position.
TEST_F(LspGetDefinitionFromNodeTest, GetDefinitionFromNodePlainExportStatement)
{
    std::vector<std::string> files = {"GetDefinitionFromNodeExportStatement.ets"};
    std::vector<std::string> texts = {R"('use static';

export class Exported {
    real(): void { }
}

export { Exported }

function fnA(): void { }

export { fnA }
)"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    Initializer initializer = Initializer();
    auto context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    NodeInfo exportedInfo {"Exported", ark::es2panda::ir::AstNodeType::EXPORT_SPECIFIER};
    std::vector<NodeInfo *> exportedInfos {&exportedInfo};
    auto res = lspApi->getDefinitionDataFromNode(context, exportedInfos);
    initializer.DestroyContext(context);

    const auto exportedPos = texts[0].find("export { Exported }") + std::string("export { ").size();
    ASSERT_EQ(res.start, exportedPos);
    ASSERT_EQ(res.length, std::string("Exported").size());
}

// The rename query recovers the same plain export statement position.
TEST_F(LspGetDefinitionFromNodeTest, FindRenameLocationsFromNodePlainExportStatement)
{
    std::vector<std::string> files = {"GetDefinitionFromNodeExportStatementRename.ets"};
    std::vector<std::string> texts = {R"('use static';

export class Exported {
    real(): void { }
}

export { Exported }

function fnA(): void { }

export { fnA }
)"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    Initializer initializer = Initializer();
    auto context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    NodeInfo fnInfo {"fnA", ark::es2panda::ir::AstNodeType::EXPORT_NAMED_DECLARATION};
    std::vector<NodeInfo *> fnInfos {&fnInfo};
    auto res = lspApi->findRenameLocationsFromNode(context, fnInfos);
    initializer.DestroyContext(context);

    const auto fnPos = texts[0].find("export { fnA }") + std::string("export { ").size();
    ASSERT_EQ(res.start, fnPos);
    ASSERT_EQ(res.end - res.start, std::string("fnA").size());
}

// The plain-export text fallback is lexer-aware: a commented-out or textually embedded
// 'export { x }' must not be taken for the real statement, so the reported position always
// anchors at the real export.
TEST_F(LspGetDefinitionFromNodeTest, GetDefinitionFromNodePlainExportStatementSkipsCommentsAndStrings)
{
    std::vector<std::string> files = {"GetDefinitionFromNodeExportStatementComments.ets"};
    std::vector<std::string> texts = {R"('use static';

// export { Exported }
/* export { Exported } */
const marker = "export { Exported }";

export class Exported {
    real(): void { }
}

export { Exported }
)"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    Initializer initializer = Initializer();
    auto context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    NodeInfo exportedInfo {"Exported", ark::es2panda::ir::AstNodeType::EXPORT_SPECIFIER};
    std::vector<NodeInfo *> exportedInfos {&exportedInfo};
    auto res = lspApi->getDefinitionDataFromNode(context, exportedInfos);
    initializer.DestroyContext(context);

    // The real statement is the last occurrence; the comment and string occurrences must
    // be skipped by the scan.
    const auto exportedPos = texts[0].rfind("export { Exported }") + std::string("export { ").size();
    ASSERT_NE(exportedPos, std::string::npos);
    ASSERT_EQ(res.start, exportedPos);
    ASSERT_EQ(res.length, std::string("Exported").size());
}

// The rename query recovers the same lexer-aware plain export statement position.
TEST_F(LspGetDefinitionFromNodeTest, FindRenameLocationsFromNodePlainExportStatementSkipsComments)
{
    std::vector<std::string> files = {"GetDefinitionFromNodeExportStatementRenameComments.ets"};
    std::vector<std::string> texts = {R"('use static';

// export { fnA }
const marker = "export { fnA }";

function fnA(): void { }

export { fnA }
)"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    Initializer initializer = Initializer();
    auto context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    NodeInfo fnInfo {"fnA", ark::es2panda::ir::AstNodeType::EXPORT_NAMED_DECLARATION};
    std::vector<NodeInfo *> fnInfos {&fnInfo};
    auto res = lspApi->findRenameLocationsFromNode(context, fnInfos);
    initializer.DestroyContext(context);

    const auto fnPos = texts[0].rfind("export { fnA }") + std::string("export { ").size();
    ASSERT_NE(fnPos, std::string::npos);
    ASSERT_EQ(res.start, fnPos);
    ASSERT_EQ(res.end - res.start, std::string("fnA").size());
}

// A source whose only 'export { x }' occurrence lives inside a comment reports no
// position: the text fallback finds no real statement.
TEST_F(LspGetDefinitionFromNodeTest, GetDefinitionFromNodePlainExportStatementOnlyInCommentStaysEmpty)
{
    std::vector<std::string> files = {"GetDefinitionFromNodeExportStatementCommentOnly.ets"};
    std::vector<std::string> texts = {R"('use static';

// export { Exported }
class Exported {
    real(): void { }
}
)"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    Initializer initializer = Initializer();
    auto context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    NodeInfo exportedInfo {"Exported", ark::es2panda::ir::AstNodeType::EXPORT_SPECIFIER};
    std::vector<NodeInfo *> exportedInfos {&exportedInfo};
    auto res = lspApi->getDefinitionDataFromNode(context, exportedInfos);
    initializer.DestroyContext(context);

    ASSERT_EQ(res.start, 0U);
    ASSERT_EQ(res.length, 0U);
}

// A comment between the 'export' keyword and the specifier braces is trivia: the text
// fallback still reaches the real statement and reports the exported name.
TEST_F(LspGetDefinitionFromNodeTest, GetDefinitionFromNodePlainExportStatementSkipsCommentBeforeBraces)
{
    std::vector<std::string> files = {"GetDefinitionFromNodeExportStatementCommentBeforeBraces.ets"};
    std::vector<std::string> texts = {R"('use static';

const foo = 1;
export /* comment */ { foo };
)"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    Initializer initializer = Initializer();
    auto context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    NodeInfo fooInfo {"foo", ark::es2panda::ir::AstNodeType::EXPORT_SPECIFIER};
    std::vector<NodeInfo *> fooInfos {&fooInfo};
    auto res = lspApi->getDefinitionDataFromNode(context, fooInfos);
    initializer.DestroyContext(context);

    const auto fooPos = texts[0].rfind("foo };");
    ASSERT_NE(fooPos, std::string::npos);
    ASSERT_EQ(res.start, fooPos);
    ASSERT_EQ(res.length, std::string("foo").size());
}

// A '}' inside a comment in the specifier list does not close the export statement: the
// fallback skips the dead brace and reports the exported name in live code.
TEST_F(LspGetDefinitionFromNodeTest, GetDefinitionFromNodePlainExportStatementSkipsClosingBraceInComment)
{
    std::vector<std::string> files = {"GetDefinitionFromNodeExportStatementBraceInComment.ets"};
    std::vector<std::string> texts = {R"('use static';

const foo = 1;
export { /* } */ foo };
)"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    Initializer initializer = Initializer();
    auto context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    NodeInfo fooInfo {"foo", ark::es2panda::ir::AstNodeType::EXPORT_SPECIFIER};
    std::vector<NodeInfo *> fooInfos {&fooInfo};
    auto res = lspApi->getDefinitionDataFromNode(context, fooInfos);
    initializer.DestroyContext(context);

    const auto fooPos = texts[0].rfind("foo };");
    ASSERT_NE(fooPos, std::string::npos);
    ASSERT_EQ(res.start, fooPos);
    ASSERT_EQ(res.length, std::string("foo").size());
}

}  // namespace