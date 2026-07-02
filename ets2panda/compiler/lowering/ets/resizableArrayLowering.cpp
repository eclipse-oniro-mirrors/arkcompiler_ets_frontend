/**
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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

#include "resizableArrayLowering.h"
#include "compiler/lowering/util.h"
#include "ir/ets/etsTypeReference.h"
#include "ir/ets/etsTypeReferencePart.h"
#include "ir/ts/tsTypeParameterInstantiation.h"

namespace ark::es2panda::compiler {

using AstNodePtr = ir::AstNode *;

static void SetElementTypeOriginalNode(ir::TypeNode *typeAnnotation, ir::TypeNode *elementType,
                                       ArenaAllocator *allocator)
{
    if (typeAnnotation == nullptr || elementType == nullptr || allocator == nullptr ||
        !typeAnnotation->IsETSTypeReference()) {
        return;
    }

    auto *part = typeAnnotation->AsETSTypeReference()->Part();
    if (part == nullptr || part->TypeParams() == nullptr || part->TypeParams()->Params().size() != 1) {
        return;
    }

    part->TypeParams()->Params()[0]->SetOriginalNode(elementType->Clone(allocator, nullptr)->AsTypeNode());
}

static ir::AstNode *ConvertToResizableArrayType(ir::TSArrayType *node, public_lib::Context *ctx, bool insideAnnotdecl)
{
    std::stringstream typeAnnotationSrc;
    typeAnnotationSrc << (insideAnnotdecl ? "FixedArray" : (node->IsReadonlyType() ? "ReadonlyArray" : "Array")) << "<"
                      << node->ElementType()->DumpEtsSrc() << ">";

    auto *parser = ctx->parser->AsETSParser();
    ir::TypeNode *typeAnnotation = parser->CreateFormattedTypeAnnotation(typeAnnotationSrc.str());
    ES2PANDA_ASSERT(typeAnnotation != nullptr);
    if (node->HasAnnotations()) {
        typeAnnotation->SetAnnotations(node->Annotations());
    }
    typeAnnotation->SetParent(node->Parent());
    typeAnnotation->SetRange(node->Range());
    typeAnnotation->SetOriginalNode(node);
    SetElementTypeOriginalNode(typeAnnotation, const_cast<ir::TypeNode *>(node->ElementType()), ctx->Allocator());
    RefineSourceRanges(node);
    auto modifier = node->Modifiers();
    if (node->IsReadonlyType()) {
        modifier &= ~ir::ModifierFlags::READONLY_PARAMETER;
    }
    typeAnnotation->AddModifier(modifier);
    return typeAnnotation;
}

bool ResizableArrayConvert::PerformForProgram(parser::Program *program)
{
    bool insideAnnotdecl = false;
    program->Ast()->PreTransformChildrenRecursively(
        [&insideAnnotdecl, ctx = Context()](ir::AstNode *node) -> AstNodePtr {
            if (node->IsAnnotationDeclaration()) {
                ES2PANDA_ASSERT(!insideAnnotdecl);
                insideAnnotdecl = true;
            }
            if (node->IsTSArrayType()) {
                return ConvertToResizableArrayType(node->AsTSArrayType(), ctx, insideAnnotdecl);
            }
            return node;
        },
        [&insideAnnotdecl](ir::AstNode *node) {
            if (node->IsAnnotationDeclaration()) {
                ES2PANDA_ASSERT(insideAnnotdecl);
                insideAnnotdecl = false;
            }
        },
        Name());

    return true;
}
}  // namespace ark::es2panda::compiler
