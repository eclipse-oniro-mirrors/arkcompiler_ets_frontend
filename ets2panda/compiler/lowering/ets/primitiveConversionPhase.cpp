/**
 * Copyright (c) 2021-2026 Huawei Device Co., Ltd.
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

#include "compiler/lowering/util.h"
#include "primitiveConversionPhase.h"

namespace ark::es2panda::compiler {

// Transform calls of the form `x.toY()`, where `x: X` and X and Y are (boxed) primitive types,
// into static calls of the form `X.toY(x)`, which will then be represented in the bytecode as single
// instructions.
// The code relies on the fact that there are no other pairs of methods in the boxed primitive classes,
// such that one is virtual and takes no arguments, the other one has the same name, is static and takes
// an object of the containing type as parameter.

static bool CheckCalleeConversionNeed(ir::Expression *callee)
{
    return !(!callee->IsMemberExpression() ||
             callee->AsMemberExpression()->Kind() != ir::MemberExpressionKind::PROPERTY_ACCESS ||
             !callee->AsMemberExpression()->Property()->IsIdentifier());
}

static ir::Expression *ConvertCallIfNeeded(public_lib::Context *ctx, ir::CallExpression *call)
{
    if (!call->Arguments().empty()) {
        return call;
    }

    auto *callee = call->Callee();
    if (!CheckCalleeConversionNeed(callee)) {
        return call;
    }

    auto *calleeObj = callee->AsMemberExpression()->Object();
    auto *calleePropId = callee->AsMemberExpression()->Property()->AsIdentifier();

    if (calleePropId->Variable()->HasFlag(varbinder::VariableFlags::STATIC)) {
        return call;
    }

    auto *calleeObjType = calleeObj->TsType();
    if (!calleeObjType->IsETSObjectType() || !calleeObjType->AsETSObjectType()->IsBoxedPrimitive()) {
        return call;
    }

    auto *staticMethodVar = calleeObjType->AsETSObjectType()->GetProperty(
        calleePropId->Name(), checker::PropertySearchFlags::SEARCH_STATIC_METHOD);
    if (staticMethodVar == nullptr) {
        return call;
    }

    auto *checker = ctx->GetChecker()->AsETSChecker();
    auto *staticSig = staticMethodVar->TsType()->AsETSFunctionType()->CallSignatures()[0];
    if (staticSig->Params().size() != 1 ||
        !checker->Relation()->IsIdenticalTo(staticSig->Params()[0]->TsType(), calleeObjType) ||
        !checker->Relation()->IsIdenticalTo(staticSig->ReturnType(), call->TsType())) {
        return call;
    }

    /* When the conversion call is `x.toX()` or `y.toX()`, replace it with `x` or a static call, but when the `x` is
       charType we cannot just replace `x.toX()` with `x`, because when we call the builtin method `x.toUpperCase()` and
       `x.toLowerCase()`, we need replace them with static call too.
    */

    auto *allocator = ctx->Allocator();

    if (!calleeObjType->IsETSCharType() && checker->Relation()->IsIdenticalTo(calleeObjType, call->TsType())) {
        calleeObj->SetParent(call->Parent());
        return calleeObj;
    }

    auto args = ArenaVector<ir::Expression *> {allocator->Adapter()};
    args.push_back(calleeObj);

    calleePropId->SetVariable(staticMethodVar);
    calleePropId->SetTsType(staticMethodVar->TsType());
    auto *newCallee = util::NodeAllocator::ForceSetParent<ir::MemberExpression>(
        allocator, allocator->New<ir::OpaqueTypeNode>(calleeObjType, allocator), calleePropId,
        ir::MemberExpressionKind::PROPERTY_ACCESS, false, false);
    newCallee->SetTsType(staticMethodVar->TsType());
    newCallee->SetObjectType(calleeObjType->AsETSObjectType());
    auto *newCall =
        util::NodeAllocator::ForceSetParent<ir::CallExpression>(allocator, newCallee, std::move(args), nullptr, false);
    newCall->SetParent(call->Parent());

    CheckLoweredNode(checker->VarBinder()->AsETSBinder(), checker, newCall);

    return newCall;
}

bool PrimitiveConversionPhase::PerformForProgram(parser::Program *program)
{
    program->Ast()->TransformChildrenRecursively(
        // clang-format off
        [ctx = Context()](ir::AstNode *ast) -> ir::AstNode* {
            if (ast->IsCallExpression()) {
                return ConvertCallIfNeeded(ctx, ast->AsCallExpression());
            }
            return ast;
        },
        // clang-format on
        "PrimitiveConversion");
    return true;
}

}  // namespace ark::es2panda::compiler
