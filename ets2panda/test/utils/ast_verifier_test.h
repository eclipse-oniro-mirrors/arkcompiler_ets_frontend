/**
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

#ifndef ES2PANDA_TEST_UTILS_AST_VERIFIER_TEST_H
#define ES2PANDA_TEST_UTILS_AST_VERIFIER_TEST_H

#include "ast_verifier/ASTVerifier.h"
#include "panda_executable_path_getter.h"
#include "compiler/lowering/phase.h"

#include <gtest/gtest.h>

namespace ir_alias = ark::es2panda::ir;
namespace verifier_alias = ark::es2panda::compiler::ast_verifier;

class LSPAPITests;

namespace test::utils {

// CC-OFFNXT(G.PRE.02) macro to improve readability
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CONTEXT(...) if (ContextHolder<ContextFromStringExtractor> h {this, __VA_ARGS__}; true)

// CC-OFFNXT(G.PRE.02) macro to improve readability
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CONTEXT_FROM_FILE(...) if (ContextHolder<ContextFromFileExtractor> h {this, __VA_ARGS__}; true)

// NOLINTNEXTLINE(fuchsia-multiple-inheritance)
class AstVerifierTest : public testing::Test, public verifier_alias::InvariantsRegistry {
public:
    AstVerifierTest();
    NO_COPY_SEMANTIC(AstVerifierTest);
    NO_MOVE_SEMANTIC(AstVerifierTest);
    ~AstVerifierTest() override;

    ark::ArenaAllocator *Allocator() const
    {
        return allocator_;
    }

    auto *GetChecker()
    {
        return reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx_)->GetChecker()->AsETSChecker();
    }

    auto *GetAst()
    {
        return reinterpret_cast<ir_alias::AstNode *>(impl_->ProgramAst(ctx_, impl_->ContextProgram(ctx_)));
    }

    auto *GetCfg()
    {
        return reinterpret_cast<ark::es2panda::parser::Program *>(impl_->ContextProgram(ctx_))->GetCFG();
    }

    auto *GetImpl()
    {
        return impl_;
    }

    auto *GetContext()
    {
        return ctx_;
    }

    void AstNodeForEach(void (*func)(es2panda_AstNode *, void *), void *arg)
    {
        impl_->AstNodeForEach(reinterpret_cast<es2panda_AstNode *>(GetAst()), func, arg);
    }

    auto ContextErrorMessage()
    {
        return std::string(impl_->ContextErrorMessage(ctx_));
    }

    struct ContextFromStringExtractor {
        ContextFromStringExtractor(AstVerifierTest *fixture, char const *source, char const *fileName = "dummy.ets")
        {
            ASSERT(fixture->ctx_ == nullptr);
            fixture->ctx_ = fixture->impl_->CreateContextFromString(fixture->cfg_, source, fileName);
        }
    };

    struct ContextFromFileExtractor {
        ContextFromFileExtractor(AstVerifierTest *fixture, char const *fileName)
        {
            ASSERT(fixture->ctx_ == nullptr);
            fixture->ctx_ = fixture->impl_->CreateContextFromFile(fixture->cfg_, fileName);
        }
    };

    template <typename ContextExtractor>
    class ContextHolder : private ContextExtractor {
    public:
        NO_COPY_SEMANTIC(ContextHolder);
        NO_MOVE_SEMANTIC(ContextHolder);

        template <typename... ExtractorArgs>
        ContextHolder(AstVerifierTest *fixture, es2panda_ContextState state, ExtractorArgs &&...args)
            : ContextHolder(fixture, state, state, std::forward<ExtractorArgs>(args)...)
        {
        }

        template <typename... ExtractorArgs>
        ContextHolder(AstVerifierTest *fixture, es2panda_ContextState target, es2panda_ContextState expected,
                      ExtractorArgs &&...args)
            : ContextExtractor(fixture, std::forward<ExtractorArgs>(args)...), fixture_ {fixture}
        {
            ASSERT(target != ES2PANDA_STATE_ERROR);
            fixture_->impl_->ProceedToState(fixture_->ctx_, target);
            EXPECT_EQ(fixture_->impl_->ContextState(fixture_->ctx_), expected)
                << fixture_->impl_->ContextErrorMessage(fixture_->ctx_);
            ;
        }

        ~ContextHolder()
        {
            ASSERT(fixture_->ctx_ != nullptr);
            fixture_->impl_->DestroyContext(fixture_->ctx_);
            fixture_->ctx_ = nullptr;
        }

    private:
        AstVerifierTest *fixture_ {};
    };

    class ExpectVerifierMessage {
    public:
        ExpectVerifierMessage() = default;
        // NOLINTNEXTLINE(*-explicit-constructor)
        explicit ExpectVerifierMessage(std::initializer_list<std::string> l) : l_(l) {}

        bool CheckMessages(const verifier_alias::Messages &messages)
        {
            if (messages.size() != l_.size()) {
                std::cerr << "Expected " << l_.size() << " messages, got " << messages.size() << std::endl;
                if (!messages.empty()) {
                    std::cerr << "Got messages:" << std::endl;
                }
                for (const auto &msg : messages) {
                    std::cerr << " " << msg.ToString() << std::endl;
                }
                return false;
            }
            auto res = true;
            // NOLINTNEXTLINE(modernize-loop-convert)
            for (size_t i = 0; i < messages.size(); i++) {
                if (messages[i].ToString().find(l_.begin()[i]) == std::string::npos) {
                    std::cerr << "Expected '" << l_.begin()[i] << "', got '" << messages[i].ToString() << "'"
                              << std::endl;
                    res = false;
                }
            }
            return res;
        }

    private:
        std::initializer_list<std::string> l_;
    };

    template <typename Invariant>
    [[nodiscard]] auto Verify(ExpectVerifierMessage expected = {})
    {
        ASSERT(ctx_ != nullptr);
        auto *inv = Get<Invariant>();
        inv->Init();

        std::function<void(const ir_alias::AstNode *)> aux {};
        aux = [inv, &aux](const ir_alias::AstNode *child) -> void {
            // Required invariants need to be manually called in tests:
            std::apply([child](auto &...requiredInv) { (((void)(requiredInv)(child)), ...); }, inv->GetRequired());
            const auto [_, action] = (*inv)(child);
            if (action == verifier_alias::CheckAction::SKIP_SUBTREE) {
                return;
            }
            child->Iterate(aux);
        };
        aux(GetAst());
        return expected.CheckMessages(inv->ViewMessages());
    }

    template <typename Invariant>
    [[nodiscard]] auto VerifyNode(const ir_alias::AstNode *ast, ExpectVerifierMessage expected = {})
    {
        auto *inv = Get<Invariant>();
        inv->Init();
        // Required invariants need to be manually called in tests:
        std::apply([ast](auto &...requiredInv) { (((void)(requiredInv)(ast)), ...); }, inv->GetRequired());
        (void)(*inv)(ast);
        return expected.CheckMessages(inv->ViewMessages());
    }

private:
    es2panda_Impl const *impl_ {};
    es2panda_Config *cfg_ {};
    es2panda_Context *ctx_ {};
    ark::ArenaAllocator *allocator_;
    ark::es2panda::compiler::PhaseManager *phaseManager_;

    friend class ::LSPAPITests;
};

}  // namespace test::utils

#endif  // ES2PANDA_TEST_UTILS_AST_VERIFIER_TEST_H
