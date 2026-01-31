/**
 * Copyright (c) 2021-2026 Huawei Device Co., Ltd.
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

#ifndef ES2PANDA_COMPILER_LOWERING_PHASE_H
#define ES2PANDA_COMPILER_LOWERING_PHASE_H

#include "libarkbase/macros.h"
#include "parser/program/program.h"
#include "public/public.h"
#include "phase_id.h"

namespace ark::es2panda::compiler {

class Phase {
public:
    /* If Apply returns false, processing is stopped. */
    bool Apply(public_lib::Context *ctx);

    virtual ~Phase() = default;
    Phase() = default;

    NO_COPY_SEMANTIC(Phase);
    NO_MOVE_SEMANTIC(Phase);

    virtual std::string_view Name() const = 0;

    virtual void Setup() {}

    virtual bool Precondition()
    {
        return true;
    }

    virtual bool Perform() = 0;

    virtual bool Postcondition()
    {
        return true;
    }

protected:
    auto *Context() const
    {
        return ctx_;
    }

    auto *DE() const
    {
        return Context()->diagnosticEngine;
    }

    const auto *Options() const
    {
        return ctx_->config->options;
    }

private:
    friend class PhaseManager;
    public_lib::Context *ctx_ {nullptr};

    int32_t id_ {INVALID_PHASE_ID};
};

template <typename ProgramsSelector>
class PhaseForSelectedPrograms : public Phase {
    bool Precondition() override
    {
        return ProgramsSelector::Apply(Context(), [this](auto *prog) { return PreconditionForProgram(prog); });
    }

    bool Perform() final
    {
        return ProgramsSelector::Apply(Context(), [this](auto *prog) { return PerformForProgram(prog); });
    }

    bool Postcondition() override
    {
        return ProgramsSelector::Apply(Context(), [this](auto *prog) { return PostconditionForProgram(prog); });
    }

    virtual bool PreconditionForProgram([[maybe_unused]] const parser::Program *program)
    {
        return true;
    }

    virtual bool PerformForProgram(parser::Program *program) = 0;

    virtual bool PostconditionForProgram([[maybe_unused]] const parser::Program *program)
    {
        return true;
    }
};

template <util::ModuleKind... PROGRAM_KINDS_TO_VISIT>
struct ProgramsByKindSelector {
    template <typename CB>
    static bool Apply(public_lib::Context *context, const CB &cb)
    {
        bool result = true;

        // 1. external sources:
        context->parserProgram->GetExternalSources()->template Visit<true, PROGRAM_KINDS_TO_VISIT...>(
            [&cb, &result](auto *extProg) { result &= extProg->IsASTLowered() || cb(extProg); });

        // 2. main program if match:
        if (((PROGRAM_KINDS_TO_VISIT == context->parserProgram->GetModuleKind()) || ...)) {
            result &= cb(context->parserProgram);
        }

        return result;
    }
};

// A helper for phases relying on "stdlib" and "simultaneous" compilation modes
struct ProgramsToBeEmittedSelector {
    template <typename CB>
    static bool Apply(public_lib::Context *context, const CB &cb)
    {
        auto mode = context->config->options->GetCompilationMode();

        parser::Program *program = context->parserProgram;

        if (mode == CompilationMode::GEN_STD_LIB) {
            program->GetExternalSources()->Visit([&cb](auto *extProg) {
                if (extProg->IsASTLowered()) {
                    return;
                }
                cb(extProg);
            });
        } else if (mode == CompilationMode::GEN_ABC_FOR_EXTERNAL_SOURCE) {
            program->GetExternalSources()->Visit([&cb](auto *extProg) {
                if (extProg->IsASTLowered() || !extProg->IsGenAbcForExternal()) {
                    return;
                }
                cb(extProg);
            });
        }

        cb(program);
        return true;
    }
};

// NOTE(dkofanov): 'PACKAGE' is to be removed.
using PhaseForSourcePrograms = PhaseForSelectedPrograms<
    ProgramsByKindSelector<util::ModuleKind::PACKAGE, util::ModuleKind::MODULE, util::ModuleKind::SOURCE_DECL>>;
using PhaseForAllPrograms =
    PhaseForSelectedPrograms<ProgramsByKindSelector<util::ModuleKind::PACKAGE, util::ModuleKind::MODULE,
                                                    util::ModuleKind::SOURCE_DECL, util::ModuleKind::ETSCACHE_DECL>>;

using PhaseForProgramsToBeEmitted = PhaseForSelectedPrograms<ProgramsToBeEmittedSelector>;

// NOTE(dkofanov): #32419.
// 'PhaseForProgramsWithBodies' is a replacement for 'PhaseForBodies'.
// It fact, 'PhaseForBodies' used to apply only to programs to be emitted, not to all the programs-with-bodies.
// While this behavior is incorrect and unexpected, 'PhaseForProgramsWithBodies_LEGACY' is introduced conservatively,
// which should be repalced with 'PhaseForProgramsWithBodies' or 'PhaseForProgramsToBeEmitted'.
// CC-OFFNXT(G.NAM.03-CPP) project codestyle
using PhaseForProgramsWithBodies_LEGACY = PhaseForProgramsToBeEmitted;
using PhaseForProgramsWithBodies =
    PhaseForSelectedPrograms<ProgramsByKindSelector<util::ModuleKind::PACKAGE, util::ModuleKind::MODULE>>;

class PhaseManager {
public:
    PhaseManager(ScriptExtension ext, ArenaAllocator *allocator) : allocator_ {allocator}, ext_ {ext}
    {
        InitializePhases();
        Reset();
    }

    PhaseManager(public_lib::Context *context, ScriptExtension ext, ArenaAllocator *allocator)
        : PhaseManager(ext, allocator)
    {
        context_ = context;
    }

    NO_COPY_SEMANTIC(PhaseManager);
    NO_MOVE_SEMANTIC(PhaseManager);

    ~PhaseManager();

    public_lib::Context *Context()
    {
        return context_;
    }

    PhaseId PreviousPhaseId() const
    {
        return prev_;
    }

    PhaseId CurrentPhaseId() const
    {
        return curr_;
    }

    void SetCurrentPhaseId(int32_t phaseId)
    {
        if (phaseId == curr_.minor) {
            return;
        }
        prev_ = curr_;

        if (curr_.minor > phaseId) {
            curr_.major++;
        }
        curr_.minor = phaseId;
    }

    void SetCurrentPhaseIdWithoutReCheck(int32_t phaseId)
    {
        if (phaseId == curr_.minor) {
            return;
        }
        curr_.major = 0;
        prev_ = {0, INVALID_PHASE_ID};
        curr_.minor = phaseId;
    }

    ArenaAllocator *Allocator() const
    {
        return allocator_;
    }

    bool IsInitialized() const
    {
        return allocator_ != nullptr && ext_ != ScriptExtension::INVALID;
    }

    void Reset();

    Phase *NextPhase()
    {
        if (next_ < static_cast<int32_t>(phases_.size())) {
            return phases_[next_++];
        }
        return nullptr;
    }

    std::vector<Phase *> AllPhases();
    std::vector<Phase *> RebindPhases();
    std::vector<Phase *> RecheckPhases();

    void SetCurrentPhaseIdToAfterParse()
    {
        GetPhaseManager()->SetCurrentPhaseId(jsPluginAfterParse_);
    }

    void SetCurrentPhaseIdToAfterBind()
    {
        GetPhaseManager()->SetCurrentPhaseId(jsPluginAfterBind_);
    }

    void SetCurrentPhaseIdToAfterCheck()
    {
        GetPhaseManager()->SetCurrentPhaseId(jsPluginAfterCheck_);
    }

    void SetCurrentPhaseIdToAfterLower()
    {
        GetPhaseManager()->SetCurrentPhaseId(jsPluginAfterLower_);
    }

    int32_t GetCurrentMajor() const;
    int32_t GetCurrentMinor() const;

    std::vector<Phase *> GetSubPhases(const std::vector<std::string_view> &phaseNames);

private:
    void InitializePhases();
    PhaseId prev_ {0, INVALID_PHASE_ID};
    PhaseId curr_ {0, INVALID_PHASE_ID};
    int32_t next_ {INVALID_PHASE_ID};
    int32_t jsPluginAfterParse_ {0};
    int32_t jsPluginAfterBind_ {0};
    int32_t jsPluginAfterCheck_ {0};
    int32_t jsPluginAfterLower_ {0};

    ArenaAllocator *allocator_ {nullptr};
    public_lib::Context *context_ {nullptr};
    ScriptExtension ext_ {ScriptExtension::INVALID};
    std::vector<Phase *> phases_;
};

}  // namespace ark::es2panda::compiler

#endif
