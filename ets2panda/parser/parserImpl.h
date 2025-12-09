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

#ifndef ES2PANDA_PARSER_CORE_PARSER_IMPL_H
#define ES2PANDA_PARSER_CORE_PARSER_IMPL_H

#include "forwardDeclForParserImpl.h"
#include "ir/base/scriptFunctionSignature.h"
#include "lexer/token/sourceLocation.h"
#include "generated/tokenType.h"
#include "parser/context/classPrivateContext.h"
#include "parser/context/parserContext.h"
#include "parser/parserFlags.h"
#include "parser/program/program.h"
#include "util/diagnosticEngine.h"
#include "util/helpers.h"
#include "util/recursiveGuard.h"

namespace ark::es2panda::lexer {
class RegExpParser;
enum class NextTokenFlags : uint32_t;
}  // namespace ark::es2panda::lexer

namespace ark::es2panda::util {
class Options;
class SourcePositionHelper;
class ImportPathManager;
}  // namespace ark::es2panda::util

namespace ark::es2panda::public_lib {
struct Context;
}  // namespace ark::es2panda::public_lib

namespace ark::es2panda::parser {
using ENUMBITOPS_OPERATORS;

enum class TypeAnnotationParsingOptions : uint32_t {
    NO_OPTS = 0U,
    IN_UNION = 1U << 0U,
    ALLOW_CONST = 1U << 1U,
    IN_INTERSECTION = 1U << 2U,
    RESTRICT_EXTENDS = 1U << 3U,
    REPORT_ERROR = 1U << 4U,
    CAN_BE_TS_TYPE_PREDICATE = 1U << 5U,
    BREAK_AT_NEW_LINE = 1U << 6U,
    RETURN_TYPE = 1U << 7U,
    POTENTIAL_CLASS_LITERAL = 1U << 8U,
    ALLOW_INTERSECTION = 1U << 9U,
    ADD_TYPE_PARAMETER_BINDING = 1U << 10U,
    DISALLOW_PRIMARY_TYPE = 1U << 11U,
    ALLOW_WILDCARD = 1U << 12U,
    IGNORE_FUNCTION_TYPE = 1U << 13U,
    ALLOW_DECLARATION_SITE_VARIANCE = 1U << 14U,
    DISALLOW_UNION = 1U << 15U,
    POTENTIAL_NEW_ARRAY = 1U << 16U,
    ANNOTATION_NOT_ALLOW = 1U << 17U,
    INSTANCEOF = 1U << 18U,
    TYPE_ALIAS_CONTEXT = 1U << 19U,
    IGNORE_KEYW_KEYOF = 1U << 20U
};

enum class ParseListOptions : uint32_t {
    NO_OPT = 0U,
    ALLOW_TRAILING_SEP = 1U << 0U,
    ALLOW_TYPE_KEYWORD = 1U << 1U,
};

class ParserImpl {
public:
    explicit ParserImpl(Program *program, const util::Options *options, util::DiagnosticEngine &diagnosticEngine,
                        ParserStatus status = ParserStatus::NO_OPTS);
    NO_COPY_SEMANTIC(ParserImpl);
    NO_MOVE_SEMANTIC(ParserImpl);
    virtual ~ParserImpl() = default;

    void ParseScript(const SourceFile &sourceFile, bool genStdLib);

    [[nodiscard]] virtual bool IsETSParser() const noexcept
    {
        return false;
    }

    [[nodiscard]] virtual bool IsValidIdentifierName([[maybe_unused]] const lexer::Token &token) const noexcept
    {
        return true;
    }

    ETSParser *AsETSParser()
    {
        ES2PANDA_ASSERT(IsETSParser());
        return reinterpret_cast<ETSParser *>(this);
    }

    const ETSParser *AsETSParser() const
    {
        ES2PANDA_ASSERT(IsETSParser());
        return reinterpret_cast<const ETSParser *>(this);
    }

    util::DiagnosticEngine &DiagnosticEngine() const
    {
        return diagnosticEngine_;
    }

    void SetParserStatus(ParserStatus status)
    {
        GetContext().Status() |= status;
    }

    bool HasParserStatus(ParserStatus status) const noexcept
    {
        return (GetContext().Status() & status) != 0;
    }

    lexer::SourcePosition GetPositionForDiagnostic() const;

    void SetContext(public_lib::Context *ctx)
    {
        ctx_ = ctx;
    }

    public_lib::Context *Context() const
    {
        return ctx_;
    }

protected:
    virtual void ParseProgram(ScriptKind kind);
    static ExpressionParseFlags CarryExpressionParserFlag(ExpressionParseFlags origin, ExpressionParseFlags carry);
    static ExpressionParseFlags CarryPatternFlags(ExpressionParseFlags flags);

    void LogIfPrivateIdent(ClassElementDescriptor *desc, const diagnostic::DiagnosticKind &diagnostic,
                           const util::DiagnosticMessageParams &diagnosticParams = {});
    void ValidateClassKey(ClassElementDescriptor *desc);
    bool ValidatePrivateIdentifier();

    ir::VariableDeclaratorFlag GetFlag(VariableParsingFlags flags);

    void ValidateAccessor(ExpressionParseFlags flags);
    void CheckPropertyKeyAsyncModifier(ParserStatus *methodStatus);
    ir::Property *ParseShorthandProperty(const lexer::LexerPosition *startPos);
    void ParseGeneratorPropertyModifier(ExpressionParseFlags flags, ParserStatus *methodStatus);
    bool ParsePropertyModifiers(ExpressionParseFlags flags, ir::PropertyKind *propertyKind, ParserStatus *methodStatus);
    ir::Expression *ParsePropertyValue(ir::PropertyKind propertyKind, ParserStatus methodStatus,
                                       ExpressionParseFlags flags = ExpressionParseFlags::NO_OPTS);
    void ParsePropertyEndErrorHendler();
    bool ParsePropertyEnd();

    // ExpressionParser.Cpp

    ir::Expression *ParseKeywordExpression();
    ir::Expression *ParseBinaryExpression(ir::Expression *left,
                                          ExpressionParseFlags flags = ExpressionParseFlags::NO_OPTS);
    void ValidateUpdateExpression(ir::Expression *returnExpression, bool isChainExpression);
    ir::Expression *ParseMemberExpression(bool ignoreCallExpression = false,
                                          ExpressionParseFlags flags = ExpressionParseFlags::NO_OPTS);
    ir::Expression *SetupChainExpr(ir::Expression *const top, lexer::SourcePosition startLoc);
    ir::MetaProperty *ParsePotentialNewTarget();
    void ValidateParenthesizedExpression(ir::Expression *lhsExpression);
    void ValidateGroupedExpression(ir::Expression *lhsExpression);
    ir::Expression *ParseImportExpression();
    ir::Expression *ParseOptionalChain(ir::Expression *leftSideExpr);
    virtual ir::Expression *ParsePropertyKey(ExpressionParseFlags flags);
    void ValidateAssignmentTarget(ExpressionParseFlags flags, ir::Expression *node);
    void ValidateLvalueAssignmentTarget(ir::Expression *node);
    void ValidateArrowParameterBindings(const ir::Expression *node);
    ir::Identifier *ParseNamedExport(lexer::Token *exportedToken);
    virtual void ParseTrailingBlock([[maybe_unused]] ir::CallExpression *callExpr) {}
    ir::Expression *CreateBinaryAssignmentExpression(ir::Expression *assignmentExpression,
                                                     ir::Expression *lhsExpression, lexer::TokenType tokenType);
    virtual bool IsInOperatorTypeSupported() const
    {
        return true;
    }

    // StatementParser.Cpp

    void CheckFunctionDeclaration(StatementParsingFlags flags);
    void CheckLabelledFunction(const ir::Statement *node);
    bool ParseDirective(ArenaVector<ir::Statement *> *statements);
    void ParseDirectivePrologue(ArenaVector<ir::Statement *> *statements);
    ir::BlockStatement *ParseFunctionBody();
    std::tuple<ForStatementKind, ir::AstNode *, ir::Expression *, ir::Expression *> ParseIsForInOf(
        ir::Expression *leftNode, ExpressionParseFlags exprFlags);
    std::tuple<ForStatementKind, ir::AstNode *, ir::Expression *, ir::Expression *> ParseForInOf(
        ir::Expression *leftNode, ExpressionParseFlags exprFlags, bool isAwait);
    std::tuple<ForStatementKind, ir::Expression *, ir::Expression *> ParseForInOf(ir::AstNode *initNode,
                                                                                  ExpressionParseFlags exprFlags,
                                                                                  bool isAwait);
    std::tuple<ir::Expression *, ir::Expression *> ParseForUpdate(bool isAwait);
    std::tuple<ir::Expression *, ir::Statement *> ParseForLoopInitializer();
    ir::SwitchCaseStatement *ParseSwitchCaseStatement(bool *seenDefault);
    virtual ir::Expression *ParseCatchParam();
    ir::CatchClause *ParseCatchClause();
    ir::VariableDeclaration *ParseContextualLet(VariableParsingFlags flags,
                                                StatementParsingFlags stmFlags = StatementParsingFlags::ALLOW_LEXICAL);
    ir::Statement *CreateForStatement(struct ForStatementNodes &&nodes, ForStatementKind forKind,
                                      const lexer::SourcePosition &startLoc, bool isAwait);

    friend class Lexer;
    friend class SavedParserContext;
    friend class SavedClassPrivateContext;
    friend class ArrowFunctionContext;
    friend class ETSNolintParser;
    friend class lexer::RegExpParser;
    friend class util::SourcePositionHelper;
    friend class JsdocHelper;
    friend class util::ImportPathManager;

    void LogExpectedToken(lexer::TokenType tokenType);
    void LogUnexpectedToken(lexer::TokenType tokenType);
    void LogUnexpectedToken(lexer::Token const &token);

public:
    void LogError(util::DiagnosticType errorType, std::string_view errorMessage, const lexer::SourcePosition &pos);
    void LogError(const diagnostic::DiagnosticKind &diagnostic,
                  const util::DiagnosticMessageParams &diagnosticParams = {});
    void LogError(const diagnostic::DiagnosticKind &diagnostic, const util::DiagnosticMessageParams &diagnosticParams,
                  const lexer::SourcePosition &pos);
    void LogSyntaxError(std::string_view errorMessage);
    void LogSyntaxError(const util::DiagnosticMessageParams &list);
    void LogSyntaxError(std::string_view errorMessage, const lexer::SourcePosition &pos);
    void LogSyntaxError(const util::DiagnosticMessageParams &list, const lexer::SourcePosition &pos);

protected:
    void LogParameterModifierError(ir::ModifierFlags status);
    // Error handling
    ir::Statement *AllocEmptyStatement();
    ir::Statement *AllocBrokenStatement(const lexer::SourcePosition &pos);
    ir::Statement *AllocBrokenStatement(const lexer::SourceRange &range);
    bool IsBrokenStatement(ir::Statement *st);

    template <typename T, typename... Args>
    T *AllocNode(Args &&...args)
    {
        auto *ret = util::NodeAllocator::ForceSetParent<T>(
            Allocator(), std::forward<Args>(args)...);  // Note: replace with AllocNode
        ES2PANDA_ASSERT(ret != nullptr);
        return ret;
    }

    ir::Identifier *AllocBrokenExpression(const lexer::SourcePosition &pos);
    ir::Identifier *AllocBrokenExpression(const lexer::SourceRange &range);
    ir::TypeNode *AllocBrokenType(const lexer::SourcePosition &pos);
    ir::TypeNode *AllocBrokenType(const lexer::SourceRange &range);

    ArenaAllocator *Allocator() const
    {
        return program_->Allocator();
    }

    bool CheckModuleAsModifier();

    // ETS extension
    virtual bool IsExternal() const
    {
        return false;
    }

    void ParseIndexSignature();
    void EatTypeAnnotation();
    bool ParsePunctuatorGreaterThan(bool throwError = true);
    util::StringView ParseSymbolIteratorIdentifier() const noexcept;
    ir::Identifier *ExpectIdentifier(bool isReference = false, bool isUserDefinedType = false,
                                     TypeAnnotationParsingOptions options = TypeAnnotationParsingOptions::REPORT_ERROR);
    void ExpectToken(lexer::TokenType tokenType, bool consumeToken = true);

    // ExpressionParser.cpp

    ir::SpreadElement *ParseSpreadElement(ExpressionParseFlags flags = ExpressionParseFlags::NO_OPTS);
    ir::YieldExpression *ParseYieldExpression();
    virtual ir::Expression *ParsePotentialExpressionSequence(ir::Expression *expr, ExpressionParseFlags flags);
    ir::ArrowFunctionExpression *ParseArrowFunctionExpressionBody(ArrowFunctionContext *arrowFunctionContext,
                                                                  ArrowFunctionDescriptor *desc,
                                                                  ir::TSTypeParameterDeclaration *typeParamDecl,
                                                                  ir::TypeNode *returnTypeAnnotation);
    ir::Expression *ParseAssignmentExpressionHelper();
    ir::Expression *ParseAssignmentExpression(ir::Expression *lhsExpression,
                                              ExpressionParseFlags flags = ExpressionParseFlags::NO_OPTS);
    ir::Expression *ParseAssignmentBinaryExpression(lexer::TokenType tokenType, ir::Expression *lhsExpression,
                                                    ExpressionParseFlags flags);
    ir::Expression *ParseAssignmentEqualExpression(lexer::TokenType tokenType, ir::Expression *lhsExpression,
                                                   ExpressionParseFlags flags);
    ir::SequenceExpression *ParseSequenceExpression(ir::Expression *startExpr, bool acceptRest = false);
    ir::FunctionExpression *ParseFunctionExpression(ParserStatus newStatus = ParserStatus::NO_OPTS);
    ir::ArrowFunctionExpression *ParseArrowFunctionExpression(ir::Expression *expr,
                                                              ir::TSTypeParameterDeclaration *typeParamDecl,
                                                              ir::TypeNode *returnTypeAnnotation, bool isAsync);
    ir::CallExpression *ParseCallExpression(ir::Expression *callee, bool isOptionalChain = false,
                                            bool handleEval = true);
    ArenaVector<ir::Expression *> ParseCallExpressionArguments(bool &trailingComma, lexer::SourcePosition &endLoc);

    ir::TemplateLiteral *ParseTemplateLiteral();
    ir::Expression *ParseLeftHandSideExpression(ExpressionParseFlags flags = ExpressionParseFlags::NO_OPTS);
    void ParseNameSpaceImport(ArenaVector<ir::AstNode *> *specifiers);
    void ParseNamedImportSpecifiers(ArenaVector<ir::AstNode *> *specifiers);
    ir::StringLiteral *ParseFromClause(bool requireFrom = true);

    ir::BooleanLiteral *ParseBooleanLiteral();
    ir::NullLiteral *ParseNullLiteral();
    ir::Literal *ParseNumberLiteral();
    ir::CharLiteral *ParseCharLiteral();
    ir::StringLiteral *ParseStringLiteral();
    ir::UndefinedLiteral *ParseUndefinedLiteral();
    virtual ir::ThisExpression *ParseThisExpression();
    ir::RegExpLiteral *ParseRegularExpression();
    ir::SuperExpression *ParseSuperExpression();
    ir::MemberExpression *ParseElementAccess(ir::Expression *primaryExpr, bool isOptional = false);
    ir::MemberExpression *ParsePrivatePropertyAccess(ir::Expression *primaryExpr);
    ir::MemberExpression *ParsePropertyAccess(ir::Expression *primaryExpr, lexer::SourcePosition periodPos,
                                              bool isOptional = false);
    void CreateAmendedBinaryExpression(ir::Expression *left, ir::Expression *right, lexer::TokenType operatorType);

    // StatementParser
    ArenaVector<ir::Statement *> ParseStatementList(StatementParsingFlags flags = StatementParsingFlags::ALLOW_LEXICAL);
    virtual bool ValidateLabeledStatement(lexer::TokenType type);
    ir::BlockStatement *ParseBlockStatement();
    ir::EmptyStatement *ParseEmptyStatement();
    ir::Statement *ParseForStatement();
    ir::Statement *ParseIfStatement();
    virtual ir::Statement *ParseFunctionStatement(StatementParsingFlags flags);
    // NOLINTNEXTLINE(google-default-arguments)
    virtual ir::Statement *ParseClassStatement(StatementParsingFlags flags, ir::ClassDefinitionModifiers modifiers,
                                               ir::ModifierFlags modFlags = ir::ModifierFlags::NONE);
    // NOLINTNEXTLINE(google-default-arguments)
    virtual ir::Statement *ParseStructStatement(StatementParsingFlags flags, ir::ClassDefinitionModifiers modifiers,
                                                ir::ModifierFlags modFlags = ir::ModifierFlags::NONE);
    virtual ir::Statement *ParseInterfaceStatement(StatementParsingFlags flags);
    virtual ir::Statement *ParseEnumStatement(StatementParsingFlags flags);
    ir::Statement *ParseStatementBasedOnTokenType(StatementParsingFlags flags);
    ir::Statement *ParseVarStatement();
    ir::Statement *ParseLetStatement(StatementParsingFlags flags);
    ir::Statement *ParseBreakStatement();
    ir::Statement *ParseContinueStatement();
    ir::Statement *ParseDoWhileStatement();
    ir::Statement *ParseWhileStatement();
    ir::Statement *ParseSwitchStatement();
    ir::Statement *ParseReturnStatement();
    ir::Statement *ParseExpressionStatement(StatementParsingFlags flags = StatementParsingFlags::NONE);
    ir::Statement *ParseLabelledStatement(const lexer::LexerPosition &pos);
    virtual void ValidateRestParameter(ir::Expression *param);
    virtual bool ValidateBreakLabel(util::StringView label);
    virtual bool ValidateContinueLabel(util::StringView label);
    bool InAmbientContext();

    ir::MethodDefinition *BuildImplicitConstructor(ir::ClassDefinitionModifiers modifiers,
                                                   const lexer::SourcePosition &startLoc);

    virtual void CreateImplicitConstructor(ir::MethodDefinition *&ctor, ArenaVector<ir::AstNode *> &properties,
                                           ir::ClassDefinitionModifiers modifiers, ir::ModifierFlags flags,
                                           const lexer::SourcePosition &startLoc);
    void CheckClassGeneratorMethod(ClassElementDescriptor *desc, char32_t *nextCp);
    void ParseClassAccessor(ClassElementDescriptor *desc, char32_t *nextCp);
    ir::Expression *ParseClassKey(ClassElementDescriptor *desc);
    ir::ClassElement *ParseClassProperty(ClassElementDescriptor *desc, const ArenaVector<ir::AstNode *> &properties,
                                         ir::Expression *propName, ir::TypeNode *typeAnnotation);
    void AddPrivateElement(const ir::ClassElement *elem);
    ir::ScriptFunction *ParseFunction(ParserStatus newStatus = ParserStatus::NO_OPTS);
    ir::ModifierFlags GetAccessability(ir::ModifierFlags modifiers);
    void CheckAccessorPair(const ArenaVector<ir::AstNode *> &properties, const ir::Expression *propName,
                           ir::MethodDefinitionKind methodKind, ir::ModifierFlags access);
    ir::Identifier *ParseNamedImport(lexer::Token *importedToken);
    void ConsumeSemicolon(ir::Statement *statement);
    ir::ExportAllDeclaration *ParseExportAllDeclaration(const lexer::SourcePosition &startLoc);
    ir::ExportNamedDeclaration *ParseExportNamedSpecifiers(const lexer::SourcePosition &startLoc);
    ir::Statement *ParseVariableDeclaration(VariableParsingFlags flags = VariableParsingFlags::NO_OPTS);
    void ValidateDeclaratorId();
    void CheckRestrictedBinding();
    void CheckRestrictedBinding(lexer::TokenType keywordType);
    void CheckRestrictedBinding(const util::StringView &ident, const lexer::SourcePosition &pos);

    ir::VariableDeclarator *ParseVariableDeclarator(VariableParsingFlags flags);
    ir::FunctionDeclaration *ParseFunctionDeclaration(bool canBeAnonymous = false,
                                                      ParserStatus newStatus = ParserStatus::NO_OPTS);
    ir::Statement *ParseStructDeclaration(ir::ClassDefinitionModifiers modifiers,
                                          ir::ModifierFlags flags = ir::ModifierFlags::NONE);
    ir::Statement *ParseClassDeclaration(ir::ClassDefinitionModifiers modifiers,
                                         ir::ModifierFlags flags = ir::ModifierFlags::NONE);
    FunctionSignature ParseFunctionSignature(ParserStatus status);

    [[nodiscard]] virtual std::unique_ptr<lexer::Lexer> InitLexer(const SourceFile &sourceFile);
    // NOLINTNEXTLINE(google-default-arguments)
    ir::Statement *ParseStatementLiteralIdentHelper(StatementParsingFlags flags = StatementParsingFlags::NONE);
    // NOLINTNEXTLINE(google-default-arguments)
    ir::Statement *ParseStatementControlFlowTokenHelper(StatementParsingFlags flags = StatementParsingFlags::NONE);
    // NOLINTNEXTLINE(google-default-arguments)
    ir::Statement *ParseStatementPunctuatorsHelper(StatementParsingFlags flags = StatementParsingFlags::NONE);
    // NOLINTNEXTLINE(google-default-arguments)
    virtual ir::Statement *ParseStatement(StatementParsingFlags flags = StatementParsingFlags::NONE);
    // NOLINTNEXTLINE(google-default-arguments)
    virtual ir::Expression *ParseExpression(ExpressionParseFlags flags = ExpressionParseFlags::NO_OPTS);
    // NOLINTNEXTLINE(google-default-arguments)
    virtual ir::Expression *ParseExpressionOrTypeAnnotation(lexer::TokenType type, ExpressionParseFlags flags);
    // NOLINTNEXTLINE(google-default-arguments)
    virtual ir::Expression *ParsePatternElement(ExpressionParseFlags flags = ExpressionParseFlags::NO_OPTS,
                                                bool allowDefault = true);
    void ParsePatternElementErrorCheck(ExpressionParseFlags flags, bool allowDefault);
    virtual bool ParsePotentialNonNullExpression(ir::Expression **returnExpression, lexer::SourcePosition startLoc);
    virtual ir::AstNode *ParseImportSpecifiers(ArenaVector<ir::AstNode *> *specifiers);
    virtual ir::Statement *ParseImportDeclaration(StatementParsingFlags flags);
    // NOLINTNEXTLINE(google-default-arguments)
    virtual ir::Expression *ParsePropertyDefinition(
        [[maybe_unused]] ExpressionParseFlags flags = ExpressionParseFlags::NO_OPTS);
    // NOLINTNEXTLINE(google-default-arguments)
    virtual ir::ObjectExpression *ParseObjectExpression(ExpressionParseFlags flags = ExpressionParseFlags::NO_OPTS);
    bool ParseArrayExpressionRightBracketHelper(bool containsRest, const lexer::SourcePosition &startLoc);
    // NOLINTNEXTLINE(google-default-arguments)
    virtual ir::ArrayExpression *ParseArrayExpression(ExpressionParseFlags flags, bool allowOmitted);
    virtual ir::ArrayExpression *ParseArrayExpression(ExpressionParseFlags flags);
    void ParseArrayExpressionErrorCheck(ir::ArrayExpression *arrayExpressionNode, ExpressionParseFlags flags,
                                        bool inPattern);
    virtual ir::ArrowFunctionExpression *ParsePotentialArrowExpression(ir::Expression **returnExpression,
                                                                       const lexer::SourcePosition &startLoc);
    virtual bool ParsePotentialGenericFunctionCall(ir::Expression *primaryExpr, ir::Expression **returnExpression,
                                                   const lexer::SourcePosition &startLoc, bool ignoreCallExpression);
    virtual ir::Expression *ParsePotentialAsExpression(ir::Expression *primaryExpression);
    virtual bool IsNamedFunctionExpression();
    virtual ir::Identifier *ParsePrimaryExpressionIdent(ExpressionParseFlags flags);
    virtual bool ValidateArrowFunctionRestParameter(ir::SpreadElement *restElement);
    virtual ir::Statement *ParsePotentialExpressionStatement(StatementParsingFlags flags);
    virtual ArenaVector<ir::Expression *> ParseFunctionParams();
    virtual ir::Expression *CreateParameterThis(ir::TypeNode *typeAnnotation);
    virtual ir::Expression *ParseFunctionParameter();
    virtual void ConvertThisKeywordToIdentIfNecessary() {}
    virtual void ParseCatchParamTypeAnnotation(ir::AnnotatedExpression *param);
    // NOLINTNEXTLINE(google-default-arguments)
    virtual ir::ClassDefinition *ParseClassDefinition(ir::ClassDefinitionModifiers modifiers,
                                                      ir::ModifierFlags flags = ir::ModifierFlags::NONE);
    virtual ir::Identifier *ParseClassIdent(ir::ClassDefinitionModifiers modifiers);
    virtual ir::Statement *ParsePotentialConstEnum(VariableParsingFlags flags);
    virtual ir::AstNode *ParseClassElement(const ArenaVector<ir::AstNode *> &properties,
                                           ir::ClassDefinitionModifiers modifiers, ir::ModifierFlags flags);
    virtual bool CheckClassElement(ir::AstNode *property, ir::MethodDefinition *&ctor,
                                   ArenaVector<ir::AstNode *> &properties);
    virtual void ValidateClassMethodStart(ClassElementDescriptor *desc, ir::TypeNode *typeAnnotation);
    virtual ir::MethodDefinition *ParseClassMethod(ClassElementDescriptor *desc,
                                                   const ArenaVector<ir::AstNode *> &properties,
                                                   ir::Expression *propName, lexer::SourcePosition *propEnd);
    // NOLINTNEXTLINE(google-default-arguments)
    virtual void ValidateGetterSetter(ir::MethodDefinitionKind methodDefinition, const ir::ScriptFunction *func,
                                      bool hasReceiver = false);
    virtual void ValidateClassSetter(ClassElementDescriptor *desc, const ArenaVector<ir::AstNode *> &properties,
                                     ir::Expression *propName, ir::ScriptFunction *func);
    virtual void ValidateClassGetter(ClassElementDescriptor *desc, const ArenaVector<ir::AstNode *> &properties,
                                     ir::Expression *propName, ir::ScriptFunction *func);
    virtual ir::ModifierFlags ParseModifiers();
    virtual ir::Statement *ParseConstStatement(StatementParsingFlags flags);

    virtual ir::AnnotatedExpression *ParseVariableDeclaratorKey(VariableParsingFlags flags);
    virtual ir::Statement *ParseAnnotationsInStatement(StatementParsingFlags flags);
    virtual ir::Statement *ParseInitModuleStatement();
    virtual ir::VariableDeclarator *ParseVariableDeclarator(ir::Expression *init, lexer::SourcePosition startLoc,
                                                            VariableParsingFlags flags);
    virtual ir::VariableDeclarator *ParseVariableDeclaratorInitializer(ir::Expression *init, VariableParsingFlags flags,
                                                                       const lexer::SourcePosition &startLoc);
    virtual bool IsModifierKind(const lexer::Token &token);
    virtual void ConsumeClassPrivateIdentifier(ClassElementDescriptor *desc, char32_t *nextCp);
    virtual void ReportPossibleOutOfBoundaryJumpError(bool allowBreak);
    virtual void ReportIllegalBreakError(const lexer::SourcePosition &startLoc);
    virtual void ReportIllegalContinueError();
    virtual void ReportIfBodyEmptyError(ir::Statement *consequent);
    virtual void ReportMultipleDefaultError();
    virtual void ReportIllegalNewLineErrorAfterThrow();
    virtual void ReportIfVarDeclaration(VariableParsingFlags flags);
    virtual ir::Expression *ParsePrefixAssertionExpression();
    // NOLINTNEXTLINE(google-default-arguments)
    virtual ir::Expression *ParseCoverParenthesizedExpressionAndArrowParameterList(
        ExpressionParseFlags flags = ExpressionParseFlags::NO_OPTS);
    virtual void CheckIfStaticConstructor(ir::ModifierFlags flags);
    virtual std::tuple<bool, bool, bool> ParseComputedClassFieldOrIndexSignature(ir::Expression **propName);
    // NOLINTNEXTLINE(google-default-arguments)
    virtual ir::Expression *ParseUnaryOrPrefixUpdateExpression(
        ExpressionParseFlags flags = ExpressionParseFlags::NO_OPTS);
    virtual ir::Expression *ParsePrimaryExpressionWithLiterals(ExpressionParseFlags flags);
    ir::Expression *ParseHashMaskOperator();
    ir::Expression *ParseClassExpression();
    ir::Expression *ParsePunctuators(ExpressionParseFlags flags);
    // NOLINTNEXTLINE(google-default-arguments)
    virtual ir::Expression *ParsePrimaryExpression(ExpressionParseFlags flags = ExpressionParseFlags::NO_OPTS);
    virtual ir::Expression *ParsePostPrimaryExpression(ir::Expression *primaryExpr, lexer::SourcePosition startLoc,
                                                       bool ignoreCallExpression, bool *isChainExpression);
    ir::Expression *ParsePostPrimaryExpressionBackTick(ir::Expression *returnExpression,
                                                       lexer::SourcePosition startLoc);
    ir::Expression *ParsePostPrimaryExpressionDot(ir::Expression *returnExpression, lexer::TokenType tokenType,
                                                  bool *isChainExpression);
    virtual ir::ClassElement *ParseClassStaticBlock();
    virtual ParserStatus ValidateArrowParameter(ir::Expression *expr, bool *seenOptional);
    void ValidateArrowParameterAssignment(ir::AssignmentExpression *expr);
    virtual ArrowFunctionDescriptor ConvertToArrowParameter(ir::Expression *expr, bool isAsync);
    virtual ir::Expression *ParseNewExpression();

    virtual ir::TSTypeParameterDeclaration *ParseFunctionTypeParameters()
    {
        return nullptr;
    }

    virtual ir::TypeNode *ParseFunctionReturnType([[maybe_unused]] ParserStatus status)
    {
        return nullptr;
    }

    using NodeFormatType = std::tuple<bool, char, std::size_t>;
    virtual ir::Identifier *ParseIdentifierFormatPlaceholder(std::optional<NodeFormatType> nodeFormat);
    virtual ir::Statement *ParseStatementFormatPlaceholder();
    virtual ir::AstNode *ParseTypeParametersFormatPlaceholder();
    virtual ArenaVector<ir::AstNode *> &ParseAstNodesArrayFormatPlaceholder();
    virtual ArenaVector<ir::Statement *> &ParseStatementsArrayFormatPlaceholder();
    virtual ArenaVector<ir::Expression *> &ParseExpressionsArrayFormatPlaceholder();

    virtual std::tuple<bool, ir::BlockStatement *, lexer::SourcePosition, bool> ParseFunctionBody(
        const ArenaVector<ir::Expression *> &params, ParserStatus newStatus, ParserStatus contextStatus);
    virtual ir::AstNode *ParseImportDefaultSpecifier(ArenaVector<ir::AstNode *> *specifiers);
    virtual ir::Statement *ParseExportDeclaration(StatementParsingFlags flags);

    // NOLINTNEXTLINE(google-default-arguments)
    virtual ir::ExportDefaultDeclaration *ParseExportDefaultDeclaration(const lexer::SourcePosition &startLoc,
                                                                        bool isExportEquals = false);
    virtual ir::Statement *ParseNamedExportDeclaration(const lexer::SourcePosition &startLoc);
    virtual bool ValidateForInStatement()
    {
        return true;
    }

    virtual ir::Statement *ParseTryStatement();
    virtual ir::Statement *ParseThrowStatement();
    virtual ir::Statement *ParseDebuggerStatement();
    // NOLINTNEXTLINE(google-default-arguments)
    virtual ir::Statement *ParseModuleDeclaration(StatementParsingFlags flags = StatementParsingFlags::NONE)
    {
        return ParsePotentialExpressionStatement(flags);
    }

    virtual ir::Statement *ParseInterfaceDeclaration([[maybe_unused]] bool isStatic)
    {
        LogUnexpectedToken(lexer::TokenType::KEYW_INTERFACE);
        return nullptr;
    }

    // NOLINTNEXTLINE(google-default-arguments)
    virtual ir::Statement *ParseEnumDeclaration([[maybe_unused]] bool isConst = false,
                                                [[maybe_unused]] bool isStatic = false)
    {
        LogUnexpectedToken(lexer::TokenType::KEYW_ENUM);
        return nullptr;
    }

    virtual std::tuple<ir::Expression *, ir::TSTypeParameterInstantiation *> ParseSuperClass();
    virtual ir::Expression *ParseSuperClassReference();

    using ClassBody = std::tuple<ir::MethodDefinition *, ArenaVector<ir::AstNode *>, lexer::SourceRange>;
    ClassBody ParseClassBody(ir::ClassDefinitionModifiers modifiers, ir::ModifierFlags flags = ir::ModifierFlags::NONE);

    Program *GetProgram() const
    {
        return program_;
    }

    void SetProgram(Program *program)
    {
        program_ = program;
    }

    lexer::Lexer *Lexer() const
    {
        return lexer_;
    }

    void SetLexer(lexer::Lexer *lexer)
    {
        lexer_ = lexer;
    }

    ParserContext &GetContext()
    {
        return context_;
    }

    const ParserContext &GetContext() const
    {
        return context_;
    }

    const auto &GetOptions() const
    {
        return *options_;
    }

    uint32_t &ClassId()
    {
        return classId_;
    }

    bool TryEatTypeKeyword();
    bool ParseList(std::optional<lexer::TokenType> termToken, lexer::NextTokenFlags flags,
                   const std::function<bool(bool &typeKeywordOnSpecifier)> &parseElement,
                   lexer::SourcePosition *sourceEnd = nullptr,
                   ParseListOptions parseListOptions = ParseListOptions::NO_OPT);

    RecursiveContext &RecursiveCtx()
    {
        return recursiveCtx_;
    }

private:
    bool GetCanBeForInOf(ir::Expression *leftNode, ir::AstNode *initNode);
    Program *program_;
    ParserContext context_;
    ClassPrivateContext classPrivateContext_;
    uint32_t classId_ {};
    lexer::Lexer *lexer_ {};
    const util::Options *options_;
    util::DiagnosticEngine &diagnosticEngine_;
    public_lib::Context *ctx_ {nullptr};
    RecursiveContext recursiveCtx_;
};
}  // namespace ark::es2panda::parser

template <>
struct enumbitops::IsAllowedType<ark::es2panda::parser::TypeAnnotationParsingOptions> : std::true_type {
};

template <>
struct enumbitops::IsAllowedType<ark::es2panda::parser::ParseListOptions> : std::true_type {
};

#endif
