/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

#include "ETSparser.h"

#include "lexer/ETSLexer.h"
#include "lexer/lexer.h"
#include "ir/typeNode.h"
#include "ir/expressions/identifier.h"
#include "ir/expressions/blockExpression.h"
#include "ir/expressions/sequenceExpression.h"
#include "ir/statements/blockStatement.h"
#include "ir/statements/expressionStatement.h"
#include "ir/base/methodDefinition.h"
#include "ir/ts/tsInterfaceBody.h"
#include "parser/parserStatusContext.h"
#include "generated/diagnostic.h"

namespace ark::es2panda::parser {
//================================================================================================//
//  Methods to create AST node(s) from the specified string (part of valid ETS-code!)
//================================================================================================//

// NOLINTBEGIN(modernize-avoid-c-arrays)
inline constexpr char const FORMAT_SIGNATURE = '@';
inline constexpr char const TYPE_FORMAT_NODE = 'T';
inline constexpr char const GENERAL_FORMAT_NODE = 'N';
inline constexpr char const IDENTIFIER_FORMAT_NODE = 'I';

// NOLINTEND(modernize-avoid-c-arrays)

ParserImpl::NodeFormatType ETSParser::GetFormatPlaceholderType()
{
    ES2PANDA_ASSERT(Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_FORMAT);
    Lexer()->NextToken();

    bool isArray = false;
    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LEFT_SQUARE_BRACKET) {
        isArray = true;
        Lexer()->NextToken();
    }

    ES2PANDA_ASSERT(Lexer()->GetToken().Type() == lexer::TokenType::LITERAL_IDENT);
    char const *const identData = Lexer()->GetToken().Ident().Bytes();

    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic, cert-err34-c)
    auto identNumber = std::atoi(identData + 1U);
    if (identNumber <= 0) {
        LogError(diagnostic::INVALID_NODE_NUMBER, {}, Lexer()->GetToken().Start());
        ES2PANDA_UNREACHABLE();
    }

    return {isArray, *identData,
            static_cast<decltype(std::get<2>(std::declval<ParserImpl::NodeFormatType>()))>(identNumber - 1)};
}

ir::Expression *ETSParser::ParseExpressionFormatPlaceholder()
{
    if (insertingNodes_.empty()) {
        LogUnexpectedToken(lexer::TokenType::PUNCTUATOR_FORMAT);
        const auto &rangeToken = Lexer()->GetToken().Loc();
        Lexer()->NextToken();
        return AllocBrokenExpression(rangeToken);
    }

    ParserImpl::NodeFormatType nodeFormat = GetFormatPlaceholderType();
    if (std::get<0>(nodeFormat)) {
        LogError(diagnostic::INVALID_NODE_TYPE, {}, Lexer()->GetToken().Start());
        ES2PANDA_UNREACHABLE();
    }

    if (auto const placeholderType = std::get<1>(nodeFormat); placeholderType == TYPE_FORMAT_NODE) {
        return ParseTypeFormatPlaceholder(std::make_optional(std::move(nodeFormat)));
    } else if (placeholderType == IDENTIFIER_FORMAT_NODE) {  // NOLINT(readability-else-after-return)
        return ParseIdentifierFormatPlaceholder(std::make_optional(std::move(nodeFormat)));
    } else if (placeholderType != EXPRESSION_FORMAT_NODE) {  // NOLINT(readability-else-after-return)
        LogError(diagnostic::INVALID_NODE_TYPE, {}, Lexer()->GetToken().Start());
        ES2PANDA_UNREACHABLE();
    }

    auto const placeholderNumber = std::get<2>(nodeFormat);
    auto *const insertingNode =
        placeholderNumber < insertingNodes_.size() ? insertingNodes_[placeholderNumber] : nullptr;
    if (insertingNode == nullptr || !insertingNode->IsExpression()) {
        LogError(diagnostic::INVALID_INSERT_NODE, {}, Lexer()->GetToken().Start());
        ES2PANDA_UNREACHABLE();
    }

    auto *const insertExpression = insertingNode->AsExpression();
    Lexer()->NextToken();
    return insertExpression;
}

ir::TypeNode *ETSParser::ParseTypeFormatPlaceholder(std::optional<ParserImpl::NodeFormatType> nodeFormat)
{
    if (!nodeFormat.has_value()) {
        if (insertingNodes_.empty()) {
            LogUnexpectedToken(lexer::TokenType::PUNCTUATOR_FORMAT);
            return AllocBrokenType(Lexer()->GetToken().Loc());
        }

        nodeFormat = GetFormatPlaceholderType();
        if (std::get<0>(*nodeFormat) || std::get<1>(*nodeFormat) != TYPE_FORMAT_NODE) {
            return nullptr;
        }
    }

    auto const placeholderNumber = std::get<2>(*nodeFormat);
    auto *const insertingNode =
        placeholderNumber < insertingNodes_.size() ? insertingNodes_[placeholderNumber] : nullptr;
    if (insertingNode == nullptr || !insertingNode->IsExpression() || !insertingNode->AsExpression()->IsTypeNode()) {
        LogError(diagnostic::INVALID_INSERT_NODE, {}, Lexer()->GetToken().Start());
        ES2PANDA_UNREACHABLE();
    }

    auto *const insertType = insertingNode->AsExpression()->AsTypeNode();
    Lexer()->NextToken();
    return insertType;
}

ir::Identifier *ETSParser::ParseIdentifierFormatPlaceholder(std::optional<ParserImpl::NodeFormatType> nodeFormat)
{
    if (!nodeFormat.has_value()) {
        if (insertingNodes_.empty()) {
            LogError(diagnostic::INSERT_NODE_ABSENT, {}, Lexer()->GetToken().Start());
            return nullptr;
        }

        nodeFormat = GetFormatPlaceholderType();
        if (std::get<0>(*nodeFormat) || std::get<1>(*nodeFormat) != IDENTIFIER_FORMAT_NODE) {
            LogError(diagnostic::INVALID_NODE_TYPE, {}, Lexer()->GetToken().Start());
            return nullptr;
        }
    }

    auto const placeholderNumber = std::get<2>(*nodeFormat);
    auto *const insertingNode =
        placeholderNumber < insertingNodes_.size() ? insertingNodes_[placeholderNumber] : nullptr;
    if (insertingNode == nullptr || !insertingNode->IsExpression() || !insertingNode->AsExpression()->IsIdentifier()) {
        LogError(diagnostic::INVALID_INSERT_NODE, {}, Lexer()->GetToken().Start());
        ES2PANDA_UNREACHABLE();
    }

    auto *const insertIdentifier = insertingNode->AsExpression()->AsIdentifier();
    Lexer()->NextToken();
    return insertIdentifier;
}

ir::Statement *ETSParser::ParseStatementFormatPlaceholder()
{
    if (insertingNodes_.empty()) {
        LogError(diagnostic::INSERT_NODE_ABSENT, {}, Lexer()->GetToken().Start());
        return AllocBrokenStatement(Lexer()->GetToken().Start());
    }

    ParserImpl::NodeFormatType nodeFormat = GetFormatPlaceholderType();
    if (std::get<0>(nodeFormat) || std::get<1>(nodeFormat) != STATEMENT_FORMAT_NODE) {
        LogError(diagnostic::INVALID_NODE_TYPE, {}, Lexer()->GetToken().Start());
        ES2PANDA_UNREACHABLE();
    }

    auto const placeholderNumber = std::get<2>(nodeFormat);
    auto *const insertingNode =
        placeholderNumber < insertingNodes_.size() ? insertingNodes_[placeholderNumber] : nullptr;
    if (insertingNode == nullptr || !insertingNode->IsStatement()) {
        LogError(diagnostic::INVALID_INSERT_NODE, {}, Lexer()->GetToken().Start());
        ES2PANDA_UNREACHABLE();
    }

    Lexer()->NextToken();
    return insertingNode->AsStatement();
}

ir::AstNode *ETSParser::ParseTypeParametersFormatPlaceholder()
{
    ParserImpl::NodeFormatType nodeFormat = GetFormatPlaceholderType();
    if (std::get<0>(nodeFormat) || std::get<1>(nodeFormat) != EXPRESSION_FORMAT_NODE) {
        LogError(diagnostic::INVALID_NODE_TYPE, {}, Lexer()->GetToken().Start());
        ES2PANDA_UNREACHABLE();
    }

    auto const placeholderNumber = std::get<2>(nodeFormat);
    if (placeholderNumber >= insertingNodes_.size()) {
        LogError(diagnostic::INSERT_NODE_ABSENT, {}, Lexer()->GetToken().Start());
        ES2PANDA_UNREACHABLE();
    }

    auto *const insertingNode = insertingNodes_[placeholderNumber];
    if (insertingNode != nullptr && !insertingNode->IsTSTypeParameterDeclaration() &&
        !insertingNode->IsTSTypeParameterInstantiation()) {
        return nullptr;
    }

    Lexer()->NextToken();
    return insertingNode;
}

ArenaVector<ir::AstNode *> &ETSParser::ParseAstNodesArrayFormatPlaceholder()
{
    if (insertingNodes_.empty()) {
        LogError(diagnostic::INSERT_NODE_ABSENT, {}, Lexer()->GetToken().Start());
        ES2PANDA_UNREACHABLE();
    }

    ParserImpl::NodeFormatType nodeFormat = GetFormatPlaceholderType();
    if (!std::get<0>(nodeFormat) || std::get<1>(nodeFormat) != GENERAL_FORMAT_NODE) {
        LogError(diagnostic::INVALID_NODE_TYPE, {}, Lexer()->GetToken().Start());
        ES2PANDA_UNREACHABLE();
    }

    auto const placeholderNumber = std::get<2>(nodeFormat);
    auto *const insertingNode =
        placeholderNumber < insertingNodes_.size() ? insertingNodes_[placeholderNumber] : nullptr;
    if (insertingNode == nullptr || !insertingNode->IsTSInterfaceBody()) {
        LogError(diagnostic::INVALID_INSERT_NODE, {}, Lexer()->GetToken().Start());
        ES2PANDA_UNREACHABLE();
    }

    Lexer()->NextToken();
    return insertingNode->AsTSInterfaceBody()->Body();
}

ArenaVector<ir::Statement *> &ETSParser::ParseStatementsArrayFormatPlaceholder()
{
    if (insertingNodes_.empty()) {
        LogError(diagnostic::INSERT_NODE_ABSENT, {}, Lexer()->GetToken().Start());
        ES2PANDA_UNREACHABLE();
    }

    ParserImpl::NodeFormatType nodeFormat = GetFormatPlaceholderType();
    if (!std::get<0>(nodeFormat) || std::get<1>(nodeFormat) != STATEMENT_FORMAT_NODE) {
        LogError(diagnostic::INVALID_NODE_TYPE, {}, Lexer()->GetToken().Start());
        ES2PANDA_UNREACHABLE();
    }

    auto const placeholderNumber = std::get<2>(nodeFormat);
    auto *const insertingNode =
        placeholderNumber < insertingNodes_.size() ? insertingNodes_[placeholderNumber] : nullptr;
    if (insertingNode == nullptr || !insertingNode->IsBlockExpression()) {
        LogError(diagnostic::INVALID_INSERT_NODE, {}, Lexer()->GetToken().Start());
        ES2PANDA_UNREACHABLE();
    }

    Lexer()->NextToken();
    return insertingNode->AsBlockExpression()->Statements();
}

ArenaVector<ir::Expression *> &ETSParser::ParseExpressionsArrayFormatPlaceholder()
{
    if (insertingNodes_.empty()) {
        LogError(diagnostic::INSERT_NODE_ABSENT, {}, Lexer()->GetToken().Start());
        ES2PANDA_UNREACHABLE();
    }

    ParserImpl::NodeFormatType nodeFormat = GetFormatPlaceholderType();
    if (!std::get<0>(nodeFormat) || std::get<1>(nodeFormat) != EXPRESSION_FORMAT_NODE) {
        LogError(diagnostic::INVALID_NODE_TYPE, {}, Lexer()->GetToken().Start());
        ES2PANDA_UNREACHABLE();
    }

    auto const placeholderNumber = std::get<2>(nodeFormat);
    auto *const insertingNode =
        placeholderNumber < insertingNodes_.size() ? insertingNodes_[placeholderNumber] : nullptr;
    if (insertingNode == nullptr || !insertingNode->IsSequenceExpression()) {
        LogError(diagnostic::INVALID_INSERT_NODE, {}, Lexer()->GetToken().Start());
        ES2PANDA_UNREACHABLE();
    }

    Lexer()->NextToken();
    return insertingNode->AsSequenceExpression()->Sequence();
}

ir::Statement *ETSParser::CreateStatement(std::string_view const sourceCode)
{
    auto const isp = InnerSourceParser(this);
    auto const lexer = InitFormattedLexer(GetContext().FormattingFileName(), sourceCode);

    lexer::SourcePosition const startLoc = lexer->GetToken().Start();
    lexer->NextToken();

    auto statements = ParseStatementList(StatementParsingFlags::STMT_GLOBAL_LEXICAL);
    auto const statementNumber = statements.size();
    if (statementNumber == 0U) {
        return nullptr;
    }

    if (statementNumber == 1U) {
        return statements[0U];
    }

    auto *const blockStmt = AllocNode<ir::BlockStatement>(Allocator(), std::move(statements));
    ES2PANDA_ASSERT(blockStmt != nullptr);
    blockStmt->SetRange({startLoc, lexer->GetToken().End()});

    for (auto *statement : blockStmt->Statements()) {
        statement->SetParent(blockStmt);
    }

    return blockStmt;
}

ir::Expression *ETSParser::CreateFormattedExpression(std::string_view const sourceCode,
                                                     ArenaVector<ir::Expression *> &args)
{
    std::vector<ir::AstNode *> insertingNodes {};
    insertingNodes.reserve(args.size());
    for (auto it : args) {
        ProcessFormattedArg(insertingNodes, it);
    }

    return CreateFormattedExpression(sourceCode, insertingNodes);
}

ir::Statement *ETSParser::CreateFormattedStatement(std::string_view const sourceCode,
                                                   std::vector<ir::AstNode *> &insertingNodes)
{
    insertingNodes_.swap(insertingNodes);
    auto const statement = CreateStatement(sourceCode);
    insertingNodes_.swap(insertingNodes);
    return statement;
}

ir::TypeNode *ETSParser::CreateFormattedTypeAnnotation(std::string_view const sourceCode)
{
    auto const isp = InnerSourceParser(this);
    auto const lexer = InitFormattedLexer(GetContext().FormattingFileName(), sourceCode);
    lexer->NextToken();
    TypeAnnotationParsingOptions options = TypeAnnotationParsingOptions::NO_OPTS;
    return ParseTypeAnnotation(&options);
}

ir::TypeNode *ETSParser::CreateFormattedTypeAnnotation(std::string_view const sourceCode,
                                                       std::vector<ir::AstNode *> &args)
{
    insertingNodes_.swap(args);
    auto typeAnnotation = CreateFormattedTypeAnnotation(sourceCode);
    insertingNodes_.swap(args);
    return typeAnnotation;
}

ArenaVector<ir::Statement *> ETSParser::CreateStatements(std::string_view const sourceCode)
{
    auto const isp = InnerSourceParser(this);
    auto const lexer = InitFormattedLexer(GetContext().FormattingFileName(), sourceCode);

    lexer->NextToken();
    return ParseStatementList(StatementParsingFlags::STMT_GLOBAL_LEXICAL);
}

ArenaVector<ir::Statement *> ETSParser::CreateFormattedStatements(std::string_view const sourceCode,
                                                                  std::vector<ir::AstNode *> &insertingNodes)
{
    insertingNodes_.swap(insertingNodes);
    auto statements = CreateStatements(sourceCode);
    insertingNodes_.swap(insertingNodes);
    return statements;
}

//  NOTE: this method returns only a single (the first) class filed definition.
//  It seems that it is possible to have several of them be parsed at a time but nobody knows how...
ir::AstNode *ETSParser::CreateFormattedClassFieldDefinition(std::string_view sourceCode,
                                                            std::vector<ir::AstNode *> &insertingNodes)
{
    thread_local static ArenaVector<ir::AstNode *> const DUMMY_ARRAY {Allocator()->Adapter()};
    insertingNodes_.swap(insertingNodes);

    auto *const property = CreateClassElement(sourceCode, DUMMY_ARRAY, ir::ClassDefinitionModifiers::NONE);
    ES2PANDA_ASSERT(property != nullptr);
    if (!property->IsTSInterfaceBody() || property->AsTSInterfaceBody()->Body().empty()) {
        LogError(diagnostic::INVALID_CLASS_FIELD, {}, Lexer()->GetToken().Start());
        ES2PANDA_UNREACHABLE();
    }

    insertingNodes_.swap(insertingNodes);
    return property->AsTSInterfaceBody()->Body().front();
}

ir::AstNode *ETSParser::CreateFormattedClassMethodDefinition(std::string_view sourceCode,
                                                             std::vector<ir::AstNode *> &insertingNodes)
{
    thread_local static ArenaVector<ir::AstNode *> const DUMMY_ARRAY {Allocator()->Adapter()};
    insertingNodes_.swap(insertingNodes);

    auto *const property = CreateClassElement(sourceCode, DUMMY_ARRAY, ir::ClassDefinitionModifiers::NONE);
    ES2PANDA_ASSERT(property != nullptr);
    if (!property->IsMethodDefinition()) {
        LogError(diagnostic::INVALID_CLASS_METHOD, {}, Lexer()->GetToken().Start());
        ES2PANDA_UNREACHABLE();
    }

    insertingNodes_.swap(insertingNodes);
    return property;
}

ir::AstNode *ETSParser::CreateFormattedClassElement(std::string_view sourceCode,
                                                    std::vector<ir::AstNode *> &insertingNodes,
                                                    const ArenaVector<ir::AstNode *> &properties,
                                                    ir::ClassDefinitionModifiers modifiers)
{
    insertingNodes_.swap(insertingNodes);
    auto *const classElement = CreateClassElement(sourceCode, properties, modifiers);
    insertingNodes_.swap(insertingNodes);
    return classElement;
}

//  NOTE: the method has limited functionality - it returns 'ir::TSInterfaceBody' placeholder for the field
//  declaration(s) (properties themselves are in ->Body() member) and does not perform any check of the node returned.
//  Also the node isn't placed in the providing properties container.
ir::AstNode *ETSParser::CreateClassElement(std::string_view sourceCode, const ArenaVector<ir::AstNode *> &properties,
                                           ir::ClassDefinitionModifiers modifiers)
{
    auto const isp = InnerSourceParser(this);
    auto const lexer = InitFormattedLexer(GetContext().FormattingFileName(), sourceCode);

    auto savedCtx = SavedStatusContext<ParserStatus::IN_CLASS_BODY>(&GetContext());
    SavedClassPrivateContext classContext(this);

    lexer->NextToken(lexer::NextTokenFlags::KEYWORD_TO_IDENT);

    return ParseClassElement(properties, modifiers, ir::ModifierFlags::NONE);
}

ir::Statement *ETSParser::CreateFormattedClassDeclaration(std::string_view sourceCode,
                                                          std::vector<ir::AstNode *> &insertingNodes,
                                                          bool const allowStatic)
{
    insertingNodes_.swap(insertingNodes);
    auto *const classDeclaration = CreateClassDeclaration(sourceCode, allowStatic);
    insertingNodes_.swap(insertingNodes);
    return classDeclaration;
}

ir::Statement *ETSParser::CreateClassDeclaration(std::string_view sourceCode, bool allowStatic)
{
    auto const isp = InnerSourceParser(this);
    auto const lexer = InitFormattedLexer(GetContext().FormattingFileName(), sourceCode);

    auto savedCtx = SavedStatusContext<ParserStatus::IN_CLASS_BODY>(&GetContext());

    auto modifiers = ir::ClassDefinitionModifiers::ID_REQUIRED | ir::ClassDefinitionModifiers::CLASS_DECL;
    ir::ModifierFlags flags = ir::ModifierFlags::NONE;

    lexer->NextToken();

    switch (auto tokenType = Lexer()->GetToken().Type(); tokenType) {
        case lexer::TokenType::KEYW_STATIC: {
            if (!allowStatic) {
                LogUnexpectedToken(Lexer()->GetToken());
            }
            [[fallthrough]];
        }
        case lexer::TokenType::KEYW_ABSTRACT:
        case lexer::TokenType::KEYW_FINAL: {
            flags = ParseClassModifiers();
            if (allowStatic && (flags & ir::ModifierFlags::STATIC) == 0U) {
                modifiers |= ir::ClassDefinitionModifiers::INNER;
            }

            if (auto const tokType = Lexer()->GetToken().Type(); tokType != lexer::TokenType::KEYW_CLASS) {
                LogUnexpectedToken(Lexer()->GetToken());
            }
            [[fallthrough]];
        }
        case lexer::TokenType::KEYW_CLASS: {
            return ParseClassDeclaration(modifiers);
        }
        default: {
            LogUnexpectedToken(Lexer()->GetToken());
            return nullptr;
        }
    }
}

ir::MethodDefinition *ETSParser::CreateConstructorDefinition(ir::ModifierFlags modifiers,
                                                             std::string_view const sourceCode)
{
    auto const isp = InnerSourceParser(this);
    auto const lexer = InitFormattedLexer(GetContext().FormattingFileName(), sourceCode);

    auto const startLoc = Lexer()->GetToken().Start();
    Lexer()->NextToken();

    if (IsClassMethodModifier(Lexer()->GetToken().Type())) {
        modifiers |= ParseClassMethodModifiers(false);
    }

    if (Lexer()->GetToken().Type() != lexer::TokenType::KEYW_CONSTRUCTOR) {
        LogExpectedToken(lexer::TokenType::KEYW_CONSTRUCTOR);
    }

    if ((modifiers & ir::ModifierFlags::ASYNC) != 0) {
        LogError(diagnostic::ASYNC_CONSTRUCTOR);
        ES2PANDA_UNREACHABLE();
    }

    auto *memberName = AllocNode<ir::Identifier>(Lexer()->GetToken().Ident(), Allocator());
    modifiers |= ir::ModifierFlags::CONSTRUCTOR;
    Lexer()->NextToken();

    auto *const methodDefinition = ParseClassMethodDefinition(memberName, modifiers, true);
    ES2PANDA_ASSERT(methodDefinition != nullptr);
    methodDefinition->SetStart(startLoc);

    return methodDefinition;
}

ir::Expression *ETSParser::CreateExpression(std::string_view const sourceCode, ExpressionParseFlags const flags)
{
    auto const isp = InnerSourceParser(this);
    auto const lexer = InitFormattedLexer(GetContext().FormattingFileName(), sourceCode);

    lexer->NextToken();

    ir::Expression *returnExpression = ParseExpression(flags);
    return returnExpression;
}

ir::Expression *ETSParser::CreateFormattedExpression(std::string_view const sourceCode,
                                                     std::vector<ir::AstNode *> &insertingNodes)
{
    ir::Expression *returnExpression;
    insertingNodes_.swap(insertingNodes);

    if (auto statements = CreateStatements(sourceCode);
        statements.size() == 1U && statements.back()->IsExpressionStatement()) {
        returnExpression = statements.back()->AsExpressionStatement()->GetExpression();
    } else {
        returnExpression = AllocNode<ir::BlockExpression>(std::move(statements));
    }

    insertingNodes_.swap(insertingNodes);
    return returnExpression;
}

[[nodiscard]] std::unique_ptr<lexer::Lexer> ETSParser::InitFormattedLexer(
    // CC-OFFNXT(G.FMT.06-CPP) project code style
    [[maybe_unused]] std::string_view formattedName, std::string_view sourceCode)
{
    util::UString source {sourceCode, Allocator()};
    GetProgram()->SetSource(source.View().Utf8(), util::Path(), "");
    auto lexer = std::make_unique<lexer::ETSLexer>(&GetContext(), DiagnosticEngine());
    SetLexer(lexer.get());
    return lexer;
}

ir::Statement *ETSParser::CreateTopLevelStatement(std::string_view const sourceCode)
{
    auto const isp = InnerSourceParser(this);
    auto const lexer = InitFormattedLexer(GetContext().FormattingFileName(), sourceCode);

    lexer->NextToken();

    return ParseTopLevelStatement();
}

ir::Statement *ETSParser::CreateFormattedTopLevelStatement(std::string_view const sourceCode,
                                                           std::vector<ir::AstNode *> &insertingNodes)
{
    insertingNodes_.swap(insertingNodes);
    auto const statement = CreateTopLevelStatement(sourceCode);
    insertingNodes_.swap(insertingNodes);
    return statement;
}

ir::TypeNode *ETSParser::CreateTypeAnnotation(TypeAnnotationParsingOptions *options, std::string_view const sourceCode)
{
    auto const isp = InnerSourceParser(this);
    auto const lexer = InitFormattedLexer(GetContext().FormattingFileName(), sourceCode);

    lexer->NextToken();
    return ParseTypeAnnotation(options);
}
}  // namespace ark::es2panda::parser
