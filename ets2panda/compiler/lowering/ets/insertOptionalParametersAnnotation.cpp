/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

#include "insertOptionalParametersAnnotation.h"
#include "compiler/lowering/util.h"

namespace ark::es2panda::compiler {

using UAlloc = util::NodeAllocator;

static ir::Identifier *GenAnnoQualifiedLeft(ArenaAllocator *allocator)
{
    return UAlloc::ForceSetParent<ir::Identifier>(allocator, Signatures::DEFAULT_ANNO_QUALIFIED_LEFT, allocator);
}

static ir::Identifier *GenDefaultAnnoId(ArenaAllocator *allocator)
{
    return UAlloc::ForceSetParent<ir::Identifier>(allocator, Signatures::DEFAULT_ANNO_FOR_FUNC, allocator);
}

static ir::Identifier *GenDefaultPropId(ArenaAllocator *allocator, util::StringView const name)
{
    return UAlloc::ForceSetParent<ir::Identifier>(allocator, name, allocator);
}

static ir::ETSTypeReference *GenDefaultAnnoNameRef(ArenaAllocator *allocator)
{
    auto defaultQualifiedAnnoName = UAlloc::ForceSetParent<ir::TSQualifiedName>(
        allocator, GenAnnoQualifiedLeft(allocator), GenDefaultAnnoId(allocator), allocator);
    auto defaultRefPart = UAlloc::ForceSetParent<ir::ETSTypeReferencePart>(allocator, defaultQualifiedAnnoName, nullptr,
                                                                           nullptr, allocator);

    auto defaultRef = UAlloc::ForceSetParent<ir::ETSTypeReference>(allocator, defaultRefPart, allocator);
    return defaultRef;
}

static ir::AstNode *GenMinArgCountItem(ArenaAllocator *allocator, const int32_t requiredArgs)
{
    auto *minArgsCountId = GenDefaultPropId(allocator, Signatures::MIN_ARGSCOUNT_OF_FUNC);
    auto *minArgsCountValue = UAlloc::ForceSetParent<ir::NumberLiteral>(allocator, lexer::Number(requiredArgs));
    auto *minArgsCount = UAlloc::ForceSetParent<ir::ClassProperty>(
        allocator, minArgsCountId, minArgsCountValue, nullptr, ir::ModifierFlags::ANNOTATION_USAGE, allocator, false);
    return minArgsCount;
}

static ir::AstNode *CreateDefaultAnnotationUsageForFunction(public_lib::Context *ctx, const ir::ScriptFunction *func)
{
    auto allocator = ctx->allocator;
    // Note: for function that contain optional param or rest param, need to create a default annotation usage for them
    // the annotation usage in the same format: `@std.functions.OptionalParametersAnnotation({minArgCount: ??})`
    int32_t requiredArgsCnt = 0;
    bool needInsert = false;
    for (auto const param : func->Params()) {
        if (!param->IsETSParameterExpression()) {
            ES2PANDA_ASSERT(param->IsIdentifier() && param->AsIdentifier()->IsErrorPlaceHolder());
            return nullptr;
        }

        if (param->AsETSParameterExpression()->IsOptional() || param->AsETSParameterExpression()->IsRestParameter()) {
            needInsert = param->AsETSParameterExpression()->IsOptional();
            break;
        }

        ++requiredArgsCnt;
    }
    if (!needInsert) {
        // Note: no optional param, no need to generate and insert OptionalParametersAnnotation.
        return nullptr;
    }

    ir::ETSTypeReference *defaultRef = GenDefaultAnnoNameRef(allocator);
    auto *minArgsCount = GenMinArgCountItem(allocator, requiredArgsCnt);
    ArenaVector<ir::AstNode *> properties(allocator->Adapter());
    properties.emplace_back(minArgsCount);
    return util::NodeAllocator::ForceSetParent<ir::AnnotationUsage>(allocator, defaultRef, std::move(properties));
}

static void TryInsertDefaultAnnotation(public_lib::Context *ctx, ir::AstNode *node)
{
    if (!node->IsMethodDefinition()) {
        return;
    }

    auto methodDef = node->AsMethodDefinition();
    if (!methodDef->IsConstructor() && !methodDef->IsAbstract() && methodDef->Function() != nullptr) {
        auto methodFunc = methodDef->Function();
        auto defaultAnno = CreateDefaultAnnotationUsageForFunction(ctx, methodFunc);
        if (defaultAnno != nullptr) {
            methodFunc->EmplaceAnnotation(defaultAnno->AsAnnotationUsage());
            defaultAnno->SetParent(methodFunc);
            RefineSourceRanges(defaultAnno);
        }
    }
}

bool InsertOptionalParametersAnnotation::PerformForModule(public_lib::Context *ctx, parser::Program *program)
{
    if (program->Extension() != ScriptExtension::ETS) {
        return true;
    }

    if (program->GetFlag(parser::ProgramFlags::AST_HAS_OPTIONAL_PARAMETER_ANNOTATION)) {
        return true;
    }

    program->Ast()->IterateRecursivelyPostorder(
        [ctx](ir::AstNode *node) -> void { TryInsertDefaultAnnotation(ctx, node); });

    program->SetFlag(parser::ProgramFlags::AST_HAS_OPTIONAL_PARAMETER_ANNOTATION);

    return true;
}
}  // namespace ark::es2panda::compiler
