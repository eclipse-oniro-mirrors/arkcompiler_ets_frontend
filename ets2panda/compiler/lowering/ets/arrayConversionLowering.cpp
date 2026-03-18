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

#include "arrayConversionLowering.h"
#include "compiler/lowering/util.h"
#include <iostream>
#include <sstream>

namespace ark::es2panda::compiler {
ir::AstNode *ModifySyntaxToConstructorCall([[maybe_unused]] public_lib::Context *ctx, ir::AstNode *node)
{
    auto *parser = ctx->parser->AsETSParser();
    auto *arrInstance = node->AsETSNewArrayInstanceExpression();
    auto *size = arrInstance->Dimension();
    auto *typeRef = arrInstance->TypeReference();

    auto *loweringResult =
        parser->CreateFormattedExpression("Array.create<@@T1>(@@E2, @@E3)", typeRef, size, arrInstance->Initializer());
    SetSourceRangesRecursively(loweringResult, node->Range());
    loweringResult->SetParent(node->Parent());
    return loweringResult;
}
bool ArrayConversionLowering::PerformForProgram(parser::Program *program)
{
    program->Ast()->TransformChildrenRecursively(
        // CC-OFFNXT(G.FMT.14-CPP) project code style
        [ctx = Context()](ir::AstNode *node) -> ir::AstNode * {
            if (!node->IsETSNewArrayInstanceExpression()) {
                return node;
            }
            return ModifySyntaxToConstructorCall(ctx, node);
        },
        Name());

    return true;
}

}  // namespace ark::es2panda::compiler