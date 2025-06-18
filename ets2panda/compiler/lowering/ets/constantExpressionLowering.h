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

#ifndef ES2PANDA_COMPILER_LOWERING_CONSTANT_EXPRESSION_LOWERING_H
#define ES2PANDA_COMPILER_LOWERING_CONSTANT_EXPRESSION_LOWERING_H

#include "compiler/lowering/phase.h"

namespace ark::es2panda::compiler {

inline bool IsValidNumberLiteral(const ir::Literal *lit)
{
    if (!lit->IsNumberLiteral()) {
        return false;
    }

    return !lit->AsNumberLiteral()->Number().ConversionError();
}

inline ir::Literal *AsSupportedLiteral(ir::Expression *const node)
{
    if (!node->IsLiteral()) {
        return nullptr;
    }

    auto literal = node->AsLiteral();
    if (IsValidNumberLiteral(literal) || literal->IsCharLiteral() || literal->IsBooleanLiteral() ||
        literal->IsStringLiteral()) {
        return literal;
    }
    return nullptr;
}

class NodeCalculator;

class ConstantExpressionLoweringImpl {
public:
    explicit ConstantExpressionLoweringImpl(public_lib::Context *context) : context_(context) {}

    bool PerformForModule(parser::Program *program, std::string_view name);

    using Variable = varbinder::Variable;

private:
    void PopulateDAGs(ir::Expression *node);

    using DNId = uint32_t;
    inline static constexpr DNId INVALID_DNID = -1;

    struct DAGNode {
        using InputsT = std::vector<DNId>;
        using UsersT = std::vector<DNId>;
        explicit DAGNode(ir::Expression *expr) : irNode_ {expr} {}
        bool AllowsCalculationAttempt()
        {
            return !dnodeInputs_.empty();
        }
        bool IsCalculated()
        {
            return AsSupportedLiteral(irNode_) != nullptr;
        }

        auto Ir()
        {
            return irNode_;
        }

        auto SetIr(ir::Expression *node)
        {
            irNode_ = node;
        }

        auto *InputsIds()
        {
            return &dnodeInputs_;
        }
        auto *UsersIds()
        {
            return &dnodeUsers_;
        }

    private:
        ir::Expression *irNode_;
        InputsT dnodeInputs_ {};
        UsersT dnodeUsers_ {};
    };

    void AddRootDNode(ir::Expression *root)
    {
        calculationArguments_.push_back(FindOrInsertElement(root));
    }

    void AddDNode(ir::Expression *node, ir::Expression *input)
    {
        AddDNode(node, {input});
    }

    void AddDNode(ir::Expression *node, std::initializer_list<ir::Expression *> inputs)
    {
        ES2PANDA_ASSERT(inputs.size() != 0);
        auto nodeId = FindOrInsertElement(node);
        for (auto input : inputs) {
            AddInput(nodeId, FindOrInsertElement(input));
        }
    }

    template <typename T, typename = typename T::iterator>
    void AddDNode(ir::TemplateLiteral *node, const T &inputs)
    {
        ES2PANDA_ASSERT(!inputs.empty());  // trivial template should be handled via pretransform.
        auto nodeId = FindOrInsertElement(node);
        for (auto input : inputs) {
            AddInput(nodeId, FindOrInsertElement(input));
        }
    }

    void AddDNodeToPretransform(ir::Expression *node)
    {
        pretransformQueue_.push_back(FindOrInsertElement(node));
    }

    void Pretransform()
    {
        for (auto nodeId : pretransformQueue_) {
            auto *dnode = DNode(nodeId);
            if (dnode->Ir()->IsTemplateLiteral()) {
                if (CalculateAndCheck(DNode(nodeId))) {
                    calculationArguments_.push_back(nodeId);
                }
            } else if (dnode->Ir()->IsNumberLiteral()) {
                // Replace broken literal to prevent further errors.
                auto broken = util::NodeAllocator::Alloc<ir::Identifier>(context_->Allocator(), context_->Allocator());
                ES2PANDA_ASSERT(broken->IsErrorPlaceHolder());
                RegisterReplacement(dnode, broken);
            } else {
                ES2PANDA_UNREACHABLE();
            }
        }
    }

    bool PerformStep()
    {
        decltype(calculationArguments_) newCalculationArguments {};
        newCalculationArguments.reserve(calculationArguments_.size());
        bool foldingOccurred = false;

        for (auto calculationArgumentId : calculationArguments_) {
            auto users = DNode(calculationArgumentId)->UsersIds();
            for (auto &userIdRef : *users) {
                if (auto userId = userIdRef; (userId != INVALID_DNID) && TryCalculateUser(&userIdRef, DNode(userId))) {
                    ES2PANDA_ASSERT(userIdRef == INVALID_DNID);
                    ES2PANDA_ASSERT(DNode(userId)->InputsIds()->empty());
                    newCalculationArguments.push_back(userId);
                    foldingOccurred = true;
                }
            }
            if (!std::all_of(users->begin(), users->end(), [](auto uid) { return uid == INVALID_DNID; })) {
                // Requeue the input-node:
                newCalculationArguments.push_back(calculationArgumentId);
            } else {
                // Speedup iteration over users in case the node was queued for the following algorithm step:
                users->clear();
            }
        }

        calculationArguments_ = std::move(newCalculationArguments);
        return foldingOccurred;
    }

    bool TryCalculateUser(DNId *edgeToUser, DAGNode *user)
    {
        ES2PANDA_ASSERT(DNode(*edgeToUser) == user);
        if (!user->AllowsCalculationAttempt()) {
            // Was calculated and requeued via another input.
            *edgeToUser = INVALID_DNID;
            return false;
        }

        for (auto iid : *user->InputsIds()) {
            if (!DNode(iid)->IsCalculated()) {
                // Node isn't ready yet:
                return false;
            }
        }

        auto res = CalculateAndCheck(user);
        user->InputsIds()->clear();
        // The rest edges will be cleaned on their visit.
        *edgeToUser = INVALID_DNID;
        return res;
    }

    bool CalculateAndCheck(DAGNode *user);

    void RegisterReplacement(DAGNode *node, ir::Expression *replacement)
    {
        auto old = node->Ir();

        ES2PANDA_ASSERT(replacements_.find(old) == replacements_.end());
        replacements_[old] = replacement;
        node->SetIr(replacement);
    }

    DAGNode *DNode(DNId id)
    {
        ES2PANDA_ASSERT(id != INVALID_DNID);
        return &dNodes_[id];
    }

    DNId FindElement(ir::Expression *node)
    {
        ES2PANDA_ASSERT(node != nullptr);
        if (node2DNode_.find(node) != node2DNode_.end()) {
            return node2DNode_[node];
        }
        return INVALID_DNID;
    }

    DNId FindOrInsertElement(ir::Expression *node)
    {
        if (auto id = FindElement(node); id != INVALID_DNID) {
            return id;
        }
        dNodes_.emplace_back(node->AsExpression());
        auto id = dNodes_.size() - 1;
        node2DNode_[node] = id;
        return id;
    }

    void AddInput(DNId node, DNId input)
    {
        dNodes_[node].InputsIds()->push_back(input);
        dNodes_[input].UsersIds()->push_back(node);
    }

    public_lib::Context *context_ {nullptr};
    // Base mapping
    std::vector<DAGNode> dNodes_ {};
    std::map<ir::Expression *, DNId> node2DNode_ {};
    // Nodes to iterate over at each step:
    std::vector<DNId> calculationArguments_ {};
    // Nodes with no inputs which lack "normalization" (i.e. need to be somehow transformed to be used as arguments):
    std::vector<DNId> pretransformQueue_ {};
    // Result:
    std::map<ir::Expression *, ir::Expression *> replacements_ {};

    friend NodeCalculator;
};

class ConstantExpressionLowering : public PhaseForDeclarations {
    std::string_view Name() const override
    {
        return "ConstantExpressionLowering";
    }

    bool PerformForModule(public_lib::Context *ctx, parser::Program *program) override
    {
        ConstantExpressionLoweringImpl cf {ctx};
        return cf.PerformForModule(program, Name());
    }
};

}  // namespace ark::es2panda::compiler

#endif  // ES2PANDA_COMPILER_LOWERING_CONSTANT_EXPRESSION_LOWERING_H
