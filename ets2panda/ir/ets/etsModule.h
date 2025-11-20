/*
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

#ifndef ES2PANDA_IR_ETS_MODULE_H
#define ES2PANDA_IR_ETS_MODULE_H

#include "ir/statements/blockStatement.h"
#include "ir/annotationAllowed.h"
#include "ir/expressions/identifier.h"
#include "ir/srcDump.h"

namespace ark::es2panda::parser {
class Program;
}  // namespace ark::es2panda::parser

namespace ark::es2panda::ir {

using ENUMBITOPS_OPERATORS;

enum class ModuleFlag : uint32_t {
    NONE = 0,
    ETSSCRIPT = 1U << 0U,
    NAMESPACE = 1U << 1U,
    NAMESPACE_CHAIN_LAST_NODE = 1U << 2U
};
}  // namespace ark::es2panda::ir

template <>
struct enumbitops::IsAllowedType<ark::es2panda::ir::ModuleFlag> : std::true_type {
};

namespace ark::es2panda::ir {

class ETSModule : public AnnotationAllowed<BlockStatement> {
public:
    // CC-OFFNXT(G.FUN.01-CPP) solid logic
    explicit ETSModule(ArenaAllocator *allocator, ArenaVector<Statement *> &&statementList, Identifier *ident,
                       ModuleFlag flag, Language lang, parser::Program *program)
        : AnnotationAllowed<BlockStatement>(allocator, std::move(statementList)),
          ident_(ident),
          flag_(flag),
          lang_(lang),
          program_(program)
    {
        SetType(AstNodeType::ETS_MODULE);
        InitHistory();
    }

    // CC-OFFNXT(G.FUN.01-CPP) solid logic
    explicit ETSModule(ArenaAllocator *allocator, ArenaVector<Statement *> &&statementList, Identifier *ident,
                       ModuleFlag flag, Language lang, parser::Program *program, AstNodeHistory *history)
        : AnnotationAllowed<BlockStatement>(allocator, std::move(statementList)),
          ident_(ident),
          flag_(flag),
          lang_(lang),
          program_(program)
    {
        SetType(AstNodeType::ETS_MODULE);
        if (history != nullptr) {
            SetHistoryInternal(history);
        } else {
            InitHistory();
        }
    }

    ir::Identifier *Ident()
    {
        return GetHistoryNodeAs<ETSModule>()->ident_;
    }

    const ir::Identifier *Ident() const
    {
        return GetHistoryNodeAs<ETSModule>()->ident_;
    }

    void SetIdent(ir::Identifier *ident)
    {
        auto newNode = GetOrCreateHistoryNodeAs<ETSModule>();
        newNode->ident_ = ident;

        if (ident != nullptr) {
            ident->SetParent(newNode);
        }
    }

    parser::Program *Program()
    {
        return GetHistoryNodeAs<ETSModule>()->program_;
    }

    const ir::ClassDefinition *GlobalClass() const
    {
        return GetHistoryNodeAs<ETSModule>()->globalClass_;
    }

    [[nodiscard]] es2panda::Language Language() const noexcept
    {
        return GetHistoryNodeAs<ETSModule>()->lang_;
    }

    ir::ClassDefinition *GlobalClass()
    {
        return GetHistoryNodeAs<ETSModule>()->globalClass_;
    }

    void SetGlobalClass(ir::ClassDefinition *globalClass)
    {
        if (globalClass != GlobalClass()) {
            GetOrCreateHistoryNode()->AsETSModule()->globalClass_ = globalClass;
        }
    }

    [[nodiscard]] bool IsETSScript() const noexcept
    {
        return (ModuleFlags() & ModuleFlag::ETSSCRIPT) != 0;
    }

    [[nodiscard]] bool IsNamespace() const noexcept
    {
        return (ModuleFlags() & ModuleFlag::NAMESPACE) != 0;
    }

    [[nodiscard]] bool IsNamespaceChainLastNode() const noexcept
    {
        return (ModuleFlags() & ModuleFlag::NAMESPACE_CHAIN_LAST_NODE) != 0;
    }

    void SetNamespaceChainLastNode() noexcept
    {
        ES2PANDA_ASSERT(IsNamespace());
        AddModuleFlag(ModuleFlag::NAMESPACE_CHAIN_LAST_NODE);
    }

    const parser::Program *Program() const
    {
        return GetHistoryNodeAs<ETSModule>()->program_;
    }
    void Dump(ir::SrcDumper *dumper) const override;
    void Accept(ASTVisitorT *v) override
    {
        v->Accept(this);
    }

    ETSModule *Construct(ArenaAllocator *allocator) override;
    void CopyTo(AstNode *other) const override;

private:
    friend class SizeOfNodeTest;
    ModuleFlag ModuleFlags() const
    {
        return GetHistoryNodeAs<ETSModule>()->flag_;
    }

    void AddModuleFlag(ModuleFlag flag) noexcept
    {
        if (!All(ModuleFlags(), flag)) {
            GetOrCreateHistoryNode()->AsETSModule()->flag_ |= flag;
        }
    }

    void ClearModuleFlag(ModuleFlag flag) noexcept
    {
        if (Any(ModuleFlags(), flag)) {
            GetOrCreateHistoryNode()->AsETSModule()->flag_ &= ~flag;
        }
    }

    Identifier *ident_;
    ModuleFlag flag_;
    es2panda::Language lang_;
    parser::Program *program_;
    ir::ClassDefinition *globalClass_ {};
};
}  // namespace ark::es2panda::ir

#endif
