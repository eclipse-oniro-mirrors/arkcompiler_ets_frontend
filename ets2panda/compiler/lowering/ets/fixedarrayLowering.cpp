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
#include "util/helpers.h"
#include <sstream>

namespace ark::es2panda::compiler {

// Skip fill loop for zero-value initializers: newarr already zero-inits, and boxed literal
// stores into ValueArray trip optimizer bug #30675.
static bool IsZeroValueInitializer(ir::Expression *arg)
{
    if (arg == nullptr) {
        return true;
    }
    if (arg->IsNumberLiteral()) {
        return arg->AsNumberLiteral()->Number().IsZero();
    }
    if (arg->IsBooleanLiteral()) {
        return !arg->AsBooleanLiteral()->Value();
    }
    if (arg->IsCharLiteral()) {
        return arg->AsCharLiteral()->Char() == 0;
    }
    return arg->IsUndefinedLiteral();
}

static ir::Expression *EvaluateInitializer(public_lib::Context *ctx, ir::ETSNewClassInstanceExpression *arrayInstance)
{
    auto *arg = arrayInstance->GetArguments()[1];
    if (arg->IsArrowFunctionExpression()) {
        ctx->GetChecker()->AsETSChecker()->LogError(diagnostic::LAMBDA_NOT_SUPPORTED, {}, arg->Start());
    }
    return arg;
}

struct LoweringOutput {
    std::stringstream &sourceCode;
    std::vector<ir::AstNode *> &newStmts;
};

static void AppendFillOrSkip(public_lib::Context *ctx, ir::ETSNewClassInstanceExpression *arrayInstance,
                             ir::Expression *convertedSize, ir::Expression *genSymArray, LoweringOutput &out)
{
    auto *allocator = ctx->GetChecker()->Allocator();
    ir::Expression *idx = Gensym(allocator);
    auto argSize = arrayInstance->GetArguments().size();
    bool isPrimitiveType = arrayInstance->TsType()->AsETSArrayType()->IsValueArray();
    // NOTE(klimentievamaria): an attempt to initialize with zeros for primitive types causes an optimizer bug, see
    // #30675
    auto *elemInit = argSize > 1 ? arrayInstance->GetArguments()[1] : nullptr;
    // undefined on reference types is equivalent to newarr's null zero-init — skip.
    bool skipFill =
        (argSize == 1 && (isPrimitiveType || arrayInstance->Signature() == nullptr)) ||
        (argSize > 1 && IsZeroValueInitializer(elemInit) && (isPrimitiveType || elemInit->IsUndefinedLiteral()));
    if (skipFill) {
        out.sourceCode << "@@I5;";
        out.newStmts.emplace_back(genSymArray->Clone(allocator, nullptr));
    } else {
        ir::Expression *initializer = EvaluateInitializer(ctx, arrayInstance);
        out.sourceCode << "for (let @@I5: int = 0; @@I6 < @@E7; ++@@I8) { @@I9[@@I10] = @@E11}";
        out.sourceCode << "@@I12;";

        out.newStmts.emplace_back(idx);
        out.newStmts.emplace_back(idx->Clone(allocator, nullptr));
        out.newStmts.emplace_back(convertedSize->Clone(allocator, nullptr));
        out.newStmts.emplace_back(idx->Clone(allocator, nullptr));
        out.newStmts.emplace_back(genSymArray->Clone(allocator, nullptr));
        out.newStmts.emplace_back(idx->Clone(allocator, nullptr));
        out.newStmts.emplace_back(initializer);
        out.newStmts.emplace_back(genSymArray->Clone(allocator, nullptr));
    }
}

ir::AstNode *ModifyArguments([[maybe_unused]] public_lib::Context *ctx, ir::AstNode *node)
{
    auto *allocator = ctx->GetChecker()->Allocator();
    auto checker = ctx->GetChecker()->AsETSChecker();
    auto *parser = ctx->parser->AsETSParser();
    auto *varbinder = ctx->parserProgram->VarBinder()->AsETSBinder();
    auto *arrayInstance = node->AsETSNewClassInstanceExpression();
    auto *size = arrayInstance->GetArguments()[0];
    ir::Expression *convertedSize = Gensym(allocator);
    auto nodeParent = node->Parent();
    auto *genSymArray = Gensym(allocator);

    std::vector<ir::AstNode *> newStmts;
    std::stringstream sourceCode;
    sourceCode << "let @@I1 = (@@E2).toInt();";
    sourceCode << "let @@I3 = @@E4;";
    newStmts.emplace_back(convertedSize);
    newStmts.emplace_back(size);
    newStmts.emplace_back(genSymArray);
    newStmts.emplace_back(CreateUninitializedFixedArray(ctx, convertedSize->Clone(allocator, nullptr)->AsIdentifier(),
                                                        arrayInstance->TsType()));
    LoweringOutput out {sourceCode, newStmts};
    AppendFillOrSkip(ctx, arrayInstance, convertedSize, genSymArray, out);

    auto *loweringResult = parser->CreateFormattedExpression(sourceCode.str(), newStmts);
    ES2PANDA_ASSERT(loweringResult != nullptr);
    SetSourceRangesRecursively(loweringResult, node->Range());
    loweringResult->SetParent(nodeParent);
    auto *scope = NearestScope(nodeParent);
    auto bscope = varbinder::LexicalScope<varbinder::Scope>::Enter(varbinder, scope);
    CheckLoweredNode(varbinder, checker, loweringResult);
    return loweringResult;
}

static bool IsLoweringCandidate(ir::AstNode *node)
{
    if (!node->IsETSNewClassInstanceExpression()) {
        return false;
    }
    auto *arrayInstanceType = node->AsETSNewClassInstanceExpression()->TsType();
    ES2PANDA_ASSERT(arrayInstanceType != nullptr);
    return arrayInstanceType->IsETSArrayType();
}
bool FixedArrayLowering::PerformForProgram(parser::Program *program)
{
    program->Ast()->TransformChildrenRecursively(
        // CC-OFFNXT(G.FMT.14-CPP) project code style
        [ctx = Context()](ir::AstNode *node) -> ir::AstNode * {
            if (!IsLoweringCandidate(node)) {
                return node;
            }

            return ModifyArguments(ctx, node);
        },
        Name());

    return true;
}

}  // namespace ark::es2panda::compiler
