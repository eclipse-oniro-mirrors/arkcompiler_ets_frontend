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
class A {}

namespace NS {
  export class A {}
  export namespace MS {
      export class B {}
  }
}

function foo(): A {
  return new A();
}

function goo(): string {
  return "";
}

function zoo(): NS.MS.B {
  return new NS.MS.B();
}
let a: Array<string> = new Array<string>();
let b = new Array<string>();

function forEach<T>(arr: ()=>Array<T>) {}
forEach(() => {return new Array<string>})

let c: (p1: Number, p2: A) => A = (p1:Number, p2: A):A => {return new A()}
let d = c
//let c: <T1, T2>(p1: T1, p2: T2) => void = <T1, T2>(p1: T1, p2: T2): void => {}
//let d: (p1: number, p2: A) => void = c<number, A>
)";

static es2panda_Impl *impl = nullptr;
es2panda_Context *context = nullptr;
es2panda_AstNode *propA = nullptr;
es2panda_AstNode *propB = nullptr;
es2panda_AstNode *funcCall = nullptr;
es2panda_AstNode *lambdaExpression = nullptr;
es2panda_AstNode *scriptFuncOfLambda = nullptr;

static void FindTargetAst(es2panda_AstNode *ast, [[maybe_unused]] void *ctx)
{
    if (!impl->IsClassProperty(ast)) {
        return;
    }

    auto id = impl->ClassElementId(context, ast);
    if (id == nullptr) {
        return;
    }

    auto name = std::string(impl->IdentifierName(context, id));
    if (name == "a") {
        propA = ast;
    }

    if (name == "b") {
        propB = ast;
    }
}

static void FindTargetFunctionCallAst(es2panda_AstNode *ast, [[maybe_unused]] void *ctx)
{
    if (!impl->IsCallExpression(ast)) {
        return;
    }

    auto callee = impl->CallExpressionCallee(context, ast);
    if (callee == nullptr || !impl->IsIdentifier(callee)) {
        return;
    }

    auto name = std::string(impl->IdentifierName(context, callee));
    if (name != "forEach") {
        return;
    }
    size_t len = 0;
    funcCall = ast;
    es2panda_AstNode **arguments = impl->CallExpressionArguments(context, funcCall, &len);
    lambdaExpression = arguments[0];
}

static void FindAndSetTypeAnnotation(es2panda_AstNode *ast, [[maybe_unused]] void *ctx)
{
    impl->AstNodeForEach(ast, FindTargetAst, context);
    impl->AstNodeForEach(ast, FindTargetFunctionCallAst, context);
    ASSERT(propA != nullptr && propB != nullptr);
    auto tsType = impl->TypedTsType(context, propA);
    ASSERT(tsType != nullptr);
    auto typeAnno = impl->CreateOpaqueTypeNode(context, tsType);
    impl->ClassPropertySetTypeAnnotation(context, propB, typeAnno);

    auto typeAnno2 = impl->CreateOpaqueTypeNode(context, tsType);
    scriptFuncOfLambda = impl->ArrowFunctionExpressionFunction(context, lambdaExpression);
    impl->ScriptFunctionSetReturnTypeAnnotation(context, scriptFuncOfLambda, typeAnno2);
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
    impl->ProceedToState(context, ES2PANDA_STATE_CHECKED);
    CheckForErrors("CHECKED", context);

    auto *program = impl->ContextProgram(context);
    es2panda_AstNode *programNode = impl->ProgramAst(context, program);
    FindAndSetTypeAnnotation(programNode, context);

    impl->AstNodeRecheck(context, programNode);
    CheckForErrors("RECHECKED", context);

    if (impl->ContextState(context) == ES2PANDA_STATE_ERROR) {
        return PROCEED_ERROR_CODE;
    }

    impl->DestroyConfig(config);
    return 0;
}

// NOLINTEND
