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
import { PI, E } from "std/math"

export declare @interface exportAnno {}

export declare enum EM1 {
    em1Prop1 = 1,

    em1Prop2 = 2
}

namespace NS {
  export interface innerInterface {

    interfaceFoo1():Array<number | string>

    interfaceFoo2():void

    private interfaceProp1:number

    interfaceProp2:number | string[]
  }

  export class innerClass {
    classFoo1():void {}

    private classProp1:number = 0;
  }

  namespace InnerNS {}
}

export declare interface InterfaceOutside {
    private interfaceFoo3():number | string[]

    interfaceProp3:number

    interfaceFoo4():void

    interfaceProp4:Array<number>
}

export class ClassOutside {
    build(p3: number) {}

    classFoo2():void {}

    classFoo3():void {}

    public classProp2:number = 0;

    classProp3:number = 0;
}

function foo(
  fooP1: number,
  fooP2: string): void {}

let val:string = "ssss"

export { ClassOutside, val, foo }

export { PI, E } from "std/math"

export NS

export declare function fooDecl(
  options: number,
  myop: string
): void

export declare abstract class ABClass<T> {
  @exportAnno
  static $_instantiate(
    factory: () => T,
    initializers?: number,
    reuseId?: string,
    @exportAnno content?: () => void
  ): T
}

export declare interface InterfaceTest {
    @exportAnno
    private interfaceFoo5(
    initializers?: number,
    reuseId?: string,
    ):number | string[]
}
)";

constexpr size_t NS_START_LINE = 11;
constexpr size_t INNERNS_START_LINE = 29;
constexpr size_t EXPORT_NAMED_DECL_START_LINE = 60;
constexpr size_t EXPORT_SINGLE_NAMED_DECL_START_LINE = 64;
constexpr size_t RE_EXPORT_DECL_START_LINE = 62;
constexpr size_t IMPORT_DECL_START_LINE = 1;
constexpr size_t INNER_INTERFACE_START_LINE = 12;
constexpr size_t INTERFACE_OUTSIDE_START_LINE = 32;
constexpr size_t CLASS_OUTSIDE_START_LINE = 42;
constexpr size_t INNER_CLASS_START_LINE = 23;
constexpr size_t CLASS_PROP1_START_LINE = 26;
constexpr size_t CLASS_PROP2_START_LINE = 49;
constexpr size_t CLASS_PROP3_START_LINE = 51;
constexpr size_t CLASS_FOO1_START_LINE = 24;
constexpr size_t CLASS_FOO2_START_LINE = 45;
constexpr size_t CLASS_FOO3_START_LINE = 47;
constexpr size_t CLASS_INSTANTIATE_START_LINE = 73;
constexpr size_t INTERFACE_PROP1_START_LINE = 18;
constexpr size_t INTERFACE_PROP2_START_LINE = 20;
constexpr size_t INTERFACE_PROP3_START_LINE = 35;
constexpr size_t INTERFACE_PROP4_START_LINE = 39;
constexpr size_t INTERFACE_FOO1_START_LINE = 14;
constexpr size_t INTERFACE_FOO2_START_LINE = 16;
constexpr size_t INTERFACE_FOO3_START_LINE = 33;
constexpr size_t INTERFACE_FOO4_START_LINE = 37;
constexpr size_t INTERFACE_FOO5_START_LINE = 83;
constexpr size_t FUNCTION_PARAM1_START_LINE = 55;
constexpr size_t FUNCTION_PARAM2_START_LINE = 56;
constexpr size_t FUNCTION_PARAM3_START_LINE = 43;
constexpr size_t FUNCTION_DECL_START_LINE = 66;

constexpr size_t NS_END_LINE = 30;
constexpr size_t INNERNS_END_LINE = 29;
constexpr size_t EXPORT_NAMED_DECL_END_LINE = 60;
constexpr size_t EXPORT_SINGLE_NAMED_DECL_END_LINE = 64;
constexpr size_t RE_EXPORT_DECL_END_LINE = 62;
constexpr size_t IMPORT_DECL_END_LINE = 1;
constexpr size_t INNER_INTERFACE_END_LINE = 21;
constexpr size_t INTERFACE_OUTSIDE_END_LINE = 40;
constexpr size_t CLASS_OUTSIDE_END_LINE = 54;
constexpr size_t INNER_CLASS_END_LINE = 29;
constexpr size_t CLASS_PROP1_END_LINE = 26;
constexpr size_t CLASS_PROP2_END_LINE = 49;
constexpr size_t CLASS_PROP3_END_LINE = 51;
constexpr size_t CLASS_FOO1_END_LINE = 24;
constexpr size_t CLASS_FOO2_END_LINE = 45;
constexpr size_t CLASS_FOO3_END_LINE = 47;
constexpr size_t CLASS_INSTANTIATE_END_LINE = 78;
constexpr size_t INTERFACE_PROP1_END_LINE = 18;
constexpr size_t INTERFACE_PROP2_END_LINE = 20;
constexpr size_t INTERFACE_PROP3_END_LINE = 35;
constexpr size_t INTERFACE_PROP4_END_LINE = 39;
constexpr size_t INTERFACE_FOO1_END_LINE = 14;
constexpr size_t INTERFACE_FOO2_END_LINE = 16;
constexpr size_t INTERFACE_FOO3_END_LINE = 33;
constexpr size_t INTERFACE_FOO4_END_LINE = 37;
constexpr size_t INTERFACE_FOO5_END_LINE = 86;
constexpr size_t FUNCTION_PARAM1_END_LINE = 55;
constexpr size_t FUNCTION_PARAM2_END_LINE = 56;
constexpr size_t FUNCTION_PARAM3_END_LINE = 43;
constexpr size_t FUNCTION_DECL_END_LINE = 69;

constexpr size_t NS_START_COL = 1;
constexpr size_t INNERNS_START_COL = 3;
constexpr size_t EXPORT_NAMED_DECL_START_COL = 8;
constexpr size_t EXPORT_SINGLE_NAMED_DECL_START_COL = 8;
constexpr size_t RE_EXPORT_DECL_START_COL = 8;
constexpr size_t IMPORT_DECL_START_COL = 1;
constexpr size_t INNER_INTERFACE_START_COL = 10;
constexpr size_t INTERFACE_OUTSIDE_START_COL = 16;
constexpr size_t CLASS_OUTSIDE_START_COL = 8;
constexpr size_t INNER_CLASS_START_COL = 10;
constexpr size_t CLASS_PROP1_START_COL = 13;
constexpr size_t CLASS_PROP2_START_COL = 12;
constexpr size_t CLASS_PROP3_START_COL = 5;
constexpr size_t CLASS_FOO1_START_COL = 5;
constexpr size_t CLASS_FOO2_START_COL = 5;
constexpr size_t CLASS_FOO3_START_COL = 5;
constexpr size_t CLASS_INSTANTIATE_START_COL = 10;
constexpr size_t INTERFACE_PROP1_START_COL = 13;
constexpr size_t INTERFACE_PROP2_START_COL = 5;
constexpr size_t INTERFACE_PROP3_START_COL = 5;
constexpr size_t INTERFACE_PROP4_START_COL = 5;
constexpr size_t INTERFACE_FOO1_START_COL = 5;
constexpr size_t INTERFACE_FOO2_START_COL = 5;
constexpr size_t INTERFACE_FOO3_START_COL = 13;
constexpr size_t INTERFACE_FOO4_START_COL = 5;
constexpr size_t INTERFACE_FOO5_START_COL = 13;
constexpr size_t FUNCTION_PARAM1_START_COL = 3;
constexpr size_t FUNCTION_PARAM2_START_COL = 3;
constexpr size_t FUNCTION_PARAM3_START_COL = 11;
constexpr size_t FUNCTION_DECL_START_COL = 8;

constexpr size_t NS_END_COL = 2;
constexpr size_t INNERNS_END_COL = 23;
constexpr size_t EXPORT_NAMED_DECL_END_COL = 32;
constexpr size_t EXPORT_SINGLE_NAMED_DECL_END_COL = 10;
constexpr size_t RE_EXPORT_DECL_END_COL = 33;
constexpr size_t IMPORT_DECL_END_COL = 33;
constexpr size_t INNER_INTERFACE_END_COL = 4;
constexpr size_t INTERFACE_OUTSIDE_END_COL = 2;
constexpr size_t CLASS_OUTSIDE_END_COL = 9;
constexpr size_t INNER_CLASS_END_COL = 12;
constexpr size_t CLASS_PROP1_END_COL = 34;
constexpr size_t CLASS_PROP2_END_COL = 33;
constexpr size_t CLASS_PROP3_END_COL = 26;
constexpr size_t CLASS_FOO1_END_COL = 24;
constexpr size_t CLASS_FOO2_END_COL = 24;
constexpr size_t CLASS_FOO3_END_COL = 24;
constexpr size_t CLASS_INSTANTIATE_END_COL = 7;
constexpr size_t INTERFACE_PROP1_END_COL = 34;
constexpr size_t INTERFACE_PROP2_END_COL = 37;
constexpr size_t INTERFACE_PROP3_END_COL = 26;
constexpr size_t INTERFACE_PROP4_END_COL = 33;
constexpr size_t INTERFACE_FOO1_END_COL = 43;
constexpr size_t INTERFACE_FOO2_END_COL = 25;
constexpr size_t INTERFACE_FOO3_END_COL = 46;
constexpr size_t INTERFACE_FOO4_END_COL = 25;
constexpr size_t INTERFACE_FOO5_END_COL = 24;
constexpr size_t FUNCTION_PARAM1_END_COL = 16;
constexpr size_t FUNCTION_PARAM2_END_COL = 16;
constexpr size_t FUNCTION_PARAM3_END_COL = 21;
constexpr size_t FUNCTION_DECL_END_COL = 8;

static std::map<std::string, size_t> startLineMap = {{"exportNamedDecl", EXPORT_NAMED_DECL_START_LINE},
                                                     {"exportSingleNamedDecl", EXPORT_SINGLE_NAMED_DECL_START_LINE},
                                                     {"reExportedDecl", RE_EXPORT_DECL_START_LINE},
                                                     {"importDecl", IMPORT_DECL_START_LINE},
                                                     {"NS", NS_START_LINE},
                                                     {"InnerNS", INNERNS_START_LINE},
                                                     {"InterfaceOutside", INTERFACE_OUTSIDE_START_LINE},
                                                     {"innerInterface", INNER_INTERFACE_START_LINE},
                                                     {"ClassOutside", CLASS_OUTSIDE_START_LINE},
                                                     {"innerClass", INNER_CLASS_START_LINE},
                                                     {"classProp1", CLASS_PROP1_START_LINE},
                                                     {"classProp2", CLASS_PROP2_START_LINE},
                                                     {"classProp3", CLASS_PROP3_START_LINE},
                                                     {"interfaceProp1", INTERFACE_PROP1_START_LINE},
                                                     {"interfaceProp2", INTERFACE_PROP2_START_LINE},
                                                     {"interfaceProp3", INTERFACE_PROP3_START_LINE},
                                                     {"interfaceProp4", INTERFACE_PROP4_START_LINE},
                                                     {"classFoo1", CLASS_FOO1_START_LINE},
                                                     {"classFoo2", CLASS_FOO2_START_LINE},
                                                     {"classFoo3", CLASS_FOO3_START_LINE},
                                                     {"interfaceFoo1", INTERFACE_FOO1_START_LINE},
                                                     {"interfaceFoo2", INTERFACE_FOO2_START_LINE},
                                                     {"interfaceFoo3", INTERFACE_FOO3_START_LINE},
                                                     {"interfaceFoo4", INTERFACE_FOO4_START_LINE},
                                                     {"interfaceFoo5", INTERFACE_FOO5_START_LINE},
                                                     {"fooP1", FUNCTION_PARAM1_START_LINE},
                                                     {"fooP2", FUNCTION_PARAM2_START_LINE},
                                                     {"p3", FUNCTION_PARAM3_START_LINE},
                                                     {"fooDecl", FUNCTION_DECL_START_LINE},
                                                     {"$_instantiate", CLASS_INSTANTIATE_START_LINE}};

static std::map<std::string, size_t> startColMap = {{"exportNamedDecl", EXPORT_NAMED_DECL_START_COL},
                                                    {"exportSingleNamedDecl", EXPORT_SINGLE_NAMED_DECL_START_COL},
                                                    {"reExportedDecl", RE_EXPORT_DECL_START_COL},
                                                    {"importDecl", IMPORT_DECL_START_COL},
                                                    {"NS", NS_START_COL},
                                                    {"InnerNS", INNERNS_START_COL},
                                                    {"InterfaceOutside", INTERFACE_OUTSIDE_START_COL},
                                                    {"innerInterface", INNER_INTERFACE_START_COL},
                                                    {"ClassOutside", CLASS_OUTSIDE_START_COL},
                                                    {"innerClass", INNER_CLASS_START_COL},
                                                    {"classProp1", CLASS_PROP1_START_COL},
                                                    {"classProp2", CLASS_PROP2_START_COL},
                                                    {"classProp3", CLASS_PROP3_START_COL},
                                                    {"interfaceProp1", INTERFACE_PROP1_START_COL},
                                                    {"interfaceProp2", INTERFACE_PROP2_START_COL},
                                                    {"interfaceProp3", INTERFACE_PROP3_START_COL},
                                                    {"interfaceProp4", INTERFACE_PROP4_START_COL},
                                                    {"classFoo1", CLASS_FOO1_START_COL},
                                                    {"classFoo2", CLASS_FOO2_START_COL},
                                                    {"classFoo3", CLASS_FOO3_START_COL},
                                                    {"interfaceFoo1", INTERFACE_FOO1_START_COL},
                                                    {"interfaceFoo2", INTERFACE_FOO2_START_COL},
                                                    {"interfaceFoo3", INTERFACE_FOO3_START_COL},
                                                    {"interfaceFoo4", INTERFACE_FOO4_START_COL},
                                                    {"interfaceFoo5", INTERFACE_FOO5_START_COL},
                                                    {"fooP1", FUNCTION_PARAM1_START_COL},
                                                    {"fooP2", FUNCTION_PARAM2_START_COL},
                                                    {"p3", FUNCTION_PARAM3_START_COL},
                                                    {"fooDecl", FUNCTION_DECL_START_COL},
                                                    {"$_instantiate", CLASS_INSTANTIATE_START_COL}};

static std::map<std::string, size_t> endLineMap = {{"exportNamedDecl", EXPORT_NAMED_DECL_END_LINE},
                                                   {"exportSingleNamedDecl", EXPORT_SINGLE_NAMED_DECL_END_LINE},
                                                   {"reExportedDecl", RE_EXPORT_DECL_END_LINE},
                                                   {"importDecl", IMPORT_DECL_END_LINE},
                                                   {"NS", NS_END_LINE},
                                                   {"InnerNS", INNERNS_END_LINE},
                                                   {"InterfaceOutside", INTERFACE_OUTSIDE_END_LINE},
                                                   {"innerInterface", INNER_INTERFACE_END_LINE},
                                                   {"ClassOutside", CLASS_OUTSIDE_END_LINE},
                                                   {"innerClass", INNER_CLASS_END_LINE},
                                                   {"classProp1", CLASS_PROP1_END_LINE},
                                                   {"classProp2", CLASS_PROP2_END_LINE},
                                                   {"classProp3", CLASS_PROP3_END_LINE},
                                                   {"interfaceProp1", INTERFACE_PROP1_END_LINE},
                                                   {"interfaceProp2", INTERFACE_PROP2_END_LINE},
                                                   {"interfaceProp3", INTERFACE_PROP3_END_LINE},
                                                   {"interfaceProp4", INTERFACE_PROP4_END_LINE},
                                                   {"classFoo1", CLASS_FOO1_END_LINE},
                                                   {"classFoo2", CLASS_FOO2_END_LINE},
                                                   {"classFoo3", CLASS_FOO3_END_LINE},
                                                   {"interfaceFoo1", INTERFACE_FOO1_END_LINE},
                                                   {"interfaceFoo2", INTERFACE_FOO2_END_LINE},
                                                   {"interfaceFoo3", INTERFACE_FOO3_END_LINE},
                                                   {"interfaceFoo4", INTERFACE_FOO4_END_LINE},
                                                   {"interfaceFoo5", INTERFACE_FOO5_END_LINE},
                                                   {"fooP1", FUNCTION_PARAM1_END_LINE},
                                                   {"fooP2", FUNCTION_PARAM2_END_LINE},
                                                   {"p3", FUNCTION_PARAM3_END_LINE},
                                                   {"fooDecl", FUNCTION_DECL_END_LINE},
                                                   {"$_instantiate", CLASS_INSTANTIATE_END_LINE}};

static std::map<std::string, size_t> endColMap = {{"exportNamedDecl", EXPORT_NAMED_DECL_END_COL},
                                                  {"exportSingleNamedDecl", EXPORT_SINGLE_NAMED_DECL_END_COL},
                                                  {"reExportedDecl", RE_EXPORT_DECL_END_COL},
                                                  {"importDecl", IMPORT_DECL_END_COL},
                                                  {"NS", NS_END_COL},
                                                  {"InnerNS", INNERNS_END_COL},
                                                  {"InterfaceOutside", INTERFACE_OUTSIDE_END_COL},
                                                  {"innerInterface", INNER_INTERFACE_END_COL},
                                                  {"ClassOutside", CLASS_OUTSIDE_END_COL},
                                                  {"innerClass", INNER_CLASS_END_COL},
                                                  {"classProp1", CLASS_PROP1_END_COL},
                                                  {"classProp2", CLASS_PROP2_END_COL},
                                                  {"classProp3", CLASS_PROP3_END_COL},
                                                  {"interfaceProp1", INTERFACE_PROP1_END_COL},
                                                  {"interfaceProp2", INTERFACE_PROP2_END_COL},
                                                  {"interfaceProp3", INTERFACE_PROP3_END_COL},
                                                  {"interfaceProp4", INTERFACE_PROP4_END_COL},
                                                  {"classFoo1", CLASS_FOO1_END_COL},
                                                  {"classFoo2", CLASS_FOO2_END_COL},
                                                  {"classFoo3", CLASS_FOO3_END_COL},
                                                  {"interfaceFoo1", INTERFACE_FOO1_END_COL},
                                                  {"interfaceFoo2", INTERFACE_FOO2_END_COL},
                                                  {"interfaceFoo3", INTERFACE_FOO3_END_COL},
                                                  {"interfaceFoo4", INTERFACE_FOO4_END_COL},
                                                  {"interfaceFoo5", INTERFACE_FOO5_END_COL},
                                                  {"fooP1", FUNCTION_PARAM1_END_COL},
                                                  {"fooP2", FUNCTION_PARAM2_END_COL},
                                                  {"p3", FUNCTION_PARAM3_END_COL},
                                                  {"fooDecl", FUNCTION_DECL_END_COL},
                                                  {"$_instantiate", CLASS_INSTANTIATE_END_COL}};

static es2panda_Impl *impl = nullptr;
es2panda_Context *context = nullptr;
es2panda_AstNode *fooDecl = nullptr;
static void FindFunctionDecl(es2panda_AstNode *ast, [[maybe_unused]] void *ctx)
{
    if (!impl->IsFunctionDeclaration(ast)) {
        return;
    }
    auto scriptFunc = impl->FunctionDeclarationFunction(context, ast);

    auto *ident = impl->ScriptFunctionId(context, scriptFunc);
    if (ident == nullptr) {
        return;
    }

    auto name = std::string(impl->IdentifierName(context, ident));
    if (name == "fooDecl") {
        fooDecl = ast;
    }
}

static std::map<std::string, es2panda_AstNode *> namespaceDecl = {{"NS", nullptr}, {"InnerNS", nullptr}};
static void FindNamespaceDecl(es2panda_AstNode *ast, [[maybe_unused]] void *ctx)
{
    if (!impl->IsETSModule(ast) || !impl->ETSModuleIsNamespaceConst(context, ast)) {
        return;
    }
    auto *ident = impl->ETSModuleIdent(context, ast);
    if (ident == nullptr) {
        return;
    }
    auto name = std::string(impl->IdentifierName(context, ident));
    if (namespaceDecl.find(name) != namespaceDecl.end()) {
        namespaceDecl[name] = ast;
    }
}

static es2panda_AstNode *exportNamedDecl = nullptr;
static es2panda_AstNode *exportSingleNamedDecl = nullptr;
static es2panda_AstNode *reExportedDecl = nullptr;
static es2panda_AstNode *importDecl = nullptr;
static void FindImportExportSpecifier(es2panda_AstNode *ast, [[maybe_unused]] void *ctx)
{
    if (impl->IsExportNamedDeclaration(ast)) {
        size_t len = 0;
        impl->ExportNamedDeclarationSpecifiersConst(context, ast, &len);
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

static std::map<std::string, es2panda_AstNode *> interfaceMap = {{"innerInterface", nullptr},
                                                                 {"InterfaceOutside", nullptr}};
static void FindInterface(es2panda_AstNode *ast, [[maybe_unused]] void *ctx)
{
    if (!impl->IsTSInterfaceDeclaration(ast)) {
        return;
    }
    auto *ident = impl->TSInterfaceDeclarationId(context, ast);
    if (ident == nullptr) {
        return;
    }
    auto name = std::string(impl->IdentifierName(context, ident));
    if (interfaceMap.find(name) != interfaceMap.end()) {
        interfaceMap[name] = ast;
    }
}

static std::map<std::string, es2panda_AstNode *> classMap = {{"ClassOutside", nullptr}, {"innerClass", nullptr}};
static void FindClass(es2panda_AstNode *ast, [[maybe_unused]] void *ctx)
{
    if (!impl->IsClassDeclaration(ast)) {
        return;
    }
    auto *ident = impl->ClassDefinitionIdent(context, impl->ClassDeclarationDefinition(context, ast));
    if (ident == nullptr) {
        return;
    }
    auto name = std::string(impl->IdentifierName(context, ident));
    if (classMap.find(name) != classMap.end()) {
        classMap[name] = ast;
    }
}

static std::map<std::string, es2panda_AstNode *> propertyMap = {
    {"classProp1", nullptr},     {"classProp2", nullptr},     {"classProp3", nullptr},    {"interfaceProp1", nullptr},
    {"interfaceProp2", nullptr}, {"interfaceProp3", nullptr}, {"interfaceProp4", nullptr}};
static void FindClassElement(es2panda_AstNode *ast, [[maybe_unused]] void *ctx)
{
    if (!impl->IsClassProperty(ast)) {
        return;
    }
    auto *ident = impl->ClassElementId(context, ast);
    if (ident == nullptr) {
        return;
    }

    auto name = std::string(impl->IdentifierName(context, ident));
    if (propertyMap.find(name) != propertyMap.end()) {
        propertyMap[name] = ast;
    }
}

static std::map<std::string, es2panda_AstNode *> enumMemberMap = {{"em1Prop1", nullptr}, {"em1Prop2", nullptr}};
static void FindEnumMember(es2panda_AstNode *ast, [[maybe_unused]] void *ctx)
{
    if (!impl->IsTSEnumMember(ast)) {
        return;
    }
    auto *ident = impl->TSEnumMemberKey(context, ast);
    if (ident == nullptr || impl->IsIdentifier(ident)) {
        return;
    }

    auto name = std::string(impl->IdentifierName(context, ident));
    if (enumMemberMap.find(name) != enumMemberMap.end()) {
        enumMemberMap[name] = ast;
    }
}

static std::map<std::string, es2panda_AstNode *> methodMap = {
    {"interfaceFoo1", nullptr}, {"interfaceFoo2", nullptr}, {"interfaceFoo3", nullptr},
    {"interfaceFoo4", nullptr}, {"interfaceFoo5", nullptr}, {"classFoo1", nullptr},
    {"classFoo2", nullptr},     {"classFoo3", nullptr},     {"$_instantiate", nullptr}};
static void FindMethodDef(es2panda_AstNode *ast, [[maybe_unused]] void *ctx)
{
    if (!impl->IsMethodDefinition(ast)) {
        return;
    }
    auto *function = impl->MethodDefinitionFunction(context, ast);
    if (function == nullptr) {
        return;
    }

    auto *ident = impl->ScriptFunctionId(context, function);
    if (ident == nullptr) {
        return;
    }
    auto name = std::string(impl->IdentifierName(context, ident));
    if (methodMap.find(name) != methodMap.end()) {
        methodMap[name] = ast;
    }
}

static std::map<std::string, es2panda_AstNode *> etsParamsMap = {
    {"fooP1", nullptr}, {"fooP2", nullptr}, {"p3", nullptr}};
static void FindETSParamDecl(es2panda_AstNode *ast, [[maybe_unused]] void *ctx)
{
    if (!impl->IsETSParameterExpression(ast)) {
        return;
    }
    auto *ident = impl->ETSParameterExpressionIdent(context, ast);
    if (ident == nullptr) {
        return;
    }
    auto name = std::string(impl->IdentifierName(context, ident));
    if (etsParamsMap.find(name) != etsParamsMap.end()) {
        etsParamsMap[name] = ast;
    }
}

static void FindTargetAst(es2panda_AstNode *ast, [[maybe_unused]] void *ctx)
{
    impl->AstNodeForEach(ast, FindNamespaceDecl, context);
    impl->AstNodeForEach(ast, FindImportExportSpecifier, context);
    impl->AstNodeForEach(ast, FindInterface, context);
    impl->AstNodeForEach(ast, FindClass, context);
    impl->AstNodeForEach(ast, FindClassElement, context);
    impl->AstNodeForEach(ast, FindMethodDef, context);
    impl->AstNodeForEach(ast, FindETSParamDecl, context);
    impl->AstNodeForEach(ast, FindEnumMember, context);
    impl->AstNodeForEach(ast, FindFunctionDecl, context);
}

static bool CheckLineAndCol(es2panda_AstNode *ast, std::string name)
{
    auto start = impl->AstNodeStartConst(context, ast);
    auto end = impl->AstNodeEndConst(context, ast);
    auto res = startLineMap[name] == impl->SourcePositionLine(context, start);
    ASSERT(startLineMap[name] == impl->SourcePositionLine(context, start));

    res &= startColMap[name] == impl->SourcePositionCol(context, start);
    ASSERT(startColMap[name] == impl->SourcePositionCol(context, start));

    res &= endLineMap[name] == impl->SourcePositionLine(context, end);
    ASSERT(endLineMap[name] == impl->SourcePositionLine(context, end));

    res &= endColMap[name] == impl->SourcePositionCol(context, end);
    ASSERT(endColMap[name] == impl->SourcePositionCol(context, end));
    return res;
}

static bool CheckAllNode()
{
    bool res = CheckLineAndCol(exportNamedDecl, "exportNamedDecl");
    res &= CheckLineAndCol(exportSingleNamedDecl, "exportSingleNamedDecl");
    res &= CheckLineAndCol(reExportedDecl, "reExportedDecl");
    res &= CheckLineAndCol(importDecl, "importDecl");
    res &= CheckLineAndCol(fooDecl, "fooDecl");
    for (const auto &[name, targetAst] : namespaceDecl) {
        res &= CheckLineAndCol(targetAst, name);
    }

    for (const auto &[name, targetAst] : interfaceMap) {
        res &= CheckLineAndCol(targetAst, name);
    }

    for (const auto &[name, targetAst] : classMap) {
        res &= CheckLineAndCol(targetAst, name);
    }

    for (const auto &[name, targetAst] : propertyMap) {
        res &= CheckLineAndCol(targetAst, name);
    }

    for (const auto &[name, targetAst] : methodMap) {
        res &= CheckLineAndCol(targetAst, name);
    }

    for (const auto &[name, targetAst] : etsParamsMap) {
        res &= CheckLineAndCol(targetAst, name);
    }
    return res;
}

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
    auto config = impl->CreateConfig(argc - 1, args);
    context = impl->CreateContextFromString(config, source.data(), argv[argc - 1]);
    if (context == nullptr) {
        return NULLPTR_CONTEXT_ERROR_CODE;
    }
    impl->ProceedToState(context, ES2PANDA_STATE_PARSED);
    CheckForErrors("PARSED", context);

    auto *program = impl->ContextProgram(context);
    es2panda_AstNode *programNode = impl->ProgramAst(context, program);
    FindTargetAst(programNode, context);
    if (!CheckAllNode()) {
        return TEST_ERROR_CODE;
        impl->DestroyConfig(config);
    }
    impl->DestroyConfig(config);
    return 0;
}
// NOLINTEND