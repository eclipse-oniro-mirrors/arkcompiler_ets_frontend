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

#include "lsp_api_test.h"

#include <gtest/gtest.h>
#include <cstddef>
#include <string>

#include "ir/astNode.h"
#include "lsp/include/internal_api.h"
#include "public/es2panda_lib.h"

namespace {

using ark::es2panda::ir::AstNode;
using ark::es2panda::lsp::Initializer;

class NodeTokenOffsetBoundaryTests : public LSPAPITests {};

TEST_F(NodeTokenOffsetBoundaryTests, TouchingTokenAtNodeStartAndEndBoundaries)
{
    const std::string source = "let foo = 1;\nfoo = 2;\n";
    Initializer initializer = Initializer();
    es2panda_Context *context =
        initializer.CreateContext("boundary_touch_node_edges.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_EQ(ContextState(context), ES2PANDA_STATE_PARSED);

    auto ast = GetAstFromContext<AstNode>(context);
    auto expected = ast->FindChild([](AstNode *node) {
        return node->IsIdentifier() && node->Parent() != nullptr && node->Parent()->IsVariableDeclarator();
    });
    ASSERT_NE(expected, nullptr);
    const size_t idStart = source.find("foo");  // position of 'f' in the declared 'foo'
    const size_t idEnd = idStart + std::string("foo").size();

    // Cursor at the first byte of the identifier returns the identifier itself.
    auto *atStart = ark::es2panda::lsp::GetTouchingToken(context, idStart, false);
    ASSERT_EQ(atStart->DumpJSON(), expected->DumpJSON());
    ASSERT_EQ(atStart->Start().index, idStart);
    ASSERT_EQ(atStart->End().index, idEnd);

    // Cursor inside the identifier (last byte) still returns the identifier.
    auto *inside = ark::es2panda::lsp::GetTouchingToken(context, idEnd - 1, false);
    ASSERT_EQ(inside->DumpJSON(), expected->DumpJSON());

    // Cursor exactly at the end boundary of the identifier (on ' ') falls back
    // to the enclosing variable declarator.
    auto *atEnd = ark::es2panda::lsp::GetTouchingToken(context, idEnd, false);
    ASSERT_EQ(atEnd->Type(), ark::es2panda::ir::AstNodeType::VARIABLE_DECLARATOR);
    ASSERT_EQ(atEnd->Start().index, idStart);
    ASSERT_EQ(atEnd->End().index, source.find(';'));

    initializer.DestroyContext(context);
}

TEST_F(NodeTokenOffsetBoundaryTests, TouchingTokenNestedNodeInnermostVsFirstMatch)
{
    const std::string source = "function outer(): number {\n    return 1;\n}\nouter();\n";
    Initializer initializer = Initializer();
    es2panda_Context *context =
        initializer.CreateContext("boundary_touch_nested.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_EQ(ContextState(context), ES2PANDA_STATE_PARSED);

    auto ast = GetAstFromContext<AstNode>(context);
    auto expected = ast->FindChild([](AstNode *node) {
        return node->IsIdentifier() && node->Parent() != nullptr && node->Parent()->IsCallExpression();
    });
    ASSERT_NE(expected, nullptr);
    const size_t callPos = source.find("outer();\n");  // position of 'o' in 'outer();'

    // Default strategy keeps drilling down and returns the innermost node at the
    // position: the call callee identifier.
    auto *innermost = ark::es2panda::lsp::GetTouchingToken(context, callPos, false);
    ASSERT_EQ(innermost->DumpJSON(), expected->DumpJSON());
    ASSERT_EQ(innermost->Start().index, callPos);
    ASSERT_EQ(innermost->End().index, callPos + std::string("outer").size());

    // flagFindFirstMatch returns the outermost statement covering the position:
    // the whole expression statement 'outer();'.
    auto *firstMatch = ark::es2panda::lsp::GetTouchingToken(context, callPos, true);
    ASSERT_EQ(firstMatch->Type(), ark::es2panda::ir::AstNodeType::EXPRESSION_STATEMENT);
    ASSERT_EQ(firstMatch->Start().index, callPos);
    ASSERT_EQ(firstMatch->End().index, source.rfind('\n'));

    initializer.DestroyContext(context);
}

TEST_F(NodeTokenOffsetBoundaryTests, TouchingTokenForLambdaObjectAndArrayExpressions)
{
    const std::string source = R"(let fn = (x: number): number => x + 1;
let obj = { foo: 1, bar: 2 };
let arr = [1, 2, 3];
)";
    Initializer initializer = Initializer();
    es2panda_Context *context =
        initializer.CreateContext("boundary_lambda_object_array.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_EQ(ContextState(context), ES2PANDA_STATE_PARSED);

    auto *ast = GetAstFromContext<AstNode>(context);
    auto *arrow = ast->FindChild([](AstNode *node) { return node->IsArrowFunctionExpression(); });
    auto *object = ast->FindChild([](AstNode *node) { return node->IsObjectExpression(); });
    auto *array = ast->FindChild([](AstNode *node) { return node->IsArrayExpression(); });
    ASSERT_NE(arrow, nullptr);
    ASSERT_NE(object, nullptr);
    ASSERT_NE(array, nullptr);

    // Punctuation that belongs to the expression itself, rather than to a child
    // identifier/literal, must resolve to the enclosing expression node.
    const auto arrowPos = source.find("=>");
    const auto objectPos = source.find("{ foo");
    const auto arrayPos = source.find("[1, 2, 3]");
    ASSERT_NE(arrowPos, std::string::npos);
    ASSERT_NE(objectPos, std::string::npos);
    ASSERT_NE(arrayPos, std::string::npos);

    auto *touchingArrow = ark::es2panda::lsp::GetTouchingToken(context, arrowPos, false);
    auto *touchingObject = ark::es2panda::lsp::GetTouchingToken(context, objectPos, false);
    auto *touchingArray = ark::es2panda::lsp::GetTouchingToken(context, arrayPos, false);
    ASSERT_NE(touchingArrow, nullptr);
    ASSERT_NE(touchingObject, nullptr);
    ASSERT_NE(touchingArray, nullptr);
    EXPECT_TRUE(touchingArrow->IsScriptFunction());
    ASSERT_NE(touchingArrow->Parent(), nullptr);
    EXPECT_EQ(touchingArrow->Parent()->DumpJSON(), arrow->DumpJSON());
    EXPECT_EQ(touchingObject->DumpJSON(), object->DumpJSON());
    EXPECT_EQ(touchingArray->DumpJSON(), array->DumpJSON());

    initializer.DestroyContext(context);
}

TEST_F(NodeTokenOffsetBoundaryTests, PrecedingTokenAtFileStartAndEOF)
{
    const std::string source = "let x = 10;\nlet y = 200;";
    Initializer initializer = Initializer();
    es2panda_Context *context =
        initializer.CreateContext("boundary_preceding_start_eof.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_EQ(ContextState(context), ES2PANDA_STATE_PARSED);

    LSPAPI const *lspApi = GetImpl();
    auto ast = GetAstFromContext<AstNode>(context);

    // No token precedes the start of file.
    ASSERT_EQ(lspApi->getPrecedingToken(context, 0), nullptr);

    // At the end of the last statement (no trailing newline) the preceding
    // token is the last number literal '200'.
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t lastNumberWithSemicolonLen = 4;  // length of the trailing "200;"
    auto numberLiteral = ast->FindChild(
        [](AstNode *node) { return node->IsNumberLiteral() && node->AsNumberLiteral()->Str() == "200"; });
    ASSERT_NE(numberLiteral, nullptr);
    auto *atEnd = reinterpret_cast<AstNode *>(lspApi->getPrecedingToken(context, source.size() - 1));
    ASSERT_EQ(atEnd->DumpJSON(), numberLiteral->DumpJSON());
    ASSERT_EQ(atEnd->Start().index, source.size() - lastNumberWithSemicolonLen);
    ASSERT_EQ(atEnd->End().index, source.size() - 1);

    // Positions at or past EOF are out of range: the traversal is pinned to
    // the first statement and returns its last token '10'.
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t firstNumberLen = 2;  // length of the "10" literal
    for (const size_t pos : {source.size(), source.size() + 1}) {
        auto *atEof = reinterpret_cast<AstNode *>(lspApi->getPrecedingToken(context, pos));
        ASSERT_EQ(atEof->Type(), ark::es2panda::ir::AstNodeType::NUMBER_LITERAL);
        ASSERT_EQ(atEof->Start().index, source.find("10;"));
        ASSERT_EQ(atEof->End().index, source.find("10;") + firstNumberLen);
    }

    initializer.DestroyContext(context);
}

TEST_F(NodeTokenOffsetBoundaryTests, PrecedingTokenInsideComments)
{
    const std::string source = "let a = 1;\n// line comment tail\n/* block comment */\nlet b = 2;\n";
    Initializer initializer = Initializer();
    es2panda_Context *context =
        initializer.CreateContext("boundary_preceding_comments.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_EQ(ContextState(context), ES2PANDA_STATE_PARSED);

    LSPAPI const *lspApi = GetImpl();
    auto ast = GetAstFromContext<AstNode>(context);
    auto numberOne =
        ast->FindChild([](AstNode *node) { return node->IsNumberLiteral() && node->AsNumberLiteral()->Str() == "1"; });
    auto numberTwo =
        ast->FindChild([](AstNode *node) { return node->IsNumberLiteral() && node->AsNumberLiteral()->Str() == "2"; });
    ASSERT_NE(numberOne, nullptr);
    ASSERT_NE(numberTwo, nullptr);

    // Inside a line comment: the preceding token is the token before the comment.
    const size_t inLineComment = source.find("tail");  // position of 't' inside the line comment
    auto *result = reinterpret_cast<AstNode *>(lspApi->getPrecedingToken(context, inLineComment));
    ASSERT_EQ(result->Type(), numberOne->Type());
    ASSERT_EQ(result->Start().index, numberOne->Start().index);
    ASSERT_EQ(result->End().index, numberOne->End().index);
    ASSERT_EQ(result->Start().index, source.find("1;"));
    ASSERT_EQ(result->End().index, source.find("1;") + 1);

    // Inside a block comment: the preceding token is still '1'.
    const size_t inBlockComment = source.find("block comment") + 2;  // inside the block comment
    result = reinterpret_cast<AstNode *>(lspApi->getPrecedingToken(context, inBlockComment));
    ASSERT_EQ(result->Type(), ark::es2panda::ir::AstNodeType::NUMBER_LITERAL);
    ASSERT_EQ(result->Start().index, source.find("1;"));
    ASSERT_EQ(result->End().index, source.find("1;") + 1);

    // Exactly on the 'l' of 'let b' the implementation resolves to the
    // synthetic identifier of the declarator (range [0,0]) and returns
    // nullptr instead of falling back to the previous token.
    const size_t beforeLetB = source.find("let b");  // position of 'l' in 'let b'
    ASSERT_EQ(lspApi->getPrecedingToken(context, beforeLetB), nullptr);

    // After '2' the preceding token is the number literal '2'.
    const size_t afterTwo = source.find("2;") + 1;  // position right after '2'
    result = reinterpret_cast<AstNode *>(lspApi->getPrecedingToken(context, afterTwo));
    ASSERT_EQ(result->DumpJSON(), numberTwo->DumpJSON());
    ASSERT_EQ(result->Start().index, source.find("2;"));
    ASSERT_EQ(result->End().index, source.find("2;") + 1);

    initializer.DestroyContext(context);
}

TEST_F(NodeTokenOffsetBoundaryTests, TokensInStringAndTemplateLiteral)
{
    const std::string source = "let s = \"hello world\";\nlet t = `abc ${s} def`;\n";
    Initializer initializer = Initializer();
    es2panda_Context *context =
        initializer.CreateContext("boundary_string_template.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_EQ(ContextState(context), ES2PANDA_STATE_PARSED);

    LSPAPI const *lspApi = GetImpl();
    auto ast = GetAstFromContext<AstNode>(context);

    // Inside a string literal the touching token is the whole string literal.
    auto stringLiteral = ast->FindChild(
        [](AstNode *node) { return node->IsStringLiteral() && node->AsStringLiteral()->ToString() == "hello world"; });
    ASSERT_NE(stringLiteral, nullptr);
    const size_t inString = source.find("world");  // position of 'w' inside the string literal
    auto *touching = ark::es2panda::lsp::GetTouchingToken(context, inString, false);
    ASSERT_EQ(touching->DumpJSON(), stringLiteral->DumpJSON());
    ASSERT_EQ(touching->Start().index, source.find("\"hello"));
    ASSERT_EQ(touching->End().index, source.find("\"hello") + std::string("\"hello world\"").size());

    // Inside a template literal the touching token is the template element.
    const size_t inTemplate = source.find("abc") + 1;  // inside the template head 'abc '
    touching = ark::es2panda::lsp::GetTouchingToken(context, inTemplate, false);
    ASSERT_EQ(touching->Type(), ark::es2panda::ir::AstNodeType::TEMPLATE_ELEMENT);
    ASSERT_EQ(touching->Start().index, source.find('`') + 1);
    ASSERT_EQ(touching->End().index, source.find("${"));

    // Inside the template substitution the touching token is the identifier 's'.
    const size_t inSubstitution = source.find("${s}") + 2;  // position of 's' inside '${s}'
    touching = ark::es2panda::lsp::GetTouchingToken(context, inSubstitution, false);
    ASSERT_EQ(touching->Type(), ark::es2panda::ir::AstNodeType::IDENTIFIER);
    ASSERT_EQ(touching->Start().index, inSubstitution);
    ASSERT_EQ(touching->End().index, inSubstitution + 1);

    // Preceding token inside a string literal is the string literal itself.
    auto *preceding = reinterpret_cast<AstNode *>(lspApi->getPrecedingToken(context, inString));
    ASSERT_EQ(preceding->DumpJSON(), stringLiteral->DumpJSON());

    initializer.DestroyContext(context);
}

TEST_F(NodeTokenOffsetBoundaryTests, AroundOperatorAndPunctuation)
{
    const std::string source = "let r = 1 + 2;\nlet v = r;\n";
    Initializer initializer = Initializer();
    es2panda_Context *context =
        initializer.CreateContext("boundary_operator_punct.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_EQ(ContextState(context), ES2PANDA_STATE_PARSED);

    LSPAPI const *lspApi = GetImpl();
    auto ast = GetAstFromContext<AstNode>(context);
    auto numberOne =
        ast->FindChild([](AstNode *node) { return node->IsNumberLiteral() && node->AsNumberLiteral()->Str() == "1"; });
    auto numberTwo =
        ast->FindChild([](AstNode *node) { return node->IsNumberLiteral() && node->AsNumberLiteral()->Str() == "2"; });
    ASSERT_NE(numberOne, nullptr);
    ASSERT_NE(numberTwo, nullptr);

    // Exactly on the '+' operator the touching token is the binary expression.
    const size_t plusPos = source.find('+');
    auto *touching = ark::es2panda::lsp::GetTouchingToken(context, plusPos, false);
    ASSERT_EQ(touching->Type(), ark::es2panda::ir::AstNodeType::BINARY_EXPRESSION);
    ASSERT_EQ(touching->Start().index, source.find("1 +"));
    ASSERT_EQ(touching->End().index, source.find("2;") + 1);

    // On the '=' punctuation the touching token is the variable declarator.
    const size_t equalPos = source.find('=');
    touching = ark::es2panda::lsp::GetTouchingToken(context, equalPos, false);
    ASSERT_EQ(touching->Type(), ark::es2panda::ir::AstNodeType::VARIABLE_DECLARATOR);
    ASSERT_EQ(touching->Start().index, source.find("r ="));
    ASSERT_EQ(touching->End().index, source.find(';'));

    // Preceding token of the '+' operator is the left operand '1'.
    auto *preceding = reinterpret_cast<AstNode *>(lspApi->getPrecedingToken(context, plusPos));
    ASSERT_EQ(preceding->DumpJSON(), numberOne->DumpJSON());
    ASSERT_EQ(preceding->Start().index, source.find("1 +"));
    ASSERT_EQ(preceding->End().index, source.find("1 +") + 1);

    // Preceding token on the space after the '+' operator is still the left
    // operand '1' (whitespace falls back to the token before the operator).
    const size_t afterPlus = plusPos + 1;  // position right after '+'
    preceding = reinterpret_cast<AstNode *>(lspApi->getPrecedingToken(context, afterPlus));
    ASSERT_EQ(preceding->DumpJSON(), numberOne->DumpJSON());
    ASSERT_EQ(preceding->Start().index, source.find("1 +"));
    ASSERT_EQ(preceding->End().index, source.find("1 +") + 1);

    // Preceding token exactly on the right operand '2' is '2' itself, because
    // the start boundary of a token hits that token.
    const size_t onTwo = source.find("2;");  // position of '2'
    preceding = reinterpret_cast<AstNode *>(lspApi->getPrecedingToken(context, onTwo));
    ASSERT_EQ(preceding->DumpJSON(), numberTwo->DumpJSON());
    ASSERT_EQ(preceding->Start().index, onTwo);
    ASSERT_EQ(preceding->End().index, onTwo + 1);

    initializer.DestroyContext(context);
}

TEST_F(NodeTokenOffsetBoundaryTests, UnicodeIdentifierTokens)
{
    const std::string source = "let \xE4\xB8\xAD\xE6\x96\x87\xE5\x8F\x98\xE9\x87\x8F = 1;\n";  // Chinese identifier
    Initializer initializer = Initializer();
    es2panda_Context *context =
        initializer.CreateContext("boundary_unicode_identifier.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_EQ(ContextState(context), ES2PANDA_STATE_PARSED);

    LSPAPI const *lspApi = GetImpl();
    auto ast = GetAstFromContext<AstNode>(context);
    auto unicodeId = ast->FindChild([](AstNode *node) { return node->IsIdentifier(); });
    ASSERT_NE(unicodeId, nullptr);

    // The Chinese identifier occupies 12 bytes (4 characters x 3 bytes).
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t unicodeIdByteLen = 12;
    const size_t idStart = source.find("\xE4\xB8\xAD");  // byte offset of the first Chinese character
    ASSERT_EQ(unicodeId->Start().index, idStart);
    ASSERT_EQ(unicodeId->End().index, idStart + unicodeIdByteLen);

    // Touching token in the middle of the multi-byte identifier (byte offsets).
    auto *touching = ark::es2panda::lsp::GetTouchingToken(context, idStart + 1, false);
    ASSERT_EQ(touching->DumpJSON(), unicodeId->DumpJSON());
    touching = ark::es2panda::lsp::GetTouchingToken(context, idStart + unicodeIdByteLen - 1, false);
    ASSERT_EQ(touching->DumpJSON(), unicodeId->DumpJSON());

    // Preceding token on the last byte of the identifier is the identifier.
    auto *preceding = reinterpret_cast<AstNode *>(lspApi->getPrecedingToken(context, idStart + unicodeIdByteLen - 1));
    ASSERT_EQ(preceding->DumpJSON(), unicodeId->DumpJSON());

    // Current token value takes code-point offsets: offset 8 (after 'let ' and
    // the whole 4-character identifier) returns the identifier text.
    const size_t codePointAfterId = 8;  // 'let ' (4) + 4 Chinese characters
    std::string tokenValue = lspApi->getCurrentTokenValue(context, codePointAfterId);
    ASSERT_EQ(tokenValue, source.substr(idStart, unicodeIdByteLen));

    initializer.DestroyContext(context);
}

TEST_F(NodeTokenOffsetBoundaryTests, CRLFLineEndingOffsets)
{
    Initializer initializer = Initializer();
    es2panda_Context *context =
        initializer.CreateContext("boundary_crlf.ets", ES2PANDA_STATE_PARSED, "let a = 1;\r\nlet b = 2;\r\n");
    ASSERT_EQ(ContextState(context), ES2PANDA_STATE_PARSED);

    auto ast = GetAstFromContext<AstNode>(context);
    auto numberOne =
        ast->FindChild([](AstNode *node) { return node->IsNumberLiteral() && node->AsNumberLiteral()->Str() == "1"; });
    ASSERT_NE(numberOne, nullptr);

    // Token search works across CRLF boundaries; '1' right before the first
    // CRLF is still found.
    const size_t onePos = 8;  // byte position of '1' right before the first CRLF
    auto *touching = ark::es2panda::lsp::GetTouchingToken(context, onePos, false);
    ASSERT_EQ(touching->DumpJSON(), numberOne->DumpJSON());
    ASSERT_EQ(touching->Start().index, onePos);
    ASSERT_EQ(touching->End().index, onePos + 1);

    // On the '\r' of the first CRLF the touching token is nullptr: the
    // declaration range excludes the line break and the second statement does
    // not cover it either.
    const size_t crPos = 11;  // byte position of '\r'
    ASSERT_EQ(ark::es2panda::lsp::GetTouchingToken(context, crPos, false), nullptr);

    initializer.DestroyContext(context);

    // Offset conversion APIs treat "\r\n" as a single line break.
    LSPAPI const *lspApi = GetImpl();
    const std::string crlf = "a\r\nb\r\nc";
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t thirdLine = 3;
    auto loc = lspApi->getColAndLineByOffset(crlf, 2);  // '\n' of the first CRLF
    ASSERT_EQ(loc.first, 1U);
    ASSERT_EQ(loc.second, 2U);
    ASSERT_EQ(lspApi->getOffsetByColAndLine(crlf, thirdLine, 1), 6U);  // 'c' on the third line
}

TEST_F(NodeTokenOffsetBoundaryTests, EmptyFileBehavior)
{
    Initializer initializer = Initializer();
    es2panda_Context *context = initializer.CreateContext("boundary_empty.ets", ES2PANDA_STATE_PARSED, "");
    ASSERT_EQ(ContextState(context), ES2PANDA_STATE_PARSED);

    LSPAPI const *lspApi = GetImpl();
    ASSERT_EQ(ark::es2panda::lsp::GetTouchingToken(context, 0, false), nullptr);
    ASSERT_EQ(lspApi->getPrecedingToken(context, 0), nullptr);
    ASSERT_EQ(lspApi->getCurrentTokenValue(context, 0), "");
    auto tokenTypes = lspApi->getTokenTypes(context, 0);
    ASSERT_EQ(tokenTypes.name, "");
    ASSERT_EQ(tokenTypes.type, "");

    initializer.DestroyContext(context);
}

TEST_F(NodeTokenOffsetBoundaryTests, LastLineWithoutNewline)
{
    const std::string source = "let a = 1;\nlet b = 2;";  // no trailing newline
    Initializer initializer = Initializer();
    es2panda_Context *context =
        initializer.CreateContext("boundary_no_trailing_newline.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_EQ(ContextState(context), ES2PANDA_STATE_PARSED);

    auto ast = GetAstFromContext<AstNode>(context);
    auto numberTwo =
        ast->FindChild([](AstNode *node) { return node->IsNumberLiteral() && node->AsNumberLiteral()->Str() == "2"; });
    ASSERT_NE(numberTwo, nullptr);

    // The last token '2' on the unterminated last line is found at EOF - 1.
    const size_t twoPos = source.size() - 2;  // position of '2'
    auto *touching = ark::es2panda::lsp::GetTouchingToken(context, twoPos, false);
    ASSERT_EQ(touching->DumpJSON(), numberTwo->DumpJSON());
    ASSERT_EQ(touching->Start().index, twoPos);
    ASSERT_EQ(touching->End().index, twoPos + 1);

    initializer.DestroyContext(context);

    // The last line still resolves to line 2 through the offset conversion APIs.
    LSPAPI const *lspApi = GetImpl();
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t lastLineNumber = 2;
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t colNine = 9;  // column of the trailing '2' on the last line
    auto loc = lspApi->getColAndLineByOffset(source, twoPos);
    ASSERT_EQ(loc.first, lastLineNumber);
    ASSERT_EQ(loc.second, colNine);
    ASSERT_EQ(lspApi->getOffsetByColAndLine(source, lastLineNumber, colNine), twoPos);
}

TEST_F(NodeTokenOffsetBoundaryTests, OutOfRangeLineColumnOffsetBehavior)
{
    LSPAPI const *lspApi = GetImpl();
    const std::string source = "ab\ncd\nef";
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t secondLine = 2;
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t lineBeyondEof = 10;

    // A column beyond the end of the line is clamped to the line content end.
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t columnBeyondLineEnd = 100;
    ASSERT_EQ(lspApi->getOffsetByColAndLine(source, secondLine, columnBeyondLineEnd), 5U);  // right after 'd'
    // A line beyond the last line yields offset 0.
    ASSERT_EQ(lspApi->getOffsetByColAndLine(source, lineBeyondEof, 1), 0U);
    // An offset past EOF stays on the last line (the line index does not grow
    // past the final unterminated line).
    auto loc = lspApi->getColAndLineByOffset(source, source.size());
    ASSERT_EQ(loc.first, 3U);
    ASSERT_EQ(loc.second, 3U);
    // Offset 0 is the first line and the first column.
    loc = lspApi->getColAndLineByOffset(source, 0);
    ASSERT_EQ(loc.first, 1U);
    ASSERT_EQ(loc.second, 1U);
}

TEST_F(NodeTokenOffsetBoundaryTests, TokenTypesAtIdentifierAndNonIdentifier)
{
    const std::string source = "class Calc {\n  native add(a: int, b: int): int;\n}\nlet c = new Calc();\n";
    Initializer initializer = Initializer();
    es2panda_Context *context =
        initializer.CreateContext("boundary_token_types.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_EQ(ContextState(context), ES2PANDA_STATE_CHECKED);

    LSPAPI const *lspApi = GetImpl();

    // The class declaration range covers its whole body, so GetTouchingToken
    // resolves to the identifier only near the end of the 'Calc' token (the
    // last 'identifier length' bytes of the token; 'c' is at offset 6).
    const size_t inCalc = 6;  // code-point offset of 'c' inside 'Calc'
    auto tokenTypes = lspApi->getTokenTypes(context, inCalc);
    ASSERT_EQ(tokenTypes.name, "Calc");
    // The class declaration itself carries no 'native' modifier, so the type
    // list is empty.
    ASSERT_EQ(tokenTypes.type, "");

    // On a non-identifier position both fields are empty.
    const size_t bracePos = source.find('{');  // code-point offset of '{'
    tokenTypes = lspApi->getTokenTypes(context, bracePos);
    ASSERT_EQ(tokenTypes.name, "");
    ASSERT_EQ(tokenTypes.type, "");

    initializer.DestroyContext(context);
}

}  // namespace
