/**
 * Copyright (c) 2023-2026 Huawei Device Co., Ltd.
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

#include "ASTVerifier.h"

namespace ark::es2panda::compiler::ast_verifier {

using AstToCheck = ArenaMap<ASTVerifier::SourcePath, const ir::AstNode *>;

template <typename Inv>
auto VerifyNode(Inv *inv, const ir::AstNode *node)
{
    auto [res, action] = (*inv)(node);
    if (action == CheckAction::SKIP_SUBTREE) {
        LOG_ASTV(DEBUG, Inv::NAME << ": SKIP_SUBTREE");
    }
    return CheckResult {res, action};
}

struct ASTVerifier::SinglePassVerifier {
    // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
    ASTVerifier *verifier {nullptr};
    bool *astCorrect;
    // NOLINTEND(misc-non-private-member-variables-in-classes)

    void operator()(ir::AstNode *ncnode) const
    {
        const auto *node = ncnode;
        auto enabledSave = verifier->enabled_;
        LOG_ASTV(DEBUG, "Verify: " << ir::ToString(node->Type()));

        verifier->Apply([this, node](auto &...inv) {
            InvArray<CheckDecision> decisions {};
            InvArray<CheckAction> actions {};
            ((std::tie(decisions[inv.ID], actions[inv.ID]) =
                  verifier->NeedCheckInvariant(inv) ? VerifyNode(&inv, node)
                                                    : CheckResult {CheckDecision::CORRECT, CheckAction::SKIP_SUBTREE}),
             ...);
            // Temporaly disable invariant, the value should be restored after node and its childs are visited:
            ((verifier->enabled_[inv.ID] &= (actions[inv.ID] == CheckAction::CONTINUE)), ...);

            for (size_t i = 0; i < VerifierInvariants::COUNT; i++) {
                LOG_ASTV(DEBUG, (actions[i] == CheckAction::CONTINUE ? "Enabled " : "Disabled ")
                                    << util::gen::ast_verifier::ToString(VerifierInvariants {i}));
            }

            (*astCorrect) &= ((decisions[inv.ID] == CheckDecision::CORRECT) && ...);
        });

        node->Iterate(*this);
        verifier->enabled_ = enabledSave;
    }
};

static auto ExtractASTs(const public_lib::Context &context)
{
    auto program = context.parserProgram;
    auto *allocator = program->Allocator();
    auto astToCheck = AstToCheck {allocator->Adapter()};
    astToCheck.insert(std::make_pair(program->SourceFilePath().Utf8(), program->Ast()));
    if (context.config->options->IsAstVerifierFullProgram()) {
        program->GetExternalSources()->Visit([&astToCheck](parser::Program *external) {
            ES2PANDA_ASSERT(external->Ast() != nullptr);
            astToCheck.insert(std::make_pair(external->SourceFilePath().Utf8(), external->Ast()));
        });
    }
    return astToCheck;
}

void ASTVerifier::Verify(std::string_view phaseName) noexcept
{
    if (context_.diagnosticEngine->IsAnyError()) {
        // NOTE(dkofanov): As for now, the policy is that ASTVerifier doesn't interrupt pipeline if there were errors
        // reported. Should be revisited.
        Suppress();
    }
    if (suppressed_) {
        return;
    }
    auto astsToCheck = ExtractASTs(context_);
    for (const auto &p : astsToCheck) {
        const auto sourceName = p.first;
        const auto *ast = p.second;
        Apply([](auto &&...inv) { ((inv.Init()), ...); });

        LOG_ASTV(INFO, "Begin traversal (" << sourceName << ')');

        bool astCorrect = true;
        // `const_cast` due to `ir::NodeTraverser` signature:
        SinglePassVerifier {this, &astCorrect}(const_cast<ir::AstNode *>(ast));

        auto reporter = [this, sourceName](auto &&inv) {
            if (inv.HasMessages()) {
                report_.back().second[sourceName][TreatAsError(inv.ID) ? "errors" : "warnings"][inv.ID] =
                    std::forward<InvariantMessages>(inv).MoveMessages();
                (TreatAsError(inv.ID) ? hasErrors_ : hasWarnings_) = true;
            }
        };
        ES2PANDA_ASSERT(astCorrect == Apply([](const auto &...inv) { return ((!inv.HasMessages()) && ...); }));
        if (!astCorrect) {
            if (report_.empty() || report_.back().first != phaseName) {
                report_.emplace_back();
                report_.back().first = phaseName;
            }
            Apply([&reporter](auto &...inv) { (reporter(std::move(inv)), ...); });
        }
    }
}

template <typename K, typename V>
void JsonAddProperty(JsonObjectBuilder &outer, K k, const V &v)
{
    outer.AddProperty(k, [&v](JsonObjectBuilder &inner) {
        for (const auto &[kInner, vInner] : v) {
            JsonAddProperty(inner, kInner, vInner);
        }
    });
}

template <>
void JsonAddProperty<VerifierInvariants, Messages>(JsonObjectBuilder &outer, VerifierInvariants k, const Messages &v)
{
    outer.AddProperty(util::gen::ast_verifier::ToString(k), [&v](JsonArrayBuilder &msgsBuilder) {
        for (const auto &msg : v) {
            msgsBuilder.Add(msg.DumpJSON());
        }
    });
}

void DumpJson(const ASTVerifier::GroupedMessages &report, const std::string &filePath)
{
    JsonObjectBuilder reportJson {};
    for (const auto &[phase, sourceMessages] : report) {
        JsonAddProperty(reportJson, phase, sourceMessages);
    }
    auto str = std::move(reportJson).Build();
    std::ofstream(filePath, std::ios::trunc | std::ios::out).write(str.data(), str.size());
}

template <bool IS_WARNING>
void DumpLogMessage(VerifierInvariants id, const Messages &msgs)
{
    auto invaraintName = util::gen::ast_verifier::ToString(id);
    if constexpr (IS_WARNING) {
        LOG(WARNING, ES2PANDA) << "    " << invaraintName << ':';
    } else {
        LOG(ERROR, ES2PANDA) << "    " << invaraintName << ':';
    }

    for (const auto &msg : msgs) {
        if constexpr (IS_WARNING) {
            LOG(WARNING, ES2PANDA) << "      " << msg.ToString();
        } else {
            LOG(ERROR, ES2PANDA) << "      " << msg.ToString();
        }
    }
}

template <Logger::Level LEVEL>
void DumpLogAstMessages(std::string_view path, const ASTVerifier::WarningsErrors &astMessages)
{
    static_assert((LEVEL == Logger::Level::WARNING) || (LEVEL == Logger::Level::ERROR));
    constexpr bool IS_WARNING = LEVEL == Logger::Level::WARNING;

    auto severityStr = IS_WARNING ? "warnings" : "errors";
    if (astMessages.find(severityStr) != astMessages.end()) {
        if constexpr (IS_WARNING) {
            LOG(WARNING, ES2PANDA) << "  In " << path << ':';
        } else {
            LOG(ERROR, ES2PANDA) << "  In " << path << ':';
        }
        for (const auto &[invaraintId, messages] : astMessages.at(severityStr)) {
            DumpLogMessage<IS_WARNING>(invaraintId, messages);
        }
    }
}

void DumpLog(const ASTVerifier::GroupedMessages &report)
{
    for (const auto &[phase, sourceMessages] : report) {
        LOG(WARNING, ES2PANDA) << "After " << phase << ':';
        for (const auto &[path, astMessages] : sourceMessages) {
            DumpLogAstMessages<Logger::Level::WARNING>(path, astMessages);
            DumpLogAstMessages<Logger::Level::ERROR>(path, astMessages);
        }
    }
}

void ASTVerifier::DumpMessages() const
{
    std::string errMsg = "ASTVerifier found broken invariants.";
    if (Options().IsAstVerifierJson()) {
        DumpJson(report_, Options().GetAstVerifierJsonPath());
        errMsg += " Dumped to '" + std::string(Options().GetAstVerifierJsonPath()) + "'.";
    } else {
        DumpLog(report_);
        errMsg += " You may want to pass '--ast-verifier:json' option for more verbose output.";
    }

    if (hasErrors_) {
        LOG(FATAL, ES2PANDA) << errMsg;
    } else if (hasWarnings_) {
        LOG(WARNING, ES2PANDA) << errMsg;
    }
}

void InvariantMessages::AddCheckMessage(const std::string &cause, const ir::AstNode &node)
{
    messages_.emplace_back(cause.data(), &node);
}

}  // namespace ark::es2panda::compiler::ast_verifier
