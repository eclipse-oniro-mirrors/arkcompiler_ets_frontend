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

int main(int argc, char **argv)
{
    if (argc < MIN_ARGC)
        return INVALID_ARGC_ERROR_CODE;
    if (GetImpl() == nullptr)
        return NULLPTR_IMPL_ERROR_CODE;
    impl = GetImpl();
    std::cout << "LOAD SUCCESS" << std::endl;

    const TestCase tests[] = {
        // --- function-level ---
        {"func-no-change", FUNC_BASE, FUNC_NO_CHANGE, false},
        {"func-body-change", FUNC_BASE, FUNC_BODY_CHANGE, true},
        {"func-sig-change", FUNC_BASE, FUNC_SIG_CHANGE, true},
        {"func-string-change", FUNC_BASE, FUNC_STRING_CHANGE, true},
        {"func-add", FUNC_BASE, FUNC_ADD, false},
        {"func-delete", FUNC_DELETE_BASE, FUNC_DELETE, false},

        // --- class method-level ---
        {"cls-body-change", CLS_BASE, CLS_BODY_CHANGE, true},
        {"cls-sig-change", CLS_SIG_CHANGE_BASE, CLS_SIG_CHANGE, true},
        {"cls-add-method", CLS_BASE, CLS_ADD, false},
        {"cls-delete-method", CLS_DELETE_BASE, CLS_DELETE, false},

        // --- field-level ---
        {"fld-type-change", FLD_BASE, FLD_TYPE_CHANGE, true},
        {"fld-init-change", FLD_BASE, FLD_INIT_CHANGE, true},
        {"fld-add", FLD_BASE, FLD_ADD, true},
        {"fld-delete", FLD_DELETE_BASE, FLD_DELETE, true},
    };

    int rc = 0;
    for (const auto &t : tests) {
        rc |= RunTest(t, argc, argv);
    }

    if (rc)
        return 1;
    std::cout << "ALL DONE" << std::endl;
    return 0;
}
// NOLINTEND
