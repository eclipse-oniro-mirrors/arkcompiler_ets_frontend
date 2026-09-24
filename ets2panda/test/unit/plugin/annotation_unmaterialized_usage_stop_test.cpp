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
#include "util/diagnostic.h"
#include "util/diagnosticEngine.h"
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
    if (g_arrow != nullptr) {
        prepared.hostFunction = g_impl->ArrowFunctionExpressionFunction(g_ctx, g_arrow);
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

struct ResolutionFailures {
    size_t total = 0;
    size_t located = 0;
};

ResolutionFailures ObserveResolutionFailures(const char *baseName)
{
    ResolutionFailures observed;
    auto *opaque = g_impl->GetSemanticErrors(g_ctx);
    const auto *errors = reinterpret_cast<const ark::es2panda::util::DiagnosticStorage *>(opaque);
    for (const auto &error : *errors) {
        ++observed.total;
        const auto message = error->Message();
        std::cerr << "  diagnostic id=" << error->GetId() << " located=" << static_cast<int>(error->HasLocation())
                  << " message=" << message << '\n';
        const bool isExpectedKind = error->GetId() == ark::es2panda::diagnostic::ANNOTATION_RESOLUTION_FAILED.Id();
        if (isExpectedKind && error->HasLocation() && message.find(baseName) != std::string::npos) {
            ++observed.located;
        }
    }
    return observed;
}

bool RunCheckerRejectsCase(const char *baseName, int argc, char **argv)
{
    auto prepared = Prepare(argc, argv);
    if (!prepared.ok) {
        Finish(&prepared);
        return false;
    }
    g_ctx = prepared.ctx;
    SynthesizeAnnotation(prepared.hostFunction, baseName);
    g_impl->ProgramSetProgramModified(g_ctx, prepared.program, true);
    g_impl->AstNodeRecheck(g_ctx, prepared.module);

    const bool errored = g_impl->IsAnyError(g_ctx);
    g_impl->ProceedToState(g_ctx, ES2PANDA_STATE_BIN_GENERATED);
    const bool reachedBin = g_impl->ContextState(g_ctx) == ES2PANDA_STATE_BIN_GENERATED;
    std::cerr << "  base=" << baseName << " erroredAfterRecheck=" << static_cast<int>(errored)
              << " reachedBin=" << static_cast<int>(reachedBin) << '\n';

    const bool pass = errored && !reachedBin;
    Finish(&prepared);
    return pass;
}

bool RunLoweringDiagnosesCase(const char *baseName, bool unmodifiedRecheck, int argc, char **argv)
{
    auto prepared = Prepare(argc, argv);
    if (!prepared.ok) {
        Finish(&prepared);
        return false;
    }
    g_ctx = prepared.ctx;
    auto *base = SynthesizeAnnotation(prepared.hostFunction, baseName);
    if (unmodifiedRecheck) {
        g_impl->ProgramSetProgramModified(g_ctx, prepared.program, false);
        g_impl->AstNodeRecheck(g_ctx, prepared.module);
    }
    const bool bound = g_impl->DeclarationFromIdentifier(g_ctx, base) != nullptr;
    const bool cleanBeforeBin = !g_impl->IsAnyError(g_ctx);

    g_impl->ProceedToState(g_ctx, ES2PANDA_STATE_BIN_GENERATED);
    const bool errFinal = g_impl->IsAnyError(g_ctx);
    const bool reachedBin = g_impl->ContextState(g_ctx) == ES2PANDA_STATE_BIN_GENERATED;
    const auto observed = ObserveResolutionFailures(baseName);
    std::cerr << "  base=" << baseName << " unmodifiedRecheck=" << static_cast<int>(unmodifiedRecheck)
              << " bound=" << static_cast<int>(bound) << " cleanBeforeBin=" << static_cast<int>(cleanBeforeBin)
              << " errFinal=" << static_cast<int>(errFinal) << " reachedBin=" << static_cast<int>(reachedBin)
              << " resolutionFailures=" << observed.located << " totalDiagnostics=" << observed.total << '\n';

    const bool pass = !bound && errFinal && !reachedBin && observed.located == 1 && observed.total == 1;
    Finish(&prepared);
    return pass;
}

struct Case {
    const char *name;
    const char *baseName;
    bool unmodifiedRecheck;
    bool checkerRejects;
};
const Case CASES[] = {
    {"missing-base-program-recheck", "MissingMarker", false, true},
    {"non-annotation-base-program-recheck", "notAnAnnotation", false, true},
    {"declared-base-no-recheck", "Marker", false, false},
    {"declared-base-unmodified-recheck", "Marker", true, false},
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
        const bool pass = entry.checkerRejects
                              ? RunCheckerRejectsCase(entry.baseName, argc, argv)
                              : RunLoweringDiagnosesCase(entry.baseName, entry.unmodifiedRecheck, argc, argv);
        std::cerr << "CASE " << entry.name << " -> " << (pass ? "PASS" : "FAIL") << '\n';
        if (!pass) {
            ++failures;
        }
    }
    std::cerr << "UNMATERIALIZED_USAGE_STOP total=" << CASE_COUNT << " failures=" << failures << '\n';
    return failures == 0 ? 0 : TEST_ERROR_CODE;
}

// NOLINTEND
