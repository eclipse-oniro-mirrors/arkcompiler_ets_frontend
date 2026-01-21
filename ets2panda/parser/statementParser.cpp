/**
 * Copyright (c) 2021-2026 Huawei Device Co., Ltd.
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

#include "parser/parserFlags.h"
#include "parser/parserStatusContext.h"
#include "util/errorRecovery.h"
#include "util/helpers.h"
#include "ir/astNode.h"
#include "ir/base/catchClause.h"
#include "ir/base/classDefinition.h"
#include "ir/base/scriptFunction.h"
#include "ir/expression.h"
#include "ir/expressions/arrayExpression.h"
#include "ir/expressions/binaryExpression.h"
#include "ir/expressions/conditionalExpression.h"
#include "ir/expressions/literals/stringLiteral.h"
#include "ir/expressions/objectExpression.h"
#include "ir/expressions/sequenceExpression.h"
#include "ir/module/exportAllDeclaration.h"
#include "ir/module/exportDefaultDeclaration.h"
#include "ir/module/exportNamedDeclaration.h"
#include "ir/module/exportSpecifier.h"
#include "ir/module/importDeclaration.h"
#include "ir/module/importDefaultSpecifier.h"
#include "ir/module/importNamespaceSpecifier.h"
#include "ir/module/importSpecifier.h"
#include "ir/statements/blockStatement.h"
#include "ir/statements/breakStatement.h"
#include "ir/statements/classDeclaration.h"
#include "ir/statements/continueStatement.h"
#include "ir/statements/debuggerStatement.h"
#include "ir/statements/doWhileStatement.h"
#include "ir/statements/emptyStatement.h"
#include "ir/statements/expressionStatement.h"
#include "ir/statements/forInStatement.h"
#include "ir/statements/forOfStatement.h"
#include "ir/statements/forUpdateStatement.h"
#include "ir/statements/functionDeclaration.h"
#include "ir/statements/ifStatement.h"
#include "ir/statements/labelledStatement.h"
#include "ir/statements/returnStatement.h"
#include "ir/statements/switchCaseStatement.h"
#include "ir/statements/switchStatement.h"
#include "ir/statements/throwStatement.h"
#include "ir/statements/tryStatement.h"
#include "ir/statements/variableDeclaration.h"
#include "ir/statements/variableDeclarator.h"
#include "ir/statements/whileStatement.h"
#include "ir/ets/etsStructDeclaration.h"
#include "lexer/lexer.h"
#include "lexer/token/letters.h"
#include "lexer/token/sourceLocation.h"
#include "util/recursiveGuard.h"
#include "util/ustring.h"
#include "generated/diagnostic.h"

#include "parserImpl.h"

namespace ark::es2panda::parser {

using namespace std::literals::string_literals;

// NOLINTNEXTLINE(google-default-arguments)
ir::Statement *ParserImpl::ParseStatementLiteralIdentHelper(StatementParsingFlags flags)
{
    if (Lexer()->GetToken().KeywordType() == lexer::TokenType::KEYW_STRUCT) {
        return ParseStructStatement(flags, ir::ClassDefinitionModifiers::NONE);
    }

    if (lexer_->Lookahead() == lexer::LEX_CHAR_COLON) {
        const auto pos = lexer_->Save();
        lexer_->NextToken();
        return ParseLabelledStatement(pos);
    }

    return ParsePotentialExpressionStatement(flags);
}

// NOLINTNEXTLINE(google-default-arguments)
ir::Statement *ParserImpl::ParseStatementPunctuatorsHelper(StatementParsingFlags flags)
{
    TrackRecursive trackRecursive(RecursiveCtx());
    if (!trackRecursive) {
        LogError(diagnostic::DEEP_NESTING);
        while (Lexer()->GetToken().Type() != lexer::TokenType::EOS) {
            Lexer()->NextToken();
        }
        return AllocBrokenStatement(Lexer()->GetToken().Loc());
    }
    switch (lexer_->GetToken().Type()) {
        case lexer::TokenType::PUNCTUATOR_LEFT_BRACE:
            return ParseBlockStatement();
        case lexer::TokenType::PUNCTUATOR_SEMI_COLON:
            return ParseEmptyStatement();
        case lexer::TokenType::PUNCTUATOR_FORMAT:  // 1
            if (lexer_->Lookahead() == static_cast<char32_t>(STATEMENT_FORMAT_NODE)) {
                return ParseStatementFormatPlaceholder();
            }
            [[fallthrough]];
        default:
            return ParseExpressionStatement(flags);
    }
}

// NOLINTNEXTLINE(google-default-arguments)
ir::Statement *ParserImpl::ParseStatementControlFlowTokenHelper(StatementParsingFlags flags)
{
    switch (lexer_->GetToken().Type()) {
        case lexer::TokenType::KEYW_IF:
            return ParseIfStatement();
        case lexer::TokenType::KEYW_DO:
            return ParseDoWhileStatement();
        case lexer::TokenType::KEYW_FOR:
            return ParseForStatement();
        case lexer::TokenType::KEYW_TRY:
            return ParseTryStatement();
        case lexer::TokenType::KEYW_WHILE:
            return ParseWhileStatement();
        case lexer::TokenType::KEYW_BREAK:
            return ParseBreakStatement();
        case lexer::TokenType::KEYW_CONTINUE:
            return ParseContinueStatement();
        case lexer::TokenType::KEYW_THROW:
            return ParseThrowStatement();
        case lexer::TokenType::KEYW_RETURN:
            return ParseReturnStatement();
        case lexer::TokenType::KEYW_SWITCH:
            return ParseSwitchStatement();
        default:
            return ParseExpressionStatement(flags);
    }
}

// NOLINTNEXTLINE(google-default-arguments)
ir::Statement *ParserImpl::ParseStatement(StatementParsingFlags flags)
{
    const auto tokenType = lexer_->GetToken().Type();
    bool isPunctuatorToken = tokenType == lexer::TokenType::PUNCTUATOR_LEFT_BRACE ||
                             tokenType == lexer::TokenType::PUNCTUATOR_SEMI_COLON ||
                             tokenType == lexer::TokenType::PUNCTUATOR_FORMAT;
    if (isPunctuatorToken) {
        return ParseStatementPunctuatorsHelper(flags);
    }

    bool isControlFlowToken = tokenType == lexer::TokenType::KEYW_IF || tokenType == lexer::TokenType::KEYW_DO ||
                              tokenType == lexer::TokenType::KEYW_FOR || tokenType == lexer::TokenType::KEYW_TRY ||
                              tokenType == lexer::TokenType::KEYW_WHILE || tokenType == lexer::TokenType::KEYW_BREAK ||
                              tokenType == lexer::TokenType::KEYW_CONTINUE ||
                              tokenType == lexer::TokenType::KEYW_THROW || tokenType == lexer::TokenType::KEYW_RETURN ||
                              tokenType == lexer::TokenType::KEYW_SWITCH;
    if (isControlFlowToken) {
        return ParseStatementControlFlowTokenHelper(flags);
    }
    return ParseStatementBasedOnTokenType(flags);
}

ir::Statement *ParserImpl::ParseStatementBasedOnTokenType(StatementParsingFlags flags)
{
    switch (lexer_->GetToken().Type()) {
        case lexer::TokenType::KEYW_EXPORT:
            return ParseExportDeclaration(flags);
        case lexer::TokenType::KEYW_IMPORT:
            return ParseImportDeclaration(flags);
        case lexer::TokenType::KEYW_FUNCTION:
            return ParseFunctionStatement(flags);
        case lexer::TokenType::KEYW_ABSTRACT:
        case lexer::TokenType::KEYW_FINAL:
        case lexer::TokenType::KEYW_CLASS:
            return ParseClassStatement(flags, ir::ClassDefinitionModifiers::NONE);
        case lexer::TokenType::KEYW_VAR:
            return ParseVarStatement();
        case lexer::TokenType::KEYW_LET:
            return ParseLetStatement(flags);
        case lexer::TokenType::KEYW_CONST:
            return ParseConstStatement(flags);
        case lexer::TokenType::KEYW_DEBUGGER:
            return ParseDebuggerStatement();
        case lexer::TokenType::LITERAL_IDENT:
            if (lexer_->GetToken().KeywordType() == lexer::TokenType::KEYW_VAR) {
                return ParseVarStatement();
            }
            return ParseStatementLiteralIdentHelper(flags);
        case lexer::TokenType::KEYW_WITH:
            LogError(diagnostic::WITH_DEPRECATED);
            lexer_->NextToken();
            return nullptr;
        case lexer::TokenType::KEYW_ENUM:
            return ParseEnumStatement(flags);
        case lexer::TokenType::KEYW_INTERFACE:
            return ParseInterfaceStatement(flags);
        case lexer::TokenType::PUNCTUATOR_AT:
            if (IsETSParser()) {
                return ParseAnnotationsInStatement(flags);
            }
            [[fallthrough]];
        default:
            return ParseExpressionStatement(flags);
    }
}

ir::Statement *ParserImpl::ParseInitModuleStatement()
{
    ES2PANDA_UNREACHABLE();
}

ir::Statement *ParserImpl::ParseAnnotationsInStatement([[maybe_unused]] StatementParsingFlags flags)
{
    ES2PANDA_UNREACHABLE();
}

ir::Statement *ParserImpl::ParseVarStatement()
{
    auto *variableDecl = ParseVariableDeclaration(VariableParsingFlags::VAR);
    ConsumeSemicolon(variableDecl);
    return variableDecl;
}

ir::Statement *ParserImpl::ParseLetStatement(StatementParsingFlags flags)
{
    if ((flags & StatementParsingFlags::ALLOW_LEXICAL) == 0) {
        LogError(diagnostic::LEXICAL_DEC_NOT_ALLOWED_IN_SINGLE_STATEMENT_CONTEXT);
    }

    auto *variableDecl = ParseVariableDeclaration(VariableParsingFlags::LET);
    ES2PANDA_ASSERT(variableDecl != nullptr);
    if (variableDecl->IsBrokenStatement()) {  // Error processing.
        return variableDecl;
    }

    ConsumeSemicolon(variableDecl);
    return variableDecl;
}

ir::Statement *ParserImpl::ParseConstStatement(StatementParsingFlags flags)
{
    if ((flags & StatementParsingFlags::ALLOW_LEXICAL) == 0) {
        LogError(diagnostic::LEXICAL_DEC_NOT_ALLOWED_IN_SINGLE_STATEMENT_CONTEXT);
    }

    lexer::SourcePosition constVarStar = lexer_->GetToken().Start();
    lexer_->NextToken();

    auto *variableDecl = ParseVariableDeclaration(VariableParsingFlags::CONST | VariableParsingFlags::NO_SKIP_VAR_KIND);
    ES2PANDA_ASSERT(variableDecl != nullptr);
    if (variableDecl->IsBrokenStatement()) {  // Error processing.
        return variableDecl;
    }

    variableDecl->SetStart(constVarStar);
    ConsumeSemicolon(variableDecl);

    return variableDecl;
}

ir::EmptyStatement *ParserImpl::ParseEmptyStatement()
{
    auto *empty = AllocNode<ir::EmptyStatement>();
    ES2PANDA_ASSERT(empty != nullptr);
    empty->SetRange(lexer_->GetToken().Loc());
    lexer_->NextToken();
    return empty;
}

ir::Statement *ParserImpl::ParseDebuggerStatement()
{
    auto *debuggerNode = AllocNode<ir::DebuggerStatement>();
    ES2PANDA_ASSERT(debuggerNode != nullptr);
    debuggerNode->SetRange(lexer_->GetToken().Loc());
    lexer_->NextToken();
    ConsumeSemicolon(debuggerNode);
    return debuggerNode;
}

ir::Statement *ParserImpl::ParseFunctionStatement(StatementParsingFlags flags)
{
    CheckFunctionDeclaration(flags);

    if ((flags & StatementParsingFlags::STMT_LEXICAL_SCOPE_NEEDED) == 0) {
        return ParseFunctionDeclaration(false, ParserStatus::NO_OPTS);
    }

    auto *funcDecl = ParseFunctionDeclaration(false, ParserStatus::NO_OPTS);
    ArenaVector<ir::Statement *> stmts(Allocator()->Adapter());
    stmts.push_back(funcDecl);

    auto *localBlockStmt = AllocNode<ir::BlockStatement>(Allocator(), std::move(stmts));
    ES2PANDA_ASSERT(localBlockStmt != nullptr);
    localBlockStmt->SetRange(funcDecl->Range());

    return funcDecl;
}

ir::Statement *ParserImpl::ParsePotentialExpressionStatement(StatementParsingFlags flags)
{
    return ParseExpressionStatement(flags);
}

// NOLINTNEXTLINE(google-default-arguments)
ir::Statement *ParserImpl::ParseStructStatement([[maybe_unused]] StatementParsingFlags flags,
                                                ir::ClassDefinitionModifiers modifiers, ir::ModifierFlags modFlags)
{
    LogError(diagnostic::ILLEGAL_START_STRUCT_CLASS, {"STRUCT"});
    return ParseStructDeclaration(modifiers, modFlags);
}

// NOLINTNEXTLINE(google-default-arguments)
ir::Statement *ParserImpl::ParseClassStatement(StatementParsingFlags flags, ir::ClassDefinitionModifiers modifiers,
                                               ir::ModifierFlags modFlags)
{
    if ((flags & StatementParsingFlags::ALLOW_LEXICAL) == 0) {
        LogError(diagnostic::LEXICAL_DEC_NOT_ALLOWED_IN_SINGLE_STATEMENT_CONTEXT);
    }

    return ParseClassDeclaration(modifiers, modFlags);
}

ir::Statement *ParserImpl::ParseInterfaceStatement(StatementParsingFlags flags)
{
    if ((flags & StatementParsingFlags::ALLOW_LEXICAL) == 0) {
        LogError(diagnostic::LEXICAL_DEC_NOT_ALLOWED_IN_SINGLE_STATEMENT_CONTEXT);
    }

    return ParseInterfaceDeclaration(false);
}

ir::Statement *ParserImpl::ParseEnumStatement(StatementParsingFlags flags)
{
    if ((flags & StatementParsingFlags::ALLOW_LEXICAL) == 0) {
        LogError(diagnostic::LEXICAL_DEC_NOT_ALLOWED_IN_SINGLE_STATEMENT_CONTEXT);
    }

    return ParseEnumDeclaration();
}

ir::Statement *ParserImpl::ParseStructDeclaration(ir::ClassDefinitionModifiers modifiers, ir::ModifierFlags flags)
{
    const lexer::SourcePosition startLoc = lexer_->GetToken().Start();
    modifiers |= ir::ClassDefinitionModifiers::DECLARATION;
    if (IsExternal()) {
        modifiers |= ir::ClassDefinitionModifiers::FROM_EXTERNAL;
    }

    ir::ClassDefinition *classDefinition = ParseClassDefinition(modifiers, flags);
    if (classDefinition == nullptr) {  // Error processing.
        // Error is logged inside ParseClassDefinition
        return AllocBrokenStatement(Lexer()->GetToken().Loc());
    }

    lexer::SourcePosition endLoc = classDefinition->End();
    auto *structDecl = AllocNode<ir::ETSStructDeclaration>(classDefinition, Allocator());
    ES2PANDA_ASSERT(structDecl != nullptr);
    structDecl->SetRange({startLoc, endLoc});
    return structDecl;
}

ir::Statement *ParserImpl::ParseClassDeclaration(ir::ClassDefinitionModifiers modifiers, ir::ModifierFlags flags)
{
    const lexer::SourcePosition startLoc = lexer_->GetToken().Start();
    modifiers |= ir::ClassDefinitionModifiers::DECLARATION;
    if (IsExternal()) {
        modifiers |= ir::ClassDefinitionModifiers::FROM_EXTERNAL;
    }

    ir::ClassDefinition *classDefinition = ParseClassDefinition(modifiers, flags);
    if (classDefinition == nullptr) {  // Error processing.
        // Error is logged inside ParseClassDefinition
        return AllocBrokenStatement(Lexer()->GetToken().Loc());
    }

    lexer::SourcePosition endLoc = classDefinition->End();
    auto *classDecl = AllocNode<ir::ClassDeclaration>(classDefinition, Allocator());
    ES2PANDA_ASSERT(classDecl != nullptr);
    classDecl->SetRange({startLoc, endLoc});
    return classDecl;
}

void ParserImpl::CheckFunctionDeclaration(StatementParsingFlags flags)
{
    if ((flags & StatementParsingFlags::LABELLED) != 0) {
        LogError(diagnostic::STRICT_MODE_FUNC_DECL);
    }

    if ((flags & StatementParsingFlags::ALLOW_LEXICAL) == 0) {
        if ((flags & (StatementParsingFlags::IF_ELSE | StatementParsingFlags::LABELLED)) == 0) {
            LogError(diagnostic::LEXICAL_DEC_NOT_ALLOWED_IN_SINGLE_STATEMENT_CONTEXT);
        }

        if (lexer_->Lookahead() == lexer::LEX_CHAR_ASTERISK) {
            LogError(diagnostic::GENERATORS_ONLY_TOP_LEVEL_OR_INSIDE_BLOCK);
        }
    }
}

void ParserImpl::ConsumeSemicolon(ir::Statement *statement)
{
    auto const &token = lexer_->GetToken();
    auto tokenType = token.Type();
    if (tokenType == lexer::TokenType::PUNCTUATOR_SEMI_COLON) {
        ES2PANDA_ASSERT(statement != nullptr);
        statement->SetEnd(token.End());
        lexer_->NextToken();
        return;
    }

    if (!token.NewLine() && tokenType != lexer::TokenType::EOS &&
        tokenType != lexer::TokenType::PUNCTUATOR_RIGHT_BRACE) {
        LogUnexpectedToken(token);
    }
}

ArenaVector<ir::Statement *> ParserImpl::ParseStatementList(StatementParsingFlags flags)
{
    if (lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_FORMAT &&
        lexer_->Lookahead() == static_cast<char32_t>(ARRAY_FORMAT_NODE)) {
        return std::move(ParseStatementsArrayFormatPlaceholder());
    }

    ArenaVector<ir::Statement *> statements(Allocator()->Adapter());
    ParseDirectivePrologue(&statements);

    auto endType =
        (flags & StatementParsingFlags::GLOBAL) != 0 ? lexer::TokenType::EOS : lexer::TokenType::PUNCTUATOR_RIGHT_BRACE;

    while (lexer_->GetToken().Type() != endType && lexer_->GetToken().Type() != lexer::TokenType::EOS) {
        util::ErrorRecursionGuard infiniteLoopBlocker(lexer_);

        if (auto statement = ParseStatement(flags); statement != nullptr) {
            statements.push_back(statement);
        }
    }

    return statements;
}

bool ParserImpl::ParseDirective(ArenaVector<ir::Statement *> *statements)
{
    ES2PANDA_ASSERT(lexer_->GetToken().Type() == lexer::TokenType::LITERAL_STRING);

    const util::StringView &str = lexer_->GetToken().String();

    const auto status = static_cast<ParserStatus>(
        context_.Status() & (ParserStatus::CONSTRUCTOR_FUNCTION | ParserStatus::HAS_COMPLEX_PARAM));
    if (status == ParserStatus::HAS_COMPLEX_PARAM && str.Is("use strict")) {
        LogError(diagnostic::ILLEGAL_USE_STRICT);
    }

    ir::Expression *exprNode = ParseExpression(ExpressionParseFlags::ACCEPT_COMMA);
    if (exprNode == nullptr) {  // Error processing.
        return false;
    }

    bool isDirective = exprNode->IsStringLiteral();

    auto *exprStatement = AllocNode<ir::ExpressionStatement>(exprNode);
    ES2PANDA_ASSERT(exprStatement != nullptr);
    exprStatement->SetRange(exprNode->Range());

    ConsumeSemicolon(exprStatement);
    statements->push_back(exprStatement);

    return isDirective;
}

void ParserImpl::ParseDirectivePrologue(ArenaVector<ir::Statement *> *statements)
{
    while (true) {
        if (lexer_->GetToken().Type() != lexer::TokenType::LITERAL_STRING || !ParseDirective(statements)) {
            break;
        }
    }
}

bool ParserImpl::ValidateLabeledStatement([[maybe_unused]] lexer::TokenType type)
{
    return true;
}

ir::BlockStatement *ParserImpl::ParseBlockStatement()
{
    lexer::SourcePosition startLoc = lexer_->GetToken().Start();
    ExpectToken(lexer::TokenType::PUNCTUATOR_LEFT_BRACE);

    auto statements = ParseStatementList();

    auto *blockNode = AllocNode<ir::BlockStatement>(Allocator(), std::move(statements));
    ES2PANDA_ASSERT(blockNode != nullptr);
    blockNode->SetRange({startLoc, lexer_->GetToken().End()});

    ExpectToken(lexer::TokenType::PUNCTUATOR_RIGHT_BRACE);

    return blockNode;
}

void ParserImpl::ReportPossibleOutOfBoundaryJumpError([[maybe_unused]] bool allowBreak) {}

void ParserImpl::ReportIllegalBreakError(const lexer::SourcePosition &startLoc)
{
    LogError(diagnostic::ILLEGAL_BREAK, {}, startLoc);
}

ir::Statement *ParserImpl::ParseBreakStatement()
{
    bool allowBreak = (context_.Status() & (ParserStatus::IN_ITERATION | ParserStatus::IN_SWITCH)) != 0;

    ReportPossibleOutOfBoundaryJumpError(allowBreak);

    lexer::SourcePosition startLoc = lexer_->GetToken().Start();
    lexer_->NextToken();

    if (lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_SEMI_COLON ||
        lexer_->GetToken().Type() == lexer::TokenType::EOS || lexer_->GetToken().NewLine() ||
        lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_RIGHT_BRACE) {
        if (!allowBreak) {
            ReportIllegalBreakError(startLoc);
        }

        auto *breakStatement = AllocNode<ir::BreakStatement>();
        ES2PANDA_ASSERT(breakStatement != nullptr);
        breakStatement->SetRange({startLoc, lexer_->GetToken().End()});

        if (lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_SEMI_COLON) {
            lexer_->NextToken();
        }

        return breakStatement;
    }

    if (lexer_->GetToken().Type() != lexer::TokenType::LITERAL_IDENT) {
        LogExpectedToken(lexer::TokenType::LITERAL_IDENT);
    }

    const auto &label = lexer_->GetToken().Ident();
    if (!ValidateBreakLabel(label)) {
        LogError(diagnostic::UNDEFINED_LABEL);
    }

    auto *identNode = AllocNode<ir::Identifier>(label, Allocator());
    ES2PANDA_ASSERT(identNode != nullptr);
    identNode->SetRange(lexer_->GetToken().Loc());

    auto *breakStatement = AllocNode<ir::BreakStatement>(identNode);
    ES2PANDA_ASSERT(breakStatement != nullptr);
    breakStatement->SetRange({startLoc, lexer_->GetToken().End()});

    lexer_->NextToken();
    ConsumeSemicolon(breakStatement);

    return breakStatement;
}

void ParserImpl::ReportIllegalContinueError()
{
    LogError(diagnostic::ILLEGALE_CONTINUE);
}

ir::Statement *ParserImpl::ParseContinueStatement()
{
    ReportPossibleOutOfBoundaryJumpError((context_.Status() & (ParserStatus::IN_ITERATION | ParserStatus::IN_SWITCH)) !=
                                         0U);

    if ((context_.Status() & ParserStatus::IN_ITERATION) == 0) {
        ReportIllegalContinueError();
    }

    lexer::SourcePosition startLoc = lexer_->GetToken().Start();
    lexer::SourcePosition endLoc = lexer_->GetToken().End();
    lexer_->NextToken();

    if (lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_SEMI_COLON) {
        auto *continueStatement = AllocNode<ir::ContinueStatement>();
        ES2PANDA_ASSERT(continueStatement != nullptr);
        continueStatement->SetRange({startLoc, lexer_->GetToken().End()});
        lexer_->NextToken();
        return continueStatement;
    }

    if (lexer_->GetToken().NewLine() || lexer_->GetToken().Type() == lexer::TokenType::EOS ||
        lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_RIGHT_BRACE) {
        auto *continueStatement = AllocNode<ir::ContinueStatement>();
        ES2PANDA_ASSERT(continueStatement != nullptr);
        continueStatement->SetRange({startLoc, endLoc});
        return continueStatement;
    }

    if (lexer_->GetToken().Type() != lexer::TokenType::LITERAL_IDENT) {
        LogExpectedToken(lexer::TokenType::LITERAL_IDENT);
    }

    const auto &label = lexer_->GetToken().Ident();
    if (!ValidateContinueLabel(label)) {
        LogError(diagnostic::UNDEFINED_LABEL);
    }

    auto *identNode = AllocNode<ir::Identifier>(label, Allocator());
    ES2PANDA_ASSERT(identNode != nullptr);
    identNode->SetRange(lexer_->GetToken().Loc());

    auto *continueStatement = AllocNode<ir::ContinueStatement>(identNode);
    ES2PANDA_ASSERT(continueStatement != nullptr);
    continueStatement->SetRange({startLoc, lexer_->GetToken().End()});

    lexer_->NextToken();
    ConsumeSemicolon(continueStatement);

    return continueStatement;
}

ir::Statement *ParserImpl::ParseDoWhileStatement()
{
    IterationContext iterCtx(&context_);

    lexer::SourcePosition startLoc = lexer_->GetToken().Start();
    lexer_->NextToken();

    ir::Statement *body = ParseStatement();
    if (IsBrokenStatement(body)) {
        LogError(diagnostic::MISSING_LOOP_BODY, {"do while"});
    }

    if (lexer_->GetToken().Type() != lexer::TokenType::KEYW_WHILE) {
        if (lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS) {
            // do {} (true)
            LogError(diagnostic::UNEXPECTED_TOKEN_WHILE);  // just skip the token
        } else {
            // do {} whle (true)
            LogExpectedToken(lexer::TokenType::KEYW_WHILE);
            lexer_->NextToken();
        }
    } else {
        lexer_->NextToken();
    }

    if (lexer_->GetToken().Type() != lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS) {
        LogExpectedToken(lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS);
    }

    lexer_->NextToken();

    ir::Expression *condition = ParseExpression(ExpressionParseFlags::ACCEPT_COMMA);
    if (condition->IsBrokenExpression()) {
        LogError(diagnostic::MISSING_LOOP_CONDITION, {"do while"});
    }

    auto endLoc = lexer_->GetToken().End();
    ExpectToken(lexer::TokenType::PUNCTUATOR_RIGHT_PARENTHESIS);

    auto *doWhileStatement = AllocNode<ir::DoWhileStatement>(body, condition);
    ES2PANDA_ASSERT(doWhileStatement != nullptr);
    doWhileStatement->SetRange({startLoc, endLoc});

    if (lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_SEMI_COLON) {
        doWhileStatement->SetEnd(lexer_->GetToken().End());
        lexer_->NextToken();
    }

    return doWhileStatement;
}

ir::FunctionDeclaration *ParserImpl::ParseFunctionDeclaration(bool canBeAnonymous, ParserStatus newStatus)
{
    lexer::SourcePosition startLoc = lexer_->GetToken().Start();

    ES2PANDA_ASSERT(lexer_->GetToken().Type() == lexer::TokenType::KEYW_FUNCTION);
    ParserStatus savedStatus = context_.Status();

    lexer_->NextToken();

    if (lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_MULTIPLY) {
        newStatus |= ParserStatus::GENERATOR_FUNCTION;
        lexer_->NextToken();
    }

    context_.Status() = savedStatus;

    if (lexer_->GetToken().Type() != lexer::TokenType::LITERAL_IDENT) {
        if (canBeAnonymous) {
            ir::ScriptFunction *func = ParseFunction(newStatus | ParserStatus::NEED_RETURN_TYPE);
            ES2PANDA_ASSERT(func != nullptr);
            func->SetStart(startLoc);

            auto *funcDecl = AllocNode<ir::FunctionDeclaration>(Allocator(), func, true);
            ES2PANDA_ASSERT(funcDecl != nullptr);
            funcDecl->SetRange(func->Range());
            return funcDecl;
        }
        // test exists for ts extension
        LogError(diagnostic::UNEXPECTED_TOKEN_ID_FUN);
        lexer_->GetToken().SetTokenType(lexer::TokenType::LITERAL_IDENT);
        lexer_->GetToken().SetTokenStr(ERROR_LITERAL);
    }

    CheckRestrictedBinding();
    auto *identNode = ExpectIdentifier();

    newStatus |= ParserStatus::FUNCTION_DECLARATION;
    ir::ScriptFunction *func = ParseFunction(newStatus | ParserStatus::NEED_RETURN_TYPE);
    ES2PANDA_ASSERT(func != nullptr);

    func->SetIdent(identNode);
    func->SetStart(startLoc);
    auto *funcDecl = AllocNode<ir::FunctionDeclaration>(Allocator(), func);
    ES2PANDA_ASSERT(funcDecl != nullptr);
    funcDecl->SetRange(func->Range());

    if (func->IsOverload() && lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_SEMI_COLON) {
        lexer_->NextToken();
    }

    return funcDecl;
}

ir::Statement *ParserImpl::ParseExpressionStatement(StatementParsingFlags flags)
{
    if (Lexer()->GetToken().KeywordType() == lexer::TokenType::KEYW_INIT_MODULE && IsETSParser()) {
        return ParseInitModuleStatement();
    }
    const auto startPos = lexer_->Save();
    ParserStatus savedStatus = context_.Status();
    auto tokenType = lexer_->GetToken().Type();
    if (tokenType == lexer::TokenType::KEYW_PUBLIC || tokenType == lexer::TokenType::KEYW_PRIVATE ||
        tokenType == lexer::TokenType::KEYW_PROTECTED) {
        lexer_->NextToken();
        if (lexer_->GetToken().Type() == lexer::TokenType::KEYW_CLASS ||
            lexer_->GetToken().Type() == lexer::TokenType::KEYW_INTERFACE) {
            LogError(diagnostic::LOCAL_CLASS_ACCESS_MOD, {}, startPos.GetToken().Start());
            // Suppose it is a class declaration.
            return ParseClassDeclaration(ir::ClassDefinitionModifiers::NONE);
        }
        if (lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_AT) {
            LogError(diagnostic::ANNOTATION_DECLARATION_ACCESS_MODIFIER, {}, startPos.GetToken().Start());
        }
        lexer_->Rewind(startPos);
    }

    if (lexer_->GetToken().IsAsyncModifier()) {
        lexer_->NextToken();

        if (lexer_->GetToken().Type() == lexer::TokenType::KEYW_FUNCTION && !lexer_->GetToken().NewLine()) {
            if ((flags & StatementParsingFlags::ALLOW_LEXICAL) == 0) {
                LogError(diagnostic::LEXICAL_DEC_NOT_ALLOWED_IN_SINGLE_STATEMENT_CONTEXT);
            }

            ir::FunctionDeclaration *functionDecl = ParseFunctionDeclaration(false, ParserStatus::ASYNC_FUNCTION);
            functionDecl->SetStart(startPos.GetToken().Start());

            return functionDecl;
        }

        lexer_->Rewind(startPos);
    }

    ir::Expression *exprNode = ParseExpression(ExpressionParseFlags::ACCEPT_COMMA);

    context_.Status() = savedStatus;
    lexer::SourcePosition endPos = exprNode->End();

    auto *exprStatementNode = AllocNode<ir::ExpressionStatement>(exprNode);
    ES2PANDA_ASSERT(exprStatementNode != nullptr);
    exprStatementNode->SetRange({startPos.GetToken().Start(), endPos});
    ConsumeSemicolon(exprStatementNode);

    return exprStatementNode;
}

// NOLINTBEGIN(cert-err58-cpp)
static std::string const INVALID_LEFT_HAND_IN_FOR_OF =
    "Invalid left-hand side in 'for' statement: must have a single binding."s;
static std::string const MISSING_LEFT_IN_FOR = "Missing left parenthesis in 'for' statement."s;
static std::string const MISSING_RIGHT_IN_FOR = "Missing right parenthesis in 'for' statement."s;
static std::string const INVALID_TYPE_ANNOTATION_IN_FOR =
    "Type annotation is not allowed when existing variable is used as loop iterator in 'for' statement."s;
// NOLINTEND(cert-err58-cpp)

std::tuple<ForStatementKind, ir::Expression *, ir::Expression *> ParserImpl::ParseForInOf(
    ir::AstNode *initNode, ExpressionParseFlags exprFlags, bool isAwait)
{
    ForStatementKind forKind = ForStatementKind::UPDATE;
    ir::Expression *updateNode = nullptr;
    ir::Expression *rightNode = nullptr;

    if (lexer_->GetToken().IsForInOf()) {
        ES2PANDA_ASSERT(initNode != nullptr);
        if (!initNode->IsVariableDeclaration()) {
            LogError(diagnostic::INVALID_LEFT_HAND_IN_FOR_OF, {}, lexer_->GetToken().Start());
            return {forKind, rightNode, updateNode};
        }
        if (initNode->AsVariableDeclaration()->Declarators().empty()) {
            LogError(diagnostic::INVALID_LEFT_HAND_IN_FOR_OF, {}, initNode->Start());
            return {forKind, rightNode, updateNode};
        }
        const ir::VariableDeclarator *varDecl = initNode->AsVariableDeclaration()->Declarators().front();

        if (lexer_->GetToken().KeywordType() == lexer::TokenType::KEYW_IN) {
            if (varDecl->Init() != nullptr) {
                LogError(diagnostic::FOR_IN_LOOP_HAS_INIT);
            }
            forKind = ForStatementKind::IN;
            exprFlags = ExpressionParseFlags::ACCEPT_COMMA;
            ValidateForInStatement();
        } else {
            if (varDecl->Init() != nullptr) {
                LogError(diagnostic::FOR_OF_LOOP_HAS_INIT);
            }

            forKind = ForStatementKind::OF;
        }

        lexer_->NextToken();
        rightNode = ParseExpression(exprFlags);
    } else {
        if (IsETSParser()) {
            ExpectToken(lexer::TokenType::PUNCTUATOR_SEMI_COLON);
        }
        if (isAwait) {
            LogUnexpectedToken(Lexer()->GetToken());
        }

        if (lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_RIGHT_PARENTHESIS) {
            LogError(diagnostic::INVALID_LEFT_FOR_IN_OF);
            return {forKind, rightNode, updateNode};
        }

        if (lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_SEMI_COLON) {
            lexer_->NextToken();
        } else {
            rightNode = ParseExpression(ExpressionParseFlags::ACCEPT_COMMA | ExpressionParseFlags::IN_FOR);
            if (lexer_->GetToken().Type() != lexer::TokenType::PUNCTUATOR_SEMI_COLON) {
                LogExpectedToken(lexer::TokenType::PUNCTUATOR_SEMI_COLON);
            }
            lexer_->NextToken();
        }

        if (lexer_->GetToken().Type() != lexer::TokenType::PUNCTUATOR_RIGHT_PARENTHESIS) {
            updateNode = ParseExpression(ExpressionParseFlags::ACCEPT_COMMA | ExpressionParseFlags::IN_FOR);
        }
    }

    return {forKind, rightNode, updateNode};
}

std::tuple<ForStatementKind, ir::AstNode *, ir::Expression *, ir::Expression *> ParserImpl::ParseIsForInOf(
    ir::Expression *leftNode, ExpressionParseFlags exprFlags)
{
    ES2PANDA_ASSERT(lexer_->GetToken().IsForInOf());
    ForStatementKind forKind = ForStatementKind::OF;

    if (lexer_->GetToken().Type() == lexer::TokenType::KEYW_IN) {
        forKind = ForStatementKind::IN;
        exprFlags = ExpressionParseFlags::ACCEPT_COMMA;
        ValidateForInStatement();
    }

    bool isValid = true;
    switch (leftNode->Type()) {
        case ir::AstNodeType::IDENTIFIER:
        case ir::AstNodeType::MEMBER_EXPRESSION: {
            break;
        }
        case ir::AstNodeType::ARRAY_EXPRESSION: {
            isValid = leftNode->AsArrayExpression()->ConvertibleToArrayPattern();
            break;
        }
        case ir::AstNodeType::OBJECT_EXPRESSION: {
            isValid = leftNode->AsObjectExpression()->ConvertibleToObjectPattern();
            break;
        }
        default: {
            isValid = false;
        }
    }

    if (!isValid) {
        ValidateLvalueAssignmentTarget(leftNode);
    }

    lexer_->NextToken();

    return {forKind, leftNode, ParseExpression(exprFlags), nullptr};
}

std::tuple<ForStatementKind, ir::AstNode *, ir::Expression *, ir::Expression *> ParserImpl::ParseForInOf(
    ir::Expression *leftNode, ExpressionParseFlags exprFlags, bool isAwait)
{
    ES2PANDA_ASSERT(lexer_ != nullptr);
    ES2PANDA_ASSERT(leftNode != nullptr);
    ir::Expression *updateNode = nullptr;
    ir::Expression *rightNode = nullptr;
    if (lexer_->GetToken().IsForInOf()) {
        return ParseIsForInOf(leftNode, exprFlags);
    }

    if (isAwait) {
        LogUnexpectedToken(Lexer()->GetToken());
    }

    ir::Expression *expr = ParseAssignmentExpression(leftNode);

    ir::AstNode *initNode =
        lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_COMMA ? ParseSequenceExpression(expr) : expr;
    ES2PANDA_ASSERT(initNode != nullptr);

    if (initNode->IsConditionalExpression()) {
        ir::ConditionalExpression *condExpr = initNode->AsConditionalExpression();
        if (condExpr->Alternate()->IsBinaryExpression() &&
            condExpr->Alternate()->AsBinaryExpression()->OperatorType() == lexer::TokenType::KEYW_IN) {
            LogError(diagnostic::INVALID_LEFT_FOR_IN);
            rightNode = AllocBrokenExpression(Lexer()->GetToken().Loc());
            updateNode = AllocBrokenExpression(Lexer()->GetToken().Loc());
            // CC-OFFNXT(G.FMT.03-CPP) project code style
            return {ForStatementKind::IN, initNode, rightNode, updateNode};
        }
    }

    if (lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_RIGHT_PARENTHESIS) {
        rightNode = AllocBrokenExpression(Lexer()->GetToken().Loc());
        updateNode = AllocBrokenExpression(Lexer()->GetToken().Loc());
        LogError(diagnostic::INVALID_LEFT_FOR_IN_OF);
        return {ForStatementKind::UPDATE, initNode, rightNode, updateNode};
    }

    ExpectToken(lexer::TokenType::PUNCTUATOR_SEMI_COLON);

    if (lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_SEMI_COLON) {
        lexer_->NextToken();
    } else {
        rightNode = ParseExpression(ExpressionParseFlags::ACCEPT_COMMA | ExpressionParseFlags::IN_FOR);
        ExpectToken(lexer::TokenType::PUNCTUATOR_SEMI_COLON);
    }

    if (lexer_->GetToken().Type() != lexer::TokenType::PUNCTUATOR_RIGHT_PARENTHESIS) {
        updateNode = ParseExpression(ExpressionParseFlags::ACCEPT_COMMA | ExpressionParseFlags::IN_FOR);
    }

    return {ForStatementKind::UPDATE, initNode, rightNode, updateNode};
}

std::tuple<ir::Expression *, ir::Expression *> ParserImpl::ParseForUpdate(bool isAwait)
{
    if (isAwait) {
        LogUnexpectedToken(Lexer()->GetToken());
    }

    ir::Expression *updateNode = nullptr;
    ir::Expression *rightNode = nullptr;

    if (lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_SEMI_COLON) {
        lexer_->NextToken();
    } else {
        rightNode = ParseExpression(ExpressionParseFlags::ACCEPT_COMMA | ExpressionParseFlags::IN_FOR);
        ExpectToken(lexer::TokenType::PUNCTUATOR_SEMI_COLON);
    }

    if (lexer_->GetToken().Type() != lexer::TokenType::PUNCTUATOR_RIGHT_PARENTHESIS) {
        updateNode = ParseExpression(ExpressionParseFlags::ACCEPT_COMMA | ExpressionParseFlags::IN_FOR);
    }

    return {rightNode, updateNode};
}

std::tuple<ir::Expression *, ir::Statement *> ParserImpl::ParseForLoopInitializer()
{
    VariableParsingFlags varFlags = VariableParsingFlags::IN_FOR;

    if (lexer_->GetToken().Type() == lexer::TokenType::KEYW_AWAIT) {
        varFlags |= VariableParsingFlags::DISALLOW_INIT;
        lexer_->NextToken();
    }

    ExpectToken(lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS);

    lexer::TokenType tokenType;
    auto const currentPosition = lexer_->Save();
    do {
        tokenType = lexer_->GetToken().Type();
        if (lexer_->GetToken().KeywordType() == lexer::TokenType::KEYW_OF) {
            varFlags |= VariableParsingFlags::FOR_OF;
            break;
        }
        if (tokenType == lexer::TokenType::KEYW_IN) {
            varFlags |= VariableParsingFlags::STOP_AT_IN;
            break;
        }
        lexer_->NextToken();
    } while (tokenType != lexer::TokenType::PUNCTUATOR_RIGHT_PARENTHESIS &&
             tokenType != lexer::TokenType::PUNCTUATOR_LEFT_BRACE &&
             tokenType != lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS && tokenType != lexer::TokenType::EOS);
    lexer_->Rewind(currentPosition);

    switch (lexer_->GetToken().Type()) {
        case lexer::TokenType::KEYW_VAR:
            return {nullptr, ParseVariableDeclaration(varFlags | VariableParsingFlags::VAR)};
        case lexer::TokenType::KEYW_LET:
            return {nullptr, ParseVariableDeclaration(varFlags | VariableParsingFlags::LET)};
        case lexer::TokenType::KEYW_CONST: {
            return {nullptr, ParseVariableDeclaration(varFlags | VariableParsingFlags::CONST |
                                                      VariableParsingFlags::ACCEPT_CONST_NO_INIT)};
        }
        case lexer::TokenType::PUNCTUATOR_SEMI_COLON:
            if ((varFlags & VariableParsingFlags::DISALLOW_INIT) != 0 /*isAsync*/) {
                LogUnexpectedToken(Lexer()->GetToken());
            }
            lexer_->NextToken();
            return {nullptr, nullptr};
        default:
            return {ParseUnaryOrPrefixUpdateExpression(ExpressionParseFlags::POTENTIALLY_IN_PATTERN), nullptr};
    }
}
bool ParserImpl::GetCanBeForInOf(ir::Expression *leftNode, ir::AstNode *initNode)
{
    bool statementIsCorrect = initNode != nullptr && !initNode->IsBrokenStatement();
    bool expressionIsCorrect = leftNode != nullptr && !leftNode->IsBrokenExpression();
    bool canBeForInOf = expressionIsCorrect || statementIsCorrect;
    if (statementIsCorrect) {
        if (lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_SEMI_COLON) {
            lexer_->NextToken();
            canBeForInOf = false;
        } else if (!initNode->IsVariableDeclaration()) {
            LogError(diagnostic::INVALID_LEFT_HAND_IN_FOR_OF);
        } else if (initNode->AsVariableDeclaration()->Declarators().size() > 1 && lexer_->GetToken().IsForInOf()) {
            LogError(diagnostic::INVALID_LEFT_HAND_IN_FOR_OF, {},
                     initNode->AsVariableDeclaration()->Declarators()[1]->Start());
        }
    }
    return canBeForInOf;
}

struct ForStatementNodes {
    ir::AstNode *init;
    ir::Expression *right;
    ir::Expression *update;
    ir::Statement *body;
};

ir::Statement *ParserImpl::CreateForStatement(ForStatementNodes &&nodes, ForStatementKind forKind,
                                              const lexer::SourcePosition &startLoc, bool isAwait)
{
    ES2PANDA_ASSERT(nodes.body != nullptr);
    ir::Statement *forStatement = nullptr;

    if (forKind == ForStatementKind::UPDATE) {
        forStatement = AllocNode<ir::ForUpdateStatement>(nodes.init, nodes.right, nodes.update, nodes.body);
    } else {
        if (nodes.init == nullptr || nodes.right == nullptr) {  // Error processing.
            // Error is logged inside ParseForLoopInitializer or ParseForUpdate, or ParseForInOf
            return AllocBrokenStatement(Lexer()->GetToken().Loc());
        }

        if (forKind == ForStatementKind::IN) {
            forStatement = AllocNode<ir::ForInStatement>(nodes.init, nodes.right, nodes.body);
        } else {
            forStatement = AllocNode<ir::ForOfStatement>(nodes.init, nodes.right, nodes.body, isAwait);
        }
    }

    ES2PANDA_ASSERT(forStatement != nullptr);
    forStatement->SetRange({startLoc, nodes.body->End()});

    return forStatement;
}

ir::Statement *ParserImpl::ParseForStatement()
{
    lexer::SourcePosition startLoc = lexer_->GetToken().Start();
    ForStatementKind forKind = ForStatementKind::UPDATE;
    ir::AstNode *initNode = nullptr;
    ir::Expression *updateNode = nullptr;
    ir::Expression *leftNode = nullptr;
    ir::Expression *rightNode = nullptr;

    lexer_->NextToken();
    bool isAwait = lexer_->GetToken().Type() == lexer::TokenType::KEYW_AWAIT;
    std::tie(leftNode, initNode) = ParseForLoopInitializer();

    IterationContext iterCtx(&context_);

    // VariableDeclaration->DeclarationSize > 1 or seen semi_colon
    if (!GetCanBeForInOf(leftNode, initNode)) {
        std::tie(rightNode, updateNode) = ParseForUpdate(isAwait);
    } else if (leftNode != nullptr) {
        // initNode was parsed as LHS
        if (lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_COLON) {
            LogError(diagnostic::INVALID_TYPE_ANNOTATION_IN_FOR);
            lexer_->NextToken();  // eat ':'
        }
        std::tie(forKind, initNode, rightNode, updateNode) =
            ParseForInOf(leftNode, ExpressionParseFlags::NO_OPTS, isAwait);
    } else if (initNode != nullptr) {
        // initNode was parsed as VariableDeclaration and declaration size = 1
        std::tie(forKind, rightNode, updateNode) = ParseForInOf(initNode, ExpressionParseFlags::NO_OPTS, isAwait);
    }

    if (lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_EXCLAMATION_MARK) {
        lexer_->NextToken();
    }
    ExpectToken(lexer::TokenType::PUNCTUATOR_RIGHT_PARENTHESIS);

    ir::Statement *bodyNode = ParseStatement();
    if (bodyNode == nullptr) {
        return AllocBrokenStatement(Lexer()->GetToken().Loc());
    }

    return CreateForStatement({initNode, rightNode, updateNode, bodyNode}, forKind, startLoc, isAwait);
}

void ParserImpl::ReportIfBodyEmptyError([[maybe_unused]] ir::Statement *consequent) {}

ir::Statement *ParserImpl::ParseIfStatement()
{
    lexer::SourcePosition startLoc = lexer_->GetToken().Start();
    lexer::SourcePosition endLoc;
    lexer_->NextToken();

    ExpectToken(lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS);
    ir::Expression *test = ParseExpression(ExpressionParseFlags::ACCEPT_COMMA);

    ExpectToken(lexer::TokenType::PUNCTUATOR_RIGHT_PARENTHESIS);
    ir::Statement *consequent = ParseStatement(StatementParsingFlags::IF_ELSE | StatementParsingFlags::ALLOW_LEXICAL);
    ES2PANDA_ASSERT(consequent != nullptr);

    ReportIfBodyEmptyError(consequent);
    endLoc = consequent->End();
    ir::Statement *alternate = nullptr;

    if (lexer_->GetToken().Type() == lexer::TokenType::KEYW_ELSE) {
        lexer_->NextToken();  // eat ELSE keyword
        alternate = ParseStatement(StatementParsingFlags::IF_ELSE | StatementParsingFlags::ALLOW_LEXICAL);
        if (alternate != nullptr) {  // Error processing.
            endLoc = alternate->End();
        }
    }

    auto *ifStatement = AllocNode<ir::IfStatement>(test, consequent, alternate);
    ES2PANDA_ASSERT(ifStatement != nullptr);
    ifStatement->SetRange({startLoc, endLoc});
    return ifStatement;
}

ir::Statement *ParserImpl::ParseLabelledStatement(const lexer::LexerPosition &pos)
{
    const util::StringView &actualLabel = pos.GetToken().Ident();

    if (lexer_->GetToken().KeywordType() == lexer::TokenType::KEYW_AWAIT && context_.IsModule()) {
        LogError(diagnostic::AWAIT_RESERVED_IDENTIFIER_IN_MODULE, {}, pos.GetToken().Start());
    }

    if (context_.FindLabel(actualLabel) != nullptr) {
        LogError(diagnostic::LABEL_ALREADY_DECLARED, {}, pos.GetToken().Start());
    }

    SavedParserContext newCtx(this, ParserStatus::IN_LABELED, actualLabel);

    auto *identNode = AllocNode<ir::Identifier>(actualLabel, Allocator());
    ES2PANDA_ASSERT(identNode != nullptr);
    identNode->SetRange(pos.GetToken().Loc());

    lexer_->NextToken();

    if (!ValidateLabeledStatement(Lexer()->GetToken().Type())) {
        return AllocBrokenStatement(Lexer()->GetToken().Loc());  // Error processing.
    }

    ir::Statement *body = ParseStatement(StatementParsingFlags::LABELLED);

    auto *labeledStatement = AllocNode<ir::LabelledStatement>(identNode, body);
    ES2PANDA_ASSERT(labeledStatement != nullptr);
    labeledStatement->SetRange({pos.GetToken().Start(), body->End()});

    return labeledStatement;
}

ir::Statement *ParserImpl::ParseReturnStatement()
{
    bool inClassInitializer =
        (context_.Status() & ParserStatus::IN_CLASS_BODY) != 0 && (context_.Status() & ParserStatus::STATIC_BLOCK) != 0;
    if ((context_.Status() & ParserStatus::FUNCTION) == 0 &&
        (context_.Status() & ParserStatus::PARSE_TRAILING_BLOCK) == 0 && !(inClassInitializer && IsETSParser())) {
        LogError(diagnostic::RETURN_IN_FUN_BODY);
    }

    lexer::SourcePosition startLoc = lexer_->GetToken().Start();
    lexer::SourcePosition endLoc = lexer_->GetToken().End();
    lexer_->NextToken();

    bool hasArgument = (lexer_->GetToken().Type() != lexer::TokenType::PUNCTUATOR_SEMI_COLON &&
                        lexer_->GetToken().Type() != lexer::TokenType::PUNCTUATOR_RIGHT_BRACE &&
                        lexer_->GetToken().Type() != lexer::TokenType::EOS && !lexer_->GetToken().NewLine());

    ir::ReturnStatement *returnStatement = nullptr;

    if (hasArgument) {
        if (inClassInitializer) {
            LogError(diagnostic::RETURN_WITH_VALUE);
        }
        ir::Expression *expression = ParseExpression(ExpressionParseFlags::ACCEPT_COMMA);
        if (expression != nullptr) {  // Error processing.
            endLoc = expression->End();
        }

        // returnStatement will have void return type if expression == nullptr
        returnStatement = AllocNode<ir::ReturnStatement>(expression);
    } else {
        returnStatement = AllocNode<ir::ReturnStatement>();
    }

    ES2PANDA_ASSERT(returnStatement != nullptr);
    returnStatement->SetRange({startLoc, endLoc});
    ConsumeSemicolon(returnStatement);

    context_.Status() |= ParserStatus::FUNCTION_HAS_RETURN_STATEMENT;
    return returnStatement;
}

void ParserImpl::ReportMultipleDefaultError()
{
    LogError(diagnostic::MULTIPLE_DEFAULT);
}

ir::SwitchCaseStatement *ParserImpl::ParseSwitchCaseStatement(bool *seenDefault)
{
    auto const &token = lexer_->GetToken();
    lexer::SourcePosition caseStartLoc = token.Start();
    ir::Expression *testExpr = nullptr;

    switch (token.KeywordType()) {
        case lexer::TokenType::KEYW_CASE: {
            lexer_->NextToken();  // eat 'case'
            testExpr = ParseExpression(ExpressionParseFlags::ACCEPT_COMMA);
            break;
        }
        case lexer::TokenType::KEYW_DEFAULT: {
            if (*seenDefault) {
                ReportMultipleDefaultError();
            }
            *seenDefault = true;
            lexer_->NextToken();  // eat 'default'
            break;
        }
        default: {
            LogError(diagnostic::UNEXPECTED_TOKEN_PARAM_EXPECTED_CASE_OR_DEFAULT,
                     {lexer::TokenToString(lexer_->GetToken().Type())});
            testExpr = AllocBrokenExpression(caseStartLoc);
        }
    }

    lexer::SourcePosition caseEndLoc = lexer_->GetToken().End();

    ExpectToken(lexer::TokenType::PUNCTUATOR_COLON);

    ArenaVector<ir::Statement *> consequents(Allocator()->Adapter());
    while (lexer_->GetToken().Type() != lexer::TokenType::KEYW_CASE &&
           lexer_->GetToken().KeywordType() != lexer::TokenType::KEYW_DEFAULT &&
           lexer_->GetToken().Type() != lexer::TokenType::PUNCTUATOR_RIGHT_BRACE &&
           lexer_->GetToken().Type() != lexer::TokenType::EOS) {
        util::ErrorRecursionGuard infiniteLoopBlocker(Lexer());
        ir::Statement *consequent = ParseStatement(StatementParsingFlags::ALLOW_LEXICAL);
        ES2PANDA_ASSERT(consequent != nullptr);
        caseEndLoc = consequent->End();
        consequents.push_back(consequent);
    }

    auto *caseNode = AllocNode<ir::SwitchCaseStatement>(testExpr, std::move(consequents));
    ES2PANDA_ASSERT(caseNode != nullptr);
    caseNode->SetRange({caseStartLoc, caseEndLoc});
    return caseNode;
}

ir::Statement *ParserImpl::ParseSwitchStatement()
{
    lexer::SourcePosition startLoc = lexer_->GetToken().Start();
    ExpectToken(lexer::TokenType::KEYW_SWITCH);
    ExpectToken(lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS);
    ir::Expression *discriminant = ParseExpression(ExpressionParseFlags::ACCEPT_COMMA);
    ExpectToken(lexer::TokenType::PUNCTUATOR_RIGHT_PARENTHESIS);

    SwitchContext switchContext(&context_);
    ExpectToken(lexer::TokenType::PUNCTUATOR_LEFT_BRACE);

    bool seenDefault = false;
    ArenaVector<ir::SwitchCaseStatement *> cases(Allocator()->Adapter());

    while (lexer_->GetToken().Type() != lexer::TokenType::PUNCTUATOR_RIGHT_BRACE &&
           lexer_->GetToken().Type() != lexer::TokenType::EOS) {
        util::ErrorRecursionGuard infiniteLoopBlocker(lexer_);
        auto caseStatement = ParseSwitchCaseStatement(&seenDefault);
        if (caseStatement != nullptr) {
            cases.push_back(caseStatement);
        }
    }

    auto endLoc = lexer_->GetToken().End();
    ExpectToken(lexer::TokenType::PUNCTUATOR_RIGHT_BRACE);

    auto *switchStatement = AllocNode<ir::SwitchStatement>(discriminant, std::move(cases));
    ES2PANDA_ASSERT(switchStatement != nullptr);
    switchStatement->SetRange({startLoc, endLoc});
    return switchStatement;
}

void ParserImpl::ReportIllegalNewLineErrorAfterThrow()
{
    LogError(diagnostic::ILLEGAL_NEW_LINE);
}

ir::Statement *ParserImpl::ParseThrowStatement()
{
    lexer::SourcePosition startLoc = lexer_->GetToken().Start();
    lexer_->NextToken();

    if (lexer_->GetToken().NewLine()) {
        ReportIllegalNewLineErrorAfterThrow();
    }

    ir::Expression *expression = ParseExpression(ExpressionParseFlags::ACCEPT_COMMA);
    ES2PANDA_ASSERT(expression != nullptr);

    lexer::SourcePosition endLoc = expression->End();

    auto *throwStatement = AllocNode<ir::ThrowStatement>(expression);
    ES2PANDA_ASSERT(throwStatement != nullptr);
    throwStatement->SetRange({startLoc, endLoc});
    ConsumeSemicolon(throwStatement);

    context_.Status() |= ParserStatus::FUNCTION_HAS_THROW_STATEMENT;
    return throwStatement;
}

void ParserImpl::ParseCatchParamTypeAnnotation([[maybe_unused]] ir::AnnotatedExpression *param) {}

ir::Expression *ParserImpl::ParseCatchParam()
{
    ir::AnnotatedExpression *param = nullptr;

    if (lexer_->GetToken().Type() != lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS) {
        return param;
    }

    lexer_->NextToken();  // eat '('

    if (auto const tokenType = lexer_->GetToken().Type(); tokenType == lexer::TokenType::LITERAL_IDENT) {
        CheckRestrictedBinding();
        param = ExpectIdentifier();
    } else if (tokenType == lexer::TokenType::PUNCTUATOR_LEFT_SQUARE_BRACKET) {
        param = ParseArrayExpression(ExpressionParseFlags::MUST_BE_PATTERN);
    } else if (tokenType == lexer::TokenType::PUNCTUATOR_LEFT_BRACE) {
        param = ParseObjectExpression(ExpressionParseFlags::MUST_BE_PATTERN);
    } else {
        LogError(diagnostic::UNEXPECTED_TOKEN_PARAM, {lexer::TokenToString(lexer_->GetToken().Type())});
    }

    if (param != nullptr) {  // Error processing.
        ParseCatchParamTypeAnnotation(param);
    }

    ExpectToken(lexer::TokenType::PUNCTUATOR_RIGHT_PARENTHESIS);

    return param;
}

ir::CatchClause *ParserImpl::ParseCatchClause()
{
    lexer::SourcePosition catchStartLoc = lexer_->GetToken().Start();
    lexer_->NextToken();  // eat 'catch' keyword

    ir::Expression *param = ParseCatchParam();

    if (lexer_->GetToken().Type() != lexer::TokenType::PUNCTUATOR_LEFT_BRACE) {
        LogError(diagnostic::EXPECTED_PARAM_GOT_PARAM, {"{", lexer::TokenToString(lexer_->GetToken().Type())});
    }

    ir::BlockStatement *catchBlock = ParseBlockStatement();
    ES2PANDA_ASSERT(catchBlock != nullptr);
    lexer::SourcePosition endLoc = catchBlock->End();

    auto *catchClause = AllocNode<ir::CatchClause>(param, catchBlock);
    ES2PANDA_ASSERT(catchClause != nullptr);
    catchClause->SetRange({catchStartLoc, endLoc});

    return catchClause;
}

ir::Statement *ParserImpl::ParseTryStatement()
{
    lexer::SourcePosition startLoc = lexer_->GetToken().Start();
    lexer::SourcePosition endLoc = lexer_->GetToken().End();

    lexer_->NextToken();  // eat 'try'

    if (lexer_->GetToken().Type() != lexer::TokenType::PUNCTUATOR_LEFT_BRACE) {
        LogError(diagnostic::EXPECTED_PARAM_GOT_PARAM, {"{", lexer::TokenToString(lexer_->GetToken().Type())});
    }

    ir::BlockStatement *body = ParseBlockStatement();

    if (lexer_->GetToken().Type() != lexer::TokenType::KEYW_CATCH &&
        lexer_->GetToken().Type() != lexer::TokenType::KEYW_FINALLY) {
        /*  If we are here, there are two options:
            1. there is neither 'catch', nor 'finally';
            2. we made typo and got identifier instead of a first keyword after Try statement.
            So, this check does not work if we write Try, Catch statements and then make a mistake
            -> only setting current token as 'catch' is a possible option to fix most cases
            Test exixts for ts extension only: catch_or_finally_1.ts
        */
        LogError(diagnostic::MISSING_CATCH_OR_FINALLY);
    }

    ir::CatchClause *catchClause = nullptr;
    ir::BlockStatement *finallyClause = nullptr;
    ArenaVector<ir::CatchClause *> catchClauses(Allocator()->Adapter());

    while (lexer_->GetToken().Type() == lexer::TokenType::KEYW_CATCH) {
        catchClause = ParseCatchClause();
        ES2PANDA_ASSERT(catchClause != nullptr);
        endLoc = catchClause->End();
        catchClauses.push_back(catchClause);
    }

    if (lexer_->GetToken().Type() == lexer::TokenType::KEYW_FINALLY) {
        lexer_->NextToken();  // eat 'finally' keyword

        finallyClause = ParseBlockStatement();
        ES2PANDA_ASSERT(finallyClause != nullptr);
        endLoc = finallyClause->End();
    }

    ArenaVector<std::pair<compiler::LabelPair, const ir::Statement *>> finalizerInsertions(Allocator()->Adapter());

    auto *tryStatement = AllocNode<ir::TryStatement>(body, std::move(catchClauses), finallyClause, finalizerInsertions);
    ES2PANDA_ASSERT(tryStatement != nullptr);
    tryStatement->SetRange({startLoc, endLoc});
    return tryStatement;
}

void ParserImpl::ValidateDeclaratorId()
{
    if (InAmbientContext()) {
        return;
    }

    CheckRestrictedBinding();
}

ir::VariableDeclarator *ParserImpl::ParseVariableDeclaratorInitializer(ir::Expression *init, VariableParsingFlags flags,
                                                                       const lexer::SourcePosition &startLoc)
{
    if ((flags & VariableParsingFlags::DISALLOW_INIT) != 0) {
        LogError(diagnostic::FOR_AWAIT_OF_VAR_NOT_INIT);
    }

    lexer_->NextToken();

    if (InAmbientContext() && (flags & VariableParsingFlags::CONST) == 0) {
        LogError(diagnostic::INITIALIZERS_IN_AMBIENT_CONTEXTS);
    }

    auto exprFlags = ((flags & VariableParsingFlags::STOP_AT_IN) != 0 ? ExpressionParseFlags::STOP_AT_IN
                                                                      : ExpressionParseFlags::NO_OPTS);

    ir::Expression *initializer = ParseExpression(exprFlags);
    if (initializer == nullptr) {  // Error processing.
        return nullptr;
    }

    lexer::SourcePosition endLoc = initializer->End();

    auto *declarator = AllocNode<ir::VariableDeclarator>(GetFlag(flags), init, initializer);
    declarator->SetRange({startLoc, endLoc});

    return declarator;
}

ir::Expression *ParserImpl::ParseVariableDeclaratorKey([[maybe_unused]] VariableParsingFlags flags)
{
    switch (lexer_->GetToken().Type()) {
        case lexer::TokenType::LITERAL_IDENT: {
            ValidateDeclaratorId();
            return ExpectIdentifier(true);
        }
        case lexer::TokenType::PUNCTUATOR_LEFT_SQUARE_BRACKET: {
            return ParseArrayExpression(ExpressionParseFlags::MUST_BE_PATTERN);
        }
        case lexer::TokenType::PUNCTUATOR_LEFT_BRACE: {
            return ParseObjectExpression(ExpressionParseFlags::MUST_BE_PATTERN);
        }
        default: {
            break;
        }
    }

    LogError(diagnostic::UNEXPECTED_TOKEN);
    return nullptr;
}

ir::VariableDeclaratorFlag ParserImpl::GetFlag(VariableParsingFlags flags)
{
    constexpr auto VARIABLE_FLAGS =
        (VariableParsingFlags::VAR | VariableParsingFlags::LET | VariableParsingFlags::CONST);
    switch (static_cast<VariableParsingFlags>(flags & VARIABLE_FLAGS)) {
        case VariableParsingFlags::CONST:
            return ir::VariableDeclaratorFlag::CONST;
        case VariableParsingFlags::VAR:
            return ir::VariableDeclaratorFlag::VAR;
        case VariableParsingFlags::LET:
            return ir::VariableDeclaratorFlag::LET;
        default:
            ES2PANDA_UNREACHABLE();
    }
}

ir::VariableDeclarator *ParserImpl::ParseVariableDeclarator(ir::Expression *init, lexer::SourcePosition startLoc,
                                                            VariableParsingFlags flags)
{
    if (lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_SUBSTITUTION) {
        return ParseVariableDeclaratorInitializer(init, flags, startLoc);
    }

    if ((flags & VariableParsingFlags::CONST) != 0U && (flags & VariableParsingFlags::ACCEPT_CONST_NO_INIT) == 0U) {
        LogError(diagnostic::MISSING_INIT_OR_CONST_DEC);
    }

    if ((flags & VariableParsingFlags::IN_FOR) == 0U && (init->IsArrayPattern() || init->IsObjectPattern())) {
        LogError(diagnostic::MISSING_INIT_IN_DEST_DEC);
    }

    lexer::SourcePosition endLoc = init->End();
    auto declarator = AllocNode<ir::VariableDeclarator>(GetFlag(flags), init);
    declarator->SetRange({startLoc, endLoc});

    return declarator;
}

ir::VariableDeclarator *ParserImpl::ParseVariableDeclarator(VariableParsingFlags flags)
{
    lexer::SourcePosition startLoc = lexer_->GetToken().Start();
    ir::Expression *init = ParseVariableDeclaratorKey(flags);
    if (init == nullptr) {  // Error processing.
        return nullptr;
    }

    return ParseVariableDeclarator(init, startLoc, flags);
}

ir::Statement *ParserImpl::ParsePotentialConstEnum([[maybe_unused]] VariableParsingFlags flags)
{
    LogError(diagnostic::VAR_DEC_EXPECTED);
    return ParseEnumDeclaration(true);
}

void ParserImpl::ReportIfVarDeclaration([[maybe_unused]] VariableParsingFlags flags) {}

ir::Statement *ParserImpl::ParseVariableDeclaration(VariableParsingFlags flags)
{
    lexer::SourcePosition startLoc = lexer_->GetToken().Start();

    if ((flags & VariableParsingFlags::NO_SKIP_VAR_KIND) == 0) {
        lexer_->NextToken();
    }

    ReportIfVarDeclaration(flags);

    if (lexer_->GetToken().Type() == lexer::TokenType::KEYW_ENUM) {
        return ParsePotentialConstEnum(flags);
    }

    ArenaVector<ir::VariableDeclarator *> declarators(Allocator()->Adapter());

    while (true) {
        ir::VariableDeclarator *declarator = ParseVariableDeclarator(flags);
        if (declarator != nullptr) {  // Error processing.
            declarators.push_back(declarator);
            if (declarator->Init() != nullptr && declarator->Init()->IsETSClassLiteral()) {
                ParseClassBody({});
                LogError(diagnostic::UNSUPPORTED_CLASS_LITERAL);
                return AllocBrokenStatement(declarator->Init()->Start());
            }
        }

        if (lexer_->GetToken().Type() != lexer::TokenType::PUNCTUATOR_COMMA) {
            break;
        }
        lexer_->NextToken();
    }

    if (declarators.empty()) {  // Error processing.
        // Error is logged inside ParseVariableDeclaratorKey or ParseVariableDeclaratorInitializer
        return AllocBrokenStatement(Lexer()->GetToken().Loc());
    }

    auto varKind = ir::VariableDeclaration::VariableDeclarationKind::VAR;

    if ((flags & VariableParsingFlags::LET) != 0) {
        varKind = ir::VariableDeclaration::VariableDeclarationKind::LET;
    } else if ((flags & VariableParsingFlags::CONST) != 0) {
        varKind = ir::VariableDeclaration::VariableDeclarationKind::CONST;
    }

    lexer::SourcePosition endLoc = declarators.back()->End();
    auto *declaration = AllocNode<ir::VariableDeclaration>(varKind, Allocator(), std::move(declarators));
    ES2PANDA_ASSERT(declaration != nullptr);
    declaration->SetRange({startLoc, endLoc});

    return declaration;
}

ir::Statement *ParserImpl::ParseWhileStatement()
{
    lexer::SourcePosition startLoc = lexer_->GetToken().Start();
    lexer_->NextToken();
    ExpectToken(lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS);

    ir::Expression *condition = ParseExpression(ExpressionParseFlags::ACCEPT_COMMA);

    ExpectToken(lexer::TokenType::PUNCTUATOR_RIGHT_PARENTHESIS);

    IterationContext iterCtx(&context_);
    ir::Statement *body = ParseStatement();
    if (IsBrokenStatement(body)) {
        LogError(diagnostic::MISSING_LOOP_BODY, {"while"});
    }

    if (condition->IsBrokenExpression()) {
        LogError(diagnostic::MISSING_LOOP_CONDITION, {"while"});
    }

    lexer::SourcePosition endLoc = body->End();
    auto *whileStatement = AllocNode<ir::WhileStatement>(condition, body);
    ES2PANDA_ASSERT(whileStatement != nullptr);
    whileStatement->SetRange({startLoc, endLoc});

    return whileStatement;
}

// NOLINTNEXTLINE(google-default-arguments)
ir::ExportDefaultDeclaration *ParserImpl::ParseExportDefaultDeclaration(const lexer::SourcePosition &startLoc,
                                                                        bool isExportEquals)
{
    lexer_->NextToken();  // eat `default` keyword or `=`

    ir::AstNode *declNode = nullptr;
    bool eatSemicolon = false;

    switch (lexer_->GetToken().Type()) {
        case lexer::TokenType::KEYW_FUNCTION:
            declNode = ParseFunctionDeclaration(true);
            break;
        case lexer::TokenType::KEYW_CLASS:
            declNode = ParseClassDeclaration(ir::ClassDefinitionModifiers::NONE);
            break;
        case lexer::TokenType::LITERAL_IDENT:
            switch (lexer_->GetToken().KeywordType()) {
                case lexer::TokenType::KEYW_STRUCT:
                    declNode = ParseStructDeclaration(ir::ClassDefinitionModifiers::NONE);
                    break;
                case lexer::TokenType::KEYW_ASYNC:
                    if ((lexer_->GetToken().Flags() & lexer::TokenFlags::HAS_ESCAPE) == 0) {
                        lexer_->NextToken();  // eat `async`
                        declNode = ParseFunctionDeclaration(false, ParserStatus::ASYNC_FUNCTION);
                        break;
                    }

                    [[fallthrough]];
                default:
                    declNode = ParseExpression();
                    eatSemicolon = true;
                    break;
            }

            break;
        default:
            declNode = ParseExpression();
            eatSemicolon = true;
            break;
    }

    ES2PANDA_ASSERT(declNode != nullptr);
    lexer::SourcePosition endLoc = declNode->End();
    auto *exportDeclaration = AllocNode<ir::ExportDefaultDeclaration>(declNode, isExportEquals);
    ES2PANDA_ASSERT(exportDeclaration != nullptr);
    exportDeclaration->SetRange({startLoc, endLoc});

    if (eatSemicolon) {
        ConsumeSemicolon(exportDeclaration);
    }

    return exportDeclaration;
}

ir::Identifier *ParserImpl::ParseNamedExport(lexer::Token *exportedToken)
{
    if (exportedToken->Type() != lexer::TokenType::LITERAL_IDENT) {
        // test exists for js and ts extensions
        LogError(diagnostic::UNEXPECTED_TOKEN_ID);
        exportedToken->SetTokenType(lexer::TokenType::LITERAL_IDENT);
        exportedToken->SetTokenStr(ERROR_LITERAL);
    }

    CheckRestrictedBinding(exportedToken->KeywordType());

    const util::StringView &exportedString = exportedToken->Ident();

    auto *exported = AllocNode<ir::Identifier>(exportedString, Allocator());
    ES2PANDA_ASSERT(exported != nullptr);
    exported->SetRange(exportedToken->Loc());

    return exported;
}

ir::ExportAllDeclaration *ParserImpl::ParseExportAllDeclaration(const lexer::SourcePosition &startLoc)
{
    lexer_->NextToken(lexer::NextTokenFlags::KEYWORD_TO_IDENT);  // eat `*` character

    ir::Identifier *exported = nullptr;

    if (CheckModuleAsModifier()) {
        lexer_->NextToken(lexer::NextTokenFlags::KEYWORD_TO_IDENT);
        exported = ParseNamedExport(&lexer_->GetToken());
        lexer_->NextToken();  // eat exported name
    }
    ir::StringLiteral *source = ParseFromClause();
    ES2PANDA_ASSERT(source != nullptr);
    lexer::SourcePosition endLoc = source->End();

    auto *exportDeclaration = AllocNode<ir::ExportAllDeclaration>(source, exported);
    ES2PANDA_ASSERT(exportDeclaration != nullptr);
    exportDeclaration->SetRange({startLoc, endLoc});

    ConsumeSemicolon(exportDeclaration);

    return exportDeclaration;
}

ir::ExportNamedDeclaration *ParserImpl::ParseExportNamedSpecifiers(const lexer::SourcePosition &startLoc)
{
    lexer_->NextToken(lexer::NextTokenFlags::KEYWORD_TO_IDENT);  // eat `{` character

    ArenaVector<ir::ExportSpecifier *> specifiers(Allocator()->Adapter());
    lexer::SourcePosition endPos;

    ParseList(
        lexer::TokenType::PUNCTUATOR_RIGHT_BRACE, lexer::NextTokenFlags::KEYWORD_TO_IDENT,
        [this, &specifiers](bool &) {
            if (lexer_->GetToken().Type() != lexer::TokenType::LITERAL_IDENT) {
                // test exists for ts extension only
                LogExpectedToken(lexer::TokenType::LITERAL_IDENT);
            }

            lexer::Token localToken = lexer_->GetToken();
            auto *local = AllocNode<ir::Identifier>(lexer_->GetToken().Ident(), Allocator());
            local->SetRange(lexer_->GetToken().Loc());

            lexer_->NextToken();  // eat local name

            ir::Identifier *exported = nullptr;

            if (CheckModuleAsModifier()) {
                lexer_->NextToken(lexer::NextTokenFlags::KEYWORD_TO_IDENT);  // eat `as` literal
                exported = ParseNamedExport(&lexer_->GetToken());
                lexer_->NextToken();  // eat exported name
            } else {
                exported = ParseNamedExport(&localToken);
            }

            if (exported != nullptr) {  // Error processing.
                auto *specifier = AllocNode<ir::ExportSpecifier>(local, exported);
                specifier->SetRange({local->Start(), exported->End()});
                specifiers.push_back(specifier);
            }
            return true;
        },
        &endPos, ParseListOptions::ALLOW_TRAILING_SEP);

    ir::StringLiteral *source = nullptr;

    if (lexer_->GetToken().KeywordType() == lexer::TokenType::KEYW_FROM) {
        source = ParseFromClause();
    }

    auto *exportDeclaration = AllocNode<ir::ExportNamedDeclaration>(Allocator(), source, std::move(specifiers));
    ES2PANDA_ASSERT(exportDeclaration != nullptr);
    exportDeclaration->SetRange({startLoc, endPos});
    ConsumeSemicolon(exportDeclaration);

    return exportDeclaration;
}

ir::Statement *ParserImpl::ParseNamedExportDeclaration(const lexer::SourcePosition &startLoc)
{
    ir::Statement *decl = nullptr;

    switch (lexer_->GetToken().Type()) {
        case lexer::TokenType::KEYW_VAR:
            decl = ParseVariableDeclaration(VariableParsingFlags::VAR);
            break;
        case lexer::TokenType::KEYW_CONST:
            decl = ParseVariableDeclaration(VariableParsingFlags::CONST);
            break;
        case lexer::TokenType::KEYW_LET:
            decl = ParseVariableDeclaration(VariableParsingFlags::LET);
            break;
        case lexer::TokenType::KEYW_FUNCTION:
            decl = ParseFunctionDeclaration(false, ParserStatus::NO_OPTS);
            break;
        case lexer::TokenType::KEYW_CLASS:
            decl = ParseClassDeclaration(ir::ClassDefinitionModifiers::ID_REQUIRED);
            break;
        case lexer::TokenType::LITERAL_IDENT:
            if (lexer_->GetToken().KeywordType() == lexer::TokenType::KEYW_STRUCT) {
                decl = ParseStructDeclaration(ir::ClassDefinitionModifiers::NONE);
                break;
            }
            [[fallthrough]];
        default:
            if (!lexer_->GetToken().IsAsyncModifier()) {
                LogExpectedToken(lexer::TokenType::KEYW_ASYNC);
            }

            lexer_->NextToken();  // eat `async` keyword
            if (lexer_->GetToken().Type() != lexer::TokenType::KEYW_FUNCTION) {
                LogExpectedToken(lexer::TokenType::KEYW_FUNCTION);
                return AllocBrokenStatement(Lexer()->GetToken().Loc());
            }
            decl = ParseFunctionDeclaration(false, ParserStatus::ASYNC_FUNCTION);
            break;
    }

    ES2PANDA_ASSERT(decl != nullptr);
    if (decl->IsVariableDeclaration()) {
        ConsumeSemicolon(decl);
    }

    lexer::SourcePosition endLoc = decl->End();
    ArenaVector<ir::ExportSpecifier *> specifiers(Allocator()->Adapter());
    auto *exportDeclaration = AllocNode<ir::ExportNamedDeclaration>(Allocator(), decl, std::move(specifiers));
    ES2PANDA_ASSERT(exportDeclaration != nullptr);
    exportDeclaration->SetRange({startLoc, endLoc});

    return exportDeclaration;
}

ir::Statement *ParserImpl::ParseExportDeclaration(StatementParsingFlags flags)
{
    if ((flags & StatementParsingFlags::GLOBAL) == 0) {
        LogError(diagnostic::IMPORT_EXPORT_ONLY_AT_TOP_LEVEL);
    }

    if (!context_.IsModule()) {
        LogError(diagnostic::IMPORT_META_ONLY_MODULE);
    }

    lexer::SourcePosition startLoc = lexer_->GetToken().Start();
    lexer_->NextToken();  // eat `export` keyword

    switch (lexer_->GetToken().Type()) {
        case lexer::TokenType::PUNCTUATOR_MULTIPLY: {
            return ParseExportAllDeclaration(startLoc);
        }
        case lexer::TokenType::PUNCTUATOR_LEFT_BRACE: {
            return ParseExportNamedSpecifiers(startLoc);
        }
        default: {
            if (lexer_->GetToken().KeywordType() == lexer::TokenType::KEYW_DEFAULT) {
                return ParseExportDefaultDeclaration(startLoc);
            }
            return ParseNamedExportDeclaration(startLoc);
        }
    }
}

void ParserImpl::ParseNameSpaceImport(ArenaVector<ir::AstNode *> *specifiers)
{
    lexer::SourcePosition namespaceStart = lexer_->GetToken().Start();
    lexer_->NextToken();  // eat `*` character

    if (!CheckModuleAsModifier()) {
        LogExpectedToken(lexer::TokenType::KEYW_AS);
    }

    lexer_->NextToken();  // eat `as` literal

    ir::Identifier *local = ParseNamedImport(&lexer_->GetToken());

    auto *specifier = AllocNode<ir::ImportNamespaceSpecifier>(local);
    ES2PANDA_ASSERT(specifier != nullptr);
    specifier->SetRange({namespaceStart, lexer_->GetToken().End()});
    specifiers->push_back(specifier);

    lexer_->NextToken();  // eat local name
}

ir::Identifier *ParserImpl::ParseNamedImport(lexer::Token *importedToken)
{
    if (importedToken->Type() != lexer::TokenType::LITERAL_IDENT) {
        LogError(diagnostic::UNEXPECTED_TOKEN_ID);
        importedToken->SetTokenType(lexer::TokenType::LITERAL_IDENT);
        importedToken->SetTokenStr(ERROR_LITERAL);
    }

    CheckRestrictedBinding(importedToken->KeywordType());

    auto *local = AllocNode<ir::Identifier>(importedToken->Ident(), Allocator());
    ES2PANDA_ASSERT(local != nullptr);
    local->SetRange(importedToken->Loc());

    return local;
}

void ParserImpl::ParseNamedImportSpecifiers(ArenaVector<ir::AstNode *> *specifiers)
{
    lexer_->NextToken(lexer::NextTokenFlags::KEYWORD_TO_IDENT);  // eat `{` character

    ParseList(
        lexer::TokenType::PUNCTUATOR_RIGHT_BRACE, lexer::NextTokenFlags::KEYWORD_TO_IDENT,
        [this, specifiers](bool &) {
            if (lexer_->GetToken().Type() != lexer::TokenType::LITERAL_IDENT) {
                // test exists for ts extension only
                LogExpectedToken(lexer::TokenType::LITERAL_IDENT);
            }

            lexer::Token importedToken = lexer_->GetToken();
            auto *imported = AllocNode<ir::Identifier>(importedToken.Ident(), Allocator());
            ir::Identifier *local = nullptr;
            imported->SetRange(lexer_->GetToken().Loc());

            lexer_->NextToken();  // eat import name

            if (CheckModuleAsModifier()) {
                lexer_->NextToken();  // eat `as` literal
                local = ParseNamedImport(&lexer_->GetToken());
                lexer_->NextToken();  // eat local name
            } else {
                local = ParseNamedImport(&importedToken);
            }

            auto *specifier = AllocNode<ir::ImportSpecifier>(imported, local);
            specifier->SetRange({imported->Start(), local->End()});
            specifiers->push_back(specifier);
            return true;
        },
        nullptr, ParseListOptions::ALLOW_TRAILING_SEP);
}

ir::AstNode *ParserImpl::ParseImportDefaultSpecifier(ArenaVector<ir::AstNode *> *specifiers)
{
    ir::Identifier *local = ParseNamedImport(&lexer_->GetToken());
    lexer_->NextToken();  // eat local name

    auto *specifier = AllocNode<ir::ImportDefaultSpecifier>(local);
    ES2PANDA_ASSERT(specifier != nullptr);
    specifier->SetRange(specifier->Local()->Range());
    specifiers->push_back(specifier);

    lexer_->NextToken();  // eat specifier name

    if (lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_COMMA) {
        lexer_->NextToken();  // eat comma
    }

    return nullptr;
}

ir::StringLiteral *ParserImpl::ParseFromClause(bool requireFrom)
{
    if (lexer_->GetToken().KeywordType() != lexer::TokenType::KEYW_FROM) {
        // unexpected_token_5.ts
        if (requireFrom) {
            LogExpectedToken(lexer::TokenType::KEYW_FROM);
            lexer_->NextToken();  // eat `from` literal
        }
    } else {
        lexer_->NextToken();  // eat `from` literal
    }

    if (lexer_->GetToken().Type() != lexer::TokenType::LITERAL_STRING) {
        // test exists for ts extension only
        LogExpectedToken(lexer::TokenType::LITERAL_STRING);
        lexer_->GetToken().SetTokenType(lexer::TokenType::LITERAL_STRING);
        lexer_->GetToken().SetTokenStr(ERROR_LITERAL);
    }

    auto *source = AllocNode<ir::StringLiteral>(lexer_->GetToken().String());
    ES2PANDA_ASSERT(source != nullptr);
    source->SetRange(lexer_->GetToken().Loc());

    lexer_->NextToken();

    return source;
}

ir::AstNode *ParserImpl::ParseImportSpecifiers(ArenaVector<ir::AstNode *> *specifiers)
{
    ES2PANDA_ASSERT(specifiers->empty());

    if (lexer_->GetToken().Type() == lexer::TokenType::LITERAL_IDENT) {
        ir::AstNode *astNode = ParseImportDefaultSpecifier(specifiers);

        if (astNode != nullptr) {
            return astNode;
        }
    }

    if (lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_MULTIPLY) {
        ParseNameSpaceImport(specifiers);
    } else if (lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LEFT_BRACE) {
        ParseNamedImportSpecifiers(specifiers);
    }
    return nullptr;
}

ir::Statement *ParserImpl::ParseImportDeclaration(StatementParsingFlags flags)
{
    if ((flags & StatementParsingFlags::GLOBAL) == 0) {
        LogError(diagnostic::IMPORT_EXPORT_ONLY_AT_TOP_LEVEL);
    }

    if (!context_.IsModule()) {
        LogError(diagnostic::IMPORT_META_ONLY_MODULE);
    }

    char32_t nextChar = lexer_->Lookahead();
    if (nextChar == lexer::LEX_CHAR_LEFT_PAREN || nextChar == lexer::LEX_CHAR_DOT) {
        return ParseExpressionStatement();
    }

    lexer::SourcePosition startLoc = lexer_->GetToken().Start();
    lexer_->NextToken();  // eat import

    ArenaVector<ir::AstNode *> specifiers(Allocator()->Adapter());

    ir::StringLiteral *source = nullptr;

    if (lexer_->GetToken().Type() != lexer::TokenType::LITERAL_STRING) {
        ParseImportSpecifiers(&specifiers);
        source = ParseFromClause(true);
    } else {
        source = ParseFromClause(false);
    }

    ES2PANDA_ASSERT(source != nullptr);
    lexer::SourcePosition endLoc = source->End();
    auto *importDeclaration = AllocNode<ir::ImportDeclaration>(source, std::move(specifiers));
    ES2PANDA_ASSERT(importDeclaration != nullptr);
    importDeclaration->SetRange({startLoc, endLoc});

    ConsumeSemicolon(importDeclaration);

    return importDeclaration;
}

ir::Statement *ParserImpl::AllocBrokenStatement(const lexer::SourcePosition &pos)
{
    return AllocBrokenStatement({pos, pos});
}

ir::Statement *ParserImpl::AllocBrokenStatement(const lexer::SourceRange &range)
{
    auto *broken = AllocNode<ir::EmptyStatement>(true);
    ES2PANDA_ASSERT(broken != nullptr);
    broken->SetRange(range);
    return broken;
}

bool ParserImpl::IsBrokenStatement(ir::Statement *st)
{
    if (st->IsEmptyStatement()) {
        return st->AsEmptyStatement()->IsBrokenStatement();
    }
    return false;
}

ir::Statement *ParserImpl::AllocEmptyStatement()
{
    auto *empty = AllocNode<ir::EmptyStatement>();
    ES2PANDA_ASSERT(empty != nullptr);
    empty->SetRange(lexer_->GetToken().Loc());
    return empty;
}

}  // namespace ark::es2panda::parser
