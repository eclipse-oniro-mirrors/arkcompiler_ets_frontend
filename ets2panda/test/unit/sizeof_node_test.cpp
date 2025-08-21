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
#include <gtest/gtest.h>

#include "ir/base/classDefinition.h"
#include "ir/base/classProperty.h"
#include "ir/base/methodDefinition.h"

#include "ir/ets/etsModule.h"
#include "ir/ets/etsReExportDeclaration.h"
#include "ir/ets/etsStructDeclaration.h"
#include "ir/ets/etsTypeReference.h"
#include "ir/ets/etsTypeReferencePart.h"

#include "ir/expressions/functionExpression.h"

#include "ir/module/importDeclaration.h"

#include "ir/statements/annotationDeclaration.h"
#include "ir/statements/classDeclaration.h"
#include "ir/statements/functionDeclaration.h"
#include "ir/statements/variableDeclaration.h"

#include "ir/ts/tsEnumDeclaration.h"
#include "ir/ts/tsInterfaceDeclaration.h"
#include "ir/ts/tsTypeAliasDeclaration.h"
#include "ir/ts/tsTypeParameter.h"
#include "ir/ts/tsTypeParameterDeclaration.h"

namespace ark::es2panda::ir {

class SizeOfNodeTest : public testing::Test {
public:
    template <typename T>
    size_t SizeOf();

    static size_t Align(size_t value)
    {
        constexpr size_t FIELD_ALIGNMENT = 8;
        return AlignUp(value, FIELD_ALIGNMENT);
    }
};

// NOLINTBEGIN(bugprone-sizeof-container)

template <>
size_t SizeOfNodeTest::SizeOf<AstNode>()
{
    constexpr size_t SIZE_OF_VTABLE = sizeof(void *);
    AstNode *node = nullptr;

    // clang-format off
    return Align(SIZE_OF_VTABLE +
        sizeof(node->parent_) +
        sizeof(node->range_) +
        sizeof(node->type_) +
        sizeof(node->flags_) +
        sizeof(node->astNodeFlags_) +
        sizeof(node->history_) +
        sizeof(node->variable_) +
        sizeof(node->originalNode_));
    // clang-format on
}

template <>
size_t SizeOfNodeTest::SizeOf<Typed<AstNode>>()
{
    Typed<AstNode> *node = nullptr;

    // clang-format off
    return SizeOf<AstNode>() +
        sizeof(node->tsType_) +
        sizeof(node->preferredType_);
    // clang-format on
}

template <>
size_t SizeOfNodeTest::SizeOf<TypedAstNode>()
{
    return SizeOf<Typed<AstNode>>();
}

template <>
size_t SizeOfNodeTest::SizeOf<AnnotationAllowed<TypedAstNode>>()
{
    AnnotationAllowed<TypedAstNode> *node = nullptr;

    // clang-format off
    return SizeOf<TypedAstNode>() +
        sizeof(node->annotations_);
    // clang-format on
}

template <>
size_t SizeOfNodeTest::SizeOf<ClassDefinition>()
{
    ClassDefinition *node = nullptr;

    // clang-format off
    return SizeOf<AnnotationAllowed<TypedAstNode>>() +
        sizeof(node->scope_) +
        sizeof(node->internalName_) +
        sizeof(node->ident_) +
        sizeof(node->typeParams_) +
        sizeof(node->superTypeParams_) +
        sizeof(node->implements_) +
        sizeof(node->ctor_) +
        sizeof(node->superClass_) +
        sizeof(node->body_) +
        sizeof(node->modifiers_) +
        sizeof(node->lang_) +
        sizeof(node->capturedVars_) +
        sizeof(node->localVariableIsNeeded_) +
        sizeof(node->origEnumDecl_) +
        sizeof(node->anonClass_) +
        Align(sizeof(node->localIndex_)) +
        sizeof(node->localPrefix_) +
        sizeof(node->functionalReferenceReferencedMethod_) +
        sizeof(node->exportedClasses_);
    // clang-format on
}

template <>
size_t SizeOfNodeTest::SizeOf<Statement>()
{
    return SizeOf<AstNode>();
}

template <>
size_t SizeOfNodeTest::SizeOf<ClassDeclaration>()
{
    ClassDeclaration *node = nullptr;

    // clang-format off
    return SizeOf<Statement>() +
        sizeof(node->def_) +
        sizeof(node->decorators_);
    // clang-format on
}

template <>
size_t SizeOfNodeTest::SizeOf<Typed<Statement>>()
{
    Typed<Statement> *node = nullptr;

    // clang-format off
    return SizeOf<Statement>() +
        sizeof(node->tsType_) +
        sizeof(node->preferredType_);
    // clang-format on
}

template <>
size_t SizeOfNodeTest::SizeOf<TypedStatement>()
{
    return SizeOf<Typed<Statement>>();
}

template <>
size_t SizeOfNodeTest::SizeOf<ClassElement>()
{
    ClassElement *node = nullptr;

    // clang-format off
    return SizeOf<TypedStatement>() +
        sizeof(node->key_) +
        sizeof(node->value_) +
        sizeof(node->decorators_) +
        Align(sizeof(node->isComputed_)) +
        sizeof(node->enumMember_);
    // clang-format on
}

template <>
size_t SizeOfNodeTest::SizeOf<AnnotationAllowed<ClassElement>>()
{
    AnnotationAllowed<ClassElement> *node = nullptr;

    // clang-format off
    return SizeOf<ClassElement>() +
        sizeof(node->annotations_);
    // clang-format on
}

template <>
size_t SizeOfNodeTest::SizeOf<ClassProperty>()
{
    ClassProperty *node = nullptr;

    // clang-format off
    return SizeOf<AnnotationAllowed<ClassElement>>() +
        sizeof(node->typeAnnotation_) +
        Align(sizeof(node->isDefault_) +
            sizeof(node->initMode_));
    // clang-format on
}

template <>
size_t SizeOfNodeTest::SizeOf<MethodDefinition>()
{
    MethodDefinition *node = nullptr;

    // clang-format off
    return SizeOf<ClassElement>() +
        Align(sizeof(node->isDefault_) +
            sizeof(node->kind_)) +
        sizeof(node->overloads_) +
        sizeof(node->baseOverloadMethod_) +
        sizeof(node->asyncPairMethod_) +
        sizeof(node->overloadInfo_);
    // clang-format on
}

template <>
size_t SizeOfNodeTest::SizeOf<AnnotationAllowed<AstNode>>()
{
    AnnotationAllowed<AstNode> *node = nullptr;

    // clang-format off
    return SizeOf<AstNode>() +
        sizeof(node->annotations_);
    // clang-format on
}

template <>
size_t SizeOfNodeTest::SizeOf<ScriptFunction>()
{
    ScriptFunction *node = nullptr;

    // clang-format off
    return SizeOf<AnnotationAllowed<AstNode>>() +
        sizeof(node->id_) +
        Align(sizeof(node->irSignature_)) +
        sizeof(node->body_) +
        sizeof(node->scope_) +
        Align(sizeof(node->funcFlags_)) +
        sizeof(node->signature_) +
        sizeof(node->preferredReturnType_) +
        sizeof(node->asyncPairFunction_) +
        Align(sizeof(node->lang_)) +
        sizeof(node->returnStatements_);
    // clang-format on
}

template <>
size_t SizeOfNodeTest::SizeOf<ImportDeclaration>()
{
    ImportDeclaration *node = nullptr;

    // clang-format off
    return SizeOf<Statement>() +
        sizeof(node->source_) +
        sizeof(node->specifiers_) +
        Align(sizeof(node->importKinds_));
    // clang-format on
}

template <>
size_t SizeOfNodeTest::SizeOf<AnnotationAllowed<Statement>>()
{
    AnnotationAllowed<Statement> *node = nullptr;

    // clang-format off
    return SizeOf<Statement>() +
        sizeof(node->annotations_);
    // clang-format on
}

template <>
size_t SizeOfNodeTest::SizeOf<FunctionDeclaration>()
{
    FunctionDeclaration *node = nullptr;

    // clang-format off
    return SizeOf<AnnotationAllowed<Statement>>() +
        sizeof(node->decorators_) +
        sizeof(node->func_) +
        Align(sizeof(node->isAnonymous_));
    // clang-format on
}

template <>
size_t SizeOfNodeTest::SizeOf<TSEnumDeclaration>()
{
    TSEnumDeclaration *node = nullptr;

    // clang-format off
    return SizeOf<TypedStatement>() +
        sizeof(node->scope_) +
        sizeof(node->decorators_) +
        sizeof(node->key_) +
        sizeof(node->members_) +
        sizeof(node->internalName_) +
        sizeof(node->boxedClass_) +
        sizeof(node->typeNode_) +
        Align(sizeof(node->isConst_));
    // clang-format on
}

template <>
size_t SizeOfNodeTest::SizeOf<AnnotationAllowed<TypedStatement>>()
{
    AnnotationAllowed<TypedStatement> *node = nullptr;

    // clang-format off
    return SizeOf<TypedStatement>() +
        sizeof(node->annotations_);
    // clang-format on
}

template <>
size_t SizeOfNodeTest::SizeOf<TSInterfaceDeclaration>()
{
    TSInterfaceDeclaration *node = nullptr;

    // clang-format off
    return SizeOf<AnnotationAllowed<TypedStatement>>() +
        sizeof(node->decorators_) +
        sizeof(node->scope_) +
        sizeof(node->id_) +
        sizeof(node->typeParams_) +
        sizeof(node->body_) +
        sizeof(node->extends_) +
        sizeof(node->internalName_) +
        Align(sizeof(node->isStatic_) +
            sizeof(node->isExternal_) +
            sizeof(node->lang_)) +
        sizeof(node->anonClass_);
    // clang-format on
}

template <>
size_t SizeOfNodeTest::SizeOf<Annotated<Statement>>()
{
    Annotated<Statement> *node = nullptr;

    // clang-format off
    return SizeOf<Statement>() +
        sizeof(node->typeAnnotation_);
    // clang-format on
}

template <>
size_t SizeOfNodeTest::SizeOf<AnnotatedStatement>()
{
    return SizeOf<Annotated<Statement>>();
}

template <>
size_t SizeOfNodeTest::SizeOf<TSTypeAliasDeclaration>()
{
    TSTypeAliasDeclaration *node = nullptr;

    // clang-format off
    return SizeOf<AnnotatedStatement>() +
        sizeof(node->decorators_) +
        sizeof(node->annotations_) +
        sizeof(node->id_) +
        sizeof(node->typeParams_) +
        sizeof(node->typeParamTypes_);
    // clang-format on
}

template <>
size_t SizeOfNodeTest::SizeOf<Expression>()
{
    [[maybe_unused]] Expression *node = nullptr;

    // clang-format off
    return SizeOf<TypedAstNode>();
    // clang-format on
}

template <>
size_t SizeOfNodeTest::SizeOf<AnnotationAllowed<Expression>>()
{
    AnnotationAllowed<Expression> *node = nullptr;

    // clang-format off
    return SizeOf<Expression>() +
        sizeof(node->annotations_);
    // clang-format on
}

template <>
size_t SizeOfNodeTest::SizeOf<VariableDeclaration>()
{
    VariableDeclaration *node = nullptr;

    // clang-format off
    return SizeOf<AnnotationAllowed<Statement>>() +
        Align(sizeof(node->kind_) +
        sizeof(node->decorators_) +
        sizeof(node->declarators_));
    // clang-format on
}

template <>
size_t SizeOfNodeTest::SizeOf<ETSParameterExpression>()
{
    ETSParameterExpression *node = nullptr;

    // clang-format off
    return SizeOf<AnnotationAllowed<Expression>>() +
        sizeof(node->ident_) +
        sizeof(node->initializer_) +
        sizeof(node->spread_) +
        sizeof(node->savedLexer_) +
        sizeof(node->extraValue_) +
        Align(sizeof(node->isOptional_));
    // clang-format on
}

template <>
size_t SizeOfNodeTest::SizeOf<TSTypeParameter>()
{
    TSTypeParameter *node = nullptr;

    // clang-format off
    return SizeOf<AnnotationAllowed<Expression>>() +
        sizeof(node->name_) +
        sizeof(node->constraint_) +
        sizeof(node->defaultType_);
    // clang-format on
}

template <>
size_t SizeOfNodeTest::SizeOf<TSTypeParameterDeclaration>()
{
    TSTypeParameterDeclaration *node = nullptr;

    // clang-format off
    return SizeOf<Expression>() +
        sizeof(node->params_) +
        sizeof(node->scope_) +
        sizeof(node->requiredParams_);
    // clang-format on
}

template <>
size_t SizeOfNodeTest::SizeOf<TypeNode>()
{
    return SizeOf<AnnotationAllowed<Expression>>();
}

template <>
size_t SizeOfNodeTest::SizeOf<ETSTypeReference>()
{
    ETSTypeReference *node = nullptr;

    // clang-format off
    return SizeOf<TypeNode>() +
        sizeof(node->part_);
    // clang-format on
}

template <>
size_t SizeOfNodeTest::SizeOf<ETSTypeReferencePart>()
{
    ETSTypeReferencePart *node = nullptr;

    // clang-format off
    return SizeOf<TypeNode>() +
        sizeof(node->name_) +
        sizeof(node->typeParams_) +
        sizeof(node->prev_);
    // clang-format on
}

template <>
size_t SizeOfNodeTest::SizeOf<BlockStatement>()
{
    BlockStatement *node = nullptr;

    // clang-format off
    return SizeOf<Statement>() +
        sizeof(node->scope_) +
        sizeof(node->statements_) +
        sizeof(node->trailingBlocks_);
    // clang-format on
}

template <>
size_t SizeOfNodeTest::SizeOf<AnnotationAllowed<BlockStatement>>()
{
    AnnotationAllowed<BlockStatement> *node = nullptr;

    // clang-format off
    return SizeOf<BlockStatement>() +
        sizeof(node->annotations_);
    // clang-format on
}

template <>
size_t SizeOfNodeTest::SizeOf<ETSModule>()
{
    ETSModule *node = nullptr;

    // clang-format off
    return SizeOf<AnnotationAllowed<BlockStatement>>() +
        sizeof(node->ident_) +
        Align(sizeof(node->flag_)) +
        sizeof(node->program_)+
        sizeof(node->globalClass_);
    // clang-format on
}

template <>
size_t SizeOfNodeTest::SizeOf<FunctionExpression>()
{
    FunctionExpression *node = nullptr;

    // clang-format off
    return SizeOf<Expression>() +
        sizeof(node->func_) +
        sizeof(node->exprName_);
    // clang-format on
}

template <>
size_t SizeOfNodeTest::SizeOf<ETSReExportDeclaration>()
{
    ETSReExportDeclaration *node = nullptr;

    // clang-format off
    return SizeOf<Statement>() +
        sizeof(node->etsImportDeclarations_) +
        sizeof(node->userPaths_) +
        sizeof(node->programPath_);
    // clang-format on
}

template <>
size_t SizeOfNodeTest::SizeOf<ETSStructDeclaration>()
{
    return SizeOf<ClassDeclaration>();
}

template <>
size_t SizeOfNodeTest::SizeOf<AnnotationDeclaration>()
{
    AnnotationDeclaration *node = nullptr;

    // clang-format off
    return SizeOf<AnnotationAllowed<Statement>>() +
        sizeof(node->internalName_) +
        sizeof(node->scope_) +
        sizeof(node->expr_) +
        sizeof(node->properties_) +
        Align(sizeof(node->policy_));
    // clang-format on
}

// NOLINTEND(bugprone-sizeof-container)

TEST_F(SizeOfNodeTest, DeclNodesBase)
{
    // IMPORTANT NOTICE: This test purpose is to warn a developer who modified fields of specific node classes.
    // Classes listed below implements CopyTo/Clone methods.
    // When you add a new field to a class, make sure you updated corresponding method.
    // Example: if you added new field to a ClassDefinition class, handle it in ClassDefinition::CopyTo/Clone.

    ASSERT_EQ(sizeof(AstNode), SizeOf<AstNode>());
    ASSERT_EQ(sizeof(Typed<AstNode>), SizeOf<Typed<AstNode>>());
    ASSERT_EQ(sizeof(TypedAstNode), SizeOf<TypedAstNode>());
    ASSERT_EQ(sizeof(AnnotationAllowed<TypedAstNode>), SizeOf<AnnotationAllowed<TypedAstNode>>());
    ASSERT_EQ(sizeof(Statement), SizeOf<Statement>());
    ASSERT_EQ(sizeof(Typed<Statement>), SizeOf<Typed<Statement>>());
    ASSERT_EQ(sizeof(TypedStatement), SizeOf<TypedStatement>());
    ASSERT_EQ(sizeof(ClassElement), SizeOf<ClassElement>());
    ASSERT_EQ(sizeof(AnnotationAllowed<ClassElement>), SizeOf<AnnotationAllowed<ClassElement>>());
    ASSERT_EQ(sizeof(AnnotationAllowed<AstNode>), SizeOf<AnnotationAllowed<AstNode>>());
    ASSERT_EQ(sizeof(AnnotationAllowed<AstNode>), SizeOf<AnnotationAllowed<AstNode>>());
    ASSERT_EQ(sizeof(AnnotationAllowed<Statement>), SizeOf<AnnotationAllowed<Statement>>());
    ASSERT_EQ(sizeof(AnnotationAllowed<Statement>), SizeOf<AnnotationAllowed<Statement>>());
    ASSERT_EQ(sizeof(AnnotationAllowed<TypedStatement>), SizeOf<AnnotationAllowed<TypedStatement>>());

    // IMPORTANT NOTICE: This test purpose is to warn a developer who modified fields of specific node classes.
    // Classes listed below implements CopyTo/Clone methods.
    // When you add a new field to a class, make sure you updated corresponding method.
    // Example: if you added new field to a ClassDefinition class, handle it in ClassDefinition::CopyTo/Clone.

    ASSERT_EQ(sizeof(Annotated<Statement>), SizeOf<Annotated<Statement>>());
    ASSERT_EQ(sizeof(AnnotatedStatement), SizeOf<AnnotatedStatement>());
    ASSERT_EQ(sizeof(AnnotatedStatement), SizeOf<AnnotatedStatement>());
    ASSERT_EQ(sizeof(Expression), SizeOf<Expression>());
    ASSERT_EQ(sizeof(AnnotationAllowed<Expression>), SizeOf<AnnotationAllowed<Expression>>());
    ASSERT_EQ(sizeof(TypeNode), SizeOf<TypeNode>());
    ASSERT_EQ(sizeof(BlockStatement), SizeOf<BlockStatement>());
    ASSERT_EQ(sizeof(AnnotationAllowed<BlockStatement>), SizeOf<AnnotationAllowed<BlockStatement>>());
}

TEST_F(SizeOfNodeTest, DeclNodesDerived)
{
    // IMPORTANT NOTICE: This test purpose is to warn a developer who modified fields of specific node classes.
    // Classes listed below implements CopyTo/Clone methods.
    // When you add a new field to a class, make sure you updated corresponding method.
    // Example: if you added new field to a ClassDefinition class, handle it in ClassDefinition::CopyTo/Clone.

    ASSERT_EQ(sizeof(ClassDefinition), SizeOf<ClassDefinition>());
    ASSERT_EQ(sizeof(ClassDeclaration), SizeOf<ClassDeclaration>());
    ASSERT_EQ(sizeof(ClassProperty), SizeOf<ClassProperty>());
    ASSERT_EQ(sizeof(MethodDefinition), SizeOf<MethodDefinition>());
    ASSERT_EQ(sizeof(ScriptFunction), SizeOf<ScriptFunction>());
    ASSERT_EQ(sizeof(ImportDeclaration), SizeOf<ImportDeclaration>());
    ASSERT_EQ(sizeof(FunctionDeclaration), SizeOf<FunctionDeclaration>());
    ASSERT_EQ(sizeof(TSEnumDeclaration), SizeOf<TSEnumDeclaration>());
    ASSERT_EQ(sizeof(TSInterfaceDeclaration), SizeOf<TSInterfaceDeclaration>());
    ASSERT_EQ(sizeof(TSTypeAliasDeclaration), SizeOf<TSTypeAliasDeclaration>());
    ASSERT_EQ(sizeof(VariableDeclaration), SizeOf<VariableDeclaration>());

    // IMPORTANT NOTICE: This test purpose is to warn a developer who modified fields of specific node classes.
    // Classes listed below implements CopyTo/Clone methods.
    // When you add a new field to a class, make sure you updated corresponding method.
    // Example: if you added new field to a ClassDefinition class, handle it in ClassDefinition::CopyTo/Clone.

    ASSERT_EQ(sizeof(ETSParameterExpression), SizeOf<ETSParameterExpression>());
    ASSERT_EQ(sizeof(TSTypeParameter), SizeOf<TSTypeParameter>());
    ASSERT_EQ(sizeof(TSTypeParameterDeclaration), SizeOf<TSTypeParameterDeclaration>());
    ASSERT_EQ(sizeof(ETSTypeReference), SizeOf<ETSTypeReference>());
    ASSERT_EQ(sizeof(ETSTypeReferencePart), SizeOf<ETSTypeReferencePart>());
    ASSERT_EQ(sizeof(ETSModule), SizeOf<ETSModule>());
    ASSERT_EQ(sizeof(FunctionExpression), SizeOf<FunctionExpression>());
    ASSERT_EQ(sizeof(ETSReExportDeclaration), SizeOf<ETSReExportDeclaration>());
    ASSERT_EQ(sizeof(ETSStructDeclaration), SizeOf<ETSStructDeclaration>());
    ASSERT_EQ(sizeof(AnnotationDeclaration), SizeOf<AnnotationDeclaration>());
}
}  // namespace ark::es2panda::ir
