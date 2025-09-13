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

#include <string>
#include <vector>
#include <utility>

#include "libarkbase/os/library_loader.h"

#include "public/es2panda_lib.h"
#include "util.h"

// NOLINTBEGIN

static es2panda_Impl *impl = nullptr;

static constexpr size_t START_INDEX = 101;
static constexpr size_t START_LINE = 4;
static constexpr size_t END_INDEX = 110;
static constexpr size_t END_LINE = 4;

static std::string source = R"(
@interface Component {}

// expected annotationUsage location: start: 'C', end: 't' of @Component.
@Component
class A {
   name: string = "Tom";
}
)";

static std::vector<es2panda_AstNode *> classDeclarations;

static void CollectDeclarations(es2panda_AstNode *node)
{
    if (impl->IsClassDeclaration(node)) {
        classDeclarations.push_back(node);
    }
}

static es2panda_AstNode *GetAnnotationUsage(es2panda_Context *context, es2panda_AstNode *classDecl)
{
    size_t n;
    auto *classDef = impl->ClassDeclarationDefinition(context, classDecl);
    auto **annotations = impl->ClassDefinitionAnnotations(context, classDef, &n);
    if (n != 1) {
        return nullptr;
    }
    return impl->AnnotationUsageIrExpr(context, annotations[0]);
}

static bool ValidateAnnotationSourceRange(es2panda_Context *context)
{
    auto *classDel = classDeclarations.front();
    auto *annotationUsage = GetAnnotationUsage(context, classDel);
    auto range = impl->AstNodeRangeConst(context, annotationUsage);
    auto start = impl->SourceRangeStart(context, const_cast<es2panda_SourceRange *>(range));
    auto end = impl->SourceRangeEnd(context, const_cast<es2panda_SourceRange *>(range));
    if (start == nullptr || end == nullptr) {
        return false;
    }
    if (START_INDEX != impl->SourcePositionIndex(context, start) ||
        START_LINE != impl->SourcePositionLine(context, start) ||
        END_INDEX != impl->SourcePositionIndex(context, end) || END_LINE != impl->SourcePositionLine(context, end)) {
        return false;
    }
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
    if (context == nullptr) {
        std::cerr << "FAILED TO CREATE CONTEXT" << std::endl;
        return NULLPTR_CONTEXT_ERROR_CODE;
    }

    impl->ProceedToState(context, ES2PANDA_STATE_PARSED);

    auto *ast = impl->ProgramAst(context, impl->ContextProgram(context));
    impl->AstNodeIterateConst(context, ast, CollectDeclarations);

    if (!ValidateAnnotationSourceRange(context)) {
        impl->DestroyContext(context);
        impl->DestroyConfig(config);
        return PROCEED_ERROR_CODE;
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

    impl->DestroyContext(context);
    impl->DestroyConfig(config);

    return 0;
}

// NOLINTEND