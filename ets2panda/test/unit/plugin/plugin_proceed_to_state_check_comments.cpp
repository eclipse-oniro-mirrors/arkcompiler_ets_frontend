/**
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

#include <cstddef>
#include <iostream>
#include <map>
#include <ostream>
#include <string>

#include "public/es2panda_lib.h"
#include "util.h"

// NOLINTBEGIN

static es2panda_Impl *impl = nullptr;

static std::map<std::string, es2panda_AstNode *> classMap = {
    {"CommentTestClass", nullptr},
    {"CommentEnum", nullptr},
};

static std::map<std::string, es2panda_AstNode *> interfaceMap = {
    {"CommentTestInterface", nullptr},
};

static std::map<std::string, es2panda_AstNode *> methodMap = {
    {"classMethod", nullptr},
    {"interfaceMethod", nullptr},
    {"testGet", nullptr},
    {"testSet", nullptr},
    {"interfaceProp", nullptr},
    {"onlyJsdocFunc", nullptr},
    {"onlyBlockFunc", nullptr},
    {"onlyLineFunc", nullptr},
    {"multiLineFunc", nullptr},
    {"mixedLineJsdocFunc", nullptr},
    {"mixedBlockJsdocFunc", nullptr},
    {"multiJsdocFunc", nullptr},
    {"paramFunc", nullptr},
    {"noCommentFunc", nullptr},
    {"withModifierFunc", nullptr},
    {"adjacentFunc", nullptr},
    {"tool", nullptr},
    {"tool2", nullptr},
    {"target", nullptr},
    {"targetWithRealComment", nullptr},
    {"targetWithUrlComment", nullptr},
    {"targetWithStarClose", nullptr},
    {"targetWithFakeStart", nullptr},
    {"targetAfterFakeStartStr", nullptr},
    {"targetAfterFakeEndStr", nullptr},
    {"targetAfterNestedComment", nullptr},
};

static std::map<std::string, es2panda_AstNode *> propertyMap = {
    {"classProp", nullptr},   {"interfaceProp", nullptr}, {"commentVar", nullptr},
    {"withAnnoVar", nullptr}, {"MEMBER", nullptr},        {"structProp", nullptr},
};

static std::map<std::string, es2panda_AstNode *> annotationMap = {
    {"CommentTestAnno", nullptr},
};

static std::map<std::string, es2panda_AstNode *> paramMap = {
    {"p1", nullptr},
};

static es2panda_AstNode *typeAlias = nullptr;
static es2panda_AstNode *structDecl = nullptr;
static es2panda_AstNode *importDecl = nullptr;
static es2panda_AstNode *reExportedDecl = nullptr;
static es2panda_AstNode *exportNamedDecl = nullptr;

static std::map<std::string, es2panda_AstNode *> callMap = {
    {"chainMethod2", nullptr},           {"chainMethod3", nullptr},
    {"chainMethod1OnArg", nullptr},      {"optionalChainMethod2", nullptr},
    {"optionalChainMethod3", nullptr},   {"optionalChainMethod1OnArg", nullptr},
    {"structChainMethod1", nullptr},     {"structChainMethod2", nullptr},
    {"structChainMethod3", nullptr},     {"structChainMethod1OnArg", nullptr},
    {"miscConstVar", nullptr},           {"miscLetVar", nullptr},
    {"miscSingleMethodCall", nullptr},   {"miscOptionalMethodCall", nullptr},
    {"miscMultiMethodCall", nullptr},    {"miscPropAccess", nullptr},
    {"miscOptionalPropAccess", nullptr}, {"miscFunctionCall", nullptr},
};

// OptionalLowering may rewrite `?.` call sites after parse; keep comment text captured at PARSED.
// Struct UI chains must also be captured at PARSED (before plugins-after-parse transforms structs).
static std::map<std::string, std::string> callCommentsMap;

static std::string g_source = R"('use static'

/**
 * import jsdoc
 */
import { PI, E } from "std/math/consts"

@interface testAnno {}

/** jsdoc only */
function onlyJsdocFunc(): void {}

/* block only */
function onlyBlockFunc(): void {}

// line only
function onlyLineFunc(): void {}

// line comment 1
// line comment 2
function multiLineFunc(): void {}

// line a
/** jsdoc b */
function mixedLineJsdocFunc(): void {}

/* block a */
/** jsdoc b */
function mixedBlockJsdocFunc(): void {}

/** jsdoc a */
/** jsdoc b */
function multiJsdocFunc(): void {}

/** class jsdoc */
export declare class CommentTestClass {
    /** method jsdoc */
    classMethod(): void {}

    /** property jsdoc */
    classProp: number

    /** getter jsdoc */
    get testGet(): number

    /** setter jsdoc */
    set testSet(n: number)
}

/** interface jsdoc */
export declare interface CommentTestInterface {
    /** interface method jsdoc */
    interfaceMethod(): void

    /** interface prop jsdoc */
    interfaceProp: number
}

/** annotation jsdoc */
@interface CommentTestAnno {}

/** type alias jsdoc */
type CommentTypeAlias = number

/** variable jsdoc */
let commentVar: number = 1

/** param jsdoc */
function paramFunc(
    /** p1 jsdoc */
    p1: number
): void {}

/** enum jsdoc */
export declare enum CommentEnum {
    /** member jsdoc */
    MEMBER = 1
}

/** struct jsdoc */
export declare struct CommentStruct {
    /** struct prop jsdoc */
    structProp: number
}

/**
 * export jsdoc
 */
export { commentVar }

/**
 * re-export jsdoc
 */
export { PI, E } from "std/math/consts"

function noCommentFunc(): void {}

/**
 * with annotation jsdoc
 */
@testAnno
let withAnnoVar: number = 1

/**
 * export declare jsdoc
 */
export declare function withModifierFunc(): void

/*
 * adjacent block
 */
/** adjacent jsdoc */
function adjacentFunc(): void {}

/**
this is jsdoc of tool
*/
export function tool(): void {}

/*
this isn't jsdoc of tool2
*/
export function tool2(): void {}

const url = "https://example.cn"
function target(): void {}

const path = "https://example.cn" // real comment after string
function targetWithRealComment(): void {}

// see https://example.cn for details
function targetWithUrlComment(): void {}

// trailing star close */
function targetWithStarClose(): void {}

// line comment with fake start /* inside
function targetWithFakeStart(): void {}

const fakeStartStr = "/* not a comment */"
function targetAfterFakeStartStr(): void {}

const fakeEndStr = "*/"
function targetAfterFakeEndStr(): void {}

/* nested outer
 * nested /* inner (inert) */
function targetAfterNestedComment(): void {}

class ChainClass {
    prop: number = 0;

    static $_invoke() {
        return new ChainClass();
    }

    chainMethod1(): this {
        return this;
    }
    chainMethod2(): this {
        return this;
    }

    chainMethod3(inner: (arg: ChainClass) => void): this {
        return this;
    }

    invokeChainMethod(): void {
        this.chainMethod1()
        // test for chain method2
        .chainMethod2()
        // test for chain method3
        .chainMethod3((arg: ChainClass) => {
            // test for chain method1 on arg
            arg.chainMethod1();
        });
    }

    invokeOptionalChainMethod(): void {
        this.chainMethod1()
        // test for optional chain method2
        ?.chainMethod2()
        // test for optional chain method3
        ?.chainMethod3((optArg: ChainClass) => {
            // test for optional chain method1 on arg
            optArg.chainMethod1();
        });
    }
}

struct StructChainClass {
    build() {
        ChainClass()
        // test for struct chain method1
        .chainMethod1()
        // test for struct chain method2
        .chainMethod2()
        // test for struct chain method3
        ?.chainMethod3((structArg: ChainClass) => {
            // test for struct chain method1 on arg
            structArg.chainMethod1();
        });
    }
}

function testMiscComments(): void {
    // test for const var declaration
    const a = ChainClass();

    // test for non-const var declaration
    let b = a.chainMethod1();

    // test for single method call
    b.chainMethod2();

    // test for single optional method call
    b?.chainMethod2();

    // test for multiple method calls
    b.chainMethod1().chainMethod2();

    // test for property access
    b.prop;

    // test for property access with optional chaining
    b?.prop;

    // test for function call
    ChainClass();
}

function testEofPropAccess(): void {
    let e = ChainClass()
    // test for eof property access
    e.prop)";

static void PrintCommentsAndJsdoc(es2panda_Context *context, es2panda_AstNode *node)
{
    std::cout << "COMMENTS:" << std::endl;
    if (node != nullptr) {
        std::cout << impl->GetCommentsStringFromDeclaration(context, node) << std::endl;
    }
    std::cout << "JSDOC:" << std::endl;
    if (node != nullptr) {
        std::cout << impl->JsdocStringFromDeclaration(context, node) << std::endl;
    }
}

static void PrintCommentsOnly(es2panda_Context *context, es2panda_AstNode *node)
{
    if (node != nullptr) {
        std::cout << impl->GetCommentsStringFromDeclaration(context, node) << std::endl;
    }
}

// Note: `GetCommentsStringFromDeclaration` must tolerate a null node (returns an empty
// string) instead of crashing the whole compilation process. The legacy
// `JsdocStringFromDeclaration` still crashes on null input, so it is not exercised here.
static void TestNullNodeComments(es2panda_Context *context)
{
    std::cout << "# Section 9: Null node handling" << std::endl;

    std::cout << "COMMENTS:" << std::endl;
    std::cout << impl->GetCommentsStringFromDeclaration(context, nullptr) << std::endl;
}

static es2panda_AstNode *LookupMethod(const std::string &name)
{
    auto it = methodMap.find(name);
    return it != methodMap.end() ? it->second : nullptr;
}

static void FindMethodDef(es2panda_AstNode *ast, void *context)
{
    auto ctx = reinterpret_cast<es2panda_Context *>(context);
    if (!impl->IsMethodDefinition(ast)) {
        return;
    }
    auto *function = impl->MethodDefinitionFunction(ctx, ast);
    if (function == nullptr) {
        return;
    }
    auto *ident = impl->ScriptFunctionId(ctx, function);
    if (ident == nullptr) {
        return;
    }
    auto name = std::string(impl->IdentifierName(ctx, ident));
    if (methodMap.find(name) != methodMap.end()) {
        methodMap[name] = ast;
    }
}

static void FindClass(es2panda_AstNode *ast, void *context)
{
    auto ctx = reinterpret_cast<es2panda_Context *>(context);
    if (!impl->IsClassDefinition(ast)) {
        return;
    }
    auto *ident = impl->ClassDefinitionIdent(ctx, ast);
    if (ident == nullptr) {
        return;
    }
    auto name = std::string(impl->IdentifierName(ctx, ident));
    if (classMap.find(name) != classMap.end()) {
        classMap[name] = ast;
    }
}

static void FindInterface(es2panda_AstNode *ast, void *context)
{
    auto ctx = reinterpret_cast<es2panda_Context *>(context);
    if (!impl->IsTSInterfaceDeclaration(ast)) {
        return;
    }
    auto *ident = impl->TSInterfaceDeclarationId(ctx, ast);
    if (ident == nullptr) {
        return;
    }
    auto name = std::string(impl->IdentifierName(ctx, ident));
    if (interfaceMap.find(name) != interfaceMap.end()) {
        interfaceMap[name] = ast;
    }
}

static void FindClassElement(es2panda_AstNode *ast, void *context)
{
    auto ctx = reinterpret_cast<es2panda_Context *>(context);
    if (!impl->IsClassProperty(ast)) {
        return;
    }
    auto *ident = impl->ClassElementId(ctx, ast);
    if (ident == nullptr) {
        return;
    }
    auto name = std::string(impl->IdentifierName(ctx, ident));
    if (propertyMap.find(name) != propertyMap.end()) {
        propertyMap[name] = ast;
    }
}

static void FindAnnotationDecl(es2panda_AstNode *ast, void *context)
{
    auto ctx = reinterpret_cast<es2panda_Context *>(context);
    if (!impl->IsAnnotationDeclaration(ast)) {
        return;
    }
    auto *ident = impl->AnnotationDeclarationGetBaseNameConst(ctx, ast);
    if (ident == nullptr) {
        return;
    }
    auto name = std::string(impl->IdentifierName(ctx, ident));
    if (annotationMap.find(name) != annotationMap.end()) {
        annotationMap[name] = ast;
    }
}

static void FindETSParamDecl(es2panda_AstNode *ast, void *context)
{
    auto ctx = reinterpret_cast<es2panda_Context *>(context);
    if (!impl->IsETSParameterExpression(ast)) {
        return;
    }
    auto *ident = impl->ETSParameterExpressionIdent(ctx, ast);
    if (ident == nullptr) {
        return;
    }
    auto name = std::string(impl->IdentifierName(ctx, ident));
    if (paramMap.find(name) != paramMap.end()) {
        paramMap[name] = ast;
    }
}

static void FindTypeAliasDecl(es2panda_AstNode *ast, void *context)
{
    auto ctx = reinterpret_cast<es2panda_Context *>(context);
    if (!impl->IsTSTypeAliasDeclaration(ast)) {
        return;
    }
    auto *ident = impl->TSTypeAliasDeclarationId(ctx, ast);
    if (ident == nullptr) {
        return;
    }
    auto name = impl->IdentifierName(ctx, ident);
    if (std::string(name) == "CommentTypeAlias") {
        typeAlias = ast;
    }
}

static void FindStructDecl(es2panda_AstNode *ast, void *context)
{
    auto ctx = reinterpret_cast<es2panda_Context *>(context);
    if (!impl->IsETSStructDeclaration(ast)) {
        return;
    }
    // Prefer the jsdoc-covered CommentStruct used by Section 2; ignore StructChainClass.
    // ETSStructDeclaration extends ClassDeclaration.
    auto *def = impl->ClassDeclarationDefinition(ctx, ast);
    if (def == nullptr) {
        return;
    }
    auto *ident = impl->ClassDefinitionIdent(ctx, def);
    if (ident == nullptr) {
        return;
    }
    if (std::string(impl->IdentifierName(ctx, ident)) == "CommentStruct") {
        structDecl = ast;
    }
}

static void FindImportExportSpecifier(es2panda_AstNode *ast, [[maybe_unused]] void *context)
{
    if (impl->IsExportNamedDeclaration(ast)) {
        exportNamedDecl = ast;
    }

    if (impl->IsETSReExportDeclaration(ast)) {
        reExportedDecl = ast;
    }

    if (impl->IsETSImportDeclaration(ast)) {
        importDecl = ast;
    }
}

static void RecordCallComments(const std::string &key, es2panda_Context *ctx, es2panda_AstNode *ast)
{
    callMap[key] = ast;
    const char *comments = impl->GetCommentsStringFromDeclaration(ctx, ast);
    callCommentsMap[key] = comments != nullptr ? comments : "";
}

static void FindChainCallExpression(es2panda_AstNode *ast, void *context)
{
    auto ctx = reinterpret_cast<es2panda_Context *>(context);
    if (!impl->IsCallExpression(ast)) {
        return;
    }

    auto *callee = impl->CallExpressionCallee(ctx, ast);
    if (callee == nullptr || !impl->IsMemberExpression(callee)) {
        return;
    }

    auto *prop = impl->MemberExpressionProperty(ctx, callee);
    if (prop == nullptr || !impl->IsIdentifier(prop)) {
        return;
    }

    auto name = std::string(impl->IdentifierName(ctx, prop));
    const bool isOptional = impl->MaybeOptionalExpressionIsOptionalConst(ctx, callee);

    if (name == "chainMethod2") {
        RecordCallComments(isOptional ? "optionalChainMethod2" : "chainMethod2", ctx, ast);
        return;
    }
    if (name == "chainMethod3") {
        RecordCallComments(isOptional ? "optionalChainMethod3" : "chainMethod3", ctx, ast);
        return;
    }
    if (name == "chainMethod1") {
        auto *object = impl->MemberExpressionObject(ctx, callee);
        if (object == nullptr || !impl->IsIdentifier(object)) {
            return;
        }
        auto objectName = std::string(impl->IdentifierName(ctx, object));
        if (objectName == "arg") {
            RecordCallComments("chainMethod1OnArg", ctx, ast);
        } else if (objectName == "optArg") {
            RecordCallComments("optionalChainMethod1OnArg", ctx, ast);
        }
    }
}

// Only walk StructChainClass so UI-style ChainClass().method() chains are recorded separately,
// and captured at PARSED (before plugins-after-parse rewrites structs).
static void FindStructChainCallExpression(es2panda_AstNode *ast, void *context)
{
    auto ctx = reinterpret_cast<es2panda_Context *>(context);
    if (!impl->IsCallExpression(ast)) {
        return;
    }

    auto *callee = impl->CallExpressionCallee(ctx, ast);
    if (callee == nullptr || !impl->IsMemberExpression(callee)) {
        return;
    }

    auto *prop = impl->MemberExpressionProperty(ctx, callee);
    if (prop == nullptr || !impl->IsIdentifier(prop)) {
        return;
    }

    auto name = std::string(impl->IdentifierName(ctx, prop));
    if (name == "chainMethod1") {
        auto *object = impl->MemberExpressionObject(ctx, callee);
        if (object != nullptr && impl->IsCallExpression(object)) {
            auto *ctorCallee = impl->CallExpressionCallee(ctx, object);
            if (ctorCallee != nullptr && impl->IsIdentifier(ctorCallee) &&
                std::string(impl->IdentifierName(ctx, ctorCallee)) == "ChainClass") {
                RecordCallComments("structChainMethod1", ctx, ast);
            }
        } else if (object != nullptr && impl->IsIdentifier(object) &&
                   std::string(impl->IdentifierName(ctx, object)) == "structArg") {
            RecordCallComments("structChainMethod1OnArg", ctx, ast);
        }
        return;
    }
    if (name == "chainMethod2") {
        RecordCallComments("structChainMethod2", ctx, ast);
        return;
    }
    if (name == "chainMethod3") {
        RecordCallComments("structChainMethod3", ctx, ast);
    }
}

static void FindStructChainClass(es2panda_AstNode *ast, void *context)
{
    auto ctx = reinterpret_cast<es2panda_Context *>(context);
    if (!impl->IsETSStructDeclaration(ast)) {
        return;
    }
    auto *def = impl->ClassDeclarationDefinition(ctx, ast);
    if (def == nullptr) {
        return;
    }
    auto *ident = impl->ClassDefinitionIdent(ctx, def);
    if (ident == nullptr || std::string(impl->IdentifierName(ctx, ident)) != "StructChainClass") {
        return;
    }
    impl->AstNodeForEach(ast, FindStructChainCallExpression, context);
}

static void RecordMiscVariable(es2panda_Context *ctx, es2panda_AstNode *ast)
{
    size_t n = 0;
    auto **declarators = impl->VariableDeclarationDeclaratorsConst(ctx, ast, &n);
    if (declarators == nullptr || n == 0) {
        return;
    }
    auto *id = impl->VariableDeclaratorId(ctx, declarators[0]);
    if (id == nullptr || !impl->IsIdentifier(id)) {
        return;
    }
    auto name = std::string(impl->IdentifierName(ctx, id));
    if (name == "a") {
        RecordCallComments("miscConstVar", ctx, ast);
    } else if (name == "b") {
        RecordCallComments("miscLetVar", ctx, ast);
    }
}

// Unwrap ChainExpression wrappers: `b?.method()` / `b?.prop` are wrapped at PARSED.
static es2panda_AstNode *UnwrapChainExpr(es2panda_Context *ctx, es2panda_AstNode *expr)
{
    if (impl->IsChainExpression(expr)) {
        return const_cast<es2panda_AstNode *>(impl->ChainExpressionGetExpressionConst(ctx, expr));
    }
    return expr;
}

static void RecordMiscCall(es2panda_Context *ctx, es2panda_AstNode *expr)
{
    auto *callee = impl->CallExpressionCallee(ctx, expr);
    if (callee == nullptr) {
        return;
    }
    callee = UnwrapChainExpr(ctx, callee);
    if (callee == nullptr) {
        return;
    }
    if (impl->IsIdentifier(callee) && std::string(impl->IdentifierName(ctx, callee)) == "ChainClass") {
        RecordCallComments("miscFunctionCall", ctx, expr);
        return;
    }
    if (!impl->IsMemberExpression(callee)) {
        return;
    }
    auto *prop = impl->MemberExpressionProperty(ctx, callee);
    auto *object = impl->MemberExpressionObject(ctx, callee);
    if (prop == nullptr || !impl->IsIdentifier(prop) || object == nullptr) {
        return;
    }
    if (std::string(impl->IdentifierName(ctx, prop)) != "chainMethod2") {
        return;
    }
    const bool isOptional = impl->MaybeOptionalExpressionIsOptionalConst(ctx, callee);
    if (impl->IsIdentifier(object) && std::string(impl->IdentifierName(ctx, object)) == "b") {
        RecordCallComments(isOptional ? "miscOptionalMethodCall" : "miscSingleMethodCall", ctx, expr);
    } else if (impl->IsCallExpression(object)) {
        RecordCallComments("miscMultiMethodCall", ctx, expr);
    }
}

static void RecordMiscPropAccess(es2panda_Context *ctx, es2panda_AstNode *expr)
{
    auto *prop = impl->MemberExpressionProperty(ctx, expr);
    auto *object = impl->MemberExpressionObject(ctx, expr);
    if (prop == nullptr || !impl->IsIdentifier(prop) || object == nullptr || !impl->IsIdentifier(object)) {
        return;
    }
    if (std::string(impl->IdentifierName(ctx, prop)) != "prop") {
        return;
    }
    const auto objectName = std::string(impl->IdentifierName(ctx, object));
    if (objectName == "b") {
        const bool isOptional = impl->MaybeOptionalExpressionIsOptionalConst(ctx, expr);
        RecordCallComments(isOptional ? "miscOptionalPropAccess" : "miscPropAccess", ctx, expr);
        return;
    }
    // Source may end right at the property (no trailing newline/semicolon); ensure the
    // punctuator scan near EOF stays in bounds and still resolves the leading comment.
    if (objectName == "e") {
        RecordCallComments("eofPropAccess", ctx, expr);
    }
}

static void FindMiscCommentNode(es2panda_AstNode *ast, void *context)
{
    auto ctx = reinterpret_cast<es2panda_Context *>(context);

    if (impl->IsVariableDeclaration(ast)) {
        RecordMiscVariable(ctx, ast);
        return;
    }

    if (!impl->IsExpressionStatement(ast)) {
        return;
    }
    auto *expr = impl->ExpressionStatementGetExpression(ctx, ast);
    if (expr == nullptr) {
        return;
    }
    expr = UnwrapChainExpr(ctx, expr);
    if (expr == nullptr) {
        return;
    }

    if (impl->IsCallExpression(expr)) {
        RecordMiscCall(ctx, expr);
        return;
    }

    if (impl->IsMemberExpression(expr)) {
        RecordMiscPropAccess(ctx, expr);
    }
}

static void FindMiscCommentsFunction(es2panda_AstNode *ast, void *context)
{
    auto ctx = reinterpret_cast<es2panda_Context *>(context);
    if (!impl->IsFunctionDeclaration(ast)) {
        return;
    }
    auto *func = impl->FunctionDeclarationFunction(ctx, ast);
    if (func == nullptr) {
        return;
    }
    auto *id = impl->ScriptFunctionId(ctx, func);
    if (id == nullptr) {
        return;
    }
    auto name = std::string(impl->IdentifierName(ctx, id));
    if (name != "testMiscComments" && name != "testEofPropAccess") {
        return;
    }
    impl->AstNodeForEach(ast, FindMiscCommentNode, context);
}

static void FindChainClassCalls(es2panda_AstNode *ast, void *context)
{
    auto ctx = reinterpret_cast<es2panda_Context *>(context);
    if (!impl->IsClassDeclaration(ast) || impl->IsETSStructDeclaration(ast)) {
        return;
    }
    auto *def = impl->ClassDeclarationDefinition(ctx, ast);
    if (def == nullptr) {
        return;
    }
    auto *ident = impl->ClassDefinitionIdent(ctx, def);
    if (ident == nullptr || std::string(impl->IdentifierName(ctx, ident)) != "ChainClass") {
        return;
    }
    impl->AstNodeForEach(ast, FindChainCallExpression, context);
}

static void FindTargetAstAfterParser(es2panda_Context *context, es2panda_AstNode *ast)
{
    impl->AstNodeForEach(ast, FindImportExportSpecifier, context);
    impl->AstNodeForEach(ast, FindStructDecl, context);
    // Capture at PARSED: before plugins-after-parse (struct UI rewrite) and before OptionalLowering.
    impl->AstNodeForEach(ast, FindChainClassCalls, context);
    impl->AstNodeForEach(ast, FindStructChainClass, context);
    impl->AstNodeForEach(ast, FindMiscCommentsFunction, context);
}

static void FindTargetAstAfterChecker(es2panda_Context *context, es2panda_AstNode *ast)
{
    impl->AstNodeForEach(ast, FindClass, context);
    impl->AstNodeForEach(ast, FindInterface, context);
    impl->AstNodeForEach(ast, FindMethodDef, context);
    impl->AstNodeForEach(ast, FindClassElement, context);
    impl->AstNodeForEach(ast, FindAnnotationDecl, context);
    impl->AstNodeForEach(ast, FindETSParamDecl, context);
    impl->AstNodeForEach(ast, FindTypeAliasDecl, context);
}

static void TestCommentTypes(es2panda_Context *context)
{
    std::cout << "# Section 1: Comment types" << std::endl;

    PrintCommentsAndJsdoc(context, LookupMethod("onlyJsdocFunc"));
    PrintCommentsAndJsdoc(context, LookupMethod("onlyBlockFunc"));
    PrintCommentsAndJsdoc(context, LookupMethod("onlyLineFunc"));
    PrintCommentsAndJsdoc(context, LookupMethod("multiLineFunc"));
    PrintCommentsAndJsdoc(context, LookupMethod("mixedLineJsdocFunc"));
    PrintCommentsAndJsdoc(context, LookupMethod("mixedBlockJsdocFunc"));
    PrintCommentsAndJsdoc(context, LookupMethod("multiJsdocFunc"));
}

static void TestNodeTypes(es2panda_Context *context)
{
    std::cout << "# Section 2: Node types" << std::endl;

    PrintCommentsOnly(context, classMap["CommentTestClass"]);
    PrintCommentsOnly(context, methodMap["classMethod"]);
    PrintCommentsOnly(context, propertyMap["classProp"]);
    PrintCommentsOnly(context, methodMap["testGet"]);
    PrintCommentsOnly(context, methodMap["testSet"]);
    PrintCommentsOnly(context, interfaceMap["CommentTestInterface"]);
    PrintCommentsOnly(context, methodMap["interfaceMethod"]);
    PrintCommentsOnly(context, methodMap["interfaceProp"] != nullptr ? methodMap["interfaceProp"]
                                                                     : propertyMap["interfaceProp"]);
    PrintCommentsOnly(context, annotationMap["CommentTestAnno"]);
    PrintCommentsOnly(context, typeAlias);
    PrintCommentsOnly(context, propertyMap["commentVar"]);
    PrintCommentsOnly(context, LookupMethod("paramFunc"));
    PrintCommentsOnly(context, paramMap["p1"]);
    PrintCommentsOnly(context, classMap["CommentEnum"]);
    PrintCommentsOnly(context, propertyMap["MEMBER"]);
    PrintCommentsOnly(context, structDecl);
    PrintCommentsOnly(context, propertyMap["structProp"]);
    PrintCommentsOnly(context, importDecl);
    PrintCommentsOnly(context, exportNamedDecl);
    PrintCommentsOnly(context, reExportedDecl);
}

static void TestEdgeCases(es2panda_Context *context)
{
    std::cout << "# Section 3: Edge cases" << std::endl;

    PrintCommentsAndJsdoc(context, LookupMethod("noCommentFunc"));
    PrintCommentsAndJsdoc(context, propertyMap["withAnnoVar"]);
    PrintCommentsAndJsdoc(context, LookupMethod("withModifierFunc"));
    PrintCommentsAndJsdoc(context, LookupMethod("adjacentFunc"));
    PrintCommentsAndJsdoc(context, LookupMethod("tool"));
    PrintCommentsAndJsdoc(context, LookupMethod("tool2"));
}

// Note: `//` inside a string literal is not a comment; `//` inside a line comment does not
// start a new comment; a trailing `*/` inside a line comment does not close a block comment.
static void TestCommentLikeContentInStrings(es2panda_Context *context)
{
    std::cout << "# Section 8: Comment-like content in string literals and comments" << std::endl;

    PrintCommentsAndJsdoc(context, LookupMethod("target"));
    PrintCommentsAndJsdoc(context, LookupMethod("targetWithRealComment"));
    PrintCommentsAndJsdoc(context, LookupMethod("targetWithUrlComment"));
    PrintCommentsAndJsdoc(context, LookupMethod("targetWithStarClose"));
    PrintCommentsAndJsdoc(context, LookupMethod("targetWithFakeStart"));
    PrintCommentsAndJsdoc(context, LookupMethod("targetAfterFakeStartStr"));
    PrintCommentsAndJsdoc(context, LookupMethod("targetAfterFakeEndStr"));
    PrintCommentsAndJsdoc(context, LookupMethod("targetAfterNestedComment"));
}

static void TestChainMethodComments([[maybe_unused]] es2panda_Context *context)
{
    std::cout << "# Section 4: Chain method comments" << std::endl;

    std::cout << callCommentsMap["chainMethod2"] << std::endl;
    std::cout << callCommentsMap["chainMethod3"] << std::endl;
    std::cout << callCommentsMap["chainMethod1OnArg"] << std::endl;

    std::cout << "# Section 5: Optional chain method comments" << std::endl;

    std::cout << callCommentsMap["optionalChainMethod2"] << std::endl;
    std::cout << callCommentsMap["optionalChainMethod3"] << std::endl;
    std::cout << callCommentsMap["optionalChainMethod1OnArg"] << std::endl;

    std::cout << "# Section 6: Struct chain comments at PARSED (before plugins-after-parse)" << std::endl;

    std::cout << callCommentsMap["structChainMethod1"] << std::endl;
    std::cout << callCommentsMap["structChainMethod2"] << std::endl;
    std::cout << callCommentsMap["structChainMethod3"] << std::endl;
    std::cout << callCommentsMap["structChainMethod1OnArg"] << std::endl;

    std::cout << "# Section 7: Misc declaration / call / property comments" << std::endl;

    std::cout << callCommentsMap["miscConstVar"] << std::endl;
    std::cout << callCommentsMap["miscLetVar"] << std::endl;
    std::cout << callCommentsMap["miscSingleMethodCall"] << std::endl;
    std::cout << callCommentsMap["miscOptionalMethodCall"] << std::endl;
    std::cout << callCommentsMap["miscMultiMethodCall"] << std::endl;
    std::cout << callCommentsMap["miscPropAccess"] << std::endl;
    std::cout << callCommentsMap["miscOptionalPropAccess"] << std::endl;
    std::cout << callCommentsMap["miscFunctionCall"] << std::endl;
    std::cout << callCommentsMap["eofPropAccess"] << std::endl;
}

int main(int argc, char **argv)
{
    if (argc < MIN_ARGC) {
        return INVALID_ARGC_ERROR_CODE;
    }

    impl = GetImpl();
    if (impl == nullptr) {
        return NULLPTR_IMPL_ERROR_CODE;
    }

    const char **args = const_cast<const char **>(&(argv[1]));
    auto config = impl->CreateConfig(argc - 1, args);
    auto context = impl->CreateContextFromString(config, g_source.data(), argv[argc - 1]);
    impl->ProceedToState(context, ES2PANDA_STATE_PARSED);
    auto *program = impl->ContextProgram(context);
    auto *entryAst = impl->ProgramAst(context, program);
    FindTargetAstAfterParser(context, entryAst);
    impl->ProceedToState(context, ES2PANDA_STATE_CHECKED);
    FindTargetAstAfterChecker(context, entryAst);

    TestCommentTypes(context);
    TestNodeTypes(context);
    TestEdgeCases(context);
    TestCommentLikeContentInStrings(context);
    TestChainMethodComments(context);
    TestNullNodeComments(context);
    return 0;
}

// NOLINTEND
