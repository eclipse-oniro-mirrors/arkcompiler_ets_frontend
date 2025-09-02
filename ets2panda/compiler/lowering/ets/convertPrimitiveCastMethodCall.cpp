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

#include "convertPrimitiveCastMethodCall.h"

#include "checker/ETSchecker.h"
#include "compiler/lowering/util.h"

namespace ark::es2panda::compiler {

std::string_view ConvertPrimitiveCastMethodCall::Name() const
{
    return "ConvertPrimitiveCastMethodCall";
}

// Convert all MemberExpressions like 'a.toInt()' where 'a' has any primitive type into AsExpression like 'a as int'
static ir::AstNode *ConvertMemberExpressionToAsExpression(ir::CallExpression *call, checker::ETSChecker *checker)
{
    auto allocator = checker->Allocator();
    auto me = call->Callee()->AsMemberExpression();
    auto toType = call->Signature()->ReturnType();

    auto res = util::NodeAllocator::ForceSetParent<ir::TSAsExpression>(
        allocator, me->Object()->Clone(allocator, nullptr)->AsExpression(),
        checker->AllocNode<ir::OpaqueTypeNode>(toType, allocator), false);
    res->SetParent(call->Parent());

    {
        auto scope = varbinder::LexicalScope<varbinder::Scope>::Enter(checker->VarBinder(), NearestScope(me));
        CheckLoweredNode(checker->VarBinder()->AsETSBinder(), checker, res);
    }
    return res;
}

bool ConvertPrimitiveCastMethodCall::PerformForModule(public_lib::Context *const ctx, parser::Program *const program)
{
    auto checker = ctx->GetChecker()->AsETSChecker();
    program->Ast()->TransformChildrenRecursively(
        // CC-OFFNXT(G.FMT.14-CPP) project code style
        [checker](checker::AstNodePtr ast) -> checker::AstNodePtr {
            if (ast->IsCallExpression() && ast->AsCallExpression()->Callee()->IsMemberExpression() &&
                ast->AsCallExpression()->Callee()->HasAstNodeFlags(
                    ir::AstNodeFlags::TMP_CONVERT_PRIMITIVE_CAST_METHOD_CALL)) {
                return ConvertMemberExpressionToAsExpression(ast->AsCallExpression(), checker);
            }
            return ast;
        },
        Name());

    return true;
}

}  // namespace ark::es2panda::compiler
