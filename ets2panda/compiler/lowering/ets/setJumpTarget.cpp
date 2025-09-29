/**
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

#include "setJumpTarget.h"
#include "checker/types/globalTypesHolder.h"
#include "compiler/lowering/util.h"

namespace ark::es2panda::compiler {

static void LogError(const public_lib::Context *ctx, const diagnostic::DiagnosticKind &diagnostic,
                     const util::DiagnosticMessageParams &diagnosticParams, const lexer::SourcePosition &pos)
{
    ctx->diagnosticEngine->LogDiagnostic(diagnostic, diagnosticParams, pos);
}

static void SetTarget(ir::AstNode *const node, ir::AstNode *const target)
{
    if (node->IsContinueStatement()) {
        node->AsContinueStatement()->SetTarget(target);
    } else {
        ES2PANDA_ASSERT(node->IsBreakStatement());
        node->AsBreakStatement()->SetTarget(target);
    }
}

void SetJumpTargetPhase::FindJumpTarget(ir::AstNode *const node) const
{
    // Look for label
    bool isContinue = node->IsContinueStatement();
    auto label = isContinue ? node->AsContinueStatement()->Ident() : node->AsBreakStatement()->Ident();
    if (label != nullptr) {
        if (auto var = label->Variable(); var == nullptr) {
            varbinder::LetDecl *decl;
            auto *varbinder = Context()->parserProgram->VarBinder()->AsETSBinder();
            std::tie(decl, var) = varbinder->NewVarDecl<varbinder::LetDecl>(
                label->Start(),
                !label->IsErrorPlaceHolder() ? label->Name() : compiler::GenName(Context()->allocator).View());
            var->SetScope(varbinder->GetScope());
            label->SetVariable(var);
            decl->BindNode(label);
            label->SetTsType(var->SetTsType(Context()->GetChecker()->GetGlobalTypesHolder()->GlobalTypeError()));
        } else if (var->Declaration()->IsLabelDecl()) {
            SetTarget(node, var->Declaration()->Node());
            return;
        }

        // Failed to resolve variable for label
        if (!label->IsErrorPlaceHolder()) {
            LogError(Context(), diagnostic::UNRESOLVED_REF, {label->Name()}, label->Start());
        }

        SetTarget(node, nullptr);
        return;
    }

    // No label, find the nearest loop or switch statement
    auto *target = node->Parent();
    while (target != nullptr) {
        switch (target->Type()) {
            case ir::AstNodeType::SWITCH_STATEMENT: {
                if (isContinue) {
                    break;
                }
                [[fallthrough]];
            }
            case ir::AstNodeType::DO_WHILE_STATEMENT:
            case ir::AstNodeType::WHILE_STATEMENT:
            case ir::AstNodeType::FOR_UPDATE_STATEMENT:
            case ir::AstNodeType::FOR_OF_STATEMENT: {
                SetTarget(node, target);
                return;
            }
            default: {
                break;
            }
        }

        target = target->Parent();
    }

    LogError(Context(), diagnostic::FLOW_REDIRECTION_INVALID_CTX, {}, node->Start());
    SetTarget(node, nullptr);
}

bool SetJumpTargetPhase::PerformForProgram(parser::Program *program)
{
    program->Ast()->IterateRecursivelyPostorder([this](ir::AstNode *const node) -> void {
        if (node->IsBreakStatement() || node->IsContinueStatement()) {
            FindJumpTarget(node);
        }
    });
    return true;
}

}  // namespace ark::es2panda::compiler
