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

#include "gradualTypeNarrowing.h"

#include "checker/ETSchecker.h"
#include "checker/types/ets/etsTupleType.h"
#include "checker/types/gradualType.h"
#include "es2panda.h"
#include "ir/astNode.h"
#include "ir/opaqueTypeNode.h"
#include "ir/typed.h"
#include "util/language.h"

namespace ark::es2panda::compiler {

checker::Type *GradualTypeNarrowing::TransformType(checker::Type *type,
                                                   const std::function<checker::Type *(checker::Type *)> &func)
{
    if (type->IsETSFunctionType()) {
        auto funcType = type->AsETSFunctionType();
        for (auto sig : funcType->CallSignaturesOfMethodOrArrow()) {
            sig->SetReturnType(TransformType(sig->ReturnType(), func));
            for (auto var : sig->Params()) {
                var->SetTsType(TransformType(var->TsType(), func));
            }

            if (sig->RestVar() != nullptr) {
                sig->RestVar()->SetTsType(TransformType(sig->RestVar()->TsType(), func));
            }
        }
    }

    if (type->IsETSUnionType()) {
        auto unionType = type->AsETSUnionType();
        ArenaVector<checker::Type *> types {checker_->ProgramAllocator()->Adapter()};
        for (auto ctype : unionType->ConstituentTypes()) {
            types.push_back(TransformType(ctype, func));
        }
        type = checker_->CreateETSUnionType(std::move(types));
    }

    if (type->IsETSArrayType()) {
        auto arrayType = type->AsETSArrayType();
        arrayType->SetElementType(TransformType(arrayType->ElementType(), func));
    }

    if (type->IsETSResizableArrayType()) {
        auto arrayType = type->AsETSResizableArrayType();
        arrayType->SetElementType(TransformType(arrayType->ElementType(), func));
    }

    if (type->IsETSTupleType()) {
        auto tupleType = type->AsETSTupleType();
        ArenaVector<checker::Type *> types {checker_->ProgramAllocator()->Adapter()};
        for (auto ctype : tupleType->GetTupleTypesList()) {
            types.push_back(TransformType(ctype, func));
        }
        type = checker_->ProgramAllocator()->New<checker::ETSTupleType>(checker_, std::move(types));
    }

    return func(type);
}

void GradualTypeNarrowing::NarrowGradualType(ir::AstNode *node)
{
    auto typedNode = node->AsTyped();
    auto typeTransformFunc = [this](checker::Type *type) -> checker::TypePtr {
        if (type->IsGradualType() || (type->IsETSObjectType() && type->AsETSObjectType()->GetDeclNode() != nullptr &&
                                      type->AsETSObjectType()->GetDeclNode()->AsTyped()->TsType() != nullptr &&
                                      type->AsETSObjectType()->GetDeclNode()->AsTyped()->TsType()->IsGradualType())) {
            return this->checker_->GlobalETSRelaxedAnyType();
        }
        return type;
    };

    if (typedNode->TsType() != nullptr) {
        typedNode->SetTsType(TransformType(typedNode->TsType(), typeTransformFunc));
    }
    if (typedNode->IsBinaryExpression()) {
        typedNode->AsBinaryExpression()->SetOperationType(
            TransformType(typedNode->AsBinaryExpression()->OperationType(), typeTransformFunc));
    }

    auto var = node->Variable();
    if (var != nullptr && var->TsType() != nullptr) {
        var->SetTsType(TransformType(var->TsType(), typeTransformFunc));
    }
}

ir::AstNode *GradualTypeNarrowing::ProcessGradualTypeNode(ir::ETSTypeReference *node)
{
    auto type = node->GetType(checker_);
    if (!type->IsGradualType()) {
        return node;
    }

    // Only narrow explicit type annotation of gradual<T> to T or to Any
    if (!node->Part()->Name()->IsIdentifier() ||
        !(node->Part()->Name()->AsIdentifier()->Name() == compiler::Signatures::GRADUAL_TYPE_NAME)) {
        return node;
    }
    auto loweredNode = context_->AllocNode<ir::OpaqueTypeNode>(checker_->GlobalETSAnyType(), context_->Allocator());
    loweredNode->SetRange(node->Range());
    loweredNode->SetParent(node->Parent());
    loweredNode->SetTsType(checker_->GlobalETSRelaxedAnyType());
    return loweredNode;
}

bool GradualTypeNarrowing::PerformForModule(public_lib::Context *ctx, parser::Program *program)
{
    context_ = ctx;
    checker_ = ctx->GetChecker()->AsETSChecker();

    program->Ast()->TransformChildrenRecursively(
        // CC-OFFNXT(G.FMT.14-CPP) project code style
        [this](ir::AstNode *ast) -> checker::AstNodePtr {
            if (ast->IsETSTypeReference()) {
                return ProcessGradualTypeNode(ast->AsETSTypeReference());
            }
            return ast;
        },
        Name());

    program->Ast()->IterateRecursively([this](ir::AstNode *ast) {
        if (ast->IsTyped()) {
            NarrowGradualType(ast);
        }
    });
    return true;
}
}  // namespace ark::es2panda::compiler