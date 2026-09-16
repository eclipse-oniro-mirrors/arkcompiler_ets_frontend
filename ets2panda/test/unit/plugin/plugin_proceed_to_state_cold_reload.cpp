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

// End-to-end C API (IDE path) cold reload tests.
// Goes through es2panda_lib.cpp pipeline: CreateConfig → CreateContextFromFile
// → ProceedToState(BIN_GENERATED) → InitPatchFixCAPI → ... → FinalizePatchFixCAPI.
//
// Each test: dump base → cold reload mod → check state + errorMessage.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <unistd.h>

#include "public/es2panda_lib.h"
#include "util.h"

// NOLINTBEGIN

static es2panda_Impl *impl = nullptr;

// ========================================================================
// Source strings — keep in sync with test/reload/coldreload/* tests
// ========================================================================

// --- helpers for function tests ---
static const char *FUNC_BASE = R"ETS(
function add(a: int, b: int): int { return a + b; }
function main(): void { let x: int = add(1, 2); }
)ETS";

static const char *FUNC_NO_CHANGE = R"ETS(
function add(a: int, b: int): int { return a + b; }
function main(): void { let x: int = add(1, 2); }
)ETS";

static const char *FUNC_BODY_CHANGE = R"ETS(
function add(a: int, b: int): int { return a * b; }
function main(): void { let x: int = add(1, 2); }
)ETS";

static const char *FUNC_SIG_CHANGE = R"ETS(
function add(a: string, b: string): string { return a + b; }
function main(): void { let s: string = add("a", "b"); }
)ETS";

static const char *FUNC_STRING_CHANGE = R"ETS(
function add(a: int, b: int): int { return a + b; }
function main(): void { let s: string = "changed"; }
)ETS";

static const char *FUNC_ADD = R"ETS(
function add(a: int, b: int): int { return a + b; }
function main(): void { let x: int = add(1, 2); }
function extra(): int { return 99; }
)ETS";

static const char *FUNC_DELETE = R"ETS(
function add(a: int, b: int): int { return a + b; }
function main(): void { let x: int = add(1, 2); }
)ETS";

static const char *FUNC_DELETE_BASE = R"ETS(
function helper(): int { return 42; }
function add(a: int, b: int): int { return a + b; }
function main(): void { let x: int = add(1, 2); }
)ETS";

// --- helpers for class method tests ---
static const char *CLS_BASE = R"ETS(
class Foo { bar(): int { return 1; } }
function main(): void { let f: Foo = new Foo(); f.bar(); }
)ETS";

static const char *CLS_BODY_CHANGE = R"ETS(
class Foo { bar(): int { return 2; } }
function main(): void { let f: Foo = new Foo(); f.bar(); }
)ETS";

static const char *CLS_SIG_CHANGE_BASE = R"ETS(
class Foo { bar(x: int): int { return x; } baz(): int { return 1; } }
function main(): void { let f: Foo = new Foo(); f.baz(); }
)ETS";

static const char *CLS_SIG_CHANGE = R"ETS(
class Foo { bar(x: string): int { return 1; } baz(): int { return 1; } }
function main(): void { let f: Foo = new Foo(); f.baz(); }
)ETS";

static const char *CLS_ADD = R"ETS(
class Foo { bar(): int { return 1; } baz(): int { return 2; } }
function main(): void { let f: Foo = new Foo(); f.bar(); }
)ETS";

static const char *CLS_DELETE_BASE = R"ETS(
class Foo { bar(): int { return 1; } baz(): int { return 2; } }
function main(): void { let f: Foo = new Foo(); f.bar(); }
)ETS";

static const char *CLS_DELETE = R"ETS(
class Foo { bar(): int { return 1; } }
function main(): void { let f: Foo = new Foo(); f.bar(); }
)ETS";

// --- helpers for field tests ---
static const char *FLD_BASE = R"ETS(
class Foo { private a: int = 0; }
function main(): void { let f: Foo = new Foo(); }
)ETS";

static const char *FLD_TYPE_CHANGE = R"ETS(
class Foo { private a: string = "hello"; }
function main(): void { let f: Foo = new Foo(); }
)ETS";

static const char *FLD_INIT_CHANGE = R"ETS(
class Foo { private a: int = 42; }
function main(): void { let f: Foo = new Foo(); }
)ETS";

static const char *FLD_ADD = R"ETS(
class Foo { private a: int = 0; private b: int = 1; }
function main(): void { let f: Foo = new Foo(); }
)ETS";

static const char *FLD_DELETE_BASE = R"ETS(
class Foo { private a: int = 0; private b: int = 1; }
function main(): void { let f: Foo = new Foo(); }
)ETS";

static const char *FLD_DELETE = R"ETS(
class Foo { private a: int = 0; }
function main(): void { let f: Foo = new Foo(); }
)ETS";

// ========================================================================
// Test infrastructure
// ========================================================================

static const char *S(es2panda_ContextState s)
{
    return s == ES2PANDA_STATE_BIN_GENERATED ? "BIN" : s == ES2PANDA_STATE_ERROR ? "ERR" : "?";
}

// Run one phase (dump or reload) through the C API pipeline.
static std::pair<es2panda_ContextState, std::string> RunPhase(int argc, char **argv, const char *extraFlag1,
                                                              const char *extraFlag2, const char *source,
                                                              const char *srcFile)
{
    // Write source to temp file (required by CreateContextFromFile).
    FILE *f = fopen(srcFile, "w");
    if (!f)
        return {ES2PANDA_STATE_ERROR, "write failed"};
    if (fputs(source, f) == EOF || fclose(f) == EOF) {
        return {ES2PANDA_STATE_ERROR, "write failed"};
    }

    // Build argv: runner's args (minus source file) + reload flags + our file.
    std::vector<const char *> a;
    for (int i = 1; i < argc - 1; ++i)
        a.push_back(argv[i]);
    if (extraFlag1)
        a.push_back(extraFlag1);
    if (extraFlag2)
        a.push_back(extraFlag2);
    a.push_back(srcFile);

    auto *config = impl->CreateConfig(a.size(), a.data());
    if (!config)
        return {ES2PANDA_STATE_ERROR, "CreateConfig failed"};

    auto *ctx = impl->CreateContextFromFile(config, srcFile);
    if (!ctx) {
        impl->DestroyConfig(config);
        return {ES2PANDA_STATE_ERROR, "CreateContextFromFile failed"};
    }

    // Step to BIN_GENERATED (config must stay alive).
    for (auto s = ES2PANDA_STATE_PARSED; s <= ES2PANDA_STATE_BIN_GENERATED;
         s = static_cast<es2panda_ContextState>(s + 1)) {
        if (s == ES2PANDA_STATE_NEW || s == ES2PANDA_STATE_ERROR)
            continue;
        impl->ProceedToState(ctx, s);
        if (impl->ContextState(ctx) == ES2PANDA_STATE_ERROR)
            break;
    }

    auto state = impl->ContextState(ctx);
    std::string errMsg = (state == ES2PANDA_STATE_ERROR && impl->ContextErrorMessage(ctx) != nullptr)
                             ? impl->ContextErrorMessage(ctx)
                             : "";

    impl->DestroyContext(ctx);
    impl->DestroyConfig(config);
    return {state, errMsg};
}

struct TestCase {
    const char *label;
    const char *baseSrc;
    const char *modSrc;
    bool expectError;
};

static int RunTest(const TestCase &tc, int argc, char **argv)
{
    std::string srcPath = std::string("/tmp/capi_src_") + std::to_string(getpid()) + "_" + tc.label + ".ets";
    std::string stPath = std::string("/tmp/capi_st_") + std::to_string(getpid()) + "_" + tc.label + ".st";
    std::string da = "--dump-symbol-table=" + stPath;
    std::string ia = "--input-symbol-table=" + stPath;

    // Phase 1: dump base
    {
        auto [state, msg] = RunPhase(argc, argv, da.c_str(), nullptr, tc.baseSrc, srcPath.c_str());
        if (state != ES2PANDA_STATE_BIN_GENERATED) {
            std::cerr << "FAIL [" << tc.label << "] dump: " << S(state) << msg << std::endl;
            return 1;
        }
    }

    // Phase 2: cold reload
    {
        auto [state, msg] = RunPhase(argc, argv, "--cold-reload", ia.c_str(), tc.modSrc, srcPath.c_str());

        if (!tc.expectError && state != ES2PANDA_STATE_BIN_GENERATED) {
            std::cerr << "FAIL [" << tc.label << "] want BIN, got " << S(state) << " [" << msg << "]" << std::endl;
            return 1;
        }
        if (tc.expectError && state == ES2PANDA_STATE_BIN_GENERATED) {
            std::cerr << "FAIL [" << tc.label << "] want ERR, got BIN" << std::endl;
            return 1;
        }
        if (tc.expectError && msg.empty()) {
            std::cerr << "FAIL [" << tc.label << "] want errorMessage, got empty" << std::endl;
            return 1;
        }
    }

    std::cout << "PASS [" << tc.label << "]" << std::endl;
    return 0;
}

// Class add/delete sources: both reload modes accept them (cold restarts, hot keeps
// the old class live / lazy-loads the new one).
static const char *CLS_ADD_BASE = R"ETS(
class Foo { m(): int { return 1; } }
function main(): void { let f: Foo = new Foo(); f.m(); }
)ETS";

static const char *CLS_ADD_NEW = R"ETS(
class Foo { m(): int { return 1; } }
class Bar { n(): int { return 2; } }
function main(): void { let f: Foo = new Foo(); f.m(); }
)ETS";

static const char *CLS_DEL_BASE = R"ETS(
class Foo { m(): int { return 1; } }
class Bar { n(): int { return 2; } }
function main(): void { let f: Foo = new Foo(); f.m(); }
)ETS";

static const char *CLS_DEL_MOD = R"ETS(
class Foo { m(): int { return 1; } }
function main(): void { let f: Foo = new Foo(); f.m(); }
)ETS";

// Enum member and top-level const changes: cold reload accepts (restart re-runs the
// static initializer from the patch).
static const char *ENUM_BASE = R"ETS(
enum Color { RED, GREEN }
function main(): void { let c: Color = Color.RED; }
)ETS";

static const char *ENUM_ADD = R"ETS(
enum Color { RED, GREEN, BLUE }
function main(): void { let c: Color = Color.RED; }
)ETS";

static const char *CONST_BASE = R"ETS(
const K: int = 5;
function main(): void { let x: int = K; }
)ETS";

static const char *CONST_CHANGE = R"ETS(
const K: int = 6;
function main(): void { let x: int = K; }
)ETS";

// Interface add/remove sources: cold reload accepts; hot reload rejects at classinfo.
static const char *IFACE_BASE = R"ETS(
interface IA { a(): int; }
class Foo implements IA {
    a(): int { return 1; }
}
function main(): void { let f: Foo = new Foo(); f.a(); }
)ETS";

static const char *IFACE_ADD = R"ETS(
interface IA { a(): int; }
interface IB { b(): int; }
class Foo implements IA, IB {
    a(): int { return 1; }
    b(): int { return 2; }
}
function main(): void { let f: Foo = new Foo(); f.a(); }
)ETS";

static const char *IFACE_DEL = R"ETS(
interface IA { a(): int; }
class Foo {
    a(): int { return 1; }
}
function main(): void { let f: Foo = new Foo(); f.a(); }
)ETS";

// Interface-list order swap: the interface set is unchanged, so the runtime swap
// check accepts it (set comparison) — the frontend must not reject it either.
static const char *IFACE_ORDER_BASE = R"ETS(
interface IA { a(): int; }
interface IB { b(): int; }
class Foo implements IA, IB {
    a(): int { return 1; }
    b(): int { return 2; }
}
function main(): void { let f: Foo = new Foo(); f.a(); }
)ETS";

static const char *IFACE_ORDER_SWAP = R"ETS(
interface IA { a(): int; }
interface IB { b(): int; }
class Foo implements IB, IA {
    a(): int { return 1; }
    b(): int { return 2; }
}
function main(): void { let f: Foo = new Foo(); f.a(); }
)ETS";

// Parent-class change source: cold reload accepts it; hot reload rejects it at
// classinfo validation — the error source for the ASM-boundary probe below.
static const char *PARENT_CHANGE_BASE = R"ETS(
class BaseA { m(): int { return 1; } }
class Foo extends BaseA { n(): int { return 2; } }
function main(): void { let f: Foo = new Foo(); f.n(); }
)ETS";

static const char *PARENT_CHANGE_MOD = R"ETS(
class BaseB { m(): int { return 1; } }
class Foo extends BaseB { n(): int { return 2; } }
function main(): void { let f: Foo = new Foo(); f.n(); }
)ETS";

// Namespace-member class: a namespace is compiled as a class, so its member
// classes live as nested ClassDeclarations — they must take part in the hot
// reload classinfo check just like top-level classes.
static const char *NS_PARENT_BASE = R"ETS(
export class BaseA { m(): int { return 1; } }
export class BaseB { m(): int { return 1; } }
namespace NS { export class Foo extends BaseA { n(): int { return 2; } } }
function main(): void { let f: NS.Foo = new NS.Foo(); f.n(); }
)ETS";

static const char *NS_PARENT_CHANGE = R"ETS(
export class BaseA { m(): int { return 1; } }
export class BaseB { m(): int { return 1; } }
namespace NS { export class Foo extends BaseB { n(): int { return 2; } } }
function main(): void { let f: NS.Foo = new NS.Foo(); f.n(); }
)ETS";

// ========================================================================
// ASM_GENERATED boundary tests (IDE / JS-binding-layer lifecycle)
// ========================================================================
// The embedder makes its decision at the ASM_GENERATED boundary: on error it
// reads messages (GetAllErrorMessages) before any teardown happens; on success
// it frees compiler memory and proceeds to BIN. Reload validation must
// therefore be finalized by the end of the ASM stage, and the message APIs
// must stay callable while the context is alive.

// Proceed through the pipeline only up to ASM_GENERATED — the point where the
// embedder decides — stopping early if the context enters the error state.
static void ProceedToAsmBoundary(es2panda_Context *ctx)
{
    for (auto s = ES2PANDA_STATE_PARSED; s <= ES2PANDA_STATE_ASM_GENERATED;
         s = static_cast<es2panda_ContextState>(s + 1)) {
        if (s == ES2PANDA_STATE_NEW || s == ES2PANDA_STATE_ERROR)
            continue;
        impl->ProceedToState(ctx, s);
        if (impl->ContextState(ctx) == ES2PANDA_STATE_ERROR)
            break;
    }
}

// Error-path expectations: the reload verdict must already be final at the
// ASM_GENERATED boundary and the message must carry the [Patch] marker.
static int CheckBoundaryError(const char *label, es2panda_ContextState state, const char *em)
{
    int rc = 0;
    if (state != ES2PANDA_STATE_ERROR) {
        std::cerr << "FAIL [" << label << "] reload error must be visible at ASM_GENERATED, got " << S(state)
                  << std::endl;
        rc = 1;
    }
    if (em == nullptr || strstr(em, "[Patch]") == nullptr) {
        std::cerr << "FAIL [" << label << "] errorMessage missing [Patch]" << std::endl;
        rc = 1;
    }
    return rc;
}

// Success-path expectations: the context must sit at ASM_GENERATED, and the
// pipeline must still complete to BIN after probing the message APIs.
static int CheckBoundaryOk(es2panda_Context *ctx, const char *label, es2panda_ContextState state)
{
    if (state != ES2PANDA_STATE_ASM_GENERATED) {
        std::cerr << "FAIL [" << label << "] want ASM, got " << S(state) << std::endl;
        return 1;
    }
    impl->ProceedToState(ctx, ES2PANDA_STATE_BIN_GENERATED);
    if (impl->ContextState(ctx) != ES2PANDA_STATE_BIN_GENERATED) {
        std::cerr << "FAIL [" << label << "] want BIN after ASM boundary" << std::endl;
        return 1;
    }
    return 0;
}

static int RunAsmBoundaryPhase(int argc, char **argv, const char *extraFlag1, const char *extraFlag2,
                               const char *source, const char *srcFile, bool expectError, const char *label)
{
    FILE *f = fopen(srcFile, "w");
    if (f == nullptr || fputs(source, f) == EOF || fclose(f) == EOF) {
        std::cerr << "FAIL [" << label << "] write failed" << std::endl;
        return 1;
    }

    std::vector<const char *> a;
    for (int i = 1; i < argc - 1; ++i)
        a.push_back(argv[i]);
    if (extraFlag1)
        a.push_back(extraFlag1);
    if (extraFlag2)
        a.push_back(extraFlag2);
    a.push_back(srcFile);

    auto *config = impl->CreateConfig(a.size(), a.data());
    if (!config) {
        std::cerr << "FAIL [" << label << "] CreateConfig failed" << std::endl;
        return 1;
    }
    auto *ctx = impl->CreateContextFromFile(config, srcFile);
    if (!ctx) {
        std::cerr << "FAIL [" << label << "] CreateContextFromFile failed" << std::endl;
        impl->DestroyConfig(config);
        return 1;
    }

    ProceedToAsmBoundary(ctx);

    auto state = impl->ContextState(ctx);
    // Message APIs are called while the context is alive, before any teardown:
    // GetAllErrorMessages dereferences the context allocator, which is freed by
    // FreeCompilerPartMemory — it must not have been torn down at this point.
    const char *all = impl->GetAllErrorMessages(ctx);
    const char *em = impl->ContextErrorMessage(ctx);

    int rc = 0;
    if (all == nullptr) {
        std::cerr << "FAIL [" << label << "] GetAllErrorMessages returned nullptr" << std::endl;
        rc = 1;
    }
    rc |= expectError ? CheckBoundaryError(label, state, em) : CheckBoundaryOk(ctx, label, state);

    impl->DestroyContext(ctx);
    impl->DestroyConfig(config);
    if (rc == 0)
        std::cout << "PASS [" << label << "]" << std::endl;
    return rc;
}

// Hot-reload variant of the ASM-boundary probe: the remaining reload verdicts
// (signature change, classinfo) are hot-reload-only, so the boundary probes run in
// hot mode.
static int RunAsmBoundaryHotCase(int argc, char **argv, const char *label, const char *baseSrc, const char *modSrc,
                                 bool expectError)
{
    std::string srcPath = std::string("/tmp/capi_src_") + std::to_string(getpid()) + "_" + label + ".ets";
    std::string stPath = std::string("/tmp/capi_st_") + std::to_string(getpid()) + "_" + label + ".st";
    std::string da = "--dump-symbol-table=" + stPath;
    std::string ia = "--input-symbol-table=" + stPath;

    auto [state, msg] = RunPhase(argc, argv, da.c_str(), nullptr, baseSrc, srcPath.c_str());
    if (state != ES2PANDA_STATE_BIN_GENERATED) {
        std::cerr << "FAIL [" << label << "] dump: " << S(state) << msg << std::endl;
        return 1;
    }
    return RunAsmBoundaryPhase(argc, argv, "--hot-reload", ia.c_str(), modSrc, srcPath.c_str(), expectError, label);
}

// Hot-reload suite: the five frontend rejections (two signature, three classinfo)
// plus one representative acceptance, verifying the C API hot-reload pipeline
// (HandleFunction -> DetectSignatureChanges, classinfo validation) end to end.
static int RunHotTest(const TestCase &tc, int argc, char **argv)
{
    std::string srcPath = std::string("/tmp/capi_src_") + std::to_string(getpid()) + "_hot_" + tc.label + ".ets";
    std::string stPath = std::string("/tmp/capi_st_") + std::to_string(getpid()) + "_hot_" + tc.label + ".st";
    std::string da = "--dump-symbol-table=" + stPath;
    std::string ia = "--input-symbol-table=" + stPath;

    auto [dState, dMsg] = RunPhase(argc, argv, da.c_str(), nullptr, tc.baseSrc, srcPath.c_str());
    if (dState != ES2PANDA_STATE_BIN_GENERATED) {
        std::cerr << "FAIL [" << tc.label << "] dump: " << S(dState) << dMsg << std::endl;
        return 1;
    }
    auto [state, msg] = RunPhase(argc, argv, "--hot-reload", ia.c_str(), tc.modSrc, srcPath.c_str());
    if (!tc.expectError && state != ES2PANDA_STATE_BIN_GENERATED) {
        std::cerr << "FAIL [" << tc.label << "] want BIN, got " << S(state) << " [" << msg << "]" << std::endl;
        return 1;
    }
    if (tc.expectError && (state != ES2PANDA_STATE_ERROR || msg.find("[Patch]") == std::string::npos)) {
        std::cerr << "FAIL [" << tc.label << "] want ERR with [Patch], got " << S(state) << " [" << msg << "]"
                  << std::endl;
        return 1;
    }
    std::cout << "PASS [" << tc.label << "]" << std::endl;
    return 0;
}

// Cold-reload suite: the IDE path must accept every change kind (cold reload does
// no compile-time validation; the runtime restart model tolerates all changes).
static int RunColdReloadTests(int argc, char **argv)
{
    const TestCase tests[] = {
        // --- function-level ---
        {"func-no-change", FUNC_BASE, FUNC_NO_CHANGE, false},
        {"func-body-change", FUNC_BASE, FUNC_BODY_CHANGE, false},
        {"func-sig-change", FUNC_BASE, FUNC_SIG_CHANGE, false},
        {"func-string-change", FUNC_BASE, FUNC_STRING_CHANGE, false},
        {"func-add", FUNC_BASE, FUNC_ADD, false},
        {"func-delete", FUNC_DELETE_BASE, FUNC_DELETE, false},

        // --- class method-level ---
        {"cls-body-change", CLS_BASE, CLS_BODY_CHANGE, false},
        {"cls-sig-change", CLS_SIG_CHANGE_BASE, CLS_SIG_CHANGE, false},
        {"cls-add-method", CLS_BASE, CLS_ADD, false},
        {"cls-delete-method", CLS_DELETE_BASE, CLS_DELETE, false},

        // --- field-level ---
        {"fld-type-change", FLD_BASE, FLD_TYPE_CHANGE, false},
        {"fld-init-change", FLD_BASE, FLD_INIT_CHANGE, false},
        {"fld-add", FLD_BASE, FLD_ADD, false},
        {"fld-delete", FLD_DELETE_BASE, FLD_DELETE, false},

        // --- class-set / top-level ---
        {"cls-add", CLS_ADD_BASE, CLS_ADD_NEW, false},
        {"cls-del", CLS_DEL_BASE, CLS_DEL_MOD, false},
        {"enum-add-member", ENUM_BASE, ENUM_ADD, false},
        {"top-level-const-change", CONST_BASE, CONST_CHANGE, false},
    };

    int rc = 0;
    for (const auto &t : tests) {
        rc |= RunTest(t, argc, argv);
    }
    return rc;
}

// Hot-reload suite: the frontend rejections (two signature, four classinfo) plus
// representative acceptances, verifying the C API hot-reload pipeline
// (HandleFunction -> DetectSignatureChanges, classinfo validation) end to end.
static int RunHotReloadTests(int argc, char **argv)
{
    const TestCase hotTests[] = {
        {"hot-func-body-change", FUNC_BASE, FUNC_BODY_CHANGE, false},
        {"hot-func-sig-change", FUNC_BASE, FUNC_SIG_CHANGE, true},
        {"hot-cls-method-sig-change", CLS_SIG_CHANGE_BASE, CLS_SIG_CHANGE, true},
        {"hot-change-parent", PARENT_CHANGE_BASE, PARENT_CHANGE_MOD, true},
        {"hot-ns-class-parent-change", NS_PARENT_BASE, NS_PARENT_CHANGE, true},
        {"hot-add-interface", IFACE_BASE, IFACE_ADD, true},
        {"hot-delete-interface", IFACE_BASE, IFACE_DEL, true},
        {"hot-iface-order-swap", IFACE_ORDER_BASE, IFACE_ORDER_SWAP, false},
    };
    int rc = 0;
    for (const auto &t : hotTests) {
        rc |= RunHotTest(t, argc, argv);
    }
    return rc;
}

// ASM_GENERATED boundary (IDE lifecycle): the reload verdict must be final at the
// boundary where the embedder reads messages and decides.
static int RunAsmBoundaryTests(int argc, char **argv)
{
    int rc = RunAsmBoundaryHotCase(argc, argv, "asm-boundary-error", PARENT_CHANGE_BASE, PARENT_CHANGE_MOD, true);
    rc |= RunAsmBoundaryHotCase(argc, argv, "asm-boundary-ok", FUNC_BASE, FUNC_ADD, false);
    return rc;
}

int main(int argc, char **argv)
{
    if (argc < MIN_ARGC)
        return INVALID_ARGC_ERROR_CODE;
    if (GetImpl() == nullptr)
        return NULLPTR_IMPL_ERROR_CODE;
    impl = GetImpl();
    std::cout << "LOAD SUCCESS" << std::endl;

    int rc = RunColdReloadTests(argc, argv);
    rc |= RunHotReloadTests(argc, argv);
    rc |= RunAsmBoundaryTests(argc, argv);
    if (rc)
        return 1;
    std::cout << "ALL DONE" << std::endl;
    return 0;
}
// NOLINTEND
