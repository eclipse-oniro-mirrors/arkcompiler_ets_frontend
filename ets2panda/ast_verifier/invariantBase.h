/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

#ifndef ES2PANDA_COMPILER_CORE_AST_VERIFIER_INVARIANTBASE_H
#define ES2PANDA_COMPILER_CORE_AST_VERIFIER_INVARIANTBASE_H

#include "generated/options.h"
#include "ir/astNode.h"
#include "libarkbase/utils/json_builder.h"

namespace ark::es2panda::compiler::ast_verifier {

enum class CheckDecision { CORRECT, INCORRECT };
enum class CheckAction { CONTINUE, SKIP_SUBTREE };

using CheckResult = std::tuple<CheckDecision, CheckAction>;
using VerifierInvariants = util::gen::ast_verifier::Enum;
using Enum = VerifierInvariants;

template <typename... Invs>
class InvariantsRegistryImpl {
public:
    InvariantsRegistryImpl() : invariants_ {Invs(this)...} {}

    using Invariants = std::tuple<Invs...>;
    template <VerifierInvariants ID>
    using InvariantClass = std::tuple_element_t<ID, Invariants>;
    template <typename T>
    using InvArray = std::array<T, VerifierInvariants::COUNT>;

    template <VerifierInvariants ID>
    constexpr auto *Get()
    {
        return Get<InvariantClass<ID>>();
    }

    template <typename Inv>
    constexpr auto *Get()
    {
        return &std::get<Inv>(invariants_);
    }

    template <typename Func>
    auto Apply(Func &&f)
    {
        return std::apply(std::forward<Func>(f), invariants_);
    }

private:
    Invariants invariants_;

    template <typename T, T... INTS>
    static constexpr bool CheckRegistry(std::integer_sequence<T, INTS...> /*unused*/)
    {
        return ((CheckRegistry<VerifierInvariants(INTS), Invs::ID>()) && ...);
    }

    template <VerifierInvariants ORDER_IN_PARAMETER_LIST, VerifierInvariants DEFINED_ENUM>
    static constexpr bool CheckRegistry()
    {
        static_assert(ORDER_IN_PARAMETER_LIST == DEFINED_ENUM,
                      "Invariant's `ID` must be equal to"
                      "index of the invariant in `InvariantsRegistryImpl` parameter-list");
        return true;
    }

    static_assert(sizeof...(Invs) == VerifierInvariants::COUNT,
                  "Parameter-list is inconsistent with invaraints' declararation in 'options.yaml'");
    static_assert(CheckRegistry(std::make_index_sequence<sizeof...(Invs)> {}));
};

class CheckMessage {
public:
    explicit CheckMessage(util::StringView cause, const ir::AstNode *node) : cause_ {cause}, node_ {node} {}

    std::function<void(JsonObjectBuilder &)> DumpJSON() const
    {
        return [this](JsonObjectBuilder &body) {
            body.AddProperty("cause", cause_);
            body.AddProperty("ast", node_->DumpJSON());
            body.AddProperty("line", node_->Start().line + 1);
        };
    }

    std::string ToString() const
    {
        auto nodeLocation = std::string(ir::ToString(node_->Type()));
        auto parent = node_->Parent();
        while (parent != nullptr) {
            nodeLocation = std::string(ir::ToString(parent->Type())).append("->").append(nodeLocation);
            parent = parent->Parent();
        }

        return cause_ + "(" + nodeLocation + ", line " + std::to_string(node_->Start().line + 1) + ')';
    }

    const auto &Cause() const
    {
        return cause_;
    }

private:
    std::string cause_;
    const ir::AstNode *node_;
};

using Messages = std::vector<CheckMessage>;

class InvariantMessages {
public:
    void Init()
    {
        messages_.clear();
    }

    void AddCheckMessage(const std::string &cause, const ir::AstNode &node);

    auto &&MoveMessages() &&
    {
        return std::move(messages_);
    }

    const auto &ViewMessages() const
    {
        return messages_;
    }

    bool HasMessages() const
    {
        return !messages_.empty();
    }

private:
    Messages messages_;
};

template <VerifierInvariants ENUM, typename... RequiredInvs>
class RequiredInvariants {
public:
    template <typename InvariantsReg>
    explicit constexpr RequiredInvariants([[maybe_unused]] InvariantsReg *reg)
        : required_ {*reg->template Get<RequiredInvs>()...}
    {
    }

    const auto &GetRequired() const
    {
        return required_;
    }

protected:
    template <typename Inv>
    const auto &Get()
    {
        return std::get<Inv &>(required_);
    }

private:
    static_assert(((RequiredInvs::ID < ENUM) && ...), "'Required' invariants should precede the related invariant");
    std::tuple<RequiredInvs &...> required_;
};

template <VerifierInvariants ENUM, typename... RequiredInvs>
// NOLINTNEXTLINE(fuchsia-multiple-inheritance)
class InvariantBase : public RequiredInvariants<ENUM, RequiredInvs...>, public InvariantMessages {
public:
    using RequiredInvariants<ENUM, RequiredInvs...>::RequiredInvariants;
    using Base = InvariantBase<ENUM, RequiredInvs...>;
    constexpr static VerifierInvariants ID = ENUM;
    constexpr static std::string_view NAME = util::gen::ast_verifier::ToString(ID);
};

}  // namespace ark::es2panda::compiler::ast_verifier

#endif  // ES2PANDA_COMPILER_CORE_AST_VERIFIER_INVARIANTBASE_H
