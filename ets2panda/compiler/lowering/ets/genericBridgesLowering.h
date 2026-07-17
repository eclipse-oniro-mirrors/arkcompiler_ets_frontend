/**
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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

#ifndef ES2PANDA_GENERIC_BRIDGES_LOWERING_H
#define ES2PANDA_GENERIC_BRIDGES_LOWERING_H

#include "public/public.h"

namespace ark::es2panda::compiler {

class GenericBridgesPhase final {
public:
    virtual ~GenericBridgesPhase() = default;
    GenericBridgesPhase() = delete;

    NO_COPY_SEMANTIC(GenericBridgesPhase);
    NO_MOVE_SEMANTIC(GenericBridgesPhase);

    explicit GenericBridgesPhase(public_lib::Context *ctx) : ctx_(ctx) {}

    void ProcessClassDefinition(ir::ClassDefinition *classDefinition) const;

private:
    struct Substitutions {
        checker::Substitution derivedSubstitutions {};
        checker::Substitution baseConstraints {};
        checker::Substitution derivedConstraints {};
    };

    void ProcessClassWithGenericSupertype(const ir::ClassDefinition *classDefinition,
                                          const checker::ETSObjectType *superType,
                                          const ArenaVector<checker::Type *> &typeParameters) const;

    void ProcessInterfaces(ir::ClassDefinition const *classDefinition, checker::ETSObjectType const *classType) const;

    Substitutions GetSubstitutions(checker::ETSObjectType const *const objectType,
                                   ArenaVector<checker::Type *> const &typeParameters) const;

    void MaybeAddGenericBridges(ir::ClassDefinition const *classDefinition, ir::MethodDefinition *baseMethod,
                                ir::MethodDefinition *derivedMethod, Substitutions const &substitutions) const;

    void ProcessScriptFunction(ir::ClassDefinition const *classDefinition, ir::ScriptFunction *baseFunction,
                               ir::MethodDefinition *derivedMethod, Substitutions const &substitutions) const;

    void AddGenericBridge(ir::ClassDefinition const *classDefinition, ir::MethodDefinition *methodDefinition,
                          checker::Signature const *baseSignature, ir::ScriptFunction *derivedFunction) const;

    std::string CreateMethodDefinitionString(ir::ClassDefinition const *classDefinition,
                                             checker::Signature const *baseSignature,
                                             ir::ScriptFunction const *derivedFunction,
                                             std::vector<ir::AstNode *> &typeNodes) const noexcept;

    std::string BuildMethodSignature(ir::ScriptFunction const *derivedFunction, checker::Signature const *baseSignature,
                                     std::vector<ir::AstNode *> &typeNodes) const noexcept;

    std::string BuildMethodBody(ir::ClassDefinition const *classDefinition, ir::ScriptFunction const *derivedFunction,
                                std::vector<ir::AstNode *> &typeNodes) const noexcept;

    std::string BuildSetterAssignment(ir::ScriptFunction const *derivedFunction,
                                      std::vector<ir::AstNode *> &typeNodes) const noexcept;

    std::string BuildMethodCall(ir::ScriptFunction const *derivedFunction,
                                std::vector<ir::AstNode *> &typeNodes) const noexcept;

    ir::OpaqueTypeNode *AllocOpaqueTypeNode(checker::Type const *type) const noexcept;

    auto *Context() const noexcept
    {
        return ctx_;
    }

    public_lib::Context *ctx_ {nullptr};
};
}  // namespace ark::es2panda::compiler

#endif
