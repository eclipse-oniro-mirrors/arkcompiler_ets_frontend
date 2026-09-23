/**
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include <iostream>
#include <iterator>
#include <string>

#include "public/es2panda_lib.h"
#include "util.h"

// NOLINTBEGIN

namespace {

es2panda_Impl *g_impl = nullptr;
es2panda_Context *g_ctx = nullptr;
es2panda_AstNode *g_arrow = nullptr;

void FindArrow(es2panda_AstNode *node)
{
    if (g_arrow == nullptr && g_impl->IsArrowFunctionExpression(node)) {
        g_arrow = node;
    }
    g_impl->AstNodeIterateConst(g_ctx, node, FindArrow);
}

struct Prepared {
    es2panda_Config *config = nullptr;
    es2panda_Context *ctx = nullptr;
    es2panda_Program *program = nullptr;
    es2panda_AstNode *module = nullptr;
    es2panda_AstNode *arrow = nullptr;
    es2panda_AstNode *hostFunction = nullptr;
    bool ok = false;
};

void Finish(Prepared *prepared)
{
    if (prepared->ctx != nullptr) {
        g_impl->DestroyContext(prepared->ctx);
    }
    if (prepared->config != nullptr) {
        g_impl->DestroyConfig(prepared->config);
    }
    prepared->ctx = nullptr;
    prepared->config = nullptr;
    g_ctx = nullptr;
}

Prepared Prepare(int argc, char **argv)
{
    Prepared prepared;
    prepared.config = g_impl->CreateConfig(argc - 1, argv + 1);
    prepared.ctx = g_impl->CreateContextFromFile(prepared.config, argv[argc - 1]);
    if (prepared.ctx == nullptr) {
        std::cerr << "FAILED TO CREATE CONTEXT" << '\n';
        return prepared;
    }
    g_ctx = prepared.ctx;
    g_impl->ProceedToState(g_ctx, ES2PANDA_STATE_CHECKED);
    if (g_impl->ContextState(g_ctx) != ES2PANDA_STATE_CHECKED) {
        CheckForErrors("CHECKED", g_ctx);
        return prepared;
    }
    g_arrow = nullptr;
    prepared.program = g_impl->ContextProgram(g_ctx);
    prepared.module = g_impl->ProgramAst(g_ctx, prepared.program);
    g_impl->AstNodeIterateConst(g_ctx, prepared.module, FindArrow);
    prepared.arrow = g_arrow;
    if (prepared.arrow != nullptr) {
        prepared.hostFunction = g_impl->ArrowFunctionExpressionFunction(g_ctx, prepared.arrow);
    }
    if (prepared.hostFunction == nullptr) {
        std::cerr << "no expression-position script function in the test source" << '\n';
        return prepared;
    }
    prepared.ok = true;
    return prepared;
}

es2panda_AstNode *SynthesizeAnnotation(es2panda_AstNode *hostFunction, const char *baseName)
{
    auto *base = CreateIdentifierFromString(g_ctx, baseName);
    g_impl->IdentifierSetAnnotationUsage(g_ctx, base);
    auto *usage = g_impl->CreateAnnotationUsageIr1(g_ctx, base, nullptr, 0);
    g_impl->AstNodeSetParent(g_ctx, base, usage);
    es2panda_AstNode *annotations[] = {usage};
    g_impl->ScriptFunctionSetAnnotations(g_ctx, hostFunction, annotations, 1);
    return base;
}

bool BaseIsBoundToAnnotationDeclaration(es2panda_AstNode *base)
{
    auto *decl = g_impl->DeclarationFromIdentifier(g_ctx, base);
    return decl != nullptr && g_impl->IsAnnotationDeclaration(decl);
}

struct RunOutcome {
    bool boundBeforeRecheck = false;
    bool bound = false;
    bool cleanAfterRecheck = false;
    bool reachedBin = false;
    bool cleanAfterBin = false;
};

RunOutcome RecheckAndCompile(const Prepared &prepared, es2panda_AstNode *base, bool recheckOwningNode)
{
    RunOutcome outcome;
    outcome.boundBeforeRecheck = BaseIsBoundToAnnotationDeclaration(base);
    g_impl->ProgramSetProgramModified(g_ctx, prepared.program, true);
    if (recheckOwningNode) {
        g_impl->AstNodeRecheck(g_ctx, prepared.hostFunction);
    } else {
        g_impl->AstNodeRecheck(g_ctx, prepared.module);
    }
    outcome.bound = BaseIsBoundToAnnotationDeclaration(base);
    outcome.cleanAfterRecheck = !g_impl->IsAnyError(g_ctx);
    g_impl->ProceedToState(g_ctx, ES2PANDA_STATE_BIN_GENERATED);
    outcome.reachedBin = g_impl->ContextState(g_ctx) == ES2PANDA_STATE_BIN_GENERATED;
    outcome.cleanAfterBin = !g_impl->IsAnyError(g_ctx);
    return outcome;
}

void ReportOutcome(const RunOutcome &outcome)
{
    std::cerr << "  boundBeforeRecheck=" << static_cast<int>(outcome.boundBeforeRecheck)
              << " bound=" << static_cast<int>(outcome.bound)
              << " cleanAfterRecheck=" << static_cast<int>(outcome.cleanAfterRecheck)
              << " reachedBin=" << static_cast<int>(outcome.reachedBin)
              << " cleanAfterBin=" << static_cast<int>(outcome.cleanAfterBin) << '\n';
}

bool IsOutcomeClean(const RunOutcome &outcome, bool expectUnboundBeforeRecheck)
{
    const bool unboundFirst = !expectUnboundBeforeRecheck || !outcome.boundBeforeRecheck;
    return unboundFirst && outcome.bound && outcome.cleanAfterRecheck && outcome.reachedBin && outcome.cleanAfterBin;
}

bool RunRecheckCase(const char *baseName, bool recheckOwningNode, int argc, char **argv)
{
    auto prepared = Prepare(argc, argv);
    if (!prepared.ok) {
        Finish(&prepared);
        return false;
    }
    std::cerr << "  base=" << baseName << " recheck=" << (recheckOwningNode ? "owning-node" : "program") << '\n';
    auto *base = SynthesizeAnnotation(prepared.hostFunction, baseName);
    const auto outcome = RecheckAndCompile(prepared, base, recheckOwningNode);
    ReportOutcome(outcome);
    const bool pass = IsOutcomeClean(outcome, true);
    Finish(&prepared);
    return pass;
}

bool RunCloneReplacementCase(int argc, char **argv)
{
    auto prepared = Prepare(argc, argv);
    if (!prepared.ok) {
        Finish(&prepared);
        return false;
    }
    auto *call = g_impl->AstNodeParent(g_ctx, prepared.arrow);
    if (call == nullptr || !g_impl->IsCallExpression(call)) {
        std::cerr << "the arrow in the test source is not a direct call argument" << '\n';
        Finish(&prepared);
        return false;
    }
    auto *replacement = g_impl->AstNodeClone(g_ctx, prepared.arrow, call);
    if (replacement == nullptr || !g_impl->IsArrowFunctionExpression(replacement)) {
        std::cerr << "cloning the arrow did not produce an arrow" << '\n';
        Finish(&prepared);
        return false;
    }
    auto *clonedFunction = g_impl->ArrowFunctionExpressionFunction(g_ctx, replacement);
    auto *base = SynthesizeAnnotation(clonedFunction, "Marker");
    es2panda_AstNode *arguments[] = {replacement};
    g_impl->CallExpressionSetArguments(g_ctx, call, arguments, 1);

    size_t argumentCount = 0;
    auto **installed = g_impl->CallExpressionArguments(g_ctx, call, &argumentCount);
    if (argumentCount != 1 || installed == nullptr || installed[0] != replacement) {
        std::cerr << "the cloned arrow was not installed as the call argument" << '\n';
        Finish(&prepared);
        return false;
    }

    const auto outcome = RecheckAndCompile(prepared, base, false);
    ReportOutcome(outcome);
    const bool pass = IsOutcomeClean(outcome, false);
    Finish(&prepared);
    return pass;
}

struct Case {
    const char *name;
    const char *baseName;
    bool recheckOwningNode;
    bool cloneReplacement;
};
const Case CASES[] = {
    {"declared-base-program-recheck", "Marker", false, false},
    {"imported-base-program-recheck", "ExportedMarker", false, false},
    {"declared-base-owning-node-recheck", "Marker", true, false},
    {"declared-base-on-cloned-replacement", "Marker", false, true},
};
const int CASE_COUNT = static_cast<int>(std::size(CASES));

}  // namespace

int main(int argc, char **argv)
{
    if (argc < MIN_ARGC) {
        return INVALID_ARGC_ERROR_CODE;
    }
    g_impl = GetImpl();
    if (g_impl == nullptr) {
        return NULLPTR_IMPL_ERROR_CODE;
    }

    int failures = 0;
    for (int index = 0; index != CASE_COUNT; ++index) {
        const auto &entry = CASES[index];
        std::cerr << "CASE " << entry.name << '\n';
        const bool pass = entry.cloneReplacement ? RunCloneReplacementCase(argc, argv)
                                                 : RunRecheckCase(entry.baseName, entry.recheckOwningNode, argc, argv);
        std::cerr << "CASE " << entry.name << " -> " << (pass ? "PASS" : "FAIL") << '\n';
        if (!pass) {
            ++failures;
        }
    }
    std::cerr << "SYNTHESIZED_USAGE_BINDING total=" << CASE_COUNT << " failures=" << failures << '\n';
    return failures == 0 ? 0 : TEST_ERROR_CODE;
}

// NOLINTEND
