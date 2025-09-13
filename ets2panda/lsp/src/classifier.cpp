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

#include "classifier.h"
#include <cstddef>
#include <vector>
#include "checker/ETSchecker.h"
#include "checker/checker.h"
#include "compiler/lowering/util.h"
#include "generated/tokenType.h"
#include "internal_api.h"
#include "ir/astNode.h"
#include "lexer/lexer.h"
#include "libarkbase/macros.h"
#include "libarkbase/mem/arena_allocator.h"
#include "public/es2panda_lib.h"
#include "public/public.h"
#include "libarkbase/utils/arena_containers.h"
#include "varbinder/declaration.h"

namespace ark::es2panda::lsp {

std::unique_ptr<lexer::Lexer> InitLexer(es2panda_Context *context)
{
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    auto parserContext = parser::ParserContext(ctx->parserProgram, parser::ParserStatus::NO_OPTS);
    std::unique_ptr<lexer::Lexer> lexer = std::make_unique<lexer::Lexer>(&parserContext, *ctx->diagnosticEngine);
    return lexer;
}

ClassificationType AstNodeTypeToClassificationType(ir::AstNodeType type)
{
    switch (type) {
        case ir::AstNodeType::CLASS_DEFINITION:
            return ClassificationType::CLASS_NAME;
        case ir::AstNodeType::TS_TYPE_PARAMETER:
            return ClassificationType::TYPE_PARAMETER_NAME;
        case ir::AstNodeType::TS_INTERFACE_DECLARATION:
            return ClassificationType::INTERFACE_NAME;
        case ir::AstNodeType::TS_ENUM_DECLARATION:
            return ClassificationType::ENUM_NAME;
        case ir::AstNodeType::ETS_PARAMETER_EXPRESSION:
            return ClassificationType::PARAMETER_NAME;
        case ir::AstNodeType::TS_TYPE_ALIAS_DECLARATION:
            return ClassificationType::TYPE_ALIAS_NAME;
        default:
            return ClassificationType::IDENTIFIER;
    }
}

ClassificationType GetClassificationType(const lexer::Token &token, ir::AstNode *astNode)
{
    if (token.IsPunctuatorToken(token.Type())) {
        return ClassificationType::PUNCTUATION;
    }

    if (token.Type() == lexer::TokenType::LITERAL_NUMBER) {
        return ClassificationType::NUMERIC_LITERAL;
    }

    if (token.Type() == lexer::TokenType::LITERAL_STRING || token.Type() == lexer::TokenType::LITERAL_REGEXP) {
        return ClassificationType::STRING_LITERAL;
    }

    if (token.Type() == lexer::TokenType::LITERAL_TRUE || token.Type() == lexer::TokenType::LITERAL_FALSE) {
        return ClassificationType::BOOLEAN_LITERAL;
    }

    if (token.Type() == lexer::TokenType::LITERAL_NULL) {
        return ClassificationType::NULL_LITERAL;
    }

    // don't use token.IsKeyword() here because token.type_ is LITERAL_IDENT for "keyword_like" keywords
    if (token.KeywordType() >= lexer::TokenType::FIRST_KEYW) {
        return ClassificationType::KEYWORD;
    }

    if (token.Type() == lexer::TokenType::LITERAL_IDENT) {
        if (astNode == nullptr) {
            return ClassificationType::IDENTIFIER;
        }
        auto parentNode = astNode->Parent();
        return AstNodeTypeToClassificationType(parentNode->Type());
    }

    UNREACHABLE();
}

char const *ClassificationTypeToString(ClassificationType type)
{
    switch (type) {
        case ClassificationType::IDENTIFIER:
            return "identifier";
        case ClassificationType::KEYWORD:
            return "keyword";
        case ClassificationType::NUMERIC_LITERAL:
            return "number";
        case ClassificationType::STRING_LITERAL:
            return "string";
        case ClassificationType::BOOLEAN_LITERAL:
            return "boolean";
        case ClassificationType::NULL_LITERAL:
            return "null";
        case ClassificationType::PUNCTUATION:
            return "punctuation";
        case ClassificationType::CLASS_NAME:
            return "class name";
        case ClassificationType::ENUM_NAME:
            return "enum name";
        case ClassificationType::INTERFACE_NAME:
            return "interface name";
        case ClassificationType::TYPE_PARAMETER_NAME:
            return "type parameter name";
        case ClassificationType::TYPE_ALIAS_NAME:
            return "type alias name";
        case ClassificationType::PARAMETER_NAME:
            return "parameter name";
        default:
            return "";
    }
}

ArenaVector<ClassifiedSpan *> GetSyntacticClassifications(es2panda_Context *context, size_t startPos, size_t length)
{
    auto lexer = InitLexer(context);
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    auto result = ArenaVector<ClassifiedSpan *>(ctx->allocator->Adapter());
    lexer->NextToken();
    while (lexer->GetToken().Type() != lexer::TokenType::EOS) {
        ir::AstNode *currentNode = nullptr;
        auto currentToken = lexer->GetToken();
        if (currentToken.Type() == lexer::TokenType::LITERAL_IDENT) {
            currentNode = GetTouchingToken(context, currentToken.Start().index, false);
        }

        if (currentToken.End().index > startPos + length) {
            break;
        }

        if (currentToken.Start().index >= startPos && currentToken.End().index <= startPos + length) {
            size_t tokenStart = currentToken.Start().index;
            size_t tokenLength = currentToken.End().index - currentToken.Start().index;
            auto classificationType = GetClassificationType(currentToken, currentNode);
            auto name = ClassificationTypeToString(classificationType);
            auto classifiedSpan = ctx->allocator->New<ClassifiedSpan>();
            classifiedSpan->start = tokenStart;
            classifiedSpan->length = tokenLength;
            classifiedSpan->name = name;
            result.push_back(classifiedSpan);
        }
        lexer->NextToken();
    }
    return result;
}

std::unordered_map<std::string, ir::AstNode *> GetDecls(ir::AstNode *astNode)
{
    auto decls = astNode->Scope()->Decls();
    std::unordered_map<std::string, ir::AstNode *> declNames = {};
    for (auto decl : decls) {
        auto name = std::string(decl->Name());
        auto node = decl->Node();
        // After enum refactoring, enum declaration is transformed to a class declaration
        if (node != nullptr && compiler::ClassDefinitionIsEnumTransformed(node)) {
            node = node->AsClassDefinition()->OrigEnumDecl()->AsTSEnumDeclaration();
        }
        declNames[name] = node;
    }
    return declNames;
}

ArenaVector<ClassifiedSpan *> GetSemanticClassifications(es2panda_Context *context, size_t startPos, size_t length)
{
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    auto ast = reinterpret_cast<ir::AstNode *>(ctx->parserProgram->Ast());
    auto result = ArenaVector<ClassifiedSpan *>(ctx->allocator->Adapter());

    auto decls = GetDecls(ast);
    auto lexer = InitLexer(context);
    lexer->NextToken();
    while (lexer->GetToken().Type() != lexer::TokenType::EOS) {
        ir::AstNode *currentNode = nullptr;
        auto currentToken = lexer->GetToken();
        if (currentToken.Type() == lexer::TokenType::LITERAL_IDENT) {
            currentNode = GetTouchingToken(context, currentToken.Start().index, false);
        }

        if (currentToken.End().index > startPos + length) {
            break;
        }

        if (currentToken.Start().index >= startPos && currentToken.End().index <= startPos + length) {
            size_t tokenStart = currentToken.Start().index;
            size_t tokenLength = currentToken.End().index - currentToken.Start().index;
            auto classificationType = GetClassificationType(currentToken, currentNode);

            auto tokenName = std::string(currentToken.Ident());
            if (classificationType == ClassificationType::IDENTIFIER && decls.count(tokenName) != 0) {
                auto classifiedSpan = ctx->allocator->New<ClassifiedSpan>();
                classifiedSpan->start = tokenStart;
                classifiedSpan->length = tokenLength;
                auto type = AstNodeTypeToClassificationType(decls[tokenName]->Type());
                classifiedSpan->name = ClassificationTypeToString(type);
                result.push_back(classifiedSpan);
            }
        }
        lexer->NextToken();
    }
    return result;
}

}  // namespace ark::es2panda::lsp