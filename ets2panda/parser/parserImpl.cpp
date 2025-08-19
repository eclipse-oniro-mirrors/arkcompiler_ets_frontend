/**
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

#include "parserImpl.h"
#include "forwardDeclForParserImpl.h"
#include "parserStatusContext.h"

#include "generated/diagnostic.h"
#include "varbinder/privateBinding.h"
#include "ir/brokenTypeNode.h"
#include "ir/base/classDefinition.h"
#include "ir/base/classProperty.h"
#include "ir/base/classStaticBlock.h"
#include "ir/base/methodDefinition.h"
#include "ir/base/scriptFunction.h"
#include "ir/base/spreadElement.h"
#include "ir/expressions/arrayExpression.h"
#include "ir/expressions/assignmentExpression.h"
#include "ir/expressions/callExpression.h"
#include "ir/expressions/dummyNode.h"
#include "ir/expressions/functionExpression.h"
#include "ir/expressions/literals/bigIntLiteral.h"
#include "ir/expressions/literals/numberLiteral.h"
#include "ir/expressions/literals/stringLiteral.h"
#include "ir/expressions/objectExpression.h"
#include "ir/expressions/superExpression.h"
#include "ir/ets/etsParameterExpression.h"
#include "ir/statements/blockStatement.h"
#include "ir/statements/expressionStatement.h"
#include "util/errorRecovery.h"

using namespace std::literals::string_literals;

namespace ark::es2panda::parser {
ParserImpl::ParserImpl(Program *program, const util::Options *options, util::DiagnosticEngine &diagnosticEngine,
                       ParserStatus status)
    : program_(program), context_(program_, status), options_(options), diagnosticEngine_(diagnosticEngine)
{
}

std::unique_ptr<lexer::Lexer> ParserImpl::InitLexer(const SourceFile &sourceFile)
{
    program_->SetSource(sourceFile);
    std::unique_ptr<lexer::Lexer> lexer = std::make_unique<lexer::Lexer>(&context_, diagnosticEngine_);
    lexer_ = lexer.get();
    return lexer;
}

void ParserImpl::ParseScript(const SourceFile &sourceFile, bool genStdLib)
{
    auto lexer = InitLexer(sourceFile);

    if (sourceFile.isModule) {
        context_.Status() |= (ParserStatus::MODULE);
        ParseProgram(ScriptKind::MODULE);
    } else if (genStdLib) {
        ParseProgram(ScriptKind::STDLIB);
    } else {
        ParseProgram(ScriptKind::SCRIPT);
    }
}

void ParserImpl::ParseProgram(ScriptKind kind)
{
    lexer::SourcePosition startLoc = lexer_->GetToken().Start();
    lexer_->NextToken();
    program_->SetKind(kind);

    auto statements = ParseStatementList(StatementParsingFlags::STMT_GLOBAL_LEXICAL);

    auto *blockStmt = AllocNode<ir::BlockStatement>(Allocator(), std::move(statements));
    ES2PANDA_ASSERT(blockStmt != nullptr);
    blockStmt->SetRange({startLoc, lexer_->GetToken().End()});

    program_->SetAst(blockStmt);
}

bool ParserImpl::InAmbientContext()
{
    return (context_.Status() & ParserStatus::IN_AMBIENT_CONTEXT) != 0;
}

ExpressionParseFlags ParserImpl::CarryExpressionParserFlag(ExpressionParseFlags origin, ExpressionParseFlags carry)
{
    return static_cast<ExpressionParseFlags>(origin & carry);
}

ExpressionParseFlags ParserImpl::CarryPatternFlags(ExpressionParseFlags flags)
{
    return CarryExpressionParserFlag(flags, ExpressionParseFlags::POTENTIALLY_IN_PATTERN |
                                                ExpressionParseFlags::OBJECT_PATTERN);
}

ir::ModifierFlags ParserImpl::GetAccessability(ir::ModifierFlags modifiers)
{
    if ((modifiers & ir::ModifierFlags::PUBLIC) != 0) {
        return ir::ModifierFlags::PUBLIC;
    }

    if ((modifiers & ir::ModifierFlags::PRIVATE) != 0) {
        return ir::ModifierFlags::PRIVATE;
    }

    if ((modifiers & ir::ModifierFlags::PROTECTED) != 0) {
        return ir::ModifierFlags::PROTECTED;
    }

    if ((modifiers & ir::ModifierFlags::INTERNAL) != 0) {
        return ir::ModifierFlags::INTERNAL;
    }

    return ir::ModifierFlags::NONE;
}

bool ParserImpl::IsModifierKind(const lexer::Token &token)
{
    switch (token.KeywordType()) {
        case lexer::TokenType::KEYW_STATIC:
        case lexer::TokenType::KEYW_ASYNC:
            return true;
        default:
            break;
    }

    return false;
}

ir::ModifierFlags ParserImpl::ParseModifiers()
{
    ir::ModifierFlags resultStatus = ir::ModifierFlags::NONE;
    ir::ModifierFlags prevStatus = ir::ModifierFlags::ALL;

    while (IsModifierKind(lexer_->GetToken())) {
        char32_t nextCp = lexer_->Lookahead();
        if (nextCp == lexer::LEX_CHAR_LEFT_PAREN) {
            return resultStatus;
        }

        lexer::TokenFlags tokenFlags = lexer_->GetToken().Flags();
        if ((tokenFlags & lexer::TokenFlags::HAS_ESCAPE) != 0) {
            LogError(diagnostic::KEYWORD_CONTAINS_ESCAPED_CHARS);
        }

        ir::ModifierFlags actualStatus = ir::ModifierFlags::NONE;
        ir::ModifierFlags nextStatus = ir::ModifierFlags::NONE;

        switch (lexer_->GetToken().KeywordType()) {
            case lexer::TokenType::KEYW_STATIC: {
                actualStatus = ir::ModifierFlags::STATIC;
                nextStatus = ir::ModifierFlags::ASYNC;
                break;
            }
            case lexer::TokenType::KEYW_ASYNC: {
                actualStatus = ir::ModifierFlags::ASYNC;
                nextStatus = ir::ModifierFlags::NONE;
                break;
            }
            default: {
                break;
            }
        }

        if (lexer_->Lookahead() == lexer::LEX_CHAR_COLON || lexer_->Lookahead() == lexer::LEX_CHAR_COMMA ||
            lexer_->Lookahead() == lexer::LEX_CHAR_RIGHT_PAREN || lexer_->Lookahead() == lexer::LEX_CHAR_QUESTION ||
            lexer_->Lookahead() == lexer::LEX_CHAR_RIGHT_BRACE || lexer_->Lookahead() == lexer::LEX_CHAR_LESS_THAN) {
            break;
        }

        if ((prevStatus & actualStatus) == 0) {
            LogError(diagnostic::UNEXPECTED_MODIFIER);
        }

        if ((resultStatus & actualStatus) != 0) {
            LogError(diagnostic::DUPLICATED_MODIFIER);
        }

        lexer_->NextToken(lexer::NextTokenFlags::KEYWORD_TO_IDENT);

        resultStatus |= actualStatus;
        prevStatus = nextStatus;
    }

    return resultStatus;
}

void ParserImpl::CheckAccessorPair(const ArenaVector<ir::AstNode *> &properties, const ir::Expression *propName,
                                   ir::MethodDefinitionKind methodKind, ir::ModifierFlags access)
{
    for (const auto &it : properties) {
        if (!it->IsMethodDefinition() || it->AsMethodDefinition()->Kind() != methodKind) {
            continue;
        }

        const ir::Expression *key = it->AsMethodDefinition()->Key();

        if (key->Type() != propName->Type()) {
            continue;
        }

        bool keyIsSame = false;

        if (key->IsIdentifier()) {
            const util::StringView &strName = propName->AsIdentifier()->Name();
            const util::StringView &compareName = (key->AsIdentifier()->Name());

            keyIsSame = strName == compareName;
        } else if (key->IsNumberLiteral()) {
            keyIsSame =
                key->AsNumberLiteral()->Number().GetDouble() == propName->AsNumberLiteral()->Number().GetDouble();
        } else if (key->IsStringLiteral()) {
            keyIsSame = *key->AsStringLiteral() == *propName->AsStringLiteral();
        }

        if (!keyIsSame) {
            continue;
        }

        ir::ModifierFlags getAccess;
        ir::ModifierFlags setAccess;

        if (methodKind == ir::MethodDefinitionKind::GET) {
            setAccess = access;
            getAccess = GetAccessability(it->Modifiers());
        } else {
            getAccess = access;
            setAccess = GetAccessability(it->Modifiers());
        }

        if ((setAccess == ir::ModifierFlags::NONE && getAccess > ir::ModifierFlags::PUBLIC) ||
            (setAccess != ir::ModifierFlags::NONE && getAccess > setAccess)) {
            LogError(diagnostic::GET_ACCESSOR_MUST_BE_AT_LEAST_AS_ACCESSIBLE, {}, key->Start());
        }
    }
}

void ParserImpl::ParseClassAccessor(ClassElementDescriptor *desc, char32_t *nextCp)
{
    ConsumeClassPrivateIdentifier(desc, nextCp);

    if (lexer_->GetToken().Type() != lexer::TokenType::LITERAL_IDENT) {
        return;
    }

    auto keywordType = lexer_->GetToken().KeywordType();
    if ((keywordType != lexer::TokenType::KEYW_GET && keywordType != lexer::TokenType::KEYW_SET) ||
        (*nextCp == lexer::LEX_CHAR_EQUALS || *nextCp == lexer::LEX_CHAR_SEMICOLON ||
         *nextCp == lexer::LEX_CHAR_LEFT_PAREN || *nextCp == lexer::LEX_CHAR_COLON ||
         *nextCp == lexer::LEX_CHAR_LESS_THAN)) {
        return;
    }

    LogIfPrivateIdent(desc, diagnostic::UNEXPECTED_ID);

    if ((lexer_->GetToken().Flags() & lexer::TokenFlags::HAS_ESCAPE) != 0) {
        LogError(diagnostic::KEYWORD_CONTAINS_ESCAPED_CHARS);
    }

    desc->methodKind =
        keywordType == lexer::TokenType::KEYW_GET ? ir::MethodDefinitionKind::GET : ir::MethodDefinitionKind::SET;
    desc->methodStart = lexer_->GetToken().Start();

    lexer_->NextToken(lexer::NextTokenFlags::KEYWORD_TO_IDENT);
    ConsumeClassPrivateIdentifier(desc, nextCp);
}

void ParserImpl::LogIfPrivateIdent(ClassElementDescriptor *desc, const diagnostic::DiagnosticKind &diagnostic,
                                   const util::DiagnosticMessageParams &diagnosticParams)
{
    if (desc->isPrivateIdent) {
        LogError(diagnostic, diagnosticParams);
    }
}

void ParserImpl::CheckIfStaticConstructor([[maybe_unused]] ir::ModifierFlags flags) {}

void ParserImpl::ValidateClassKey(ClassElementDescriptor *desc)
{
    if (((desc->modifiers & ir::ModifierFlags::ASYNC) != 0 || desc->isGenerator) &&
        (desc->methodKind == ir::MethodDefinitionKind::GET || desc->methodKind == ir::MethodDefinitionKind::SET)) {
        LogError(diagnostic::INVALID_ACCESSOR);
    }

    const util::StringView &propNameStr = lexer_->GetToken().Ident();

    if (propNameStr.Is("constructor")) {
        if (lexer_->Lookahead() != lexer::LEX_CHAR_LEFT_PAREN) {
            // test-class-constructor3.ts
            LogError(diagnostic::CLASS_FIELD_CONSTRUCTOR);
        }

        LogIfPrivateIdent(desc, diagnostic::PRIVATE_IDENTIFIER_NOT_CONSTRUCTOR);

        if ((desc->modifiers & ir::ModifierFlags::STATIC) == 0) {
            if ((desc->modifiers & ir::ModifierFlags::ASYNC) != 0 ||
                desc->methodKind == ir::MethodDefinitionKind::GET ||
                desc->methodKind == ir::MethodDefinitionKind::SET || desc->isGenerator) {
                LogError(diagnostic::SPECIAL_METHOD_CONSTRUCTOR);
            }

            desc->methodKind = ir::MethodDefinitionKind::CONSTRUCTOR;
            desc->methodStart = lexer_->GetToken().Start();
            desc->newStatus |= ParserStatus::CONSTRUCTOR_FUNCTION;

            if (desc->hasSuperClass) {
                desc->newStatus |= ParserStatus::ALLOW_SUPER_CALL;
            }
        }

        CheckIfStaticConstructor(desc->modifiers);
    } else if (propNameStr.Is("prototype") && (desc->modifiers & ir::ModifierFlags::STATIC) != 0) {
        LogError(diagnostic::STATIC_PROPERTY_PROTOTYPE);
    }
}

std::tuple<bool, bool, bool> ParserImpl::ParseComputedClassFieldOrIndexSignature(ir::Expression **propName)
{
    lexer_->NextToken();  // eat left square bracket
    *propName = ParseExpression(ExpressionParseFlags::ACCEPT_COMMA);

    if (lexer_->GetToken().Type() != lexer::TokenType::PUNCTUATOR_RIGHT_SQUARE_BRACKET) {
        // test exists for js extension only
        LogExpectedToken(lexer::TokenType::PUNCTUATOR_RIGHT_SQUARE_BRACKET);
    }

    return {true, false, false};
}

// CC-OFFNXT(huge_method[C++], G.FUN.01-CPP) solid logic
ir::Expression *ParserImpl::ParseClassKey(ClassElementDescriptor *desc)
{
    ir::Expression *propName = nullptr;
    if (lexer_->GetToken().IsKeyword()) {
        lexer_->GetToken().SetTokenType(lexer::TokenType::LITERAL_IDENT);
        Lexer()->GetToken().SetTokenStr(ERROR_LITERAL);
    }

    switch (lexer_->GetToken().Type()) {
        case lexer::TokenType::LITERAL_IDENT: {
            ValidateClassKey(desc);

            propName = AllocNode<ir::Identifier>(lexer_->GetToken().Ident(), Allocator());
            ES2PANDA_ASSERT(propName != nullptr);
            propName->SetRange(lexer_->GetToken().Loc());
            propName->AsIdentifier()->SetPrivate(desc->isPrivateIdent);
            break;
        }
        case lexer::TokenType::LITERAL_STRING: {
            LogIfPrivateIdent(desc, diagnostic::PRIVATE_IDENTIFIER_STRING);

            if (lexer_->GetToken().Ident().Is("constructor")) {
                LogError(diagnostic::CLASS_FIELD_CONSTRUCTOR);
            }

            if (lexer_->GetToken().Ident().Is("prototype") && (desc->modifiers & ir::ModifierFlags::STATIC) != 0) {
                LogError(diagnostic::STATIC_PROPERTY_PROTOTYPE);
            }

            propName = AllocNode<ir::StringLiteral>(lexer_->GetToken().String());
            ES2PANDA_ASSERT(propName != nullptr);
            propName->SetRange(lexer_->GetToken().Loc());
            break;
        }
        case lexer::TokenType::LITERAL_NUMBER: {
            LogIfPrivateIdent(desc, diagnostic::PRIVATE_IDENTIFIER_NUMBER);

            if ((lexer_->GetToken().Flags() & lexer::TokenFlags::NUMBER_BIGINT) != 0) {
                propName = AllocNode<ir::BigIntLiteral>(lexer_->GetToken().BigInt());
            } else {
                propName = AllocNode<ir::NumberLiteral>(lexer_->GetToken().GetNumber());
            }
            ES2PANDA_ASSERT(propName != nullptr);

            propName->SetRange(lexer_->GetToken().Loc());
            break;
        }
        case lexer::TokenType::PUNCTUATOR_LEFT_SQUARE_BRACKET: {
            LogIfPrivateIdent(desc, diagnostic::UNEXPECTED_CHAR_PRIVATE_IDENTIFIER);
            std::tie(desc->isComputed, desc->invalidComputedProperty, desc->isIndexSignature) =
                ParseComputedClassFieldOrIndexSignature(&propName);
            break;
        }
        default: {
            LogError(diagnostic::UNEXPECTED_TOKEN);
            propName = AllocBrokenExpression(Lexer()->GetToken().Loc());
        }
    }

    lexer_->NextToken(lexer::NextTokenFlags::KEYWORD_TO_IDENT);

    return propName;
}

void ParserImpl::ValidateClassMethodStart(ClassElementDescriptor *desc, [[maybe_unused]] ir::TypeNode *typeAnnotation)
{
    if (lexer_->GetToken().Type() != lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS) {
        return;
    }
    desc->classMethod = true;

    if ((desc->modifiers & ir::ModifierFlags::ASYNC) != 0) {
        desc->newStatus |= ParserStatus::ASYNC_FUNCTION;
    }

    if (desc->isGenerator) {
        desc->newStatus |= ParserStatus::GENERATOR_FUNCTION;
    }
}

void ParserImpl::ValidateGetterSetter(ir::MethodDefinitionKind methodDefinition, size_t number)
{
    if (methodDefinition == ir::MethodDefinitionKind::SET) {
        if (number != 1) {
            LogError(diagnostic::SETTER_FORMAL_PARAMS);
        }
    } else if (methodDefinition == ir::MethodDefinitionKind::GET) {
        if (number != 0) {
            LogError(diagnostic::GETTER_FORMAL_PARAMS);
        }
    }
}

void ParserImpl::ValidateClassSetter([[maybe_unused]] ClassElementDescriptor *desc,
                                     [[maybe_unused]] const ArenaVector<ir::AstNode *> &properties,
                                     [[maybe_unused]] ir::Expression *propName, ir::ScriptFunction *func)
{
    ValidateGetterSetter(ir::MethodDefinitionKind::SET, func->Params().size());
    if (func->ReturnTypeAnnotation() != nullptr) {
        LogError(diagnostic::SETTER_NO_RETURN_TYPE);
    }
}

void ParserImpl::ValidateClassGetter([[maybe_unused]] ClassElementDescriptor *desc,
                                     [[maybe_unused]] const ArenaVector<ir::AstNode *> &properties,
                                     [[maybe_unused]] ir::Expression *propName, ir::ScriptFunction *func)
{
    ValidateGetterSetter(ir::MethodDefinitionKind::GET, func->Params().size());
}

ir::MethodDefinition *ParserImpl::ParseClassMethod(ClassElementDescriptor *desc,
                                                   const ArenaVector<ir::AstNode *> &properties,
                                                   ir::Expression *propName, lexer::SourcePosition *propEnd)
{
    if (desc->methodKind != ir::MethodDefinitionKind::SET &&
        (desc->newStatus & ParserStatus::CONSTRUCTOR_FUNCTION) == 0) {
        desc->newStatus |= ParserStatus::NEED_RETURN_TYPE;
    }

    ir::ScriptFunction *func = ParseFunction(desc->newStatus);

    auto *funcExpr = AllocNode<ir::FunctionExpression>(func);
    ES2PANDA_ASSERT(funcExpr != nullptr);
    funcExpr->SetRange(func->Range());

    if (desc->methodKind == ir::MethodDefinitionKind::SET) {
        ValidateClassSetter(desc, properties, propName, func);
    } else if (desc->methodKind == ir::MethodDefinitionKind::GET) {
        ValidateClassGetter(desc, properties, propName, func);
    }

    *propEnd = func->End();
    func->AddFlag(ir::ScriptFunctionFlags::METHOD);

    auto *ident = !propName->IsArrowFunctionExpression() && !propName->IsFunctionExpression()
                      ? propName->Clone(Allocator(), nullptr)->AsExpression()
                      : propName;
    auto *method = AllocNode<ir::MethodDefinition>(desc->methodKind, ident, funcExpr, desc->modifiers, Allocator(),
                                                   desc->isComputed);
    ES2PANDA_ASSERT(method != nullptr);
    method->SetRange(funcExpr->Range());

    return method;
}

ir::ClassElement *ParserImpl::ParseClassProperty(ClassElementDescriptor *desc,
                                                 const ArenaVector<ir::AstNode *> &properties, ir::Expression *propName,
                                                 ir::TypeNode *typeAnnotation)
{
    ES2PANDA_ASSERT(propName != nullptr);
    lexer::SourcePosition propEnd = propName->End();
    ir::ClassElement *property = nullptr;

    if (desc->classMethod) {
        property = ParseClassMethod(desc, properties, propName, &propEnd);
        ES2PANDA_ASSERT(property != nullptr);
        property->SetRange({desc->propStart, propEnd});
        return property;
    }

    ir::Expression *value = nullptr;

    if (lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_SUBSTITUTION) {
        lexer_->NextToken();  // eat equals

        if (InAmbientContext() || (desc->modifiers & ir::ModifierFlags::DECLARE) != 0) {
            LogError(diagnostic::INITIALIZERS_IN_AMBIENT_CONTEXTS);
        }

        value = ParseExpression();
        propEnd = value->End();
    }

    property =
        AllocNode<ir::ClassProperty>(propName, value, typeAnnotation, desc->modifiers, Allocator(), desc->isComputed);
    ES2PANDA_ASSERT(property != nullptr);
    property->SetRange({desc->propStart, propEnd});

    return property;
}

void ParserImpl::CheckClassGeneratorMethod(ClassElementDescriptor *desc, char32_t *nextCp)
{
    if (lexer_->GetToken().Type() != lexer::TokenType::PUNCTUATOR_MULTIPLY) {
        return;
    }

    desc->isGenerator = true;
    lexer_->NextToken(lexer::NextTokenFlags::KEYWORD_TO_IDENT);
    *nextCp = lexer_->Lookahead();
}

bool ParserImpl::ValidatePrivateIdentifier()
{
    size_t iterIdx = lexer_->GetToken().Start().index;
    lexer_->NextToken(lexer::NextTokenFlags::KEYWORD_TO_IDENT);

    if (lexer_->GetToken().Type() != lexer::TokenType::LITERAL_IDENT ||
        (lexer_->GetToken().Start().index - iterIdx > 1)) {
        LogError(diagnostic::UNEXPECTED_TOKEN_IN_PRIVATE);
        return false;
    }

    return true;
}

void ParserImpl::ConsumeClassPrivateIdentifier(ClassElementDescriptor *desc, char32_t *nextCp)
{
    if (lexer_->GetToken().Type() != lexer::TokenType::PUNCTUATOR_HASH_MARK) {
        return;
    }

    desc->isPrivateIdent = true;
    ValidatePrivateIdentifier();
    *nextCp = lexer_->Lookahead();
}

void ParserImpl::AddPrivateElement(const ir::ClassElement *elem)
{
    if (!classPrivateContext_.AddElement(elem)) {
        LogError(diagnostic::PRIVATE_FIELD_REDEC);
    }
}

ir::ClassElement *ParserImpl::ParseClassStaticBlock()
{
    const lexer::SourcePosition &startPos = lexer_->GetToken().Start();

    lexer_->NextToken();  // eat 'static'

    SavedParserContext context(this, ParserStatus::ALLOW_SUPER | ParserStatus::STATIC_BLOCK);
    context_.Status() &= ~(ParserStatus::ASYNC_FUNCTION | ParserStatus::GENERATOR_FUNCTION);

    lexer_->NextToken();  // eat '{'

    ArenaVector<ir::Expression *> params(Allocator()->Adapter());

    ArenaVector<ir::Statement *> statements = ParseStatementList();

    // redundant check -> we have it in parse statements
    // we even do not consume '}' after
    ExpectToken(lexer::TokenType::PUNCTUATOR_RIGHT_BRACE, false);

    auto *body = AllocNode<ir::BlockStatement>(Allocator(), std::move(statements));
    // clang-format off
    auto *func = AllocNode<ir::ScriptFunction>(
        Allocator(), ir::ScriptFunction::ScriptFunctionData {
            body, ir::FunctionSignature(nullptr, std::move(params), nullptr),
            ir::ScriptFunctionFlags::EXPRESSION | ir::ScriptFunctionFlags::STATIC_BLOCK,
            ir::ModifierFlags::STATIC, context_.GetLanguage()});
    // clang-format on

    auto *funcExpr = AllocNode<ir::FunctionExpression>(func);
    auto *staticBlock = AllocNode<ir::ClassStaticBlock>(funcExpr, Allocator());
    ES2PANDA_ASSERT(staticBlock != nullptr);
    staticBlock->SetRange({startPos, lexer_->GetToken().End()});

    lexer_->NextToken();  // eat '}'

    return staticBlock;
}

ir::AstNode *ParserImpl::ParseClassElement(const ArenaVector<ir::AstNode *> &properties,
                                           ir::ClassDefinitionModifiers modifiers,
                                           [[maybe_unused]] ir::ModifierFlags flags)
{
    if (lexer_->GetToken().KeywordType() == lexer::TokenType::KEYW_STATIC &&
        lexer_->Lookahead() == lexer::LEX_CHAR_LEFT_BRACE) {
        return ParseClassStaticBlock();
    }

    ClassElementDescriptor desc(Allocator());

    desc.methodKind = ir::MethodDefinitionKind::METHOD;
    desc.newStatus = ParserStatus::ALLOW_SUPER;
    desc.hasSuperClass = (modifiers & ir::ClassDefinitionModifiers::HAS_SUPER) != 0U;
    desc.propStart = lexer_->GetToken().Start();
    desc.modifiers = ParseModifiers();

    char32_t nextCp = lexer_->Lookahead();
    CheckClassGeneratorMethod(&desc, &nextCp);
    ParseClassAccessor(&desc, &nextCp);

    if ((desc.modifiers & ir::ModifierFlags::STATIC) == 0) {
        context_.Status() |= ParserStatus::ALLOW_THIS_TYPE;
    }

    ir::Expression *propName = ParseClassKey(&desc);
    ValidateClassMethodStart(&desc, nullptr);
    ir::ClassElement *property = ParseClassProperty(&desc, properties, propName, nullptr);

    if (property != nullptr && lexer_->GetToken().Type() != lexer::TokenType::PUNCTUATOR_SEMI_COLON &&
        lexer_->GetToken().Type() != lexer::TokenType::PUNCTUATOR_RIGHT_BRACE &&
        (lexer_->GetToken().Flags() & lexer::TokenFlags::NEW_LINE) == 0 &&
        !(property->IsMethodDefinition() &&
          property->AsMethodDefinition()->Value()->AsFunctionExpression()->Function()->Body() != nullptr)) {
        // test exists for js extension only
        LogExpectedToken(lexer::TokenType::PUNCTUATOR_SEMI_COLON);
    }

    if (lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_SEMI_COLON) {
        lexer_->NextToken(lexer::NextTokenFlags::KEYWORD_TO_IDENT);
    }

    context_.Status() &= ~ParserStatus::ALLOW_THIS_TYPE;

    // if Id() is nullptr, ParseClassKey has logged an error
    if (desc.isPrivateIdent && property != nullptr && property->Id() != nullptr) {
        AddPrivateElement(property);
    }

    return property;
}

ir::MethodDefinition *ParserImpl::BuildImplicitConstructor(ir::ClassDefinitionModifiers modifiers,
                                                           const lexer::SourcePosition &startLoc)
{
    ArenaVector<ir::Expression *> params(Allocator()->Adapter());
    ArenaVector<ir::Statement *> statements(Allocator()->Adapter());

    if ((modifiers & ir::ClassDefinitionModifiers::HAS_SUPER) != 0U) {
        util::StringView argsStr = "args";
        params.push_back(AllocNode<ir::SpreadElement>(ir::AstNodeType::REST_ELEMENT, Allocator(),
                                                      AllocNode<ir::Identifier>(argsStr, Allocator())));
        ArenaVector<ir::Expression *> callArgs(Allocator()->Adapter());
        auto *superExpr = AllocNode<ir::SuperExpression>();
        callArgs.push_back(AllocNode<ir::SpreadElement>(ir::AstNodeType::SPREAD_ELEMENT, Allocator(),
                                                        AllocNode<ir::Identifier>(argsStr, Allocator())));

        auto *callExpr = AllocNode<ir::CallExpression>(superExpr, std::move(callArgs), nullptr, false);
        statements.push_back(AllocNode<ir::ExpressionStatement>(callExpr));
    }

    auto *body = AllocNode<ir::BlockStatement>(Allocator(), std::move(statements));
    auto *func = AllocNode<ir::ScriptFunction>(
        Allocator(), ir::ScriptFunction::ScriptFunctionData {body,
                                                             ir::FunctionSignature(nullptr, std::move(params), nullptr),
                                                             ir::ScriptFunctionFlags::CONSTRUCTOR |
                                                                 ir::ScriptFunctionFlags::IMPLICIT_SUPER_CALL_NEEDED,
                                                             {},
                                                             context_.GetLanguage()});

    auto *funcExpr = AllocNode<ir::FunctionExpression>(func);
    auto *key = AllocNode<ir::Identifier>("constructor", Allocator());

    if ((modifiers & ir::ClassDefinitionModifiers::SET_CTOR_ID) != 0U) {
        ES2PANDA_ASSERT(key != nullptr);
        func->SetIdent(key->Clone(Allocator(), nullptr));
    }

    auto *ctor = AllocNode<ir::MethodDefinition>(ir::MethodDefinitionKind::CONSTRUCTOR, key, funcExpr,
                                                 ir::ModifierFlags::CONSTRUCTOR, Allocator(), false);
    ES2PANDA_ASSERT(ctor != nullptr);

    const auto rangeImplicitContstuctor = lexer::SourceRange(startLoc, startLoc);
    ctor->IterateRecursively(
        [&rangeImplicitContstuctor](ir::AstNode *node) -> void { node->SetRange(rangeImplicitContstuctor); });

    return ctor;
}

void ParserImpl::CreateImplicitConstructor(ir::MethodDefinition *&ctor,
                                           [[maybe_unused]] ArenaVector<ir::AstNode *> &properties,
                                           ir::ClassDefinitionModifiers modifiers, ir::ModifierFlags flags,
                                           const lexer::SourcePosition &startLoc)
{
    if (ctor != nullptr) {
        return;
    }

    ctor = BuildImplicitConstructor(modifiers, startLoc);
    if ((flags & ir::ModifierFlags::DECLARE) != 0) {
        ES2PANDA_ASSERT(ctor != nullptr);
        auto *ctorFunc = ctor->Function();
        ES2PANDA_ASSERT(ctorFunc != nullptr);
        ctorFunc->AddFlag(ir::ScriptFunctionFlags::EXTERNAL);
    }
}

ir::Identifier *ParserImpl::ParseClassIdent(ir::ClassDefinitionModifiers modifiers)
{
    if (lexer_->GetToken().Type() == lexer::TokenType::LITERAL_IDENT) {
        return ExpectIdentifier();
    }

    auto idRequired =
        static_cast<ir::ClassDefinitionModifiers>(modifiers & ir::ClassDefinitionModifiers::DECLARATION_ID_REQUIRED);

    if (idRequired == ir::ClassDefinitionModifiers::DECLARATION_ID_REQUIRED) {
        LogError(diagnostic::UNEXPECTED_TOKEN_ID);
        return AllocBrokenExpression(Lexer()->GetToken().Loc());
    }

    return nullptr;
}

bool ParserImpl::CheckClassElement(ir::AstNode *property, ir::MethodDefinition *&ctor,
                                   [[maybe_unused]] ArenaVector<ir::AstNode *> &properties)
{
    if (!property->IsMethodDefinition()) {
        return false;
    }

    ir::MethodDefinition *def = property->AsMethodDefinition();
    if (!def->IsConstructor()) {
        return false;
    }

    if (ctor != nullptr) {
        LogError(diagnostic::MULTIPLE_CONSTRUCTOR_IMPLEMENTATIONS, {}, property->Start());
    }
    ctor = def;

    return true;
}

ir::Expression *ParserImpl::ParseSuperClassReference()
{
    if (lexer_->GetToken().Type() == lexer::TokenType::KEYW_EXTENDS) {
        lexer_->NextToken();
        return ParseLeftHandSideExpression();
    }

    return nullptr;
}

std::tuple<ir::Expression *, ir::TSTypeParameterInstantiation *> ParserImpl::ParseSuperClass()
{
    return {ParseSuperClassReference(), nullptr};
}

// NOLINTNEXTLINE(google-default-arguments)
ir::ClassDefinition *ParserImpl::ParseClassDefinition(ir::ClassDefinitionModifiers modifiers, ir::ModifierFlags flags)
{
    ExpectToken(lexer::TokenType::KEYW_CLASS);

    ir::Identifier *identNode = ParseClassIdent(modifiers);

    if (identNode == nullptr && (modifiers & ir::ClassDefinitionModifiers::DECLARATION) != 0U) {
        LogError(diagnostic::UNEXPECTED_TOKEN_ID);
        return nullptr;  // ir::ClassDefinition
    }

    varbinder::PrivateBinding privateBinding(Allocator(), classId_++);

    // Parse SuperClass
    auto [superClass, superTypeParams] = ParseSuperClass();

    if (superClass != nullptr) {
        modifiers |= ir::ClassDefinitionModifiers::HAS_SUPER;
    }

    // test exists for js extension only
    ExpectToken(lexer::TokenType::PUNCTUATOR_LEFT_BRACE, false);

    auto [ctor, properties, bodyRange] = ParseClassBody(modifiers, flags);

    ArenaVector<ir::TSClassImplements *> implements(Allocator()->Adapter());
    auto *classDefinition =
        AllocNode<ir::ClassDefinition>(Allocator(), identNode, nullptr, superTypeParams, std::move(implements), ctor,
                                       superClass, std::move(properties), modifiers, flags, GetContext().GetLanguage());
    ES2PANDA_ASSERT(classDefinition != nullptr);
    classDefinition->SetInternalName(privateBinding.View());

    classDefinition->SetRange(bodyRange);

    return classDefinition;
}

ParserImpl::ClassBody ParserImpl::ParseClassBody(ir::ClassDefinitionModifiers modifiers, ir::ModifierFlags flags)
{
    auto savedCtx = SavedStatusContext<ParserStatus::IN_CLASS_BODY>(&context_);

    lexer::SourcePosition startLoc = lexer_->GetToken().Start();
    const auto startOfInnerBody = lexer_->GetToken().End();
    lexer_->NextToken(lexer::NextTokenFlags::KEYWORD_TO_IDENT);

    ir::MethodDefinition *ctor = nullptr;
    ArenaVector<ir::AstNode *> properties(Allocator()->Adapter());

    SavedClassPrivateContext classContext(this);

    if (lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_FORMAT &&
        lexer_->Lookahead() == static_cast<char32_t>(ARRAY_FORMAT_NODE)) {
        properties = std::move(ParseAstNodesArrayFormatPlaceholder());
        if (lexer_->GetToken().Type() != lexer::TokenType::PUNCTUATOR_RIGHT_BRACE) {
            // NOTE(schernykh): add info about LoC
            LOG(FATAL, ES2PANDA) << "Unexpected " << lexer::TokenToString(lexer_->GetToken().Type())
                                 << ", expected '}'.";
        }
    } else {
        while (lexer_->GetToken().Type() != lexer::TokenType::PUNCTUATOR_RIGHT_BRACE &&
               lexer_->GetToken().Type() != lexer::TokenType::EOS) {
            if (lexer_->TryEatTokenType(lexer::TokenType::PUNCTUATOR_SEMI_COLON)) {
                continue;
            }

            util::ErrorRecursionGuard infiniteLoopBlocker(Lexer());
            ir::AstNode *property = ParseClassElement(properties, modifiers, flags);
            if (property->IsBrokenStatement()) {  // Error processing.
                continue;
            }

            if (CheckClassElement(property, ctor, properties)) {
                continue;
            }
            auto isIndexer = [](ir::AstNode *node) {
                return node->IsDummyNode() && node->AsDummyNode()->IsDeclareIndexer();
            };
            if (std::any_of(properties.begin(), properties.end(),
                            [&isIndexer](ir::AstNode *node) { return isIndexer(node); }) &&
                isIndexer(property)) {
                LogError(diagnostic::MORE_INDEXER, {}, property->Start());
                continue;
            }

            properties.push_back(property);
        }
    }

    CreateImplicitConstructor(ctor, properties, modifiers, flags, startOfInnerBody);
    ExpectToken(lexer::TokenType::PUNCTUATOR_RIGHT_BRACE);

    lexer::SourcePosition endLoc = lexer_->GetToken().End();
    return {ctor, std::move(properties), lexer::SourceRange {startLoc, endLoc}};
}

void ParserImpl::ValidateRestParameter(ir::Expression *param)
{
    if (!param->IsIdentifier()) {
        context_.Status() |= ParserStatus::HAS_COMPLEX_PARAM;
        if (!param->IsRestElement()) {
            return;
        }
        if (lexer_->GetToken().Type() != lexer::TokenType::PUNCTUATOR_RIGHT_PARENTHESIS) {
            // for now test exists for js extension only
            LogError(diagnostic::REST_PARAM_NOT_LAST);

            lexer_->GetToken().SetTokenType(lexer::TokenType::PUNCTUATOR_RIGHT_PARENTHESIS);
        }
    }
}

bool ParserImpl::ValidateBreakLabel(util::StringView label)
{
    return context_.FindLabel(label) != nullptr;
}

bool ParserImpl::ValidateContinueLabel(util::StringView label)
{
    const ParserContext *labelCtx = context_.FindLabel(label);
    return labelCtx != nullptr && ((labelCtx->Status() & ParserStatus::IN_ITERATION) != 0);
}

ArenaVector<ir::Expression *> ParserImpl::ParseFunctionParams()
{
    ExpectToken(lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS);

    ArenaVector<ir::Expression *> params(Allocator()->Adapter());

    auto parseFunc = [this, &params]() {
        ir::Expression *parameter = ParseFunctionParameter();
        if (parameter == nullptr) {
            return false;
        }
        bool seenOptional = false;
        for (auto const param : params) {
            if (param->IsETSParameterExpression() && param->AsETSParameterExpression()->IsOptional()) {
                seenOptional = true;
                break;
            }
        }

        if (seenOptional && !(parameter->IsETSParameterExpression() &&
                              (parameter->AsETSParameterExpression()->IsOptional() ||
                               parameter->AsETSParameterExpression()->RestParameter() != nullptr))) {
            LogError(diagnostic::REQUIRED_PARAM_AFTER_OPTIONAL, {}, parameter->Start());
        }

        if (parameter->IsETSParameterExpression() && parameter->AsETSParameterExpression()->Ident()->IsReceiver() &&
            !params.empty()) {
            LogError(diagnostic::FUNC_PARAM_THIS_FIRST);
            return false;
        }

        ValidateRestParameter(parameter);
        params.push_back(parameter);
        return true;
    };

    if (lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_FORMAT &&
        lexer_->Lookahead() == static_cast<char32_t>(ARRAY_FORMAT_NODE)) {
        params = std::move(ParseExpressionsArrayFormatPlaceholder());
    } else {
        ParseList(lexer::TokenType::PUNCTUATOR_RIGHT_PARENTHESIS, lexer::NextTokenFlags::NONE, parseFunc, nullptr,
                  true);
    }

    return params;
}

ir::Expression *ParserImpl::CreateParameterThis([[maybe_unused]] ir::TypeNode *typeAnnotation)
{
    LogError(diagnostic::UNEXPECTED_TOKEN_ID_FUN);
    return AllocBrokenExpression(Lexer()->GetToken().Loc());
}

std::tuple<bool, ir::BlockStatement *, lexer::SourcePosition, bool> ParserImpl::ParseFunctionBody(
    // CC-OFFNXT(G.FMT.06-CPP) project code style
    [[maybe_unused]] const ArenaVector<ir::Expression *> &params, [[maybe_unused]] ParserStatus newStatus,
    [[maybe_unused]] ParserStatus contextStatus)
{
    if (lexer_->GetToken().Type() != lexer::TokenType::PUNCTUATOR_LEFT_BRACE) {
        LogExpectedToken(lexer::TokenType::PUNCTUATOR_LEFT_BRACE);
    }

    ir::BlockStatement *body = ParseBlockStatement();
    ES2PANDA_ASSERT(body != nullptr);

    return {true, body, body->End(), false};
}

FunctionSignature ParserImpl::ParseFunctionSignature(ParserStatus status)
{
    ir::TSTypeParameterDeclaration *typeParamDecl = ParseFunctionTypeParameters();

    // Check if constructor has type parameters
    if ((status & ParserStatus::CONSTRUCTOR_FUNCTION) != 0 && typeParamDecl != nullptr) {
        LogError(diagnostic::CONSTRUCTOR_TYPE_PARAMETERS);
    }

    if (lexer_->GetToken().Type() != lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS) {
        auto parameter = (status & ParserStatus::ARROW_FUNCTION) != 0 ? ParseFunctionParameter() : nullptr;
        if (parameter != nullptr) {
            ArenaVector<ir::Expression *> param(Allocator()->Adapter());
            param.push_back(parameter);
            auto res = ir::FunctionSignature(typeParamDecl, std::move(param), nullptr, false);
            return {std::move(res), ir::ScriptFunctionFlags::NONE};
        }
        LogExpectedToken(lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS);
    }

    FunctionParameterContext funcParamContext(&context_);

    auto params = ParseFunctionParams();

    ir::TypeNode *returnTypeAnnotation = nullptr;
    bool hasReceiver = !params.empty() && params[0]->IsETSParameterExpression() &&
                       params[0]->AsETSParameterExpression()->Ident()->IsReceiver();
    if (hasReceiver) {
        SavedParserContext contextAfterParseParams(this, GetContext().Status() | ParserStatus::HAS_RECEIVER);
        returnTypeAnnotation = ParseFunctionReturnType(status);
    } else {
        returnTypeAnnotation = ParseFunctionReturnType(status);
    }

    if (GetContext().IsExtensionAccessor() && !hasReceiver) {
        LogError(diagnostic::EXTENSION_ACCESSOR_RECEIVER);
    }

    auto res = ir::FunctionSignature(typeParamDecl, std::move(params), returnTypeAnnotation, hasReceiver);
    return {std::move(res), ir::ScriptFunctionFlags::NONE};
}

ir::ScriptFunction *ParserImpl::ParseFunction(ParserStatus newStatus)
{
    FunctionContext functionContext(this, newStatus | ParserStatus::FUNCTION | ParserStatus::ALLOW_NEW_TARGET);

    lexer::SourcePosition startLoc = lexer_->GetToken().Start();

    auto [signature, throw_marker] = ParseFunctionSignature(newStatus);

    auto [letDeclare, body, endLoc, isOverload] = ParseFunctionBody(signature.Params(), newStatus, context_.Status());

    if (isOverload) {
        functionContext.AddFlag(ir::ScriptFunctionFlags::OVERLOAD);
    }

    functionContext.AddFlag(throw_marker);
    auto *funcNode = AllocNode<ir::ScriptFunction>(
        Allocator(),
        ir::ScriptFunction::ScriptFunctionData {body,
                                                std::move(signature),      // CC-OFFNXT(G.FMT.02-CPP) project code style
                                                functionContext.Flags(),   // CC-OFFNXT(G.FMT.02-CPP) project code style
                                                {},                        // CC-OFFNXT(G.FMT.02-CPP) project code style
                                                context_.GetLanguage()});  // CC-OFF(G.FMT.02-CPP) project code style
    ES2PANDA_ASSERT(funcNode != nullptr);

    funcNode->SetRange({startLoc, endLoc});

    return funcNode;
}

ir::SpreadElement *ParserImpl::ParseSpreadElement(ExpressionParseFlags flags)
{
    ES2PANDA_ASSERT(lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_PERIOD_PERIOD_PERIOD);
    lexer::SourcePosition startLocation = lexer_->GetToken().Start();
    bool inPattern = (flags & ExpressionParseFlags::MUST_BE_PATTERN) != 0;
    lexer_->NextToken();

    ir::Expression *argument {};
    if (inPattern) {
        argument = ParsePatternElement(ExpressionParseFlags::IN_REST);
        if ((flags & ExpressionParseFlags::OBJECT_PATTERN) != 0 && !argument->IsIdentifier()) {
            LogError(diagnostic::RESTPARAM_ID_IN_DEC_CONTEXT);
        }
    } else {
        argument = ParseExpression(flags);
    }

    auto nodeType = inPattern ? ir::AstNodeType::REST_ELEMENT : ir::AstNodeType::SPREAD_ELEMENT;
    auto *spreadElementNode = AllocNode<ir::SpreadElement>(nodeType, Allocator(), argument);
    ES2PANDA_ASSERT(spreadElementNode != nullptr);
    spreadElementNode->SetRange({startLocation, argument->End()});
    return spreadElementNode;
}

void ParserImpl::CheckRestrictedBinding()
{
    ES2PANDA_ASSERT(lexer_->GetToken().Type() == lexer::TokenType::LITERAL_IDENT);
    CheckRestrictedBinding(lexer_->GetToken().KeywordType());
}

void ParserImpl::CheckRestrictedBinding(lexer::TokenType keywordType)
{
    if (keywordType == lexer::TokenType::KEYW_ARGUMENTS || keywordType == lexer::TokenType::KEYW_EVAL) {
        LogError(diagnostic::EVAL_OR_ARGUMENTS_IN_STRICT_MODE, {}, lexer_->GetToken().Start());
    }
}

void ParserImpl::CheckRestrictedBinding(const util::StringView &ident, const lexer::SourcePosition &pos)
{
    if (ident.Is("eval") || ident.Is("arguments")) {
        LogError(diagnostic::EVAL_OR_ARGUMENTS_IN_STRICT_MODE, {}, pos);
    }
}

ir::Expression *ParserImpl::ParseFunctionParameter()
{
    ConvertThisKeywordToIdentIfNecessary();

    if (lexer_->GetToken().Type() == lexer::TokenType::LITERAL_IDENT) {
        CheckRestrictedBinding();
    }

    return ParsePatternElement(ExpressionParseFlags::NO_OPTS, true);
}

void ParserImpl::ValidateLvalueAssignmentTarget(ir::Expression *node)
{
    switch (node->Type()) {
        case ir::AstNodeType::IDENTIFIER: {
            CheckRestrictedBinding(node->AsIdentifier()->Name(), node->Start());
            break;
        }
        case ir::AstNodeType::MEMBER_EXPRESSION: {
            break;
        }
        default: {
            LogError(diagnostic::INVALID_LEFT_SIDE_IN_ASSIGNMENT);
        }
    }
}

void ParserImpl::ValidateAssignmentTarget(ExpressionParseFlags flags, ir::Expression *node)
{
    switch (node->Type()) {
        case ir::AstNodeType::ARRAY_PATTERN:
        case ir::AstNodeType::OBJECT_PATTERN: {
            break;
        }
        case ir::AstNodeType::ARRAY_EXPRESSION:
        case ir::AstNodeType::OBJECT_EXPRESSION: {
            if ((flags & ExpressionParseFlags::POTENTIALLY_IN_PATTERN) != 0) {
                return;
            }

            [[fallthrough]];
        }
        default: {
            return ValidateLvalueAssignmentTarget(node);
        }
    }
}

void ParserImpl::ValidateArrowParameterBindings(const ir::Expression *node)
{
    switch (node->Type()) {
        case ir::AstNodeType::IDENTIFIER: {
            CheckRestrictedBinding(node->AsIdentifier()->Name(), node->Start());
            break;
        }
        case ir::AstNodeType::OMITTED_EXPRESSION: {
            break;
        }
        case ir::AstNodeType::REST_ELEMENT: {
            ValidateArrowParameterBindings(node->AsRestElement()->Argument());
            break;
        }
        case ir::AstNodeType::PROPERTY: {
            break;
        }
        case ir::AstNodeType::OBJECT_PATTERN: {
            const auto &props = node->AsObjectPattern()->Properties();

            for (auto *it : props) {
                ValidateArrowParameterBindings(it);
            }
            break;
        }
        case ir::AstNodeType::ARRAY_PATTERN: {
            const auto &elements = node->AsArrayPattern()->Elements();

            for (auto *it : elements) {
                ValidateArrowParameterBindings(it);
            }
            break;
        }
        case ir::AstNodeType::ASSIGNMENT_PATTERN: {
            ValidateArrowParameterBindings(node->AsAssignmentPattern()->Left());
            // CC-OFFNXT(G.FMT.06-CPP) project code style
            break;
        }
        default: {
            LogError(diagnostic::UNEXPECTED_ARROWPARAM_ELEMENT);
        }
    }
}

void ParserImpl::LogParameterModifierError(ir::ModifierFlags status)
{
    LogError(diagnostic::PARAM_MODIFIER_CANNOT_APPEAR_ON_PARAMETER,
             {(status & ir::ModifierFlags::STATIC) != 0  ? "static"
              : (status & ir::ModifierFlags::ASYNC) != 0 ? "async"
                                                         : "declare"},
             lexer_->GetToken().Start());
}

ir::Identifier *ParserImpl::ParseIdentifierFormatPlaceholder([[maybe_unused]] std::optional<NodeFormatType> nodeFormat)
{
    // NOTE(schernykh): add info about LoC
    LOG(FATAL, ES2PANDA) << "Format placeholder with identifier is not supported";
    return nullptr;
}

ir::Statement *ParserImpl::ParseStatementFormatPlaceholder()
{
    // NOTE(schernykh): add info about LoC
    LOG(FATAL, ES2PANDA) << "Statement with format placeholder is not supported";
    return nullptr;
}

ir::AstNode *ParserImpl::ParseTypeParametersFormatPlaceholder()
{
    // NOTE(schernykh): add info about LoC
    LOG(FATAL, ES2PANDA) << "Format placeholder with type parameter(s) is not supported";
    return nullptr;
}

ArenaVector<ir::Statement *> &ParserImpl::ParseStatementsArrayFormatPlaceholder()
{
    // NOTE(schernykh): add info about LoC
    LOG(FATAL, ES2PANDA) << "Format placeholder from statements array is not supported";
    ES2PANDA_UNREACHABLE();
}

ArenaVector<ir::AstNode *> &ParserImpl::ParseAstNodesArrayFormatPlaceholder()
{
    // NOTE(schernykh): add info about LoC
    LOG(FATAL, ES2PANDA) << "Format placeholder from AST nodes is not supported";
    ES2PANDA_UNREACHABLE();
}

ArenaVector<ir::Expression *> &ParserImpl::ParseExpressionsArrayFormatPlaceholder()
{
    // NOTE(schernykh): add info about LoC
    LOG(FATAL, ES2PANDA) << "Format placeholder from expressions array is not supported";
    ES2PANDA_UNREACHABLE();
}

bool ParserImpl::ParsePunctuatorGreaterThan(bool throwError)
{
    switch (Lexer()->GetToken().Type()) {
        case lexer::TokenType::PUNCTUATOR_GREATER_THAN_EQUAL:
        case lexer::TokenType::PUNCTUATOR_RIGHT_SHIFT: {
            Lexer()->BackwardToken(lexer::TokenType::PUNCTUATOR_GREATER_THAN, 1);
            break;
        }
        case lexer::TokenType::PUNCTUATOR_RIGHT_SHIFT_EQUAL:
        case lexer::TokenType::PUNCTUATOR_UNSIGNED_RIGHT_SHIFT: {
            Lexer()->BackwardToken(lexer::TokenType::PUNCTUATOR_GREATER_THAN, 2U);
            break;
        }
        case lexer::TokenType::PUNCTUATOR_UNSIGNED_RIGHT_SHIFT_EQUAL: {
            Lexer()->BackwardToken(lexer::TokenType::PUNCTUATOR_GREATER_THAN, 3U);
            break;
        }
        case lexer::TokenType::PUNCTUATOR_GREATER_THAN: {
            break;
        }
        default: {
            if (throwError) {
                LogError(diagnostic::EXPECTED_PARAM_GOT_PARAM,
                         {TokenToString(lexer::TokenType::PUNCTUATOR_GREATER_THAN),
                          TokenToString(Lexer()->GetToken().Type())});
            }
            return false;
        }
    }
    Lexer()->NextToken();  // eat `>`
    return true;
}

util::StringView ParserImpl::ParseSymbolIteratorIdentifier() const noexcept
{
    // Duplicate check - just in case of improper call!
    if (lexer_->GetToken().Type() != lexer::TokenType::PUNCTUATOR_LEFT_SQUARE_BRACKET) {
        return util::StringView {};
    }

    lexer_->NextToken();
    if (lexer_->GetToken().Type() != lexer::TokenType::LITERAL_IDENT || !lexer_->GetToken().Ident().Is("Symbol")) {
        return util::StringView {};
    }

    lexer_->NextToken();
    if (lexer_->GetToken().Type() != lexer::TokenType::PUNCTUATOR_PERIOD) {
        return util::StringView {};
    }

    lexer_->NextToken();
    if (lexer_->GetToken().Type() != lexer::TokenType::LITERAL_IDENT || !lexer_->GetToken().Ident().Is("iterator")) {
        return util::StringView {};
    }

    lexer_->NextToken();
    if (lexer_->GetToken().Type() != lexer::TokenType::PUNCTUATOR_RIGHT_SQUARE_BRACKET) {
        return util::StringView {};
    }

    //  Just replace '[Symbol.iterator]` identifier with the standard '$_iterator' name.
    return util::StringView {compiler::Signatures::ITERATOR_METHOD};
}

void ParserImpl::EatTypeAnnotation()
{
    lexer_->NextToken();  // eat ':' or '|'
    if (lexer_->GetToken().Type() != lexer::TokenType::LITERAL_IDENT) {
        LogUnexpectedToken(lexer_->GetToken());
        auto pos = lexer_->Save();
        lexer_->NextToken();
        while (lexer_->GetToken().Type() != lexer::TokenType::LITERAL_IDENT) {
            // just skip usls tokens, we have an identifier after
            lexer_->Rewind(pos);
            lexer_->NextToken();
            pos = lexer_->Save();
        }
        if (lexer_->GetToken().Type() == lexer::TokenType::LITERAL_IDENT) {
            // if next token is not an ident, so current token should be an identifier
            // and we set it as literal ident
            lexer_->GetToken().SetTokenType(lexer::TokenType::LITERAL_IDENT);
            lexer_->GetToken().SetTokenStr(ERROR_LITERAL);
        }
    }
    lexer_->NextToken();  // eat type
    if (lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_BITWISE_OR) {
        EatTypeAnnotation();
    }
}

void ParserImpl::ParseIndexSignature()
{
    if (lexer_->GetToken().Type() != lexer::TokenType::LITERAL_IDENT) {
        return;
    }
    lexer_->NextToken();  // eat param

    if (lexer_->GetToken().Type() != lexer::TokenType::PUNCTUATOR_COLON) {
        return;
    }

    EatTypeAnnotation();

    if (!lexer_->TryEatTokenType(lexer::TokenType::PUNCTUATOR_RIGHT_SQUARE_BRACKET)) {
        return;
    }
    EatTypeAnnotation();
}

ir::Identifier *ParserImpl::ExpectIdentifier([[maybe_unused]] bool isReference, bool isUserDefinedType,
                                             TypeAnnotationParsingOptions options)
{
    auto const &token = lexer_->GetToken();
    auto const tokenType = token.Type();
    if (tokenType == lexer::TokenType::PUNCTUATOR_FORMAT) {
        if (auto *ident = ParseIdentifierFormatPlaceholder(std::nullopt); ident != nullptr) {
            return ident;
        }
    }

    auto const &tokenStart = token.Start();
    if (!IsValidIdentifierName(token) && ((options & TypeAnnotationParsingOptions::ADD_TYPE_PARAMETER_BINDING) == 0)) {
        LogError(diagnostic::PREDEFINED_TYPE_AS_IDENTIFIER, {token.Ident()}, tokenStart);
        lexer_->NextToken();
        return AllocBrokenExpression(tokenStart);
    }

    if (token.IsDefinableTypeName() && isUserDefinedType) {
        LogError(diagnostic::NOT_ALLOWED_USER_DEFINED_TYPE);
    }

    util::StringView tokenName {};

    if (tokenType == lexer::TokenType::LITERAL_IDENT) {
        tokenName = token.Ident();
    } else if (tokenType == lexer::TokenType::PUNCTUATOR_LEFT_SQUARE_BRACKET) {
        // Special case for processing of special '[Symbol.iterator]` identifier using in stdlib.
        tokenName = ParseSymbolIteratorIdentifier();
        if (tokenName.Empty()) {
            LogError(diagnostic::ERROR_ARKTS_NO_PROPERTIES_BY_INDEX, {});
            ParseIndexSignature();
            return AllocBrokenExpression(Lexer()->GetToken().Start());
        }
    }

    if (tokenName.Empty()) {
        if ((options & TypeAnnotationParsingOptions::REPORT_ERROR) == 0) {
            return nullptr;
        }
        if (token.IsLiteral()) {
            LogError(diagnostic::LITERAL_VALUE_IDENT, {token.ToString()}, tokenStart);
        } else if (token.IsKeyword()) {
            LogError(diagnostic::HARD_KEYWORD_IDENT, {token.ToString()}, tokenStart);
        }
        LogError(diagnostic::IDENTIFIER_EXPECTED_HERE, {TokenToString(tokenType)}, tokenStart);
        lexer_->NextToken();
        return AllocBrokenExpression(tokenStart);
    }

    auto *ident = AllocNode<ir::Identifier>(tokenName, Allocator());
    ES2PANDA_ASSERT(ident != nullptr);
    //  NOTE: here actual token can be changed!
    ident->SetRange({tokenStart, lexer_->GetToken().End()});
    lexer_->NextToken();
    return ident;
}

void ParserImpl::ExpectToken(lexer::TokenType tokenType, bool consumeToken)
{
    auto const &token = lexer_->GetToken();
    auto const actualType = token.Type();
    if (actualType == tokenType) {
        if (consumeToken) {
            lexer_->NextToken();
        }
        return;
    }

    if (tokenType != lexer::TokenType::LITERAL_IDENT) {
        LogError(diagnostic::EXPECTED_PARAM_GOT_PARAM, {TokenToString(tokenType), TokenToString(actualType)});
    } else {
        LogError(diagnostic::UNEXPECTED_TOKEN_ID);
    }

    if (!consumeToken) {
        return;
    }

    if (!lexer::Token::IsPunctuatorToken(actualType)) {
        return;
    }

    auto savedPos = lexer_->Save();
    lexer_->NextToken();
    if (lexer_->GetToken().Type() == tokenType) {
        lexer_->NextToken();
        return;
    }

    lexer_->Rewind(savedPos);
}

void ParserImpl::LogUnexpectedToken(lexer::TokenType tokenType)
{
    LogError(diagnostic::UNEXPECTED_TOKEN_PARAM, {TokenToString(tokenType)});
}

void ParserImpl::LogUnexpectedToken(lexer::Token const &token)
{
    if (token.ToString() != ERROR_LITERAL) {
        LogError(diagnostic::UNEXPECTED_TOKEN_PARAM, {token.ToString()});
    }
}

void ParserImpl::LogExpectedToken(lexer::TokenType tokenType)
{
    if (tokenType != lexer::TokenType::LITERAL_IDENT && tokenType != lexer::TokenType::LITERAL_STRING) {
        LogError(diagnostic::UNEXPECTED_TOKEN_EXPECTED_PARAM, {TokenToString(tokenType)});
    } else if (tokenType == lexer::TokenType::LITERAL_IDENT) {
        LogError(diagnostic::UNEXPECTED_TOKEN_ID);
        lexer_->GetToken().SetTokenStr(ERROR_LITERAL);
    } else if (tokenType == lexer::TokenType::LITERAL_STRING) {
        LogError(diagnostic::UNEXPECTED_TOKEN_STRING_LITERAL);
        lexer_->GetToken().SetTokenStr(ERROR_LITERAL);
    }
    lexer_->GetToken().SetTokenType(tokenType);
}

void ParserImpl::LogSyntaxError(std::string_view errorMessage, const lexer::SourcePosition &pos)
{
    diagnosticEngine_.LogSyntaxError(errorMessage, pos);
}

void ParserImpl::LogSyntaxError(std::string_view const errorMessage)
{
    diagnosticEngine_.LogSyntaxError(errorMessage, lexer_->GetToken().Start());
}

void ParserImpl::LogSyntaxError(const util::DiagnosticMessageParams &list)
{
    diagnosticEngine_.LogSyntaxError(list, lexer_->GetToken().Start());
}

void ParserImpl::LogSyntaxError(const util::DiagnosticMessageParams &list, const lexer::SourcePosition &pos)
{
    diagnosticEngine_.LogSyntaxError(list, pos);
}

void ParserImpl::LogError(const diagnostic::DiagnosticKind &diagnostic,
                          const util::DiagnosticMessageParams &diagnosticParams, const lexer::SourcePosition &pos)
{
    diagnosticEngine_.LogDiagnostic(diagnostic, diagnosticParams, pos);
}

void ParserImpl::LogError(const diagnostic::DiagnosticKind &diagnostic,
                          const util::DiagnosticMessageParams &diagnosticParams)
{
    LogError(diagnostic, diagnosticParams, lexer_->GetToken().Start());
}

lexer::SourcePosition ParserImpl::GetPositionForDiagnostic() const
{
    return Lexer()->GetToken().Start();
}

bool ParserImpl::CheckModuleAsModifier()
{
    if (lexer_->GetToken().KeywordType() != lexer::TokenType::KEYW_AS) {
        return false;
    }

    if ((lexer_->GetToken().Flags() & lexer::TokenFlags::HAS_ESCAPE) != 0U) {
        LogError(diagnostic::ESCAPE_SEQUENCES_IN_AS);
    }

    return true;
}

bool ParserImpl::ParseList(std::optional<lexer::TokenType> termToken, lexer::NextTokenFlags flags,
                           const std::function<bool()> &parseElement, lexer::SourcePosition *sourceEnd,
                           bool allowTrailingSep)
{
    bool success = true;
    auto sep = lexer::TokenType::PUNCTUATOR_COMMA;
    while (Lexer()->GetToken().Type() != termToken && Lexer()->GetToken().Type() != lexer::TokenType::EOS) {
        // ErrorRecursionGuard is not feasible because we can break without consuming any tokens
        auto savedPos = lexer_->Save();
        auto elemSuccess = parseElement();
        bool hasSep = false;
        if (Lexer()->GetToken().Type() == sep) {
            Lexer()->NextToken(flags);
            hasSep = true;
        }
        if (!elemSuccess) {
            // list element is invalid
            success = false;
            if (savedPos == lexer_->Save()) {
                lexer_->NextToken();
            }
            continue;
        }
        if (termToken == Lexer()->GetToken().Type() || (!termToken.has_value() && !hasSep)) {
            if (hasSep && !allowTrailingSep) {
                LogError(diagnostic::TRAILING_COMMA_NOT_ALLOWED);
            }
            break;
        }
        if (hasSep) {
            continue;
        }
        if (termToken.has_value()) {
            LogError(diagnostic::UNEXPECTED_TOKEN_EXPECTED_PARAM_OR_PARAM,
                     {lexer::TokenToString(sep), lexer::TokenToString(termToken.value())});
        } else {
            LogExpectedToken(sep);
        }
        // comma or terminator not found
        return false;
    }
    if (termToken) {
        if (sourceEnd != nullptr) {
            *sourceEnd = Lexer()->GetToken().End();
        }
        ExpectToken(termToken.value());
    }
    return success;
}

ir::Identifier *ParserImpl::AllocBrokenExpression(const lexer::SourcePosition &pos)
{
    return AllocBrokenExpression({pos, pos});
}

ir::Identifier *ParserImpl::AllocBrokenExpression(const lexer::SourceRange &range)
{
    auto *node = AllocNode<ir::Identifier>(Allocator());
    ES2PANDA_ASSERT(node != nullptr);
    node->SetRange(range);
    return node;
}

ir::TypeNode *ParserImpl::AllocBrokenType(const lexer::SourcePosition &pos)
{
    return AllocBrokenType({pos, pos});
}

ir::TypeNode *ParserImpl::AllocBrokenType(const lexer::SourceRange &range)
{
    auto node = AllocNode<ir::BrokenTypeNode>(Allocator());
    ES2PANDA_ASSERT(node != nullptr);
    node->SetRange(range);
    return node;
}

}  // namespace ark::es2panda::parser
