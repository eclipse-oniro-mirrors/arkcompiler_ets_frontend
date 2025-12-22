/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

#ifndef ES2PANDA_COMPILER_CORE_ASTVERIFIER_H
#define ES2PANDA_COMPILER_CORE_ASTVERIFIER_H

#include <algorithm>
#include <iterator>
#include <regex>
#include <string>
#include <unordered_set>

#include "ast_verifier/invariantBase.h"
#include "ast_verifier/invariants/sequenceExpressionHasLastType.h"
#include "ast_verifier/invariants/checkAbstractMethod.h"
#include "ast_verifier/invariants/everyChildHasValidParent.h"
#include "ast_verifier/invariants/everyChildInParentRange.h"
#include "ast_verifier/invariants/getterSetterValidation.h"
#include "ast_verifier/invariants/identifierHasVariable.h"
#include "ast_verifier/invariants/nodeHasParent.h"
#include "ast_verifier/invariants/nodeHasSourceRange.h"
#include "ast_verifier/invariants/nodeHasType.h"
#include "ast_verifier/invariants/referenceTypeAnnotationIsNull.h"
#include "ast_verifier/invariants/variableHasScope.h"
#include "ast_verifier/invariants/variableHasEnclosingScope.h"
#include "ast_verifier/invariants/forLoopCorrectlyInitialized.h"
#include "ast_verifier/invariants/modifierAccessValid.h"
#include "ast_verifier/invariants/importExportAccessValid.h"
#include "ast_verifier/invariants/arithmeticOperationValid.h"
#include "ast_verifier/invariants/variableNameIdentifierNameSame.h"
#include "ast_verifier/invariants/checkScopeDeclaration.h"
#include "ast_verifier/invariants/checkStructDeclaration.h"
#include "ast_verifier/invariants/checkConstProperties.h"

#include "ir/astNode.h"
#include "ir/statements/blockStatement.h"
#include "lexer/token/sourceLocation.h"
#include "parser/program/program.h"
#include "util/ustring.h"
#include "util/options.h"
#include "libarkbase/utils/arena_containers.h"
#include "varbinder/variable.h"
#include "public/public.h"

#ifdef ASTV_ENABLE_LOGGING
// CC-OFFNXT(G.PRE.02) macro to enable conditionally
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_ASTV(lvl, msg) LOG(lvl, ES2PANDA) << "[ASTV] " << msg
#else
// CC-OFFNXT(G.PRE.02) macro to enable conditionally
#define LOG_ASTV(lvl, msg)
#endif  // ASTV_ENABLE_LOGGING

namespace ark::es2panda::compiler::ast_verifier {

// NOTE(dkofanov) Fix and enable ImportExportAccessValid:
using InvariantsRegistry =
    InvariantsRegistryImpl<NodeHasParent, NodeHasSourceRange, EveryChildHasValidParent, EveryChildInParentRange,
                           CheckStructDeclaration, VariableHasScope, NodeHasType, NoPrimitiveTypes, EnumHasCorrectType,
                           IdentifierHasVariable, ReferenceTypeAnnotationIsNull, ArithmeticOperationValid,
                           SequenceExpressionHasLastType, ForLoopCorrectlyInitialized, VariableHasEnclosingScope,
                           ModifierAccessValid, VariableNameIdentifierNameSame, CheckAbstractMethod,
                           GetterSetterValidation, CheckScopeDeclaration, CheckConstProperties>;

/*
 * ASTVerifier checks whether various conditions are invariant (across AST transformations).
 */
class ASTVerifier : public InvariantsRegistry {
public:
    NO_COPY_SEMANTIC(ASTVerifier);
    NO_MOVE_SEMANTIC(ASTVerifier);

    ASTVerifier(const public_lib::Context &context, const parser::Program &program)
        : program_ {program}, context_ {context}
    {
        for (size_t i = VerifierInvariants::BASE_FIRST; i <= VerifierInvariants::BASE_LAST; i++) {
            allowed_[i] = true;
        }
        for (size_t i = 0; i < VerifierInvariants::COUNT; i++) {
            enabled_[i] = TreatAsWarning(VerifierInvariants {i}) || TreatAsError(VerifierInvariants {i});
        }
    }

    ~ASTVerifier()
    {
        ES2PANDA_ASSERT(!HasErrors());
        ES2PANDA_ASSERT(!HasWarnings());
    }

    void After()
    {
        if (Options().IsAstVerifierAfterPhases()) {
            Verify("after");
        }
        if (!suppressed_ && (HasErrors() || HasWarnings())) {
            DumpMessages();
            hasErrors_ = false;
            hasWarnings_ = false;
        }
    }

    void Before()
    {
        if (Options().IsAstVerifierBeforePhases()) {
            Verify("before");
        }
    }

    void Verify(std::string_view phaseName) noexcept;

    void IntroduceNewInvariants(std::string_view occurredPhaseName)
    {
        if (occurredPhaseName == "plugins-after-parse") {
            for (size_t i = VerifierInvariants::AFTER_PLUGINS_AFTER_PARSE_FIRST;
                 i <= VerifierInvariants::AFTER_PLUGINS_AFTER_PARSE_LAST; i++) {
                allowed_[i] = true;
            }
        }
        if (occurredPhaseName == "ScopesInitPhase") {
            for (size_t i = VerifierInvariants::AFTER_SCOPES_INIT_PHASE_FIRST;
                 i <= VerifierInvariants::AFTER_SCOPES_INIT_PHASE_LAST; i++) {
                allowed_[i] = true;
            }
        }
        if (occurredPhaseName == "CheckerPhase") {
            for (size_t i = VerifierInvariants::AFTER_CHECKER_PHASE_FIRST;
                 i <= VerifierInvariants::AFTER_CHECKER_PHASE_LAST; i++) {
                allowed_[i] = true;
            }
        }
        if (occurredPhaseName == "Unbox") {
            Get<NoPrimitiveTypes>()->SetNumberLoweringOccured();
        }
        if (occurredPhaseName == "UnionLowering") {
            Get<IdentifierHasVariable>()->SetUnionLoweringOccurred();
        }
    }

    void Suppress()
    {
        suppressed_ = true;
    }

    void DumpMessages() const;

    bool TreatAsWarning(VerifierInvariants id) const
    {
        return Options().GetAstVerifierWarnings()[id];
    }
    bool TreatAsError(VerifierInvariants id) const
    {
        return Options().GetAstVerifierErrors()[id];
    }
    bool HasErrors() const
    {
        return hasErrors_;
    }
    bool HasWarnings() const
    {
        return hasWarnings_;
    }

private:
    template <typename T, std::enable_if_t<std::is_base_of_v<InvariantMessages, T>, void *> = nullptr>
    bool NeedCheckInvariant(const T & /*unused*/)
    {
        return enabled_[T::ID] && allowed_[T::ID];
    }

    const util::Options &Options() const
    {
        return *context_.config->options;
    }

public:
    using SourcePath = std::string_view;
    using PhaseName = std::string_view;
    using InvariantsMessages = std::map<VerifierInvariants, Messages>;
    using WarningsErrors = std::map<std::string_view, InvariantsMessages>;
    using SourceMessages = std::map<SourcePath, WarningsErrors>;
    using GroupedMessages = std::vector<std::pair<PhaseName, SourceMessages>>;

private:
    const parser::Program &program_;
    const public_lib::Context &context_;
    InvArray<bool> enabled_ {};
    InvArray<bool> allowed_ {};

    bool hasErrors_ {false};
    bool hasWarnings_ {false};
    bool suppressed_ {false};
    GroupedMessages report_;

    struct SinglePassVerifier;
};

}  // namespace ark::es2panda::compiler::ast_verifier

#endif  // ES2PANDA_COMPILER_CORE_ASTVERIFIER_H
