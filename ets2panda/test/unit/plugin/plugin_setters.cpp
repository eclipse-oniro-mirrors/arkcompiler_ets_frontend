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

#include <iostream>
#include <ostream>
#include <string>
#include "public/es2panda_lib.h"
#include "util.h"

// NOLINTBEGIN
static std::string source = R"(
)";

static es2panda_Impl *impl = nullptr;
static es2panda_Config *config = nullptr;
static es2panda_Context *context = nullptr;

constexpr int TEST_ARRAY_LEN = 2;

int TestClassDefinitionSetImplements()
{
    auto *def = impl->CreateClassDefinition1(context, nullptr, nullptr, 0,
                                             Es2pandaClassDefinitionModifiers::CLASS_DEFINITION_MODIFIERS_NONE,
                                             Es2pandaModifierFlags::MODIFIER_FLAGS_NONE);
    auto *implements0 = impl->CreateTSClassImplements1(context, nullptr);
    auto *implements1 = impl->CreateTSClassImplements1(context, nullptr);
    es2panda_AstNode *implements[] = {implements0, implements1};

    impl->ClassDefinitionSetImplements(context, def, implements, TEST_ARRAY_LEN);

    size_t returnedImplementsLen;
    es2panda_AstNode **returnedImplements = impl->ClassDefinitionImplementsConst(context, def, &returnedImplementsLen);
    if (returnedImplementsLen != TEST_ARRAY_LEN) {
        return TEST_ERROR_CODE;
    }
    if (returnedImplements[0] != implements0) {
        return TEST_ERROR_CODE;
    }
    if (returnedImplements[1] != implements1) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, implements0) != def) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, implements1) != def) {
        return TEST_ERROR_CODE;
    }
    return 0;
}

int TestClassStaticBlockSetFunction()
{
    auto *block = impl->CreateClassStaticBlock(context, impl->CreateFunctionExpression(context, nullptr));
    auto *func = impl->CreateScriptFunction(
        context, nullptr, impl->CreateFunctionSignature(context, nullptr, nullptr, 0, nullptr, false), 0, 0);

    impl->ClassStaticBlockSetFunction(context, block, func);

    if (impl->ClassStaticBlockFunctionConst(context, block) != func) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, func) != impl->ClassElementValueConst(context, block)) {
        return TEST_ERROR_CODE;
    }
    return 0;
}

int TestMethodDefinitionSetOverloads()
{
    auto *def = impl->CreateMethodDefinition(context, Es2pandaMethodDefinitionKind::METHOD_DEFINITION_KIND_NONE,
                                             nullptr, nullptr, Es2pandaModifierFlags::MODIFIER_FLAGS_NONE, false);
    auto *overload0 = impl->CreateMethodDefinition(context, Es2pandaMethodDefinitionKind::METHOD_DEFINITION_KIND_NONE,
                                                   nullptr, nullptr, Es2pandaModifierFlags::MODIFIER_FLAGS_NONE, false);
    auto *overload1 = impl->CreateMethodDefinition(context, Es2pandaMethodDefinitionKind::METHOD_DEFINITION_KIND_NONE,
                                                   nullptr, nullptr, Es2pandaModifierFlags::MODIFIER_FLAGS_NONE, false);
    es2panda_AstNode *overloads[] = {overload0, overload1};

    impl->MethodDefinitionSetOverloads(context, def, overloads, TEST_ARRAY_LEN);

    size_t returnedOverloadsLen;
    es2panda_AstNode **returnedOverloads = impl->MethodDefinitionOverloadsConst(context, def, &returnedOverloadsLen);
    if (returnedOverloadsLen != TEST_ARRAY_LEN) {
        return TEST_ERROR_CODE;
    }
    if (returnedOverloads[0] != overload0) {
        return TEST_ERROR_CODE;
    }
    if (returnedOverloads[1] != overload1) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, overload0) != def) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, overload1) != def) {
        return TEST_ERROR_CODE;
    }
    return 0;
}

int TestPropertySetKey()
{
    auto *prop = impl->CreateProperty(context, nullptr, nullptr);
    auto *key = impl->CreateIdentifier(context);

    impl->PropertySetKey(context, prop, key);

    if (impl->PropertyKeyConst(context, prop) != key) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, key) != prop) {
        return TEST_ERROR_CODE;
    }
    return 0;
}

int TestScriptFunctionSetTypeParams()
{
    auto *func = impl->CreateScriptFunction(
        context, nullptr, impl->CreateFunctionSignature(context, nullptr, nullptr, 0, nullptr, false), 0, 0);
    auto *typeParams = impl->CreateTSTypeParameterDeclaration(context, nullptr, 0, 0);

    impl->ScriptFunctionSetTypeParams(context, func, typeParams);

    if (impl->ScriptFunctionTypeParamsConst(context, func) != typeParams) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, typeParams) != func) {
        return TEST_ERROR_CODE;
    }
    return 0;
}

int TestScriptFunctionSetIdent()
{
    auto *func = impl->CreateScriptFunction(
        context, nullptr, impl->CreateFunctionSignature(context, nullptr, nullptr, 0, nullptr, false), 0, 0);
    auto *ident = impl->CreateIdentifier(context);

    impl->ScriptFunctionSetIdent(context, func, ident);

    if (impl->ScriptFunctionIdConst(context, func) != ident) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, ident) != func) {
        return TEST_ERROR_CODE;
    }
    return 0;
}

int TestETSFunctionTypeSetTypeParams()
{
    auto *funcT = impl->CreateETSFunctionTypeIr(
        context, impl->CreateFunctionSignature(context, nullptr, nullptr, 0, nullptr, false),
        Es2pandaScriptFunctionFlags::SCRIPT_FUNCTION_FLAGS_NONE);
    auto *typeParams = impl->CreateTSTypeParameterDeclaration(context, nullptr, 0, 0);

    impl->ETSFunctionTypeIrSetTypeParams(context, funcT, typeParams);

    if (impl->ETSFunctionTypeIrTypeParamsConst(context, funcT) != typeParams) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, typeParams) != funcT) {
        return TEST_ERROR_CODE;
    }
    return 0;
}

int TestETSFunctionTypeSetReturnType()
{
    auto *funcT = impl->CreateETSFunctionTypeIr(
        context, impl->CreateFunctionSignature(context, nullptr, nullptr, 0, nullptr, false),
        Es2pandaScriptFunctionFlags::SCRIPT_FUNCTION_FLAGS_NONE);
    auto *returnType = impl->CreateOpaqueTypeNode1(context);

    impl->ETSFunctionTypeIrSetReturnType(context, funcT, returnType);

    if (impl->ETSFunctionTypeIrReturnTypeConst(context, funcT) != returnType) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, returnType) != funcT) {
        return TEST_ERROR_CODE;
    }
    return 0;
}

int TestETSModuleSetIdent()
{
    auto *etsModule =
        impl->CreateETSModule(context, nullptr, 0, nullptr, Es2pandaModuleFlag::MODULE_FLAG_NAMESPACE, nullptr);
    auto *ident = impl->CreateIdentifier(context);

    impl->ETSModuleSetIdent(context, etsModule, ident);

    if (impl->ETSModuleIdentConst(context, etsModule) != ident) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, ident) != etsModule) {
        return TEST_ERROR_CODE;
    }
    return 0;
}

int TestETSNewClassInstanceExpressionSetTypeRef()
{
    auto *expr = impl->CreateETSNewClassInstanceExpression(context, nullptr, nullptr, 0);
    auto *typeRef = impl->CreateETSTypeReference(context, nullptr);

    impl->ETSNewClassInstanceExpressionSetTypeRef(context, expr, typeRef);

    if (impl->ETSNewClassInstanceExpressionGetTypeRefConst(context, expr) != typeRef) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, typeRef) != expr) {
        return TEST_ERROR_CODE;
    }
    return 0;
}

int TestETSNewClassInstanceExpressionSetArguments()
{
    auto *expr = impl->CreateETSNewClassInstanceExpression(context, nullptr, nullptr, 0);
    auto *expr0 = impl->CreateThisExpression(context);
    auto *expr1 = impl->CreateThisExpression(context);
    es2panda_AstNode *args[] = {expr0, expr1};

    impl->ETSNewClassInstanceExpressionSetArguments(context, expr, args, TEST_ARRAY_LEN);

    size_t returnedArgumentsLen;
    es2panda_AstNode **returnedArguments =
        impl->ETSNewClassInstanceExpressionGetArgumentsConst(context, expr, &returnedArgumentsLen);
    if (returnedArgumentsLen != TEST_ARRAY_LEN) {
        return TEST_ERROR_CODE;
    }
    if (returnedArguments[0] != expr0) {
        return TEST_ERROR_CODE;
    }
    if (returnedArguments[1] != expr1) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, expr0) != expr) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, expr1) != expr) {
        return TEST_ERROR_CODE;
    }
    return 0;
}

int TestETSTypeReferenceSetPart()
{
    auto *typeRef = impl->CreateETSTypeReference(context, nullptr);
    auto *part = impl->CreateETSTypeReferencePart1(context, nullptr);

    impl->ETSTypeReferenceSetPart(context, typeRef, part);

    if (impl->ETSTypeReferencePartConst(context, typeRef) != part) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, part) != typeRef) {
        return TEST_ERROR_CODE;
    }
    return 0;
}

int TestETSTypeReferencePartSetName()
{
    auto *part = impl->CreateETSTypeReferencePart1(context, nullptr);
    auto *name = impl->CreateIdentifier(context);

    impl->ETSTypeReferencePartSetName(context, part, name);

    if (impl->ETSTypeReferencePartNameConst(context, part) != name) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, name) != part) {
        return TEST_ERROR_CODE;
    }
    return 0;
}

int TestETSTypeReferencePartSetTypeParams()
{
    auto *part = impl->CreateETSTypeReferencePart1(context, nullptr);
    auto *typeParams = impl->CreateTSTypeParameterInstantiation(context, nullptr, 0);

    impl->ETSTypeReferencePartSetTypeParams(context, part, typeParams);

    if (impl->ETSTypeReferencePartTypeParamsConst(context, part) != typeParams) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, typeParams) != part) {
        return TEST_ERROR_CODE;
    }
    return 0;
}

int TestETSTypeReferencePartSetPrevious()
{
    auto *part = impl->CreateETSTypeReferencePart1(context, nullptr);
    auto *prev = impl->CreateETSTypeReferencePart1(context, nullptr);

    impl->ETSTypeReferencePartSetPrevious(context, part, prev);

    if (impl->ETSTypeReferencePartPreviousConst(context, part) != prev) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, prev) != part) {
        return TEST_ERROR_CODE;
    }
    return 0;
}

int TestETSUnionTypeSetTypes()
{
    auto *type = impl->CreateETSUnionTypeIr(context, nullptr, 0);
    auto *type0 = impl->CreateETSUnionTypeIr(context, nullptr, 0);
    auto *type1 = impl->CreateETSUnionTypeIr(context, nullptr, 0);
    es2panda_AstNode *types[] = {type0, type1};

    impl->ETSUnionTypeIrSetTypes(context, type, types, TEST_ARRAY_LEN);

    size_t returnedTypesLen;
    es2panda_AstNode **returnedTypes = impl->ETSUnionTypeIrTypesConst(context, type, &returnedTypesLen);
    if (returnedTypesLen != TEST_ARRAY_LEN) {
        return TEST_ERROR_CODE;
    }
    if (returnedTypes[0] != type0) {
        return TEST_ERROR_CODE;
    }
    if (returnedTypes[1] != type1) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, type0) != type) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, type1) != type) {
        return TEST_ERROR_CODE;
    }
    return 0;
}

int TestAnnotatedExpressionSetTypeAnnotation()
{
    auto *expr = impl->CreateIdentifier(context);
    auto *type = impl->CreateOpaqueTypeNode1(context);

    impl->AnnotatedExpressionSetTypeAnnotation(context, expr, type);

    if (impl->AnnotatedExpressionTypeAnnotationConst(context, expr) != type) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, type) != expr) {
        return TEST_ERROR_CODE;
    }
    return 0;
}

int TestArrowFunctionExpressionSetFunction()
{
    auto *arrow = impl->CreateArrowFunctionExpression(context, nullptr);
    auto *func = impl->CreateScriptFunction(
        context, nullptr, impl->CreateFunctionSignature(context, nullptr, nullptr, 0, nullptr, false), 0, 0);

    impl->ArrowFunctionExpressionSetFunction(context, arrow, func);

    if (impl->ArrowFunctionExpressionFunctionConst(context, arrow) != func) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, func) != arrow) {
        return TEST_ERROR_CODE;
    }
    return 0;
}

int TestBlockExpressionSetStatements()
{
    auto *block = impl->CreateBlockExpression(context, nullptr, 0);
    auto *statement0 = impl->CreateIfStatement(context, nullptr, nullptr, nullptr);
    auto *statement1 = impl->CreateBreakStatement(context);
    es2panda_AstNode *statements[] = {statement0, statement1};

    impl->BlockExpressionSetStatements(context, block, statements, TEST_ARRAY_LEN);

    size_t returnedStatementsLen;
    es2panda_AstNode **returnedStatements = impl->ETSUnionTypeIrTypesConst(context, block, &returnedStatementsLen);
    if (returnedStatementsLen != TEST_ARRAY_LEN) {
        return TEST_ERROR_CODE;
    }
    if (returnedStatements[0] != statement0) {
        return TEST_ERROR_CODE;
    }
    if (returnedStatements[1] != statement1) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, statement0) != block) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, statement1) != block) {
        return TEST_ERROR_CODE;
    }
    return 0;
}

int TestChainExpressionSetExpression()
{
    auto *chain = impl->CreateChainExpression(context, nullptr);
    auto *expr = impl->CreateChainExpression(context, nullptr);

    impl->ChainExpressionSetExpression(context, chain, expr);

    if (impl->ChainExpressionGetExpressionConst(context, chain) != expr) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, expr) != chain) {
        return TEST_ERROR_CODE;
    }
    return 0;
}

int TestFunctionExpressionSetFunction()
{
    auto *expr = impl->CreateFunctionExpression(context, nullptr);
    auto *func = impl->CreateScriptFunction(
        context, nullptr, impl->CreateFunctionSignature(context, nullptr, nullptr, 0, nullptr, false), 0, 0);

    impl->FunctionExpressionSetFunction(context, expr, func);

    if (impl->FunctionExpressionFunctionConst(context, expr) != func) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, func) != expr) {
        return TEST_ERROR_CODE;
    }
    return 0;
}

int TestFunctionExpressionSetId()
{
    auto *expr = impl->CreateFunctionExpression(context, nullptr);
    auto *id = impl->CreateIdentifier(context);

    impl->FunctionExpressionSetId(context, expr, id);

    if (impl->FunctionExpressionId(context, expr) != id) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, id) != expr) {
        return TEST_ERROR_CODE;
    }
    return 0;
}

int TestObjectExpressionSetProperties()
{
    auto *obj =
        impl->CreateObjectExpression(context, Es2pandaAstNodeType::AST_NODE_TYPE_OBJECT_EXPRESSION, nullptr, 0, false);
    auto *prop0 = impl->CreateExpressionStatement(context, nullptr);
    auto *prop1 = impl->CreateExpressionStatement(context, nullptr);
    es2panda_AstNode *properties[] = {prop0, prop1};

    impl->ObjectExpressionSetProperties(context, obj, properties, TEST_ARRAY_LEN);

    size_t returnedPropertiesLen;
    es2panda_AstNode **returnedProperties = impl->ObjectExpressionPropertiesConst(context, obj, &returnedPropertiesLen);
    if (returnedPropertiesLen != TEST_ARRAY_LEN) {
        return TEST_ERROR_CODE;
    }
    if (returnedProperties[0] != prop0) {
        return TEST_ERROR_CODE;
    }
    if (returnedProperties[1] != prop1) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, prop0) != obj) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, prop1) != obj) {
        return TEST_ERROR_CODE;
    }
    return 0;
}

int TestTemplateLiteralSetQuasis()
{
    auto *literal = impl->CreateTemplateLiteral(context, nullptr, 0, nullptr, 0, nullptr);
    auto *quasis0 = impl->CreateTemplateElement(context);
    auto *quasis1 = impl->CreateTemplateElement(context);
    es2panda_AstNode *quasisList[] = {quasis0, quasis1};

    impl->TemplateLiteralSetQuasis(context, literal, quasisList, TEST_ARRAY_LEN);

    size_t returnedQuasisLen;
    es2panda_AstNode **returnedQuasis = impl->TemplateLiteralQuasisConst(context, literal, &returnedQuasisLen);
    if (returnedQuasisLen != TEST_ARRAY_LEN) {
        return TEST_ERROR_CODE;
    }
    if (returnedQuasis[0] != quasis0) {
        return TEST_ERROR_CODE;
    }
    if (returnedQuasis[1] != quasis1) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, quasis0) != literal) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, quasis1) != literal) {
        return TEST_ERROR_CODE;
    }
    return 0;
}

int TestTemplateLiteralSetExpressions()
{
    auto *literal = impl->CreateTemplateLiteral(context, nullptr, 0, nullptr, 0, nullptr);
    auto *expr0 = impl->CreateThisExpression(context);
    auto *expr1 = impl->CreateThisExpression(context);
    es2panda_AstNode *expressions[] = {expr0, expr1};

    impl->TemplateLiteralSetExpressions(context, literal, expressions, TEST_ARRAY_LEN);

    size_t returnedExpressionsLen;
    es2panda_AstNode **returnedExpressions =
        impl->TemplateLiteralExpressionsConst(context, literal, &returnedExpressionsLen);
    if (returnedExpressionsLen != TEST_ARRAY_LEN) {
        return TEST_ERROR_CODE;
    }
    if (returnedExpressions[0] != expr0) {
        return TEST_ERROR_CODE;
    }
    if (returnedExpressions[1] != expr1) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, expr0) != literal) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, expr1) != literal) {
        return TEST_ERROR_CODE;
    }
    return 0;
}

int TestUpdateExpressionSetArgument()
{
    auto *expr =
        impl->CreateUpdateExpression(context, nullptr, Es2pandaTokenType::TOKEN_TYPE_PUNCTUATOR_PLUS_PLUS, false);
    auto *id = impl->CreateIdentifier(context);

    impl->UpdateExpressionSetArgument(context, expr, id);

    if (impl->UpdateExpressionArgumentConst(context, expr) != id) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, id) != expr) {
        return TEST_ERROR_CODE;
    }
    return 0;
}

int TestImportDeclarationSetSource()
{
    auto *import = impl->CreateImportDeclaration(context, nullptr, nullptr, 0, Es2pandaImportKinds::IMPORT_KINDS_ALL);
    auto *literal = impl->CreateStringLiteral(context);

    impl->ImportDeclarationSetSource(context, import, literal);

    if (impl->ImportDeclarationSourceConst(context, import) != literal) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, literal) != import) {
        return TEST_ERROR_CODE;
    }
    return 0;
}

int TestImportDeclarationSetSpecifiers()
{
    auto *import = impl->CreateImportDeclaration(context, nullptr, nullptr, 0, Es2pandaImportKinds::IMPORT_KINDS_ALL);
    auto *spec0 = impl->CreateImportSpecifier(context, nullptr, nullptr);
    auto *spec1 = impl->CreateImportSpecifier(context, nullptr, nullptr);
    es2panda_AstNode *specifiers[] = {spec0, spec1};

    impl->ImportDeclarationSetSpecifiers(context, import, specifiers, TEST_ARRAY_LEN);

    size_t returnedSpecifiersLen;
    es2panda_AstNode **returnedSpecifiers =
        impl->ImportDeclarationSpecifiersConst(context, import, &returnedSpecifiersLen);
    if (returnedSpecifiersLen != TEST_ARRAY_LEN) {
        return TEST_ERROR_CODE;
    }
    if (returnedSpecifiers[0] != spec0) {
        return TEST_ERROR_CODE;
    }
    if (returnedSpecifiers[1] != spec1) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, spec0) != import) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, spec1) != import) {
        return TEST_ERROR_CODE;
    }
    return 0;
}

int TestDoWhileStatementSetBody()
{
    auto *loop = impl->CreateDoWhileStatement(context, nullptr, nullptr);
    auto *statement = impl->CreateEmptyStatement(context);

    impl->DoWhileStatementSetBody(context, loop, statement);

    if (impl->DoWhileStatementBodyConst(context, loop) != statement) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, statement) != loop) {
        return TEST_ERROR_CODE;
    }
    return 0;
}

int TestDoWhileStatementSetTest()
{
    auto *loop = impl->CreateDoWhileStatement(context, nullptr, nullptr);
    auto *expr = impl->CreateThisExpression(context);

    impl->DoWhileStatementSetTest(context, loop, expr);

    if (impl->DoWhileStatementTestConst(context, loop) != expr) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, expr) != loop) {
        return TEST_ERROR_CODE;
    }
    return 0;
}

int TestForInStatementSetLeft()
{
    auto *loop = impl->CreateForInStatement(context, nullptr, nullptr, nullptr);
    auto *expr = impl->CreateThisExpression(context);

    impl->ForInStatementSetLeft(context, loop, expr);

    if (impl->ForInStatementLeftConst(context, loop) != expr) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, expr) != loop) {
        return TEST_ERROR_CODE;
    }
    return 0;
}

int TestForInStatementSetRight()
{
    auto *loop = impl->CreateForInStatement(context, nullptr, nullptr, nullptr);
    auto *expr = impl->CreateThisExpression(context);

    impl->ForInStatementSetRight(context, loop, expr);

    if (impl->ForInStatementRightConst(context, loop) != expr) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, expr) != loop) {
        return TEST_ERROR_CODE;
    }
    return 0;
}

int TestForInStatementSetBody()
{
    auto *loop = impl->CreateForInStatement(context, nullptr, nullptr, nullptr);
    auto *body = impl->CreateEmptyStatement(context);

    impl->ForInStatementSetBody(context, loop, body);

    if (impl->ForInStatementBodyConst(context, loop) != body) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, body) != loop) {
        return TEST_ERROR_CODE;
    }
    return 0;
}

int TestForOfStatementSetLeft()
{
    auto *loop = impl->CreateForOfStatement(context, nullptr, nullptr, nullptr, false);
    auto *expr = impl->CreateThisExpression(context);

    impl->ForOfStatementSetLeft(context, loop, expr);

    if (impl->ForOfStatementLeftConst(context, loop) != expr) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, expr) != loop) {
        return TEST_ERROR_CODE;
    }
    return 0;
}

int TestForOfStatementSetRight()
{
    auto *loop = impl->CreateForOfStatement(context, nullptr, nullptr, nullptr, false);
    auto *expr = impl->CreateThisExpression(context);

    impl->ForOfStatementSetRight(context, loop, expr);

    if (impl->ForOfStatementRightConst(context, loop) != expr) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, expr) != loop) {
        return TEST_ERROR_CODE;
    }
    return 0;
}

int TestForOfStatementSetBody()
{
    auto *loop = impl->CreateForOfStatement(context, nullptr, nullptr, nullptr, false);
    auto *body = impl->CreateEmptyStatement(context);

    impl->ForOfStatementSetBody(context, loop, body);

    if (impl->ForOfStatementBodyConst(context, loop) != body) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, body) != loop) {
        return TEST_ERROR_CODE;
    }
    return 0;
}

int TestForUpdateStatementSetInit()
{
    auto *loop = impl->CreateForUpdateStatement(context, nullptr, nullptr, nullptr, nullptr);
    auto *expr = impl->CreateThisExpression(context);

    impl->ForUpdateStatementSetInit(context, loop, expr);

    if (impl->ForUpdateStatementInitConst(context, loop) != expr) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, expr) != loop) {
        return TEST_ERROR_CODE;
    }
    return 0;
}

int TestForUpdateStatementSetTest()
{
    auto *loop = impl->CreateForUpdateStatement(context, nullptr, nullptr, nullptr, nullptr);
    auto *expr = impl->CreateThisExpression(context);

    impl->ForUpdateStatementSetTest(context, loop, expr);

    if (impl->ForUpdateStatementTestConst(context, loop) != expr) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, expr) != loop) {
        return TEST_ERROR_CODE;
    }
    return 0;
}

int TestForUpdateStatementSetUpdate()
{
    auto *loop = impl->CreateForUpdateStatement(context, nullptr, nullptr, nullptr, nullptr);
    auto *expr = impl->CreateThisExpression(context);

    impl->ForUpdateStatementSetUpdate(context, loop, expr);

    if (impl->ForUpdateStatementUpdateConst(context, loop) != expr) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, expr) != loop) {
        return TEST_ERROR_CODE;
    }
    return 0;
}

int TestForUpdateStatementSetBody()
{
    auto *loop = impl->CreateForUpdateStatement(context, nullptr, nullptr, nullptr, nullptr);
    auto *statement = impl->CreateEmptyStatement(context);

    impl->ForUpdateStatementSetBody(context, loop, statement);

    if (impl->ForUpdateStatementBodyConst(context, loop) != statement) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, statement) != loop) {
        return TEST_ERROR_CODE;
    }
    return 0;
}

int TestFunctionDeclarationSetFunction()
{
    auto *decl = impl->CreateFunctionDeclaration1(context, nullptr, false);
    auto *func = impl->CreateScriptFunction(
        context, nullptr, impl->CreateFunctionSignature(context, nullptr, nullptr, 0, nullptr, false), 0, 0);

    impl->FunctionDeclarationSetFunction(context, decl, func);

    if (impl->FunctionDeclarationFunctionConst(context, decl) != func) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, func) != decl) {
        return TEST_ERROR_CODE;
    }
    return 0;
}

int TestIfStatementSetTest()
{
    auto *ifst = impl->CreateIfStatement(context, nullptr, nullptr, nullptr);
    auto *test = impl->CreateThisExpression(context);

    impl->IfStatementSetTest(context, ifst, test);

    if (impl->IfStatementTestConst(context, ifst) != test) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, test) != ifst) {
        return TEST_ERROR_CODE;
    }
    return 0;
}

int TestIfStatementSetConsequent()
{
    auto *ifst = impl->CreateIfStatement(context, nullptr, nullptr, nullptr);
    auto *statement = impl->CreateEmptyStatement(context);

    impl->IfStatementSetConsequent(context, ifst, statement);

    if (impl->IfStatementConsequentConst(context, ifst) != statement) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, statement) != ifst) {
        return TEST_ERROR_CODE;
    }
    return 0;
}

int TestSwitchCaseStatementSetConsequent()
{
    auto *switchCase = impl->CreateSwitchCaseStatement(context, nullptr, nullptr, 0);
    auto *conseq0 = impl->CreateEmptyStatement(context);
    auto *conseq1 = impl->CreateEmptyStatement(context);
    es2panda_AstNode *consequentList[] = {conseq0, conseq1};

    impl->SwitchCaseStatementSetConsequent(context, switchCase, consequentList, TEST_ARRAY_LEN);

    size_t returnedConsequentLen;
    es2panda_AstNode **returnedConsequent =
        impl->SwitchCaseStatementConsequentConst(context, switchCase, &returnedConsequentLen);
    if (returnedConsequentLen != TEST_ARRAY_LEN) {
        return TEST_ERROR_CODE;
    }
    if (returnedConsequent[0] != conseq0) {
        return TEST_ERROR_CODE;
    }
    if (returnedConsequent[1] != conseq1) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, conseq0) != switchCase) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, conseq1) != switchCase) {
        return TEST_ERROR_CODE;
    }
    return 0;
}

int TestSwitchStatementSetCases()
{
    auto *switchStmt = impl->CreateSwitchStatement(context, nullptr, nullptr, 0);
    auto *case0 = impl->CreateSwitchCaseStatement(context, nullptr, nullptr, 0);
    auto *case1 = impl->CreateSwitchCaseStatement(context, nullptr, nullptr, 0);
    es2panda_AstNode *cases[] = {case0, case1};

    impl->SwitchStatementSetCases(context, switchStmt, cases, TEST_ARRAY_LEN);

    size_t returnedCasesLen;
    es2panda_AstNode **returnedCases = impl->SwitchStatementCasesConst(context, switchStmt, &returnedCasesLen);
    if (returnedCasesLen != TEST_ARRAY_LEN) {
        return TEST_ERROR_CODE;
    }
    if (returnedCases[0] != case0) {
        return TEST_ERROR_CODE;
    }
    if (returnedCases[1] != case1) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, case0) != switchStmt) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, case1) != switchStmt) {
        return TEST_ERROR_CODE;
    }
    return 0;
}

int TestTryStatementSetFinallyBlock()
{
    auto *tryStmt = impl->CreateTryStatement(context, nullptr, nullptr, 0, nullptr, nullptr, 0, nullptr, 0);
    auto *statement = impl->CreateEmptyStatement(context);

    impl->TryStatementSetFinallyBlock(context, tryStmt, statement);

    if (impl->TryStatementFinallyBlockConst(context, tryStmt) != statement) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, statement) != tryStmt) {
        return TEST_ERROR_CODE;
    }
    return 0;
}

int TestTryStatementSetBlock()
{
    auto *tryStmt = impl->CreateTryStatement(context, nullptr, nullptr, 0, nullptr, nullptr, 0, nullptr, 0);
    auto *statement = impl->CreateEmptyStatement(context);

    impl->TryStatementSetBlock(context, tryStmt, statement);

    if (impl->TryStatementBlockConst(context, tryStmt) != statement) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, statement) != tryStmt) {
        return TEST_ERROR_CODE;
    }
    return 0;
}

int TestTryStatementSetCatchClauses()
{
    auto *tryStmt = impl->CreateTryStatement(context, nullptr, nullptr, 0, nullptr, nullptr, 0, nullptr, 0);
    auto *catch0 = impl->CreateCatchClause(context, nullptr, nullptr);
    auto *catch1 = impl->CreateCatchClause(context, nullptr, nullptr);
    es2panda_AstNode *catchClauses[] = {catch0, catch1};

    impl->TryStatementSetCatchClauses(context, tryStmt, catchClauses, TEST_ARRAY_LEN);

    size_t returnedCatchClausesLen;
    es2panda_AstNode **returnedCatchClauses =
        impl->TryStatementCatchClausesConst(context, tryStmt, &returnedCatchClausesLen);
    if (returnedCatchClausesLen != TEST_ARRAY_LEN) {
        return TEST_ERROR_CODE;
    }
    if (returnedCatchClauses[0] != catch0) {
        return TEST_ERROR_CODE;
    }
    if (returnedCatchClauses[1] != catch1) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, catch0) != tryStmt) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, catch1) != tryStmt) {
        return TEST_ERROR_CODE;
    }
    return 0;
}

int TestVariableDeclarationSetDeclarators()
{
    auto *declaration = impl->CreateVariableDeclaration(
        context, Es2pandaVariableDeclarationKind::VARIABLE_DECLARATION_KIND_CONST, nullptr, 0);
    auto *declarator0 = impl->CreateVariableDeclarator(
        context, Es2pandaVariableDeclaratorFlag::VARIABLE_DECLARATOR_FLAG_CONST, nullptr);
    auto *declarator1 = impl->CreateVariableDeclarator(
        context, Es2pandaVariableDeclaratorFlag::VARIABLE_DECLARATOR_FLAG_CONST, nullptr);
    es2panda_AstNode *declarators[] = {declarator0, declarator1};

    impl->VariableDeclarationSetDeclarators(context, declaration, declarators, TEST_ARRAY_LEN);

    size_t returnedDeclaratorsLen;
    es2panda_AstNode **returnedDeclarators =
        impl->VariableDeclarationDeclaratorsConst(context, declaration, &returnedDeclaratorsLen);
    if (returnedDeclaratorsLen != TEST_ARRAY_LEN) {
        return TEST_ERROR_CODE;
    }
    if (returnedDeclarators[0] != declarator0) {
        return TEST_ERROR_CODE;
    }
    if (returnedDeclarators[1] != declarator1) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, declarator0) != declaration) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, declarator1) != declaration) {
        return TEST_ERROR_CODE;
    }
    return 0;
}

int TestVariableDeclaratorSetId()
{
    auto *declarator = impl->CreateVariableDeclarator(
        context, Es2pandaVariableDeclaratorFlag::VARIABLE_DECLARATOR_FLAG_CONST, nullptr);
    auto *ident = impl->CreateIdentifier(context);

    impl->VariableDeclaratorSetId(context, declarator, ident);

    if (impl->VariableDeclaratorIdConst(context, declarator) != ident) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, ident) != declarator) {
        return TEST_ERROR_CODE;
    }
    return 0;
}

int TestWhileStatementSetBody()
{
    auto *loop = impl->CreateWhileStatement(context, nullptr, nullptr);
    auto *statement = impl->CreateEmptyStatement(context);

    impl->WhileStatementSetBody(context, loop, statement);

    if (impl->WhileStatementBodyConst(context, loop) != statement) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, statement) != loop) {
        return TEST_ERROR_CODE;
    }
    return 0;
}

int TestTSInterfaceBodySetBody()
{
    auto *interface = impl->CreateTSInterfaceBody(context, nullptr, 0);
    auto *statement0 = impl->CreateEmptyStatement(context);
    auto *statement1 = impl->CreateEmptyStatement(context);
    es2panda_AstNode *statements[] = {statement0, statement1};

    impl->TSInterfaceBodySetBody(context, interface, statements, TEST_ARRAY_LEN);

    size_t returnedBodyLen;
    es2panda_AstNode **returnedBody = impl->TSInterfaceBodyBodyConst(context, interface, &returnedBodyLen);
    if (returnedBodyLen != TEST_ARRAY_LEN) {
        return TEST_ERROR_CODE;
    }
    if (returnedBody[0] != statement0) {
        return TEST_ERROR_CODE;
    }
    if (returnedBody[1] != statement1) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, statement0) != interface) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, statement1) != interface) {
        return TEST_ERROR_CODE;
    }
    return 0;
}

int TestTSInterfaceDeclarationSetId()
{
    auto *interface = impl->CreateTSInterfaceDeclaration(context, nullptr, 0, nullptr, nullptr, nullptr, false, false);
    auto *ident = impl->CreateIdentifier(context);

    impl->TSInterfaceDeclarationSetId(context, interface, ident);

    if (impl->TSInterfaceDeclarationIdConst(context, interface) != ident) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, ident) != interface) {
        return TEST_ERROR_CODE;
    }
    return 0;
}

int TestTSInterfaceDeclarationSetTypeParams()
{
    auto *interface = impl->CreateTSInterfaceDeclaration(context, nullptr, 0, nullptr, nullptr, nullptr, false, false);
    auto *typeParams = impl->CreateTSTypeParameterDeclaration(context, nullptr, 0, 0);

    impl->TSInterfaceDeclarationSetTypeParams(context, interface, typeParams);

    if (impl->TSInterfaceDeclarationTypeParamsConst(context, interface) != typeParams) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, typeParams) != interface) {
        return TEST_ERROR_CODE;
    }
    return 0;
}

int TestTSInterfaceDeclarationSetBody()
{
    auto *interface = impl->CreateTSInterfaceDeclaration(context, nullptr, 0, nullptr, nullptr, nullptr, false, false);
    auto *body = impl->CreateTSInterfaceBody(context, nullptr, 0);

    impl->TSInterfaceDeclarationSetBody(context, interface, body);

    if (impl->TSInterfaceDeclarationBodyConst(context, interface) != body) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, body) != interface) {
        return TEST_ERROR_CODE;
    }
    return 0;
}

int TestTSInterfaceDeclarationSetExtends()
{
    auto *interface = impl->CreateTSInterfaceDeclaration(context, nullptr, 0, nullptr, nullptr, nullptr, false, false);
    auto *extends0 = impl->CreateTSInterfaceHeritage(context, nullptr);
    auto *extends1 = impl->CreateTSInterfaceHeritage(context, nullptr);
    es2panda_AstNode *extends[] = {extends0, extends1};

    impl->TSInterfaceDeclarationSetExtends(context, interface, extends, TEST_ARRAY_LEN);

    size_t returnedExtendsLen;
    es2panda_AstNode **returnedExtends =
        impl->TSInterfaceDeclarationExtendsConst(context, interface, &returnedExtendsLen);
    if (returnedExtendsLen != TEST_ARRAY_LEN) {
        return TEST_ERROR_CODE;
    }
    if (returnedExtends[0] != extends0) {
        return TEST_ERROR_CODE;
    }
    if (returnedExtends[1] != extends1) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, extends0) != interface) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, extends1) != interface) {
        return TEST_ERROR_CODE;
    }
    return 0;
}

int TestTSNonNullExpressionSetExpr()
{
    auto *expr = impl->CreateTSNonNullExpression(context, nullptr);
    auto *inner = impl->CreateThisExpression(context);

    impl->TSNonNullExpressionSetExpr(context, expr, inner);

    if (impl->TSNonNullExpressionExprConst(context, expr) != inner) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, inner) != expr) {
        return TEST_ERROR_CODE;
    }
    return 0;
}

int TestTSTypeAliasDeclarationSetId()
{
    auto *expr = impl->CreateTSTypeAliasDeclaration1(context, nullptr);
    auto *id = impl->CreateIdentifier(context);

    impl->TSTypeAliasDeclarationSetId(context, expr, id);

    if (impl->TSTypeAliasDeclarationIdConst(context, expr) != id) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, id) != expr) {
        return TEST_ERROR_CODE;
    }
    return 0;
}

int TestTSTypeAliasDeclarationSetTypeAnnotation()
{
    auto *expr = impl->CreateTSTypeAliasDeclaration1(context, nullptr);
    auto *type = impl->CreateOpaqueTypeNode1(context);

    impl->TSTypeAliasDeclarationSetTypeAnnotation(context, expr, type);

    if (impl->TSTypeAliasDeclarationTypeAnnotationConst(context, expr) != type) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, type) != expr) {
        return TEST_ERROR_CODE;
    }
    return 0;
}

int TestTSTypeParameterDeclarationSetParams()
{
    auto *decl = impl->CreateTSTypeParameterDeclaration(context, nullptr, 0, 0);
    auto *param0 = impl->CreateTSTypeParameter(context, nullptr, nullptr, nullptr);
    auto *param1 = impl->CreateTSTypeParameter(context, nullptr, nullptr, nullptr);
    es2panda_AstNode *params[] = {param0, param1};

    impl->TSTypeParameterDeclarationSetParams(context, decl, params, TEST_ARRAY_LEN);

    size_t returnedParamsLen;
    es2panda_AstNode **returnedParams = impl->TSTypeParameterDeclarationParamsConst(context, decl, &returnedParamsLen);
    if (returnedParamsLen != TEST_ARRAY_LEN) {
        return TEST_ERROR_CODE;
    }
    if (returnedParams[0] != param0) {
        return TEST_ERROR_CODE;
    }
    if (returnedParams[1] != param1) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, param0) != decl) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, param1) != decl) {
        return TEST_ERROR_CODE;
    }
    return 0;
}

int TestTSTypeParameterInstantiationSetParams()
{
    auto *inst = impl->CreateTSTypeParameterInstantiation(context, nullptr, 0);
    auto *param0 = impl->CreateOpaqueTypeNode1(context);
    auto *param1 = impl->CreateOpaqueTypeNode1(context);
    es2panda_AstNode *params[] = {param0, param1};

    impl->TSTypeParameterInstantiationSetParams(context, inst, params, TEST_ARRAY_LEN);

    size_t returnedParamsLen;
    es2panda_AstNode **returnedParams =
        impl->TSTypeParameterInstantiationParamsConst(context, inst, &returnedParamsLen);
    if (returnedParamsLen != TEST_ARRAY_LEN) {
        return TEST_ERROR_CODE;
    }
    if (returnedParams[0] != param0) {
        return TEST_ERROR_CODE;
    }
    if (returnedParams[1] != param1) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, param0) != inst) {
        return TEST_ERROR_CODE;
    }
    if (impl->AstNodeParentConst(context, param1) != inst) {
        return TEST_ERROR_CODE;
    }
    return 0;
}

auto tests = {
    TestClassDefinitionSetImplements,
    TestClassStaticBlockSetFunction,
    TestMethodDefinitionSetOverloads,
    TestPropertySetKey,
    TestScriptFunctionSetTypeParams,
    TestScriptFunctionSetIdent,
    TestETSFunctionTypeSetTypeParams,
    TestETSFunctionTypeSetReturnType,
    TestETSModuleSetIdent,
    TestETSNewClassInstanceExpressionSetTypeRef,
    TestETSNewClassInstanceExpressionSetArguments,
    TestETSTypeReferenceSetPart,
    TestETSTypeReferencePartSetName,
    TestETSTypeReferencePartSetTypeParams,
    TestETSTypeReferencePartSetPrevious,
    TestETSUnionTypeSetTypes,
    TestAnnotatedExpressionSetTypeAnnotation,
    TestArrowFunctionExpressionSetFunction,
    TestBlockExpressionSetStatements,
    TestChainExpressionSetExpression,
    TestFunctionExpressionSetFunction,
    TestFunctionExpressionSetId,
    TestObjectExpressionSetProperties,
    TestTemplateLiteralSetQuasis,
    TestTemplateLiteralSetExpressions,
    TestUpdateExpressionSetArgument,
    TestImportDeclarationSetSource,
    TestImportDeclarationSetSpecifiers,
    TestDoWhileStatementSetBody,
    TestDoWhileStatementSetTest,
    TestForInStatementSetLeft,
    TestForInStatementSetRight,
    TestForInStatementSetBody,
    TestForOfStatementSetLeft,
    TestForOfStatementSetRight,
    TestForOfStatementSetBody,
    TestForUpdateStatementSetInit,
    TestForUpdateStatementSetTest,
    TestForUpdateStatementSetUpdate,
    TestForUpdateStatementSetBody,
    TestFunctionDeclarationSetFunction,
    TestIfStatementSetTest,
    TestIfStatementSetConsequent,
    TestSwitchCaseStatementSetConsequent,
    TestSwitchStatementSetCases,
    TestTryStatementSetFinallyBlock,
    TestTryStatementSetBlock,
    TestTryStatementSetCatchClauses,
    TestVariableDeclarationSetDeclarators,
    TestVariableDeclaratorSetId,
    TestWhileStatementSetBody,
    TestTSInterfaceBodySetBody,
    TestTSInterfaceDeclarationSetId,
    TestTSInterfaceDeclarationSetTypeParams,
    TestTSInterfaceDeclarationSetBody,
    TestTSInterfaceDeclarationSetExtends,
    TestTSNonNullExpressionSetExpr,
    TestTSTypeAliasDeclarationSetId,
    TestTSTypeAliasDeclarationSetTypeAnnotation,
    TestTSTypeParameterDeclarationSetParams,
    TestTSTypeParameterInstantiationSetParams,
};

int main(int argc, char **argv)
{
    if (argc < MIN_ARGC) {
        return INVALID_ARGC_ERROR_CODE;
    }

    if (GetImpl() == nullptr) {
        return NULLPTR_IMPL_ERROR_CODE;
    }
    impl = GetImpl();

    const char **args = const_cast<const char **>(&(argv[1]));
    config = impl->CreateConfig(argc - 1, args);
    context = impl->CreateContextFromString(config, source.data(), argv[argc - 1]);
    if (context == nullptr) {
        return NULLPTR_CONTEXT_ERROR_CODE;
    }

    for (auto *test : tests) {
        const int result = test();
        if (result != 0) {
            return result;
        }
    }

    impl->DestroyContext(context);
    impl->DestroyConfig(config);

    return 0;
}

// NOLINTEND