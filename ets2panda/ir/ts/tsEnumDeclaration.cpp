/**
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

#include "tsEnumDeclaration.h"
#include <cstddef>

#include "checker/TSchecker.h"
#include "compiler/core/ETSGen.h"
#include "compiler/core/pandagen.h"
#include "util/helpers.h"
#include "ir/astDump.h"
#include "ir/srcDump.h"
#include "libarkbase/utils/arena_containers.h"

namespace ark::es2panda::ir {

void TSEnumDeclaration::SetInternalName(util::StringView internalName)
{
    this->GetOrCreateHistoryNodeAs<TSEnumDeclaration>()->internalName_ = internalName;
}

void TSEnumDeclaration::SetBoxedClass(ClassDefinition *boxedClass)
{
    this->GetOrCreateHistoryNodeAs<TSEnumDeclaration>()->boxedClass_ = boxedClass;
}

void TSEnumDeclaration::SetKey(Identifier *key)
{
    this->GetOrCreateHistoryNodeAs<TSEnumDeclaration>()->key_ = key;
}

void TSEnumDeclaration::TransformChildren(const NodeTransformer &cb, std::string_view transformationName)
{
    auto const key = Key();
    if (auto *transformedNode = cb(key); key != transformedNode) {
        key->SetTransformedNode(transformationName, transformedNode);
        SetKey(transformedNode->AsIdentifier());
    }

    auto const &members = Members();
    for (size_t ix = 0; ix < members.size(); ix++) {
        if (auto *transformedNode = cb(members[ix]); members[ix] != transformedNode) {
            members[ix]->SetTransformedNode(transformationName, transformedNode);
            SetValueMembers(transformedNode, ix);
        }
    }
}

void TSEnumDeclaration::Iterate(const NodeTraverser &cb) const
{
    auto const key = GetHistoryNode()->AsTSEnumDeclaration()->key_;
    cb(key);

    for (auto *it : VectorIterationGuard(Members())) {
        cb(it);
    }
}

void TSEnumDeclaration::Dump(ir::AstDumper *dumper) const
{
    dumper->Add({{"type", "TSEnumDeclaration"},
                 {"id", Key()},
                 {"members", Members()},
                 {"const", IsConst()},
                 {"declare", IsDeclare()}});
}

bool TSEnumDeclaration::RegisterUnexportedForDeclGen(ir::SrcDumper *dumper) const
{
    ES2PANDA_ASSERT(dumper->IsDeclgen());

    if (dumper->GetDeclgen()->IsPostDumpIndirectDepsPhase()) {
        return false;
    }

    if (key_->Parent()->IsDefaultExported() || key_->Parent()->IsExported()) {
        return false;
    }

    auto name = key_->AsIdentifier()->Name().Mutf8();
    dumper->GetDeclgen()->AddNode(name, this);
    return true;
}

void TSEnumDeclaration::Dump(ir::SrcDumper *dumper) const
{
    auto guard = dumper->BuildAmbientContextGuard();
    ES2PANDA_ASSERT(isConst_ == false);
    ES2PANDA_ASSERT(key_ != nullptr);
    if (dumper->IsDeclgen() && RegisterUnexportedForDeclGen(dumper)) {
        return;
    }
    dumper->DumpJsdocBeforeTargetNode(this);
    if (key_->Parent()->IsExported() && dumper->IsDeclgen()) {
        dumper->Add("export ");
    } else if (key_->Parent()->IsDefaultExported() && dumper->IsDeclgen()) {
        dumper->Add("export default ");
        dumper->SetDefaultExport();
    }
    if (dumper->IsDeclgen()) {
        dumper->GetDeclgen()->TryDeclareAmbientContext(dumper);
    } else if (IsDeclare()) {
        dumper->Add("declare ");
    }
    dumper->Add("enum ");
    Key()->Dump(dumper);
    dumper->Add(" {");
    auto const members = Members();
    if (!members.empty()) {
        dumper->IncrIndent();
        dumper->Endl();
        for (auto member : members) {
            member->Dump(dumper);
            if (member != members.back()) {
                dumper->Add(",");
                dumper->Endl();
            }
        }
        dumper->DecrIndent();
        dumper->Endl();
    }
    dumper->Add("}");
    dumper->Endl();
}

// NOTE (csabahurton): this method has not been moved to TSAnalyizer.cpp, because it is not used.
varbinder::EnumMemberResult EvaluateMemberExpression(checker::TSChecker *checker,
                                                     [[maybe_unused]] varbinder::EnumVariable *enumVar,
                                                     ir::MemberExpression *expr)
{
    if (checker::TSChecker::IsConstantMemberAccess(expr->AsExpression())) {
        if (expr->Check(checker)->TypeFlags() == checker::TypeFlag::ENUM) {
            util::StringView name;
            if (!expr->IsComputed()) {
                name = expr->Property()->AsIdentifier()->Name();
            } else {
                ES2PANDA_ASSERT(checker::TSChecker::IsStringLike(expr->Property()));
                name = reinterpret_cast<const ir::StringLiteral *>(expr->Property())->Str();
            }

            // NOTE: aszilagyi.
        }
    }

    return false;
}

void TSEnumDeclaration::Compile(compiler::PandaGen *pg) const
{
    pg->GetAstCompiler()->Compile(this);
}

void TSEnumDeclaration::Compile(compiler::ETSGen *etsg) const
{
    etsg->GetAstCompiler()->Compile(this);
}

checker::Type *TSEnumDeclaration::Check(checker::TSChecker *checker)
{
    return checker->GetAnalyzer()->Check(this);
}

checker::VerifiedType TSEnumDeclaration::Check(checker::ETSChecker *const checker)
{
    return {this, checker->GetAnalyzer()->Check(this)};
}

TSEnumDeclaration *TSEnumDeclaration::Construct(ArenaAllocator *allocator)
{
    ArenaVector<AstNode *> members(allocator->Adapter());
    return allocator->New<TSEnumDeclaration>(allocator, nullptr, std::move(members),
                                             ConstructorFlags {false, false, false}, lang_);
}

void TSEnumDeclaration::CopyTo(AstNode *other) const
{
    auto otherImpl = other->AsTSEnumDeclaration();

    otherImpl->scope_ = scope_;
    otherImpl->key_ = key_;
    otherImpl->members_ = members_;
    otherImpl->internalName_ = internalName_;
    otherImpl->boxedClass_ = boxedClass_;
    otherImpl->isConst_ = isConst_;

    TypedStatement::CopyTo(other);
}

void TSEnumDeclaration::EmplaceMembers(AstNode *source)
{
    auto newNode = this->GetOrCreateHistoryNodeAs<TSEnumDeclaration>();
    newNode->members_.emplace_back(source);
}

void TSEnumDeclaration::ClearMembers()
{
    auto newNode = this->GetOrCreateHistoryNodeAs<TSEnumDeclaration>();
    newNode->members_.clear();
}

void TSEnumDeclaration::SetValueMembers(AstNode *source, size_t index)
{
    auto newNode = this->GetOrCreateHistoryNodeAs<TSEnumDeclaration>();
    auto &arenaVector = newNode->members_;
    ES2PANDA_ASSERT(arenaVector.size() > index);
    arenaVector[index] = source;
}

[[nodiscard]] ArenaVector<AstNode *> &TSEnumDeclaration::MembersForUpdate()
{
    auto newNode = this->GetOrCreateHistoryNodeAs<TSEnumDeclaration>();
    return newNode->members_;
}

}  // namespace ark::es2panda::ir
