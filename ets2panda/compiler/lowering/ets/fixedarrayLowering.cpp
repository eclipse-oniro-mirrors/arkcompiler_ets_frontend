/**
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "fixedarrayLowering.h"
#include "compiler/lowering/util.h"
#include <sstream>

namespace ark::es2panda::compiler {

ir::AstNode *ModifyArguments([[maybe_unused]] public_lib::Context *ctx, ir::AstNode *node)
{
    auto *allocator = ctx->GetChecker()->Allocator();
    auto checker = ctx->GetChecker()->AsETSChecker();
    auto *parser = ctx->parser->AsETSParser();
    auto *varbinder = ctx->parserProgram->VarBinder()->AsETSBinder();
    auto *arrayInstance = node->AsETSNewClassInstanceExpression();
    auto *elementType = arrayInstance->TsType()->AsETSArrayType()->ElementType();
    bool isprimitiveType = checker->MaybeUnboxType(elementType)->IsETSPrimitiveType();
    if (arrayInstance->GetArguments().size() == 1 && (isprimitiveType || arrayInstance->Signature() == nullptr)) {
        return node;
    }

    auto *size = arrayInstance->GetArguments()[0];

    ir::Expression *initializer = nullptr;
    auto *genSymArray = Gensym(allocator);
    auto *type = checker->AllocNode<ir::OpaqueTypeNode>(elementType, allocator);
    std::stringstream sourceCode;
    if (arrayInstance->GetArguments().size() == 1) {
        std::string initializerCode = "new @@T1()";
        initializer = parser->CreateFormattedExpression(
            initializerCode, checker->AllocNode<ir::OpaqueTypeNode>(arrayInstance->Signature()->Owner(), allocator));
    } else {
        initializer = arrayInstance->GetArguments()[1];
    }

    sourceCode << "let @@I1 : FixedArray<@@T2> = new FixedArray<@@T3>((@@E4));";
    sourceCode << "for (let i: int = 0; i < (@@E5).toInt(); ++i) { @@I6[i] = @@E7}";
    sourceCode << "@@I8;";

    auto *loweringResult = parser->CreateFormattedExpression(
        sourceCode.str(), genSymArray, type, type->Clone(allocator, nullptr), size, size->Clone(allocator, nullptr),
        genSymArray->Clone(allocator, nullptr), initializer, genSymArray->Clone(allocator, nullptr));
    ES2PANDA_ASSERT(loweringResult != nullptr);
    loweringResult->SetRange(node->Range());
    loweringResult->SetParent(node->Parent());

    auto *scope = NearestScope(node->Parent());
    auto bscope = varbinder::LexicalScope<varbinder::Scope>::Enter(varbinder, scope);
    CheckLoweredNode(varbinder, checker, loweringResult);

    return loweringResult;
}
bool FixedArrayLowering::PerformForModule(public_lib::Context *ctx, parser::Program *program)
{
    program->Ast()->TransformChildrenRecursively(
        // CC-OFFNXT(G.FMT.14-CPP) project code style
        [ctx](ir::AstNode *node) -> ir::AstNode * {
            if (!node->IsETSNewClassInstanceExpression() ||
                node->AsETSNewClassInstanceExpression()->TsType() == nullptr ||
                !node->AsETSNewClassInstanceExpression()->TsType()->IsETSArrayType()) {
                return node;
            }
            return ModifyArguments(ctx, node);
        },
        Name());

    return true;
}

bool FixedArrayLowering::PostconditionForModule([[maybe_unused]] public_lib::Context *ctx,
                                                [[maybe_unused]] const parser::Program *program)
{
    return true;
}

}  // namespace ark::es2panda::compiler