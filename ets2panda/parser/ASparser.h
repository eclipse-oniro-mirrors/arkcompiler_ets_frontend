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

#ifndef ES2PANDA_PARSER_CORE_AS_PARSER_H
#define ES2PANDA_PARSER_CORE_AS_PARSER_H

#include "ThrowingTypedParser.h"
#include "parserFlags.h"

namespace ark::es2panda::parser {
class ASParser : public ThrowingTypedParser {
public:
    ASParser(Program *program, const util::Options &options, util::DiagnosticEngine &diagnosticEngine,
             ParserStatus status = ParserStatus::NO_OPTS)
        : ThrowingTypedParser(program, &options, diagnosticEngine, status)
    {
    }

private:
    [[nodiscard]] std::unique_ptr<lexer::Lexer> InitLexer(const SourceFile &sourceFile) override;
    ir::TypeNode *ParseParenthesizedOrFunctionType(bool throwError);
    ir::TypeNode *ParseFunctionType(lexer::SourcePosition startLoc);
    void ParseOptionalFunctionParameter(ir::AnnotatedExpression *returnNode, bool inRest = false);

    // NOLINTNEXTLINE(google-default-arguments)
    ir::Statement *ParseStatement(StatementParsingFlags flags = StatementParsingFlags::NONE) override;
    std::tuple<ir::AnnotatedExpression *, bool> ParsePatternElementToken(ExpressionParseFlags flags);
    ir::Expression *ParsePatternElement(ExpressionParseFlags flags, bool allowDefault) override;
    // NOLINTNEXTLINE(google-default-arguments)
    ir::Expression *ParsePropertyDefinition(
        [[maybe_unused]] ExpressionParseFlags flags = ExpressionParseFlags::NO_OPTS) override;
    bool CurrentIsBasicType() override;
    ir::TypeNode *ParseTypeAnnotationLiteralIdentHelper(ir::TypeNode *type, TypeAnnotationParsingOptions *options);
    ir::TypeNode *ParseTypeAnnotationTokens(ir::TypeNode *type, bool throwError, TypeAnnotationParsingOptions *options);
    ir::TypeNode *ParseTypeAnnotationTokensBitwiseOr(ir::TypeNode *type, bool throwError, bool isNullable);
    ir::TypeNode *ParseTypeAnnotationTokenLeftSquareBracket(ir::TypeNode *type, bool throwError, bool isNullable);
    ir::TypeNode *ParseTypeAnnotation(TypeAnnotationParsingOptions *options) override;
    ir::ArrowFunctionExpression *ParsePotentialArrowExpression(ir::Expression **returnExpression,
                                                               const lexer::SourcePosition &startLoc) override;
    bool ParsePotentialGenericFunctionCall(ir::Expression *primaryExpr, ir::Expression **returnExpression,
                                           const lexer::SourcePosition &startLoc, bool ignoreCallExpression) override;
    bool ParsePotentialNonNullExpression(ir::Expression **returnExpression, lexer::SourcePosition startLoc) override;
    bool IsNamedFunctionExpression() override;
    ir::Expression *ParsePotentialAsExpression(ir::Expression *primaryExpression) override;
    ir::Identifier *ParsePrimaryExpressionIdent(ExpressionParseFlags flags) override;
    bool ValidateArrowFunctionRestParameter(ir::SpreadElement *restElement) override;
    ir::Decorator *ParseDecorator() override;
    void AddDecorators(ir::AstNode *node, ArenaVector<ir::Decorator *> &decorators) override;
    ir::TSTypeAliasDeclaration *ParseTypeAliasDeclaration() override;
    ArenaVector<ir::TSInterfaceHeritage *> ParseInterfaceExtendsClause() override;
    ir::AstNode *ParseTypeLiteralOrInterfaceMember() override;
    // NOLINTNEXTLINE(google-default-arguments)
    ir::TSIndexSignature *ParseIndexSignature(const lexer::SourcePosition &startLoc, bool isReadonly = false) override;
    ir::AstNode *ParsePropertyOrMethodSignature(const lexer::SourcePosition &startLoc, bool isReadonly) override;
    ir::TypeNode *ParseClassKeyAnnotation() override;
    void ValidateClassMethodStart(ClassElementDescriptor *desc, ir::TypeNode *typeAnnotation) override;
    void ValidateClassSetter(ClassElementDescriptor *desc, const ArenaVector<ir::AstNode *> &properties,
                             ir::Expression *propName, ir::ScriptFunction *func) override;
    void ValidateClassGetter(ClassElementDescriptor *desc, const ArenaVector<ir::AstNode *> &properties,
                             ir::Expression *propName, ir::ScriptFunction *func) override;
    bool IsModifierKind(const lexer::Token &token) override;
    void ConsumeClassPrivateIdentifier(ClassElementDescriptor *desc, char32_t *nextCp) override;
    std::tuple<bool, bool, bool> ParseComputedClassFieldOrIndexSignature(ir::Expression **propName) override;
    std::tuple<bool, ir::BlockStatement *, lexer::SourcePosition, bool> ParseFunctionBody(
        const ArenaVector<ir::Expression *> &params, ParserStatus newStatus, ParserStatus contextStatus) override;
    ir::AstNode *ParseImportDefaultSpecifier(ArenaVector<ir::AstNode *> *specifiers) override;
    std::tuple<ir::Expression *, bool> ParseInterfacePropertyKey() override;
    // NOLINTNEXTLINE(google-default-arguments)
    ir::Expression *ParseCoverParenthesizedExpressionAndArrowParameterList(
        ExpressionParseFlags flags = ExpressionParseFlags::NO_OPTS) override;
    ir::Expression *ParseArrowFunctionRestParameter(lexer::SourcePosition start);
    ir::Expression *ParseArrowFunctionNoParameter(lexer::SourcePosition start);
    ir::Expression *ParsePrefixAssertionExpression() override;
    ir::Statement *ParseConstStatement(StatementParsingFlags flags) override;
    ir::Expression *ParseVariableDeclaratorKey(VariableParsingFlags flags) override;
    ir::Statement *ParsePotentialConstEnum(VariableParsingFlags flags) override;
    // NOLINTNEXTLINE(google-default-arguments)
    ir::ExportDefaultDeclaration *ParseExportDefaultDeclaration(const lexer::SourcePosition &startLoc,
                                                                bool isExportEquals = false) override;
    class ParseNamedExportDeclarationHelper;
    ir::Statement *ParseNamedExportDeclaration(const lexer::SourcePosition &startLoc) override;
    ir::AstNode *ParseImportSpecifiers(ArenaVector<ir::AstNode *> *specifiers) override;
    ir::Statement *ParseImportDeclaration(StatementParsingFlags flags) override;
    ArenaVector<ir::TSClassImplements *> ParseClassImplementClause() override;
    ir::ClassElement *ParseClassStaticBlock() override;
    void ParseOptionalClassElement(ClassElementDescriptor *desc) override;
    void ValidateIndexSignatureTypeAnnotation(ir::TypeNode *typeAnnotation) override;
    ArrowFunctionDescriptor ConvertToArrowParameter(ir::Expression *expr, bool isAsync) override;
    ParserStatus ValidateArrowExprIdentifier(ir::Expression *expr, bool *seenOptional);
    ParserStatus ValidateArrowAssignmentExpr(ir::Expression *expr);
    ParserStatus ValidateArrowParameter(ir::Expression *expr, bool *seenOptional) override;
    void ReportIllegalBreakError(const lexer::SourcePosition &pos) override;
    void ReportIllegalContinueError() override;
};
}  // namespace ark::es2panda::parser

#endif
