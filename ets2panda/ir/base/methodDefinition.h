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

#ifndef ES2PANDA_PARSER_INCLUDE_AST_METHOD_DEFINITION_H
#define ES2PANDA_PARSER_INCLUDE_AST_METHOD_DEFINITION_H

#include "scriptFunction.h"
#include "ir/base/classElement.h"

namespace ark::es2panda::checker {
class ETSAnalyzer;
}  // namespace ark::es2panda::checker

namespace ark::es2panda::ir {

class Expression;
class ScriptFunction;

enum class MethodDefinitionKind {
    NONE,
    CONSTRUCTOR,
    METHOD,
    EXTENSION_METHOD,
    GET,
    SET,
    EXTENSION_GET,
    EXTENSION_SET,
};

struct OverloadInfo {
    uint32_t minArg = 0;
    size_t maxArg = 0;
    bool needHelperOverload = false;
    bool isDeclare = false;
    bool hasRestVar = false;
    bool returnVoid = false;
};

class MethodDefinition : public ClassElement {
public:
    MethodDefinition() = delete;
    ~MethodDefinition() override = default;

    NO_COPY_SEMANTIC(MethodDefinition);
    NO_MOVE_SEMANTIC(MethodDefinition);

    using OverloadsT = ArenaVector<MethodDefinition *>;
    // CC-OFFNXT(G.FUN.01-CPP) solid logic
    explicit MethodDefinition(MethodDefinitionKind const kind, Expression *const key, Expression *const value,
                              ModifierFlags const modifiers, [[maybe_unused]] ArenaAllocator *const allocator,
                              bool const isComputed)
        : ClassElement(AstNodeType::METHOD_DEFINITION, key, value, modifiers, allocator, isComputed),
          kind_(kind),
          overloads_(allocator->Adapter()),
          baseOverloadMethod_(nullptr),
          asyncPairMethod_(nullptr)
    {
        InitHistory();
    }

    // CC-OFFNXT(G.FUN.01-CPP) solid logic
    explicit MethodDefinition(MethodDefinitionKind const kind, Expression *const key, Expression *const value,
                              ModifierFlags const modifiers, [[maybe_unused]] ArenaAllocator *const allocator,
                              bool const isComputed, AstNodeHistory *history)
        : ClassElement(AstNodeType::METHOD_DEFINITION, key, value, modifiers, allocator, isComputed),
          kind_(kind),
          overloads_(allocator->Adapter()),
          baseOverloadMethod_(nullptr),
          asyncPairMethod_(nullptr)
    {
        if (history != nullptr) {
            history_ = history;
        } else {
            InitHistory();
        }
    }

    // NOTE (csabahurton): these friend relationships can be removed once there are getters for private fields
    friend class checker::ETSAnalyzer;

    MethodDefinitionKind Kind() const
    {
        return GetHistoryNodeAs<MethodDefinition>()->kind_;
    }

    [[nodiscard]] bool IsConstructor() const noexcept
    {
        return Kind() == MethodDefinitionKind::CONSTRUCTOR;
    }

    [[nodiscard]] bool IsMethod() const noexcept
    {
        return Kind() == MethodDefinitionKind::METHOD;
    }

    [[nodiscard]] bool IsExtensionMethod() const noexcept
    {
        auto const kind = Kind();
        return (kind == MethodDefinitionKind::EXTENSION_METHOD) || (kind == MethodDefinitionKind::EXTENSION_GET) ||
               (kind == MethodDefinitionKind::EXTENSION_SET);
    }

    [[nodiscard]] bool IsGetter() const noexcept
    {
        return Kind() == MethodDefinitionKind::GET;
    }

    [[nodiscard]] bool IsSetter() const noexcept
    {
        return Kind() == MethodDefinitionKind::SET;
    }

    [[nodiscard]] bool IsDefaultAccessModifier() const noexcept
    {
        return GetHistoryNodeAs<MethodDefinition>()->isDefault_;
    }

    void SetDefaultAccessModifier(bool isDefault);

    [[nodiscard]] const OverloadsT &Overloads() const noexcept
    {
        return GetHistoryNodeAs<MethodDefinition>()->overloads_;
    }

    [[nodiscard]] const MethodDefinition *BaseOverloadMethod() const noexcept
    {
        return GetHistoryNodeAs<MethodDefinition>()->baseOverloadMethod_;
    }

    [[nodiscard]] MethodDefinition *BaseOverloadMethod() noexcept
    {
        return GetHistoryNodeAs<MethodDefinition>()->baseOverloadMethod_;
    }

    [[nodiscard]] const MethodDefinition *AsyncPairMethod() const noexcept
    {
        return GetHistoryNodeAs<MethodDefinition>()->asyncPairMethod_;
    }

    [[nodiscard]] MethodDefinition *AsyncPairMethod() noexcept
    {
        return GetHistoryNodeAs<MethodDefinition>()->asyncPairMethod_;
    }

    [[nodiscard]] const OverloadInfo &GetOverloadInfo() noexcept
    {
        auto newNode = this->GetHistoryNode()->AsMethodDefinition();
        return newNode->overloadInfo_;
    }

    [[nodiscard]] OverloadInfo &GetOverloadInfoForUpdate() noexcept
    {
        auto newNode = this->GetOrCreateHistoryNode()->AsMethodDefinition();
        return newNode->overloadInfo_;
    }

    [[nodiscard]] const OverloadInfo &GetOverloadInfo() const noexcept
    {
        return GetHistoryNodeAs<MethodDefinition>()->overloadInfo_;
    }

    void SetOverloadInfo(OverloadInfo &&overloadInfo)
    {
        auto newNode = this->GetOrCreateHistoryNode()->AsMethodDefinition();
        newNode->overloadInfo_ = overloadInfo;
    }

    void SetOverloadInfo(OverloadInfo &overloadInfo)
    {
        auto newNode = this->GetOrCreateHistoryNode()->AsMethodDefinition();
        newNode->overloadInfo_ = overloadInfo;
    }

    void SetOverloads(OverloadsT &&overloads)
    {
        auto newNode = this->GetOrCreateHistoryNode()->AsMethodDefinition();
        newNode->overloads_ = std::move(overloads);
    }

    void AddOverload(MethodDefinition *const overload)
    {
        ES2PANDA_ASSERT(overload != nullptr);
        auto newNode = this->GetOrCreateHistoryNode()->AsMethodDefinition();
        newNode->overloads_.emplace_back(overload);
        ES2PANDA_ASSERT(overload->Function() != nullptr);
        overload->Function()->AddFlag((ir::ScriptFunctionFlags::OVERLOAD));
        overload->SetBaseOverloadMethod(this);
    }

    void SetBaseOverloadMethod(MethodDefinition *const baseOverloadMethod);

    void SetAsyncPairMethod(MethodDefinition *const asyncPairMethod);

    [[nodiscard]] bool HasOverload(MethodDefinition *overload) noexcept
    {
        auto const overloads = Overloads();
        return std::find(overloads.begin(), overloads.end(), overload) != overloads.end();
    }

    ScriptFunction *Function();
    const ScriptFunction *Function() const;
    void InitializeOverloadInfo();
    PrivateFieldKind ToPrivateFieldKind(bool isStatic) const override;

    [[nodiscard]] MethodDefinition *Clone(ArenaAllocator *allocator, AstNode *parent) override;

    void TransformChildren(const NodeTransformer &cb, std::string_view transformationName) override;
    void Iterate(const NodeTraverser &cb) const override;

    void ResolveReferences(const NodeTraverser &cb) const;

    void Dump(ir::AstDumper *dumper) const override;
    void Dump(ir::SrcDumper *dumper) const override;
    void Compile(compiler::PandaGen *pg) const override;
    void Compile(compiler::ETSGen *etsg) const override;
    checker::Type *Check(checker::TSChecker *checker) override;
    checker::VerifiedType Check(checker::ETSChecker *checker) override;

    void Accept(ASTVisitorT *v) override
    {
        v->Accept(this);
    }

    void CleanUp() override;

    void EmplaceOverloads(MethodDefinition *overloads);
    void ClearOverloads();
    void SetValueOverloads(MethodDefinition *overloads, size_t index);
    [[nodiscard]] ArenaVector<MethodDefinition *> &OverloadsForUpdate();

protected:
    MethodDefinition *Construct(ArenaAllocator *allocator) override;
    void CopyTo(AstNode *other) const override;

private:
    void ResetOverloads();
    bool DumpNamespaceForDeclGen(ir::SrcDumper *dumper) const;
    void DumpPrefixForDeclGen(ir::SrcDumper *dumper) const;
    bool FilterForDeclGen() const;

    friend class SizeOfNodeTest;
    bool isDefault_ = false;
    MethodDefinitionKind kind_;
    // Overloads are stored like in an 1:N fashion.
    // The very firstly processed method becomes the base(1) and the others tied into it as overloads(N).
    OverloadsT overloads_;
    // Base overload method points at the first overload of the overloads.
    MethodDefinition *baseOverloadMethod_;
    // Pair method points at the original async method in case of an implement method and vice versa an implement
    // method's point at the async method
    MethodDefinition *asyncPairMethod_;
    OverloadInfo overloadInfo_;
};
}  // namespace ark::es2panda::ir

#endif
