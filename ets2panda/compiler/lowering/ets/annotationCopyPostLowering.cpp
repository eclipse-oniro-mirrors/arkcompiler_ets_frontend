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

#include "annotationCopyPostLowering.h"

#include "compiler/lowering/util.h"

namespace ark::es2panda::compiler {

std::string_view AnnotationCopyPostLowering::Name() const
{
    return "AnnotationCopyPostLowering";
}

void DoCopyAnnotationProperties(public_lib::Context *ctx, ir::AnnotationUsage *st)
{
    if (st->Properties().size() == 1 &&
        st->Properties().front()->AsClassProperty()->Id()->Name() == compiler::Signatures::ANNOTATION_KEY_VALUE) {
        return;
    }

    ES2PANDA_ASSERT(st->GetBaseName()->Variable() != nullptr);
    auto *annoDecl = st->GetBaseName()->Variable()->Declaration()->Node()->AsAnnotationDeclaration();

    auto propertyExist = [&st](util::StringView name) {
        return std::any_of(st->Properties().begin(), st->Properties().end(),
                           [name](ir::AstNode *property) { return property->AsClassProperty()->Id()->Name() == name; });
    };

    for (auto *it : annoDecl->Properties()) {
        auto *field = it->AsClassProperty();
        if (propertyExist(field->Id()->Name())) {
            continue;
        }
        auto *clone = field->Clone(ctx->Allocator(), st);
        st->AddProperty(clone);
    }
}

bool AnnotationCopyPostLowering::PerformForModule([[maybe_unused]] public_lib::Context *const ctx,
                                                  parser::Program *const program)
{
    program->Ast()->TransformChildrenRecursively(
        // CC-OFFNXT(G.FMT.14-CPP) project code style
        [ctx](ir::AstNode *ast) {
            if (ast->IsAnnotationUsage()) {
                DoCopyAnnotationProperties(ctx, ast->AsAnnotationUsage());
            }

            return ast;
        },
        Name());

    return true;
}

}  // namespace ark::es2panda::compiler
