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

#include <cstddef>
#include <iostream>
#include <ostream>
#include <string>

#include "os/library_loader.h"

#include "public/es2panda_lib.h"
#include "util.h"

// NOLINTBEGIN

static es2panda_Impl *impl = nullptr;

// Note: namespace declaration and enum declaration will be lowered to class definition before checker.
static std::map<std::string, es2panda_AstNode *> classMap = {{"A", nullptr},
                                                             {"B", nullptr},
                                                             {"JsDocClass", nullptr},
                                                             {"JsDocClassOutside", nullptr},
                                                             {"JsDocClassDefault", nullptr},
                                                             {"JsdocNS", nullptr},
                                                             {"JsdocInnerNS", nullptr},
                                                             {"EM1", nullptr}};

static std::map<std::string, es2panda_AstNode *> interfaceMap = {{"JsdocInterface", nullptr},
                                                                 {"JsdocInterfaceOutside", nullptr}};

// Note: the interface property will be lowered to "getter" and "setter" before check, they will be found in methodMap.
static std::map<std::string, es2panda_AstNode *> methodMap = {
    {"interfaceFoo1", nullptr},  {"interfaceFoo2", nullptr},  {"interfaceFoo3", nullptr},  {"interfaceFoo4", nullptr},
    {"classFoo1", nullptr},      {"classFoo2", nullptr},      {"classFoo3", nullptr},      {"interfaceProp1", nullptr},
    {"interfaceProp2", nullptr}, {"interfaceProp3", nullptr}, {"interfaceProp4", nullptr}, {"jsDocFunc", nullptr},
    {"intefaceGet", nullptr},    {"intefaceSet", nullptr},    {"testGet", nullptr},        {"testSet", nullptr},
    {"tool", nullptr},           {"tool2", nullptr}};

// Note: the variableDecl witll be transferred to property of ETSGLOBAL after lowerings.
static std::map<std::string, es2panda_AstNode *> propertyMap = {
    {"classProp1", nullptr}, {"classProp2", nullptr}, {"classProp3", nullptr}, {"jsdocVal1", nullptr},
    {"jsdocVal2", nullptr},  {"em1Prop1", nullptr},   {"em1Prop2", nullptr}};

static std::map<std::string, es2panda_AstNode *> annotationMap = {
    {"myAnno", nullptr}, {"myAnnoWithAnno", nullptr}, {"exportAnno", nullptr}};

static std::map<std::string, es2panda_AstNode *> etsParamsMap = {{"fooP1", nullptr}, {"fooP2", nullptr}};
static es2panda_AstNode *typeAlias = nullptr;
static es2panda_AstNode *indexerClass = nullptr;
static std::map<std::string, es2panda_AstNode *> indexerTransferredAccessor = {{"$_get", nullptr}, {"$_set", nullptr}};
static es2panda_AstNode *exportNamedDecl = nullptr;
static es2panda_AstNode *exportSingleNamedDecl = nullptr;
static es2panda_AstNode *reExportedDecl = nullptr;
static es2panda_AstNode *importDecl = nullptr;
static es2panda_AstNode *overloadDecl = nullptr;
static es2panda_AstNode *structDecl = nullptr;

static std::string g_source = R"('use static'
/*
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

/**
 * ==== import specifier jsdoc ====
 * @param1 {} behindStr
 * @param2 preStr { p }
*/
import { PI, E } from "std/math"

@interface myMultiAnno {}

/**
 * ==== my Annotation jsdoc ====
 * @param1 preStr { p } behindStr
 * @param2 preStr {} behindStr
 */
@interface myAnno {}

/**
 * ==== my Annotation With Annotation jsdoc ====
 * @param1 preStr { p } behindStr
 * @param2 preStr {} behindStr
*/
@Retention({policy: "SOURCE"})
@interface myAnnoWithAnno {}

/**
 * ==== export Annotation jsdoc ====
 * @param1 preStr { p } behindStr
 * @param2 preStr {} behindStr
*/
export declare @interface exportAnno {}

/**
 * ==== export declare class A ====
 * @param1 {} behindStr
 * @param2 preStr { p }
*/
@myAnno
export declare class A {}

/**
 * ==== export abstract class B ====
 * @param1 {} behindStr
 * @param2 preStr { p }
*/
export declare abstract class B {}

declare class IndexerClass {
    /**
    * ==== ambient indexer jsdoc ====
    * @param1 {} behindStr
    * @param2 preStr { p }
    */
    [idx: number] : string
}

/**
 * ==== export enum EM1 ====
 * @param1 {} behindStr
 * @param2 preStr { p }
*/
export declare enum EM1 {
    /**
     * ==== enum prop1 jsdoc ====
     * @param1 {} behindStr
     * @param2 preStr { p }
    */
    em1Prop1 = 1,

    /**
     * ==== enum prop2 jsdoc ====
     * @param1 {} behindStr
     * @param2 preStr { p }
    */
    em1Prop2 = 2
}

/**
* ==== JsdocNS ====
* @param1 {} behindStr
* @param2 preStr { p }
*/
namespace JsdocNS {
  /**
  * ==== JsdocInterface ====
  * @param1 {} behindStr
  * @param2 preStr { p }
  */
  export interface JsdocInterface {
    /**
    * ==== interfaceFoo1 ====
    * @param1 {} behindStr
    * @param2 preStr { p }
    */
    interfaceFoo1():void

    /**
    * ==== interfaceFoo2 ====
    * @param1 {} behindStr
    * @param2 preStr { p }
    */
    interfaceFoo2():void

    /**
    * ==== private interfaceProp1 ====
    * @param1 {} behindStr
    * @param2 preStr { p }
    */
    private interfaceProp1:number

    /**
    * ==== interfaceProp2 ====
    * @param1 {} behindStr
    * @param2 preStr { p }
    */
    interfaceProp2:number | string[]
  }

  /**
  * ==== JsDocClass ====
  * @param1 {} behindStr
  * @param2 preStr { p }
  */
  export class JsDocClass {
    /**
    * ==== classFoo1 ====
    * @param1 {} behindStr
    * @param2 preStr { p }
    */
    classFoo1():void {}

    /**
    * ==== private classProp1 ====
    * @param1 {} behindStr
    * @param2 preStr { p }
    */
    private classProp1:number = 0;

    /**
    * ==== test class getter ====
    * @param1 {} behindStr
    * @param2 preStr { p }
    */
    get testGet():number {return 1.0}

    /**
    * ==== test class setter ====
    * @param1 {} behindStr
    * @param2 preStr { p }
    */
    set testSet(n: number) {}
  }

  /**
  * ==== JsdocInnerNS ====
  * @param1 {} behindStr
  * @param2 preStr { p }
  * @param3 preStr { p } behindStr
  */
  namespace JsdocInnerNS {}
}
/**
 * ==== JsdocInterfaceOutside ====
 * @param1 {} behindStr
 * @param2 preStr { p }
 */
export declare interface JsdocInterfaceOutside {
    /**
    * ==== interfaceFoo3 ====
    * @param1 {} behindStr
    * @param2 preStr { p }
    */
    interfaceFoo3():void

    /**
    * ==== interfaceProp3 ====
    * @param1 {} behindStr
    * @param2 preStr { p }
    */
    interfaceProp3:number

    /**
    * ==== interfaceFoo4 ====
    * @param1 {} behindStr
    * @param2 preStr { p }
    */
    interfaceFoo4():void

    /**
    * ==== interfaceProp4 ====
    * @param1 {} behindStr
    * @param2 preStr { p }
    */
    interfaceProp4:number

    /**
    * ==== interface getter ====
    * @param1 {} behindStr
    * @param2 preStr { p }
    */
    get intefaceGet(): number

    /**
    * ==== interface setter ====
    * @param1 {} behindStr
    * @param2 preStr { p }
    */
    set intefaceSet(n: number)
}

/**
* ==== JsDocClassDefault with multi-anno ====
* @param1 {} behindStr
* @param2 preStr { p }
*/
@myMultiAnno
@myAnno
export default class JsDocClassDefault {}

/**
* ==== JsDocClassOutside jsdoc1 ====
* @param7
*/
/**
* ==== JsDocClassOutside jsdoc2 ====
* @param1 {} behindStr
* @param2 preStr { p }
*/
export class JsDocClassOutside {
    /**
    * ==== classFoo2 ====
    * @param1 {} behindStr
    * @param2 preStr { p }
    */
    classFoo2():void {}

    /**
    * ==== classFoo3 ====
    * @param1 {} behindStr
    * @param2 preStr { p }
    */
    classFoo3():void {}

    /**
    * ==== public classProp2 jsdoc1 ====
    * @param7
    */

    /**
    * ==== public classProp2 jsdoc2 ====
    * @param1 {} behindStr
    * @param2 preStr { p }
    */
    public classProp2:number = 0;

    /**
    * ==== classProp3 ====
    * @param1 {} behindStr
    * @param2 preStr { p }
    */
    classProp3:number = 0;
}

/**
* ==== function decl jsdoc ====
* @param1 {} behindStr
* @param2 preStr { p }
*/
function jsDocFunc(
/**
 * ==== function param p1 jsdoc ====
 * @param1 {} behindStr
 * @param2 preStr { p }
*/ fooP1: number,

/**
 * ==== function param p2 jsdoc ====
 * @param1 {} behindStr
 * @param2 preStr { p }
*/ fooP2: string): void {}

/**
* ==== typeAlas jsdoc ====
* @param1 {} behindStr
* @param2 preStr { p }
*/
type typeAlias = 'somexx3'

/**
 * ==== variable decl ====
 * @param1 {} behindStr
 * @param2 preStr { p }
 */
let jsdocVal1:string = "ssss"

/**
 * ==== variable decl with annotation ====
 * @param1 {} behindStr
 * @param2 preStr { p }
 */
@myAnnoWithAnno
let jsdocVal2:string = "ssss"

/**
 * ==== export specifier jsdoc1 ====
 * @param1 {} behindStr
 * @param2 preStr { p }
*/
export { JsDocClassOutside, jsdocVal1, jsDocFunc }

/**
 * ==== export specifier jsdoc2 ====
 * @param1 {} behindStr
 * @param2 preStr { p }
*/
export { PI, E } from "std/math"

/**
 * ==== export specifier jsdoc3 ====
 * @param1 {} behindStr
 * @param2 preStr { p }
*/
export jsdocVal2

/**
 * ==== function overload declaration jsdoc ====
 * @param1 {} behindStr
 * @param2 preStr { p }
*/
overload zoo {
  jsDocFunc
}

/**
this is jsdoc of tool
*/
export function tool() {}

/*
this isn't jsdoc of tool2
*/
export function tool2() {}

/**
this is jsdoc of struct
*/
@myAnnoWithAnno
export default struct myStruct {}

)";

static void FindStructDecl(es2panda_AstNode *ast, void *context)
{
    [[maybe_unused]] auto ctx = reinterpret_cast<es2panda_Context *>(context);
    if (impl->IsETSStructDeclaration(ast)) {
        structDecl = ast;
    }
}

static void FindOverloadDecl(es2panda_AstNode *ast, void *context)
{
    [[maybe_unused]] auto ctx = reinterpret_cast<es2panda_Context *>(context);
    if (impl->IsOverloadDeclaration(ast)) {
        overloadDecl = ast;
    }
}

static void FindImportExportSpecifier(es2panda_AstNode *ast, void *context)
{
    auto ctx = reinterpret_cast<es2panda_Context *>(context);
    if (impl->IsExportNamedDeclaration(ast)) {
        size_t len = 0;
        impl->ExportNamedDeclarationSpecifiersConst(ctx, ast, &len);
        if (len == 1) {
            exportSingleNamedDecl = ast;
        } else {
            exportNamedDecl = ast;
        }
    }

    if (impl->IsETSReExportDeclaration(ast)) {
        reExportedDecl = ast;
    }

    if (impl->IsETSImportDeclaration(ast)) {
        importDecl = ast;
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

    if (name == "IndexerClass") {
        indexerClass = ast;
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
    if (etsParamsMap.find(name) != etsParamsMap.end()) {
        etsParamsMap[name] = ast;
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
    if (std::string(name) == "typeAlias") {
        typeAlias = ast;
    }
}

// Note: should be called after `FindClass`
static void FindIndexerTransferredGetterSetter(es2panda_AstNode *ast, void *context)
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
    if (indexerTransferredAccessor.find(name) != indexerTransferredAccessor.end()) {
        indexerTransferredAccessor[name] = ast;
    }
}

static void FindTargetAstAfterChecker(es2panda_Context *context, es2panda_AstNode *ast)
{
    impl->AstNodeForEach(ast, FindClass, context);
    impl->AstNodeForEach(ast, FindInterface, context);
    impl->AstNodeForEach(ast, FindMethodDef, context);
    impl->AstNodeForEach(ast, FindClassElement, context);
    impl->AstNodeForEach(ast, FindTypeAliasDecl, context);
    impl->AstNodeForEach(ast, FindAnnotationDecl, context);
    impl->AstNodeForEach(ast, FindETSParamDecl, context);
    impl->AstNodeForEach(indexerClass, FindIndexerTransferredGetterSetter, context);
    impl->AstNodeForEach(ast, FindOverloadDecl, context);
}

static void FindTargetAstAfterParser(es2panda_Context *context, es2panda_AstNode *ast)
{
    impl->AstNodeForEach(ast, FindImportExportSpecifier, context);
    impl->AstNodeForEach(ast, FindStructDecl, context);
}

static void TestJSDoc(es2panda_Context *context, es2panda_AstNode *entryAst)
{
    std::cout << impl->GetLicenseFromRootNode(context, entryAst) << std::endl;

    for (const auto &[name, targetAst] : classMap) {
        std::cout << impl->JsdocStringFromDeclaration(context, targetAst) << std::endl;
    }

    for (const auto &[name, targetAst] : interfaceMap) {
        std::cout << impl->JsdocStringFromDeclaration(context, targetAst) << std::endl;
    }

    for (const auto &[name, targetAst] : methodMap) {
        std::cout << impl->JsdocStringFromDeclaration(context, targetAst) << std::endl;
    }

    for (const auto &[name, targetAst] : indexerTransferredAccessor) {
        std::cout << impl->JsdocStringFromDeclaration(context, targetAst) << std::endl;
    }

    for (const auto &[name, targetAst] : propertyMap) {
        std::cout << impl->JsdocStringFromDeclaration(context, targetAst) << std::endl;
    }

    impl->JsdocStringFromDeclaration(context, typeAlias);

    for (const auto &[name, targetAst] : annotationMap) {
        std::cout << impl->JsdocStringFromDeclaration(context, targetAst) << std::endl;
    }

    for (const auto &[name, targetAst] : etsParamsMap) {
        std::cout << impl->JsdocStringFromDeclaration(context, targetAst) << std::endl;
    }

    std::cout << impl->JsdocStringFromDeclaration(context, exportNamedDecl) << std::endl;
    std::cout << impl->JsdocStringFromDeclaration(context, reExportedDecl) << std::endl;
    std::cout << impl->JsdocStringFromDeclaration(context, exportSingleNamedDecl) << std::endl;
    std::cout << impl->JsdocStringFromDeclaration(context, importDecl) << std::endl;
    std::cout << impl->JsdocStringFromDeclaration(context, overloadDecl) << std::endl;
    std::cout << impl->JsdocStringFromDeclaration(context, structDecl) << std::endl;
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
    TestJSDoc(context, entryAst);
    return 0;
}

// NOLINTEND