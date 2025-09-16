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

#include "annotationCopyLowering.h"

#include "compiler/lowering/util.h"

namespace ark::es2panda::compiler {

std::string_view AnnotationCopyLowering::Name() const
{
    return "AnnotationCopyLowering";
}

void CopyAnnotationProperties(public_lib::Context *ctx, ir::AnnotationUsage *st)
{
    if (st->GetBaseName()->Variable() == nullptr ||
        !st->GetBaseName()->Variable()->Declaration()->Node()->IsAnnotationDeclaration()) {
        // Will be handled in Checker
        return;
    }

    auto *annoDecl = st->GetBaseName()->Variable()->Declaration()->Node()->AsAnnotationDeclaration();

    if (annoDecl->Properties().size() < st->Properties().size()) {
        // Will be handled in Checker
        return;
    }

    if (st->Properties().size() == 1 &&
        st->Properties().front()->AsClassProperty()->Id()->Name() == compiler::Signatures::ANNOTATION_KEY_VALUE) {
        auto *param = st->Properties().front()->AsClassProperty();
        auto singleField = annoDecl->Properties().front()->AsClassProperty();
        // annotationDecl must have a name and type annotation; otherwise, it means it is a broken node.
        if (singleField->Key() == nullptr || singleField->TypeAnnotation() == nullptr) {
            ES2PANDA_ASSERT(ctx->GetChecker()->AsETSChecker()->IsAnyError());
            return;
        }
        if (singleField->Key()->IsBrokenExpression() ||
            (singleField->Value() != nullptr && singleField->Value()->IsBrokenExpression())) {
            ES2PANDA_ASSERT(ctx->GetChecker()->AsETSChecker()->IsAnyError());
            return;
        }
        auto clone = singleField->TypeAnnotation()->Clone(ctx->Allocator(), param);
        param->SetTypeAnnotation(clone);
        return;
    }

    auto findProperty = [&props = annoDecl->Properties()](util::StringView name) {
        auto it = std::find_if(props.begin(), props.end(),
                               [&name](ir::AstNode *node) { return node->AsClassProperty()->Id()->Name() == name; });
        return it == props.end() ? nullptr : (*it)->AsClassProperty();
    };

    for (auto *it : st->Properties()) {
        auto *param = it->AsClassProperty();
        auto *property = findProperty(param->Id()->Name());
        if (property == nullptr || property->TypeAnnotation() == nullptr) {
            // Will be handled in Checker
            continue;
        }

        auto *clone = property->TypeAnnotation()->Clone(ctx->Allocator(), param);
        param->SetTypeAnnotation(clone);
    }
}

bool AnnotationCopyLowering::PerformForModule([[maybe_unused]] public_lib::Context *const ctx,
                                              parser::Program *const program)
{
    program->Ast()->TransformChildrenRecursively(
        // CC-OFFNXT(G.FMT.14-CPP) project code style
        [ctx](ir::AstNode *ast) {
            if (ast->IsAnnotationUsage()) {
                CopyAnnotationProperties(ctx, ast->AsAnnotationUsage());
            }

            return ast;
        },
        Name());

    return true;
}

}  // namespace ark::es2panda::compiler
