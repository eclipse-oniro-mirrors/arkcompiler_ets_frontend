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

#include "phase.h"

#include "checker/checker.h"
#include "compiler/lowering/checkerPhase.h"
#include "compiler/lowering/ets/asyncMethodLowering.h"
#include "compiler/lowering/ets/annotationCopyLowering.h"
#include "compiler/lowering/ets/annotationCopyPostLowering.h"
#include "compiler/lowering/ets/ambientLowering.h"
#include "compiler/lowering/ets/arrayLiteralLowering.h"
#include "compiler/lowering/ets/bigintLowering.h"
#include "compiler/lowering/ets/binaryExpressionLowering.h"
#include "compiler/lowering/ets/boxingForLocals.h"
#include "compiler/lowering/ets/capturedVariables.h"
#include "compiler/lowering/ets/constantExpressionLowering.h"
#include "compiler/lowering/ets/convertPrimitiveCastMethodCall.h"
#include "compiler/lowering/ets/declareOverloadLowering.h"
#include "compiler/lowering/ets/cfgBuilderPhase.h"
#include "compiler/lowering/ets/declGenPhase.h"
#include "compiler/lowering/ets/defaultParametersInConstructorLowering.h"
#include "compiler/lowering/ets/defaultParametersLowering.h"
#include "compiler/lowering/ets/enumLowering.h"
#include "compiler/lowering/ets/enumPostCheckLowering.h"
#include "compiler/lowering/ets/enumPropertiesInAnnotationsLowering.h"
#include "compiler/lowering/ets/restTupleLowering.h"
#include "compiler/lowering/ets/expandBrackets.h"
#include "compiler/lowering/ets/exportAnonymousConst.h"
#include "compiler/lowering/ets/expressionLambdaLowering.h"
#include "compiler/lowering/ets/extensionAccessorLowering.h"
#include "compiler/lowering/ets/genericBridgesLowering.h"
#include "compiler/lowering/ets/initModuleLowering.h"
#include "compiler/lowering/ets/insertOptionalParametersAnnotation.h"
#include "compiler/lowering/ets/interfaceObjectLiteralLowering.h"
#include "compiler/lowering/ets/interfacePropertyDeclarations.h"
#include "compiler/lowering/ets/lambdaLowering.h"
#include "compiler/lowering/ets/dynamicImport.h"
#include "compiler/lowering/ets/objectIndexAccess.h"
#include "compiler/lowering/ets/objectIterator.h"
#include "compiler/lowering/ets/objectLiteralLowering.h"
#include "compiler/lowering/ets/opAssignment.h"
#include "compiler/lowering/ets/optionalArgumentsLowering.h"
#include "compiler/lowering/ets/optionalLowering.h"
#include "compiler/lowering/ets/overloadMappingLowering.h"
#include "compiler/lowering/ets/packageImplicitImport.h"
#include "compiler/lowering/ets/partialExportClassGen.h"
#include "compiler/lowering/ets/primitiveConversionPhase.h"
#include "compiler/lowering/ets/promiseVoid.h"
#include "compiler/lowering/ets/recordLowering.h"
#include "compiler/lowering/ets/relaxedAnyLowering.h"
#include "compiler/lowering/ets/resizableArrayLowering.h"
#include "compiler/lowering/ets/lateInitialization.h"
#include "compiler/lowering/ets/restArgsLowering.h"
#include "compiler/lowering/ets/setJumpTarget.h"
#include "compiler/lowering/ets/setterLowering.h"
#include "compiler/lowering/ets/spreadLowering.h"
#include "compiler/lowering/ets/stringComparison.h"
#include "compiler/lowering/ets/stringConstantsLowering.h"
#include "compiler/lowering/ets/stringConstructorLowering.h"
#include "compiler/lowering/ets/topLevelStmts/topLevelStmts.h"
#include "compiler/lowering/ets/unboxLowering.h"
#include "compiler/lowering/ets/unionLowering.h"
#include "compiler/lowering/ets/typeFromLowering.h"
#include "compiler/lowering/plugin_phase.h"
#include "compiler/lowering/resolveIdentifiers.h"
#include "compiler/lowering/scopesInit/scopesInitPhase.h"
#include "generated/diagnostic.h"
#include "lexer/token/sourceLocation.h"
#include "public/es2panda_lib.h"
#include "util/options.h"

namespace ark::es2panda::compiler {

// NOLINTBEGIN(fuchsia-statically-constructed-objects)
static CheckerPhase g_checkerPhase;
static InitScopesPhaseETS g_initScopesPhaseEts;
static InitScopesPhaseAS g_initScopesPhaseAs;
static InitScopesPhaseTs g_initScopesPhaseTs;
static InitScopesPhaseJs g_initScopesPhaseJs;
// NOLINTEND(fuchsia-statically-constructed-objects)
const static inline char *g_pluginsAfterParse = "plugins-after-parse";
const static inline char *g_pluginsAfterBind = "plugins-after-bind";
const static inline char *g_pluginsAfterCheck = "plugins-after-check";
const static inline char *g_pluginsAfterLowering = "plugins-after-lowering";

// CC-OFFNXT(huge_method, G.FUN.01-CPP) long initialization list
std::vector<Phase *> GetETSPhaseList()
{
    // clang-format off
    // NOLINTBEGIN
    return {
        new PluginPhase {g_pluginsAfterParse, ES2PANDA_STATE_PARSED, &util::Plugin::AfterParse},
        new StringConstantsLowering,
        new PackageImplicitImport,
        new ExportAnonymousConstPhase,
        new TopLevelStatements,
        new InitModuleLowering,
        new ResizableArrayConvert,
        new ExpressionLambdaConstructionPhase,
        new InsertOptionalParametersAnnotation,
        new DefaultParametersInConstructorLowering,
        new DefaultParametersLowering,
        new AmbientLowering,
        new RestTupleConstructionPhase,
        new InitScopesPhaseETS,
        new OptionalLowering,
        new PromiseVoidInferencePhase,
        new InterfacePropertyDeclarationsPhase,
        new ConstantExpressionLowering,
        new EnumLoweringPhase,
        new ResolveIdentifiers,
        new PluginPhase {g_pluginsAfterBind, ES2PANDA_STATE_BOUND, &util::Plugin::AfterBind},
        new CapturedVariables,
        new SetJumpTargetPhase,
        new CFGBuilderPhase,
        new AnnotationCopyLowering,
        // please DO NOT change order of these two phases: checkerPhase and pluginsAfterCheck
        new CheckerPhase,
        // Dump declaration right after checker
        new DeclGenPhase,
        // pluginsAfterCheck has to go right after checkerPhase
        new PluginPhase {g_pluginsAfterCheck, ES2PANDA_STATE_CHECKED, &util::Plugin::AfterCheck},
        // new ConvertPrimitiveCastMethodCall,
        new DynamicImport,
        new RelaxedAnyLoweringPhase,
        new AnnotationCopyPostLowering,
        new AsyncMethodLowering,
        new DeclareOverloadLowering,
        new EnumPostCheckLoweringPhase,
        new SpreadConstructionPhase,
        new RestArgsLowering,
        new ArrayLiteralLowering,
        new BigIntLowering,
        new OpAssignmentLowering,
        new BinaryExpressionLowering, // should be after BigIntLowering and OpAssignmentLowering
        new SetterLowering,  // must be put before ObjectIndexLowering
        new LateInitializationConvert,
        new ExtensionAccessorPhase,
        new BoxingForLocals,
        new RecordLowering,
        new ObjectIndexLowering,
        new ObjectIteratorLowering,
        new OverloadMappingLowering,
        new LambdaConversionPhase,
        new UnionLowering,
        new ExpandBracketsPhase,
        new PartialExportClassGen,
        new StringConstructorLowering,
        new StringComparisonLowering,
        new OptionalArgumentsLowering, // #22952 could be moved to earlier phase
        new GenericBridgesPhase,
        new InterfaceObjectLiteralLowering,
        new ObjectLiteralLowering,
        new TypeFromLowering,
        new PrimitiveConversionPhase,
        new UnboxPhase,
        // pluginsAfterLowerings has to come at the very end, nothing should go after it
        new PluginPhase{g_pluginsAfterLowering, ES2PANDA_STATE_LOWERED,
                        &util::Plugin::AfterLowerings},
    };
    // NOLINTEND
    // clang-format on
}

void DestoryETSPhaseList(std::vector<Phase *> &list)
{
    for (auto *phase : list) {
        delete phase;
    }
}

std::vector<Phase *> GetASPhaseList()
{
    return {
        &g_initScopesPhaseAs,
        &g_checkerPhase,
    };
}

std::vector<Phase *> GetTSPhaseList()
{
    return {
        &g_initScopesPhaseTs,
        &g_checkerPhase,
    };
}

std::vector<Phase *> GetJSPhaseList()
{
    return {
        &g_initScopesPhaseJs,
        &g_checkerPhase,
    };
}

thread_local PhaseManager *g_phaseManager {nullptr};

PhaseManager *GetPhaseManager()
{
    ES2PANDA_ASSERT(g_phaseManager != nullptr && g_phaseManager->IsInitialized());
    return g_phaseManager;
}

void SetPhaseManager(PhaseManager *phaseManager)
{
    g_phaseManager = phaseManager;
}

void PhaseManager::Reset()
{
    prev_ = {0, INVALID_PHASE_ID};
    curr_ = {0, PARSER_PHASE_ID};
    next_ = PARSER_PHASE_ID + 1;
    ES2PANDA_ASSERT(next_ == 0);

    SetPhaseManager(this);
}

bool Phase::Apply(public_lib::Context *ctx, parser::Program *program)
{
    SetPhaseManager(ctx->phaseManager);
    GetPhaseManager()->SetCurrentPhaseId(id_);

#ifndef NDEBUG
    if (!Precondition(ctx, program)) {
        ctx->GetChecker()->LogError(diagnostic::PRECOND_FAILED, {Name()}, lexer::SourcePosition {});
        return false;
    }
#endif

    if (!Perform(ctx, program)) {
        return false;  // NOLINT(readability-simplify-boolean-expr)
    }

#ifndef NDEBUG
    if (!Postcondition(ctx, program)) {
        ctx->GetChecker()->LogError(diagnostic::POSTCOND_FAILED, {Name()}, lexer::SourcePosition {});
        return false;
    }
#endif

    return true;
}

bool PhaseForDeclarations::Precondition(public_lib::Context *ctx, const parser::Program *program)
{
    for (auto &[_, extPrograms] : program->ExternalSources()) {
        (void)_;
        for (auto *extProg : extPrograms) {
            if (extProg->IsASTLowered()) {
                continue;
            }
            if (!Precondition(ctx, extProg)) {
                return false;
            }
        }
    }

    return PreconditionForModule(ctx, program);
}

bool PhaseForDeclarations::Perform(public_lib::Context *ctx, parser::Program *program)
{
    FetchCache(ctx, program);
    bool result = true;
    for (auto &[_, extPrograms] : program->ExternalSources()) {
        (void)_;
        for (auto *extProg : extPrograms) {
            if (!extProg->IsASTLowered()) {
                result &= Perform(ctx, extProg);
            }
        }
    }

    result &= PerformForModule(ctx, program);
    return result;
}

bool PhaseForDeclarations::Postcondition(public_lib::Context *ctx, const parser::Program *program)
{
    for (auto &[_, extPrograms] : program->ExternalSources()) {
        (void)_;
        for (auto *extProg : extPrograms) {
            if (extProg->IsASTLowered()) {
                continue;
            }
            if (!Postcondition(ctx, extProg)) {
                return false;
            }
        }
    }

    return PostconditionForModule(ctx, program);
}

// CC-OFFNXT(huge_method, huge_depth) solid logic
bool PhaseForBodies::Precondition(public_lib::Context *ctx, const parser::Program *program)
{
    auto cMode = ctx->config->options->GetCompilationMode();

    auto iterateExternal = [&cMode, this](public_lib::Context *context, const parser::Program *localProgram) {
        for (auto &[_, extPrograms] : localProgram->ExternalSources()) {
            (void)_;
            for (auto *prog : extPrograms) {
                if (!prog->IsGenAbcForExternal() && cMode == CompilationMode::GEN_ABC_FOR_EXTERNAL_SOURCE) {
                    continue;
                }

                if (!Precondition(context, prog)) {
                    return false;
                }
            }
        }
        return true;
    };
    if (cMode == CompilationMode::GEN_STD_LIB || cMode == CompilationMode::GEN_ABC_FOR_EXTERNAL_SOURCE) {
        if (!iterateExternal(ctx, program)) {
            return false;
        }
    }

    return PreconditionForModule(ctx, program);
}

bool PhaseForBodies::ProcessExternalPrograms(public_lib::Context *ctx, parser::Program *program)
{
    bool result = true;
    for (auto &[_, extPrograms] : program->ExternalSources()) {
        (void)_;
        for (auto *extProg : extPrograms) {
            if (!extProg->IsASTLowered()) {
                result &= Perform(ctx, extProg);
            }
        }
    }
    return result;
}

// CC-OFFNXT(huge_method, huge_depth) solid logic
bool PhaseForBodies::Perform(public_lib::Context *ctx, parser::Program *program)
{
    FetchCache(ctx, program);
    bool result = true;
    auto cMode = ctx->config->options->GetCompilationMode();
    auto iterateExternal = [&result, &cMode, this](public_lib::Context *context, parser::Program *localProgram) {
        for (auto &[_, extPrograms] : localProgram->ExternalSources()) {
            (void)_;
            for (auto *extProg : extPrograms) {
                if (!extProg->IsGenAbcForExternal() && cMode == CompilationMode::GEN_ABC_FOR_EXTERNAL_SOURCE) {
                    continue;
                }

                if (!extProg->IsASTLowered()) {
                    result &= Perform(context, extProg);
                }
            }
        }
    };
    if (cMode == CompilationMode::GEN_STD_LIB || cMode == CompilationMode::GEN_ABC_FOR_EXTERNAL_SOURCE) {
        iterateExternal(ctx, program);
    }

    result &= PerformForModule(ctx, program);
    return result;
}

// CC-OFFNXT(huge_method, huge_depth) solid logic
bool PhaseForBodies::Postcondition(public_lib::Context *ctx, const parser::Program *program)
{
    auto cMode = ctx->config->options->GetCompilationMode();
    auto iterateExternal = [&cMode, this](public_lib::Context *context, const parser::Program *localProgram) {
        for (auto &[_, extPrograms] : localProgram->ExternalSources()) {
            (void)_;
            for (auto *prog : extPrograms) {
                if (!prog->IsGenAbcForExternal() && cMode == CompilationMode::GEN_ABC_FOR_EXTERNAL_SOURCE) {
                    continue;
                }

                if (!Postcondition(context, prog)) {
                    return false;
                }
            }
        }
        return true;
    };
    if (cMode == CompilationMode::GEN_STD_LIB || cMode == CompilationMode::GEN_ABC_FOR_EXTERNAL_SOURCE) {
        if (!iterateExternal(ctx, program)) {
            return false;
        }
    }

    return PostconditionForModule(ctx, program);
}

PhaseManager::~PhaseManager()
{
    if (ScriptExtension::ETS == ext_) {
        DestoryETSPhaseList(phases_);
    }
}

void PhaseManager::InitializePhases()
{
    switch (ext_) {
        case ScriptExtension::ETS:
            phases_ = GetETSPhaseList();
            break;
        case ScriptExtension::AS:
            phases_ = GetASPhaseList();
            break;
        case ScriptExtension::TS:
            phases_ = GetTSPhaseList();
            break;
        case ScriptExtension::JS:
            phases_ = GetJSPhaseList();
            break;
        default:
            ES2PANDA_UNREACHABLE();
    }

    int id = 0;
    for (auto phase : phases_) {
        // js side UI plugin needs an extra phaseID, which is different from c++ side plugin phase
        if (phase->Name() == std::string(g_pluginsAfterParse)) {
            jsPluginAfterParse_ = id++;
        }
        if (phase->Name() == std::string(g_pluginsAfterBind)) {
            jsPluginAfterBind_ = id++;
        }
        if (phase->Name() == std::string(g_pluginsAfterCheck)) {
            jsPluginAfterCheck_ = id++;
        }
        if (phase->Name() == std::string(g_pluginsAfterLowering)) {
            jsPluginAfterLower_ = id++;
        }
        phase->id_ = id++;
    }
}

std::vector<Phase *> PhaseManager::AllPhases()
{
    ES2PANDA_ASSERT(IsInitialized());
    return phases_;
}

std::vector<Phase *> PhaseManager::RebindPhases()
{
    ES2PANDA_ASSERT(IsInitialized());
    return GetSubPhases({ScopesInitPhase::NAME, ResolveIdentifiers::NAME});
}

std::vector<Phase *> PhaseManager::GetSubPhases(const std::vector<std::string_view> &phaseNames)
{
    std::vector<Phase *> phases;
    for (auto &phaseName : phaseNames) {
        for (auto phase : phases_) {
            if (phase->Name() == phaseName) {
                phases.emplace_back(phase);
            }
        }
    }
    return phases;
}

std::vector<Phase *> PhaseManager::RecheckPhases()
{
    ES2PANDA_ASSERT(IsInitialized());
    return GetSubPhases({ScopesInitPhase::NAME, ResolveIdentifiers::NAME, "CapturedVariables", CheckerPhase::NAME});
}

int32_t PhaseManager::GetCurrentMajor() const
{
    return curr_.major;
}

int32_t PhaseManager::GetCurrentMinor() const
{
    return curr_.minor;
}

}  // namespace ark::es2panda::compiler
