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
#include <string>
#include <vector>
#include <utility>

#include "libarkbase/os/library_loader.h"

#include "public/es2panda_lib.h"
#include "util.h"

// NOLINTBEGIN
static es2panda_Impl *impl = nullptr;

static std::string source = R"(
// Annotation declaration:
@interface Component {}
// Annotation use:
@Component()
class A {}
)";

static es2panda_AstNode *classDef = nullptr;
static es2panda_Context *ctx = nullptr;

void FindClassDef(es2panda_AstNode *ast)
{
    if (!impl->IsClassDefinition(ast) ||
        std::string("A") != impl->IdentifierName(ctx, impl->ClassDefinitionIdent(ctx, ast))) {
        impl->AstNodeIterateConst(ctx, ast, FindClassDef);
        return;
    }
    classDef = ast;
}

bool TestClassAnnotation(es2panda_Context *context, es2panda_AstNode *ast)
{
    impl->AstNodeIterateConst(context, ast, FindClassDef);
    if (classDef == nullptr) {
        std::cerr << "CLASS DEF NOT FOUND" << std::endl;
        return false;
    }

    size_t n;
    auto **annotations = impl->ClassDefinitionAnnotations(context, classDef, &n);
    if (annotations == nullptr || n != 1) {
        std::cerr << "ANNOTATION NOT FOUND" << n << std::endl;
        return false;
    }

    impl->ClassDefinitionSetAnnotations(context, classDef, nullptr, 0);

    return true;
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
    auto config = impl->CreateConfig(argc - 1, argv + 1);
    auto context = impl->CreateContextFromString(config, source.data(), argv[argc - 1]);
    ctx = context;
    if (context == nullptr) {
        std::cerr << "FAILED TO CREATE CONTEXT" << std::endl;
        return NULLPTR_CONTEXT_ERROR_CODE;
    }

    impl->ProceedToState(context, ES2PANDA_STATE_PARSED);

    auto *ast = impl->ProgramAst(context, impl->ContextProgram(context));
    if (!TestClassAnnotation(context, ast)) {
        return TEST_ERROR_CODE;
    }

    impl->ProceedToState(context, ES2PANDA_STATE_CHECKED);
    if (impl->ContextState(context) == ES2PANDA_STATE_ERROR) {
        return PROCEED_ERROR_CODE;
    }

    impl->AstNodeRecheck(context, ast);

    impl->ProceedToState(context, ES2PANDA_STATE_BIN_GENERATED);
    if (impl->ContextState(context) == ES2PANDA_STATE_ERROR) {
        return PROCEED_ERROR_CODE;
    }
    impl->DestroyConfig(config);

    return 0;
}

// NOLINTEND
