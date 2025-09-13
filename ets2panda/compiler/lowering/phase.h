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
    bool Apply(public_lib::Context *ctx, parser::Program *program);

    virtual ~Phase() = default;
    Phase() = default;

    NO_COPY_SEMANTIC(Phase);
    NO_MOVE_SEMANTIC(Phase);

    virtual std::string_view Name() const = 0;

    virtual void FetchCache([[maybe_unused]] public_lib::Context *ctx, [[maybe_unused]] parser::Program *program) {}

    virtual bool Precondition([[maybe_unused]] public_lib::Context *ctx,
                              [[maybe_unused]] const parser::Program *program)
    {
        return true;
    }
    virtual bool Perform(public_lib::Context *ctx, parser::Program *program) = 0;
    virtual bool Postcondition([[maybe_unused]] public_lib::Context *ctx,
                               [[maybe_unused]] const parser::Program *program)
    {
        return true;
    }

private:
    friend class PhaseManager;
    int32_t id_ {INVALID_PHASE_ID};
};

/* Phase that modifies declarations. Need to process external dependencies */
class PhaseForDeclarations : public Phase {
    bool Precondition(public_lib::Context *ctx, const parser::Program *program) override;
    bool Perform(public_lib::Context *ctx, parser::Program *program) override;
    bool Postcondition(public_lib::Context *ctx, const parser::Program *program) override;

    /* Called from Perform, Pre/Postcondition */
    virtual bool PreconditionForModule([[maybe_unused]] public_lib::Context *ctx,
                                       [[maybe_unused]] const parser::Program *program)
    {
        return true;
    }
    virtual bool PerformForModule(public_lib::Context *ctx, parser::Program *program) = 0;
    virtual bool PostconditionForModule([[maybe_unused]] public_lib::Context *ctx,
                                        [[maybe_unused]] const parser::Program *program)
    {
        return true;
    }
};

/* Phase that only modifies bodies.
   No need to process external dependencies unless we are compiling stdlib.
*/
class PhaseForBodies : public Phase {
    bool Precondition(public_lib::Context *ctx, const parser::Program *program) override;
    bool ProcessExternalPrograms(public_lib::Context *ctx, parser::Program *program);
    bool Perform(public_lib::Context *ctx, parser::Program *program) override;
    bool Postcondition(public_lib::Context *ctx, const parser::Program *program) override;

    /* Called from Perform, Pre/Postcondition */
    virtual bool PreconditionForModule([[maybe_unused]] public_lib::Context *ctx,
                                       [[maybe_unused]] const parser::Program *program)
    {
        return true;
    }
    virtual bool PerformForModule(public_lib::Context *ctx, parser::Program *program) = 0;
    virtual bool PostconditionForModule([[maybe_unused]] public_lib::Context *ctx,
                                        [[maybe_unused]] const parser::Program *program)
    {
        return true;
    }
};

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
