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

#ifndef ES2PANDA_PARSER_CORE_TYPED_PARSER_H
#define ES2PANDA_PARSER_CORE_TYPED_PARSER_H

#include "parserImpl.h"

namespace ark::es2panda::ir {
class TSClassImplements;
}  // namespace ark::es2panda::ir

namespace ark::es2panda::parser {
class TypedParser : public ParserImpl {
public:
    TypedParser(Program *program, const util::Options *options, util::DiagnosticEngine &diagnosticEngine,
                ParserStatus status = ParserStatus::NO_OPTS)
        : ParserImpl(program, options, diagnosticEngine, status)
    {
    }

protected:
    void ParseDecorators(ArenaVector<ir::Decorator *> &decorators);
    void CheckDeclare();
    ir::TSModuleDeclaration *ParseAmbientExternalModuleDeclaration(const lexer::SourcePosition &startLoc);
    ir::TSModuleBlock *ParseTsModuleBlock();
    ir::TSModuleDeclaration *ParseModuleOrNamespaceDeclaration(const lexer::SourcePosition &startLoc);
    ArenaVector<ir::AstNode *> ParseTypeLiteralOrInterface();
    ArenaVector<ir::AstNode *> ParseTypeLiteralOrInterfaceBody();
    void CheckObjectTypeForDuplicatedProperties(ir::Expression *key, ArenaVector<ir::AstNode *> &members);

    ir::ArrowFunctionExpression *ParseGenericArrowFunction();
    ir::TSTypeAssertion *ParseTypeAssertion();

    ir::TSTypeParameterInstantiation *ParseTypeParameterInstantiation(TypeAnnotationParsingOptions *options);
    ir::AstNode *ParseTypeParameterInstantiationImpl(TypeAnnotationParsingOptions *options);

    ir::TSTypeParameterDeclaration *ParseTypeParameterDeclaration(TypeAnnotationParsingOptions *options);
    ir::AstNode *ParseTypeParameterDeclarationImpl(TypeAnnotationParsingOptions *options);

    ir::Expression *ParseQualifiedName(ExpressionParseFlags flags = ExpressionParseFlags::NO_OPTS);
    ir::Expression *ParseQualifiedReference(ir::Expression *typeName,
                                            ExpressionParseFlags flags = ExpressionParseFlags::NO_OPTS);
    void ParsePotentialOptionalFunctionParameter(ir::AnnotatedExpression *returnNode);
    // NOLINTNEXTLINE(google-default-arguments)
    ir::Expression *ParseExpression(ExpressionParseFlags flags = ExpressionParseFlags::NO_OPTS) override;
    ir::Statement *ParseInterfaceDeclaration(bool isStatic) override;
    // NOLINTNEXTLINE(google-default-arguments)
    ir::Statement *ParseEnumDeclaration(bool isConst = false, bool isStatic = false) override;
    ir::Statement *ParsePotentialExpressionStatement(StatementParsingFlags flags) override;
    void ConvertThisKeywordToIdentIfNecessary() override;
    ir::TypeNode *ParseFunctionReturnType(ParserStatus status) override;
    ir::TSTypeParameterDeclaration *ParseFunctionTypeParameters() override;
    // NOLINTNEXTLINE(google-default-arguments)
    ir::ClassDefinition *ParseClassDefinition(ir::ClassDefinitionModifiers modifiers,
                                              ir::ModifierFlags flags = ir::ModifierFlags::NONE) override;
    ir::AstNode *ParseClassElement(const ArenaVector<ir::AstNode *> &properties, ir::ClassDefinitionModifiers modifiers,
                                   ir::ModifierFlags flags) override;

    static bool CheckClassElementInterfaceBody(ir::AstNode *property, ArenaVector<ir::AstNode *> &properties);
    bool CheckClassElement(ir::AstNode *property, ir::MethodDefinition *&ctor,
                           ArenaVector<ir::AstNode *> &properties) override;
    bool IsNamespaceDecl();

    ir::ModifierFlags ParseModifiers() override;
    ParserStatus ValidateArrowParameter(ir::Expression *expr, bool *seenOptional) override;
    ir::Expression *ParsePotentialAsExpression(ir::Expression *primaryExpression) override;

    std::tuple<ir::Expression *, ir::TSTypeParameterInstantiation *> ParseSuperClass() override;
    ir::Expression *ParseSuperClassReference() override;
    virtual ArenaVector<ir::TSClassImplements *> ParseClassImplementClause();

    // NOLINTNEXTLINE(google-default-arguments)
    ir::Statement *ParseModuleDeclaration(StatementParsingFlags flags = StatementParsingFlags::NONE) override;
    virtual ir::Statement *ParseNamespace(ir::ModifierFlags flags);
    virtual void CheckIfTypeParameterNameIsReserved() {};
    virtual ArenaVector<ir::TSInterfaceHeritage *> ParseInterfaceExtendsClause();
    virtual ir::Statement *ParseDeclareAndDecorators(StatementParsingFlags flags);
    virtual void ParseOptionalClassElement(ClassElementDescriptor *desc);
    virtual ir::TSTypeParameter *ParseTypeParameter(TypeAnnotationParsingOptions *options);
    virtual ir::TSEnumDeclaration *ParseEnumMembers(ir::Identifier *key, const lexer::SourcePosition &enumStart,
                                                    bool isConst, bool isStatic);
    virtual std::tuple<ir::Expression *, ir::TSTypeParameterInstantiation *> ParseClassImplementsElement();
    virtual ir::TypeNode *ParseInterfaceExtendsElement();

    virtual void ValidateIndexSignatureTypeAnnotation(ir::TypeNode *typeAnnotation);
    virtual ir::Decorator *ParseDecorator()
    {
        return nullptr;
    }
    virtual bool CurrentIsBasicType()
    {
        return false;
    }
    virtual ir::Statement *ParseTypeAliasStatement();
    virtual ir::TSTypeAliasDeclaration *ParseTypeAliasDeclaration()
    {
        return nullptr;
    }
    virtual ir::AstNode *ParseTypeLiteralOrInterfaceMember()
    {
        return nullptr;
    }
    // NOLINTNEXTLINE(google-default-arguments)
    virtual ir::TSIndexSignature *ParseIndexSignature([[maybe_unused]] const lexer::SourcePosition &startLoc,
                                                      [[maybe_unused]] bool isReadonly = false)
    {
        return nullptr;
    }
    virtual ir::AstNode *ParsePropertyOrMethodSignature([[maybe_unused]] const lexer::SourcePosition &startLoc,
                                                        [[maybe_unused]] bool isReadonly)
    {
        return nullptr;
    }
    virtual std::tuple<ir::Expression *, bool> ParseInterfacePropertyKey()
    {
        return {nullptr, false};
    }
    virtual ir::TypeNode *ParseClassKeyAnnotation()
    {
        return nullptr;
    }
    virtual ir::TypeNode *ParseTypeAnnotation([[maybe_unused]] TypeAnnotationParsingOptions *options)
    {
        return nullptr;
    }
    virtual void AddDecorators([[maybe_unused]] ir::AstNode *node,
                               [[maybe_unused]] ArenaVector<ir::Decorator *> &decorators)
    {
    }

    ir::VariableDeclarator *ParseVariableDeclarator(ir::Expression *init, lexer::SourcePosition startLoc,
                                                    VariableParsingFlags flags) override;

    using InterfaceId = std::tuple<ir::Identifier *, varbinder::InterfaceDecl *, bool>;
    InterfaceId ParseInterfaceDeclarationId();

    virtual bool AllowInterfaceRedeclaration()
    {
        return false;
    }

private:
    ParserStatus ValidateArrowParameterAssignment(ir::AssignmentExpression *assignmentExpr);
    ParserStatus ValidateArrowParameterArray(ir::ArrayExpression *arrayPattern);
    ParserStatus ValidateArrowParameterObject(ir::ObjectExpression *objectPattern);

    ir::Expression *ParseLiteralIndent(ir::Expression *typeName, ExpressionParseFlags flags,
                                       const lexer::SourcePosition &startLoc);

    ir::AstNode *ParseProperty(const ArenaVector<ir::AstNode *> &properties, ClassElementDescriptor &desc,
                               ir::Expression *propName);
};
}  // namespace ark::es2panda::parser

#endif
