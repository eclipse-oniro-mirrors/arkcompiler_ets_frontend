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

#ifndef ES2PANDA_PARSER_CORE_TS_PARSER_H
#define ES2PANDA_PARSER_CORE_TS_PARSER_H

#include "ThrowingTypedParser.h"
#include "parserFlags.h"

namespace ark::es2panda::ir {
class Decorator;
enum class TSTupleKind;
}  // namespace ark::es2panda::ir

namespace ark::es2panda::parser {
class TSParser : public ThrowingTypedParser {
public:
    TSParser(Program *program, const util::Options &options, util::DiagnosticEngine &diagnosticEngine,
             ParserStatus status = ParserStatus::NO_OPTS)
        : ThrowingTypedParser(program, &options, diagnosticEngine, status)
    {
    }

private:
    [[nodiscard]] std::unique_ptr<lexer::Lexer> InitLexer(const SourceFile &sourceFile) override;
    bool IsStartOfMappedType() const;
    bool IsStartOfTypePredicate() const;
    bool IsStartOfAbstractConstructorType() const;
    bool CurrentLiteralIsBasicType() const;
    ir::TypeNode *ParseTypeAnnotationElement(ir::TypeNode *typeAnnotation, TypeAnnotationParsingOptions *options);
    class ParseTypeAnnotationElementHelper;
    class ParsePotentialArrowExpressionHelper;
    ir::TypeNode *ParseTypeOperatorOrTypeReference();
    ir::TypeNode *ParseIdentifierReference();
    class ParseBasicTypeHelper;
    ir::TypeNode *ParseBasicType();
    ir::TSTypeReference *ParseConstExpression();
    ir::TSIntersectionType *ParseIntersectionType(ir::Expression *type, bool inUnion, bool restrictExtends);
    ir::TSUnionType *ParseUnionType(ir::TypeNode *type, bool restrictExtends);
    ir::TypeNode *ParseParenthesizedOrFunctionType(ir::TypeNode *typeAnnotation, bool throwError);
    ir::TSArrayType *ParseArrayType(ir::TypeNode *elementType);
    ir::TypeNode *ParseFunctionType(lexer::SourcePosition startLoc, bool isConstructionType, bool throwError,
                                    bool abstractConstructor = false);
    ir::TSTypeParameter *ParseMappedTypeParameter();
    ir::MappedOption ParseMappedOption(lexer::TokenType tokenType);
    ir::TSMappedType *ParseMappedType();
    ir::TSTypePredicate *ParseTypePredicate();
    ir::TypeNode *ParseConditionalType(ir::Expression *checkType, bool restrictExtends);
    ir::TypeNode *ParseThisType(bool throwError);
    ir::TypeNode *ParseIndexAccessType(ir::TypeNode *typeName);
    ir::TypeNode *ParseTypeReferenceOrQuery(bool parseQuery = false);
    ir::TypeNode *ParseTupleElement(ir::TSTupleKind *kind, bool *seenOptional);
    ir::TSTupleType *ParseTupleType();
    ir::TSImportType *ParseImportType(const lexer::SourcePosition &startLoc, bool isTypeof = false);
    ir::TypeNode *ParseTypeLiteralOrMappedType(ir::TypeNode *typeAnnotation);
    ir::TypeNode *ParseTypeReferenceOrTypePredicate(ir::TypeNode *typeAnnotation, bool canBeTsTypePredicate);
    ir::TypeNode *ParseThisTypeOrTypePredicate(ir::TypeNode *typeAnnotation, bool canBeTsTypePredicate,
                                               bool throwError);
    ir::TSSignatureDeclaration *ParseSignatureMember(bool isCallSignature);
    bool IsPotentiallyIndexSignature();
    void CreateTSVariableForProperty(ir::AstNode *node, const ir::Expression *key, varbinder::VariableFlags flags);
    void ValidateFunctionParam(const ArenaVector<ir::Expression *> &params, const ir::Expression *parameter,
                               bool *seenOptional);
    ir::TSParameterProperty *CreateParameterProperty(ir::Expression *parameter, ir::ModifierFlags modifiers);
    void ValidateFunctionOverloadParams(const ArenaVector<ir::Expression *> &params);
    ir::Expression *ParseModuleReference();
    ir::TSImportEqualsDeclaration *ParseTsImportEqualsDeclaration(const lexer::SourcePosition &startLoc,
                                                                  bool isExport = false);
    void ParseOptionalFunctionParameter(ir::AnnotatedExpression *returnNode, bool isRest = false);

    // NOLINTNEXTLINE(google-default-arguments)
    ir::Statement *ParseStatement(StatementParsingFlags flags = StatementParsingFlags::NONE) override;
    ir::AnnotatedExpression *ParsePatternElementGetReturnNode(ExpressionParseFlags &flags, bool &isOptional);
    // NOLINTNEXTLINE(google-default-arguments)
    ir::Expression *ParsePatternElement(ExpressionParseFlags flags = ExpressionParseFlags::NO_OPTS,
                                        bool allowDefault = true) override;
    bool CurrentIsBasicType() override;
    ir::TypeNode *ParseTypeAnnotation(TypeAnnotationParsingOptions *options) override;
    // NOLINTNEXTLINE(google-default-arguments)
    ir::ObjectExpression *ParseObjectExpression(ExpressionParseFlags flags = ExpressionParseFlags::NO_OPTS) override;
    // NOLINTNEXTLINE(google-default-arguments)
    ir::ArrayExpression *ParseArrayExpression(ExpressionParseFlags flags = ExpressionParseFlags::NO_OPTS) override;
    ir::ArrowFunctionExpression *ParsePotentialArrowExpression(ir::Expression **returnExpression,
                                                               const lexer::SourcePosition &startLoc) override;
    bool ParsePotentialGenericFunctionCall(ir::Expression *primaryExpr, ir::Expression **returnExpression,
                                           const lexer::SourcePosition &startLoc, bool ignoreCallExpression) override;
    bool ParsePotentialNonNullExpression(ir::Expression **returnExpression, lexer::SourcePosition startLoc) override;
    bool IsNamedFunctionExpression() override;
    ir::Identifier *ParsePrimaryExpressionIdent(ExpressionParseFlags flags) override;
    bool ValidateArrowFunctionRestParameter(ir::SpreadElement *restElement) override;
    ir::Decorator *ParseDecorator() override;
    void AddDecorators(ir::AstNode *node, ArenaVector<ir::Decorator *> &decorators) override;
    ir::TSTypeAliasDeclaration *ParseTypeAliasDeclaration() override;
    ir::AstNode *ParseTypeLiteralOrInterfaceMember() override;
    // NOLINTNEXTLINE(google-default-arguments)
    ir::TSIndexSignature *ParseIndexSignature(const lexer::SourcePosition &startLoc, bool isReadonly = false) override;
    ir::AstNode *ParsePropertyOrMethodSignature(const lexer::SourcePosition &startLoc, bool isReadonly) override;
    std::tuple<ir::Expression *, bool> ParseInterfacePropertyKey() override;
    ArenaVector<ir::Expression *> ParseFunctionParams() override;
    ir::Expression *ParseFunctionParameter() override;
    ir::TypeNode *ParseClassKeyAnnotation() override;
    void ValidateClassMethodStart(ClassElementDescriptor *desc, ir::TypeNode *typeAnnotation) override;
    ir::MethodDefinition *ParseClassMethod(ClassElementDescriptor *desc, const ArenaVector<ir::AstNode *> &properties,
                                           ir::Expression *propName, lexer::SourcePosition *propEnd) override;
    void ValidateClassSetter(ClassElementDescriptor *desc, const ArenaVector<ir::AstNode *> &properties,
                             ir::Expression *propName, ir::ScriptFunction *func) override;
    void ValidateClassGetter(ClassElementDescriptor *desc, const ArenaVector<ir::AstNode *> &properties,
                             ir::Expression *propName, ir::ScriptFunction *func) override;
    bool IsModifierKind(const lexer::Token &token) override;
    void CheckIfTypeParameterNameIsReserved() override;
    void CheckIfStaticConstructor(ir::ModifierFlags flags) override;
    std::tuple<bool, bool, bool> ParseComputedClassFieldOrIndexSignature(ir::Expression **propName) override;
    ir::TypeNode *ParseFunctionReturnType(ParserStatus status) override;
    std::tuple<bool, ir::BlockStatement *, lexer::SourcePosition, bool> ParseFunctionBody(
        const ArenaVector<ir::Expression *> &params, ParserStatus newStatus, ParserStatus contextStatus) override;
    ir::AstNode *ParseImportDefaultSpecifier(ArenaVector<ir::AstNode *> *specifiers) override;
    ir::Statement *ParseExportDeclaration(StatementParsingFlags flags) override;
    // NOLINTNEXTLINE(google-default-arguments)
    ir::Expression *ParseCoverParenthesizedExpressionAndArrowParameterList(
        ExpressionParseFlags flags = ExpressionParseFlags::NO_OPTS) override;
    ir::Expression *ParseArrowFunctionRestParameter(lexer::SourcePosition start);
    ir::Expression *ParseArrowFunctionNoParameter(lexer::SourcePosition start);
    ir::Statement *ParseConstStatement(StatementParsingFlags flags) override;
    ir::Statement *ParsePotentialConstEnum(VariableParsingFlags flags) override;
    void ParseCatchParamTypeAnnotation(ir::AnnotatedExpression *param) override;
    ir::Expression *ParseVariableDeclaratorKey(VariableParsingFlags flags) override;
    void ReportPossibleOutOfBoundaryJumpError(bool allowBreak) override;
    void ReportIllegalBreakError(const lexer::SourcePosition &pos) override;
    void ReportIllegalContinueError() override;
    void ReportIfBodyEmptyError(ir::Statement *consequent) override;
    void ReportMultipleDefaultError() override;
    void ReportIllegalNewLineErrorAfterThrow() override;
    // NOLINTNEXTLINE(google-default-arguments)
    ir::ExportDefaultDeclaration *ParseExportDefaultDeclaration(const lexer::SourcePosition &startLoc,
                                                                bool isExportEquals = false) override;
    ir::Statement *GetDeclarationForNamedExport(ir::ClassDefinitionModifiers &classModifiers, ir::ModifierFlags &flags);
    ir::Statement *ParseNamedExportDeclaration(const lexer::SourcePosition &startLoc) override;
    ir::Statement *ParseImportDeclaration(StatementParsingFlags flags) override;
    void ValidateIndexSignatureTypeAnnotation(ir::TypeNode *typeAnnotation) override;
    ir::Expression *ParsePotentialAsExpression(ir::Expression *expr) override;

    bool AllowInterfaceRedeclaration() override
    {
        return true;
    }
};
}  // namespace ark::es2panda::parser

#endif
