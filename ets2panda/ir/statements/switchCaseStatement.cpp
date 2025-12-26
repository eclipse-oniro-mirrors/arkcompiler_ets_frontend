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

#include "switchCaseStatement.h"

#include "checker/TSchecker.h"
#include "compiler/core/ETSGen.h"
#include "compiler/core/pandagen.h"
#include "checker/ets/typeRelationContext.h"
#include "ir/astDump.h"
#include "ir/srcDump.h"

namespace ark::es2panda::ir {
void SwitchCaseStatement::TransformChildren(const NodeTransformer &cb, std::string_view transformationName)
{
    if (test_ != nullptr) {
        if (auto *transformedNode = cb(test_); test_ != transformedNode) {
            test_->SetTransformedNode(transformationName, transformedNode);
            test_ = transformedNode->AsExpression();
        }
    }

    for (auto *&it : VectorIterationGuard(consequent_)) {
        if (auto *transformedNode = cb(it); it != transformedNode) {
            it->SetTransformedNode(transformationName, transformedNode);
            it = transformedNode->AsStatement();
        }
    }
}

void SwitchCaseStatement::Iterate(const NodeTraverser &cb) const
{
    if (test_ != nullptr) {
        cb(test_);
    }

    for (auto *it : VectorIterationGuard(consequent_)) {
        cb(it);
    }
}

void SwitchCaseStatement::Dump(ir::AstDumper *dumper) const
{
    dumper->Add({{"type", "SwitchCase"}, {"test", AstDumper::Nullish(test_)}, {"consequent", consequent_}});
}

void SwitchCaseStatement::Dump(ir::SrcDumper *dumper) const
{
    if (test_ != nullptr) {
        dumper->Add("case ");
        test_->Dump(dumper);
        dumper->Add(":");
    } else {
        dumper->Add("default:");
    }
    if (!consequent_.empty()) {
        dumper->Add(" {");
        dumper->IncrIndent();
        dumper->Endl();
        for (auto cs : consequent_) {
            cs->Dump(dumper);
            if (!cs->IsBlockStatement() && cs != consequent_.back()) {
                dumper->Endl();
            }
        }
        dumper->DecrIndent();
        dumper->Endl();
        dumper->Add("}");
    }
}

void SwitchCaseStatement::Compile(compiler::PandaGen *pg) const
{
    pg->GetAstCompiler()->Compile(this);
}

void SwitchCaseStatement::Compile(compiler::ETSGen *etsg) const
{
    etsg->GetAstCompiler()->Compile(this);
}

checker::Type *SwitchCaseStatement::Check(checker::TSChecker *checker)
{
    return checker->GetAnalyzer()->Check(this);
}

checker::VerifiedType SwitchCaseStatement::Check(checker::ETSChecker *checker)
{
    return {this, checker->GetAnalyzer()->Check(this)};
}

// Auxilary function extracted from the 'Check' method for 'SwitchStatement' to reduce function's size.
void SwitchCaseStatement::CheckAndTestCase(checker::ETSChecker *checker, checker::Type *comparedExprType,
                                           checker::Type *unboxedDiscType, ir::Expression *node, bool &isDefaultCase)
{
    if (test_ != nullptr) {
        auto *caseType = checker->MaybeUnboxType(test_->Check(checker));
        bool validCaseType = true;

        if (caseType->HasTypeFlag(checker::TypeFlag::CHAR)) {
            validCaseType = comparedExprType->HasTypeFlag(checker::TypeFlag::ETS_INTEGRAL);
        } else if (caseType->IsETSEnumType() || comparedExprType->IsETSEnumType()) {
            validCaseType = checker->Relation()->IsIdenticalTo(caseType, comparedExprType);
        } else {
            const checker::AssignmentContext ctx {
                checker->Relation(),
                node,
                caseType,
                unboxedDiscType,
                test_->Start(),
                util::DiagnosticWithParams {diagnostic::SWITCH_CASE_TYPE_INCOMPARABLE, {caseType, comparedExprType}}};
            if (!ctx.IsAssignable()) {
                return;
            }
        }

        if (!validCaseType) {
            checker->LogError(diagnostic::SWITCH_CASE_TYPE_INCOMPARABLE, {caseType, comparedExprType}, test_->Start());
            return;
        }
    } else {
        isDefaultCase = true;
    }

    for (auto *caseStmt : consequent_) {
        caseStmt->Check(checker);
    }
}

SwitchCaseStatement *SwitchCaseStatement::Clone(ArenaAllocator *const allocator, AstNode *const parent)
{
    Expression *test = nullptr;
    if (test_ != nullptr) {
        test = test_ == nullptr ? nullptr : test_->Clone(allocator, nullptr)->AsExpression();
    }
    ArenaVector<Statement *> consequent(allocator->Adapter());

    for (auto *statement : consequent_) {
        consequent.push_back(statement->Clone(allocator, nullptr)->AsStatement());
    }

    auto clone = util::NodeAllocator::ForceSetParent<ir::SwitchCaseStatement>(allocator, test, std::move(consequent));

    clone->SetParent(parent);
    clone->SetRange(Range());
    return clone;
}

}  // namespace ark::es2panda::ir
