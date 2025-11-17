/*
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

#include "astNode.h"
#include "compiler/lowering/phase.h"
#include "ir/astDump.h"
#include "ir/astNodeHistory.h"
#include "ir/srcDump.h"
#include "ir/typed.h"

namespace ark::es2panda::ir {

AstNode::AstNode(AstNode const &other)
{
    auto otherHistoryNode = other.GetHistoryNode();
    range_ = otherHistoryNode->range_;
#ifndef NDEBUG
    type_ = otherHistoryNode->type_;
    flags_ = otherHistoryNode->flags_;
    astNodeFlags_ = otherHistoryNode->astNodeFlags_;
#else
    bitFields_ = otherHistoryNode->bitFields_;
#endif
    if (otherHistoryNode->variable_ != nullptr) {
        variable_ = otherHistoryNode->variable_;
    }
    // boxing_unboxing_flags_ {};  leave default value!
}

[[nodiscard]] bool AstNode::IsExported() const noexcept
{
    if (UNLIKELY(IsClassDefinition())) {
        return GetHistoryNode()->parent_->IsExported();
    }

    return (Modifiers() & ModifierFlags::EXPORT) != 0;
}

[[nodiscard]] bool AstNode::IsDefaultExported() const noexcept
{
    if (UNLIKELY(IsClassDefinition())) {
        return GetHistoryNode()->parent_->IsDefaultExported();
    }

    return (Modifiers() & ModifierFlags::DEFAULT_EXPORT) != 0;
}

[[nodiscard]] bool AstNode::IsExportedType() const noexcept
{
    if (UNLIKELY(IsClassDefinition())) {
        return GetHistoryNode()->parent_->IsExportedType();
    }

    return (Modifiers() & ModifierFlags::EXPORT_TYPE) != 0;
}

[[nodiscard]] bool AstNode::HasExportAlias() const noexcept
{
    if (UNLIKELY(IsClassDefinition())) {
        return GetHistoryNode()->parent_->HasExportAlias();
    }
#ifndef NDEBUG
    return (GetHistoryNode()->flags_ & ModifierFlags::EXPORT_WITH_ALIAS) != 0;
#else
    return (Modifiers() & ModifierFlags::EXPORT_WITH_ALIAS) != 0;
#endif
}

bool AstNode::IsScopeBearer() const noexcept
{
    return false;
}

varbinder::Scope *AstNode::Scope() const noexcept
{
    ES2PANDA_UNREACHABLE();
}

void AstNode::ClearScope() noexcept
{
    ES2PANDA_UNREACHABLE();
}

ir::ClassElement *AstNode::AsClassElement()
{
    ES2PANDA_ASSERT(IsMethodDefinition() || IsClassProperty() || IsClassStaticBlock() || IsOverloadDeclaration());
    return reinterpret_cast<ir::ClassElement *>(this);
}

const ir::ClassElement *AstNode::AsClassElement() const
{
    ES2PANDA_ASSERT(IsMethodDefinition() || IsClassProperty() || IsClassStaticBlock() || IsOverloadDeclaration());
    return reinterpret_cast<const ir::ClassElement *>(this);
}

template <typename R, typename T>
static R GetTopStatementImpl(T *self)
{
    auto iter = self;

    while (iter->Parent()) {
        iter = iter->Parent();
    }

    return reinterpret_cast<R>(iter);
}

ir::BlockStatement *AstNode::GetTopStatement()
{
    return GetTopStatementImpl<ir::BlockStatement *>(this);
}

const ir::BlockStatement *AstNode::GetTopStatement() const
{
    return GetTopStatementImpl<const ir::BlockStatement *>(this);
}

AstNode *AstNode::Clone([[maybe_unused]] ArenaAllocator *const allocator, [[maybe_unused]] AstNode *const parent)
{
    ES2PANDA_UNREACHABLE();
}

varbinder::Scope *AstNode::EnclosingScope(const ir::AstNode *expr) noexcept
{
    while (expr != nullptr && !expr->IsScopeBearer()) {
        expr = expr->Parent();
    }
    return expr != nullptr ? expr->Scope() : nullptr;
}

std::string AstNode::DumpJSON() const
{
    ir::AstDumper dumper {this};
    return dumper.Str();
}

std::string AstNode::DumpEtsSrc() const
{
    ir::SrcDumper dumper {};
    Dump(&dumper);
    return dumper.Str();
}

std::string AstNode::DumpEtsSrcWithJsdoc() const
{
    ir::SrcDumper dumper {this, true, nullptr};
    Dump(&dumper);
    return dumper.Str();
}

std::string AstNode::DumpDecl(public_lib::Context *context) const
{
    Declgen dg {context};

    ir::SrcDumper dumper {&dg};
    Dump(&dumper);
    dumper.GetDeclgen()->Run();
    return dumper.Str();
}

void AstNode::SetOriginalNode(AstNode *originalNode) noexcept
{
    if (OriginalNode() != originalNode) {
        GetOrCreateHistoryNode()->originalNode_ = originalNode;
    }
}

AstNode *AstNode::OriginalNode() const noexcept
{
    return GetHistoryNode()->originalNode_;
}

void AstNode::SetTransformedNode([[maybe_unused]] std::string_view const transformationName, AstNode *transformedNode)
{
    if (transformedNode != nullptr) {
        transformedNode->SetOriginalNode(this);
    }
}

void AstNode::CleanUp()
{
    SetVariable(nullptr);
    if (IsScopeBearer()) {
        ClearScope();
    }
    if (IsTyped()) {
        this->AsTyped()->SetTsType(nullptr);
        this->AsTyped()->SetPreferredType(nullptr);
    }
}

bool AstNode::IsReadonly() const noexcept
{
    return (Modifiers() & ModifierFlags::READONLY) != 0;
}

// NOTE: For readonly parameter type
bool AstNode::IsReadonlyType() const noexcept
{
    return (Modifiers() & ModifierFlags::READONLY_PARAMETER) != 0;
}

bool AstNode::IsOptionalDeclaration() const noexcept
{
    return (Modifiers() & ModifierFlags::OPTIONAL) != 0;
}

bool AstNode::IsDefinite() const noexcept
{
    return (Modifiers() & ModifierFlags::DEFINITE) != 0;
}

bool AstNode::IsConstructor() const noexcept
{
    return (Modifiers() & ModifierFlags::CONSTRUCTOR) != 0;
}

bool AstNode::IsOverride() const noexcept
{
    return (Modifiers() & ModifierFlags::OVERRIDE) != 0;
}

AstNode *AstNode::ShallowClone(ArenaAllocator *allocator)
{
    auto clone = Construct(allocator);
    CopyTo(clone);
    return clone;
}

void AstNode::CopyTo(AstNode *other) const
{
#ifndef NDEBUG
    ES2PANDA_ASSERT(other->type_ == type_);
#else
    ES2PANDA_ASSERT(TypeField::Decode(other->bitFields_) == TypeField::Decode(bitFields_));
#endif

    other->parent_ = parent_;
    other->range_ = range_;
#ifndef NDEBUG
    other->flags_ = flags_;
    other->astNodeFlags_ = astNodeFlags_;
#else
    FlagsField::Set(FlagsField::Decode(bitFields_), &(other->bitFields_));
    AstNodeFlagsField::Set(AstNodeFlagsField::Decode(bitFields_), &(other->bitFields_));
#endif
#ifdef ES2PANDA_ENABLE_AST_HISTORY
    other->history_ = history_;
#endif
    other->variable_ = variable_;
    other->originalNode_ = originalNode_;
}

AstNode *AstNode::Construct([[maybe_unused]] ArenaAllocator *allocator)
{
    ES2PANDA_UNREACHABLE();
}

#ifdef ES2PANDA_ENABLE_AST_HISTORY
bool AstNode::IsValidInCurrentPhase() const
{
    if (!HistoryInitialized()) {
        return true;
    }
    return compiler::GetPhaseManager()->CurrentPhaseId() >= history_->FirstCreated();
}

AstNode *AstNode::GetFromExistingHistory() const
{
    ES2PANDA_ASSERT(HistoryInitialized());
    auto node = history_->Get(compiler::GetPhaseManager()->CurrentPhaseId());
    if (UNLIKELY(node == nullptr)) {
        // the callee assumes the nullptr might be returned, but
        // the caller asserts it is not possible, so the explicit check is inserted
        ES2PANDA_UNREACHABLE();
    }
    return node;
}

AstNode *AstNode::GetOrCreateHistoryNode() const
{
    AstNode *node = nullptr;

    if (HistoryInitialized()) {
        node = history_->At(compiler::GetPhaseManager()->CurrentPhaseId());
        if (node == nullptr) {
            node = history_->Get(compiler::GetPhaseManager()->PreviousPhaseId());
            ES2PANDA_ASSERT(node != nullptr);
            node = node->ShallowClone(compiler::GetPhaseManager()->Allocator());
            history_->Set(node, compiler::GetPhaseManager()->CurrentPhaseId());
        }
    } else {
        node = const_cast<AstNode *>(this);
    }

    return node;
}

void AstNode::InitHistory()
{
    if (!g_enableContextHistory || HistoryInitialized()) {
        return;
    }

    history_ = compiler::GetPhaseManager()->Allocator()->New<AstNodeHistory>(
        this, compiler::GetPhaseManager()->CurrentPhaseId(), compiler::GetPhaseManager()->Allocator());
}

bool AstNode::HistoryInitialized() const
{
    return history_ != nullptr;
}
#endif  // ES2PANDA_ENABLE_AST_HISTORY

void AstNode::AddModifier(ModifierFlags const flags) noexcept
{
    if (!All(Modifiers(), flags)) {
#ifndef NDEBUG
        GetOrCreateHistoryNode()->flags_ |= flags;
#else
        FlagsField::Set(FlagsField::Decode(GetOrCreateHistoryNode()->bitFields_) | static_cast<uint64_t>(flags),
                        &(GetOrCreateHistoryNode()->bitFields_));
#endif
    }
}

void AstNode::ClearModifier(ModifierFlags const flags) noexcept
{
    if (Any(Modifiers(), flags)) {
#ifndef NDEBUG
        GetOrCreateHistoryNode()->flags_ &= ~flags;
#else
        FlagsField::Set(FlagsField::Decode(GetOrCreateHistoryNode()->bitFields_) & ~static_cast<uint64_t>(flags),
                        &(GetOrCreateHistoryNode()->bitFields_));
#endif
    }
}

void AstNode::CleanCheckInformation()
{
    if (!IsTyped()) {
        return;
    }
    if (IsOpaqueTypeNode()) {
        return;
    }
    RemoveAstNodeFlags(ir::AstNodeFlags::GENERATE_VALUE_OF);
    if (AsTyped()->PreferredType() != nullptr) {
        AsTyped()->SetTsType(nullptr);
        AsTyped()->SetPreferredType(nullptr);
        Iterate([&](auto *childNode) { childNode->CleanCheckInformation(); });
    }
    if (AsTyped()->TsType() != nullptr && AsTyped()->TsType()->IsTypeError()) {
        SetVariable(nullptr);
        AsTyped()->SetTsType(nullptr);
        AsTyped()->SetPreferredType(nullptr);
        Iterate([&](auto *childNode) { childNode->CleanCheckInformation(); });
    }
}

}  // namespace ark::es2panda::ir
