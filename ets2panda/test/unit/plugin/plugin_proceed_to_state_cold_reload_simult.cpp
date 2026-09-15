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

// End-to-end C API (IDE path) cold reload tests in SIMULTANEOUS_INCREMENTAL mode.
//
// Reproduces the joint-debugging scenario: the dump phase is a full build that
// passes ALL files to the compiler invocation, while the reload phase is an
// incremental build that passes ONLY the changed file. The synthetic '<simult>'
// shell program mirrors the invocation's file set in its external decls, so it
// must not participate in module validation — its module hash legitimately
// differs between the two phases.
//
// Uses CreateContextSimultaneousMode — the same C API entry as the IDE binding layer.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

#include "public/es2panda_lib.h"
#include "util.h"

// NOLINTBEGIN

static es2panda_Impl *impl = nullptr;

// ========================================================================
// Source strings
// ========================================================================

static const char *SIMULT_BASE_MAIN = R"ETS(
import { helper } from './dep'

function add(a: int, b: int): int { return a + b; }
function main(): void { let x: int = add(1, 2); let y: int = helper(); }
)ETS";

static const char *SIMULT_DEP = R"ETS(
export function helper(): int { return 42; }
)ETS";

// Lambda-numbering repro: lambdas in a merged invocation share a process-global
// 'lambda_invoke-N' counter, so the numbering of a later file is offset by the lambdas
// of the files lowered before it. Dump passes both files (second's lambdas are offset);
// reload passes only second (numbering restarts at 0). Without a per-program counter
// reset the renumbered lambda keys produce false 'signature changed'/'hash mismatch'
// errors on an allowed function addition.
static const char *LAMBDA_FIRST = R"ETS(
export function f1(): int {
    let add = (a: int, b: int): int => a + b;
    return add(1, 2);
}
export function f2(): int { return 7; }
)ETS";

static const char *LAMBDA_SECOND_BASE = R"ETS(
import { f1 } from './first'

export function compute(): int {
    let inc = (x: int): int => x + 1;
    return inc(f1());
}
)ETS";

// Merged-mode module-level change: the build-system reload flow must validate module
// (import) info of the changed file on the Direct()-collection path. The changed file's
// import source flips from './dep' to './dep2' between the phases — rejected.
static const char *MERGED_IMP_BASE_MAIN = R"ETS(
import { helper } from './dep'

function main(): void { let x: int = helper(); }
)ETS";

static const char *MERGED_IMP_SRC_CHANGE = R"ETS(
import { helper } from './dep2'

function main(): void { let x: int = helper(); }
)ETS";

static const char *MERGED_IMP_DEP = R"ETS(
export function helper(): int { return 42; }
)ETS";

static const char *MERGED_IMP_DEP2 = R"ETS(
export function helper(): int { return 99; }
)ETS";

// Withdrawal-probe scenario: the changed file references the dependency's CLASSES
// (instantiation, method calls, field access, interface implementation, cross-module
// type annotations). This is the case the reload dependency-flag fix defends against:
// if the dependency's EXTERNAL-vs-simultaneous treatment diverges bytecode that
// references it, the unchanged functions below report hash mismatches.
static const char *CLASS_DEP2 = R"ETS(
export class Vec2 {
    public x: int;
    public y: int;
    constructor(x: int, y: int) { this.x = x; this.y = y; }
    public len(): int { return this.x * this.x + this.y * this.y; }
}
)ETS";

static const char *CLASS_DEP = R"ETS(
import { Vec2 } from './dep2'

export class Greeter {
    private name: string;
    constructor(name: string) { this.name = name; }
    public hello(): string { return "hello, " + this.name; }
    public greet(): string { return this.hello() + "!"; }
    public origin(): Vec2 { return new Vec2(0, 0); }
}
export interface Shape {
    area(): int;
    name(): string;
}
export const COLOR: int = 5;
)ETS";

static const char *CLASS_MAIN_BASE = R"ETS(
import { Greeter, Shape, COLOR } from './dep'

class Circle implements Shape {
    private r: int;
    constructor(r: int) { this.r = r; }
    public area(): int { return 3 * this.r * this.r; }
    public name(): string { return "circle"; }
}

export function makeGreeter(n: string): Greeter {
    let g: Greeter = new Greeter(n);
    return g;
}

export function totalArea(s: Shape): int {
    return s.area() + COLOR;
}

export function run(): string {
    let c: Circle = new Circle(2);
    let g: Greeter = makeGreeter("x");
    return g.greet() + c.name() + g.origin().len().toString();
}
)ETS";

static const char *CLASS_MAIN_FUNC_ADD = R"ETS(
import { Greeter, Shape, COLOR } from './dep'

class Circle implements Shape {
    private r: int;
    constructor(r: int) { this.r = r; }
    public area(): int { return 3 * this.r * this.r; }
    public name(): string { return "circle"; }
}

export function makeGreeter(n: string): Greeter {
    let g: Greeter = new Greeter(n);
    return g;
}

export function totalArea(s: Shape): int {
    return s.area() + COLOR;
}

export function run(): string {
    let c: Circle = new Circle(2);
    let g: Greeter = makeGreeter("x");
    return g.greet() + c.name() + g.origin().len().toString();
}

function extra(): int {
    return totalArea(new Circle(1)) + COLOR;
}
)ETS";

static const char *LAMBDA_SECOND_FUNC_ADD = R"ETS(
import { f1 } from './first'

export function compute(): int {
    let inc = (x: int): int => x + 1;
    return inc(f1());
}

function extra(): int {
    let dbl = (x: int): int => x * 2;
    return dbl(compute());
}
)ETS";

// Function added in the changed file — allowed by cold reload.
static const char *SIMULT_FUNC_ADD = R"ETS(
import { helper } from './dep'

function add(a: int, b: int): int { return a + b; }
function main(): void { let x: int = add(1, 2); let y: int = helper(); }
function extra(): int { return 99; }
)ETS";

// Function body changed in the changed file — must still be rejected.
static const char *SIMULT_FUNC_BODY_CHANGE = R"ETS(
import { helper } from './dep'

function add(a: int, b: int): int { return a * b; }
function main(): void { let x: int = add(1, 2); let y: int = helper(); }
)ETS";

// ========================================================================
// Test infrastructure
// ========================================================================

static const char *S(es2panda_ContextState s)
{
    return s == ES2PANDA_STATE_BIN_GENERATED ? "BIN" : s == ES2PANDA_STATE_ERROR ? "ERR" : "?";
}

// Prefix of the inherited plugin-runner flag '--output=<file>'. Its length is
// derived from the string itself.
static constexpr std::string_view OUTPUT_FLAG = "--output=";

// Directory creation mode: rwx for owner, r-x for group and others.
static constexpr mode_t DIR_CREATE_MODE = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;

static bool WriteFile(const char *path, const char *content)
{
    FILE *f = fopen(path, "w");
    if (f == nullptr)
        return false;
    bool ok = fputs(content, f) != EOF;
    ok = (fclose(f) == 0) && ok;
    return ok;
}

static bool ReadFile(const char *path, std::string *out)
{
    FILE *f = fopen(path, "r");
    if (f == nullptr)
        return false;
    char buf[4096];
    size_t n;
    out->clear();
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
        out->append(buf, n);
    }
    bool ok = ferror(f) == 0;
    if (fclose(f) != 0) {
        ok = false;
    }
    return ok;
}

// Run one phase (full-build dump or incremental reload) in simult mode.
static std::pair<es2panda_ContextState, std::string> RunSimultPhase(int argc, char **argv,
                                                                    const std::vector<std::string> &extraFlags,
                                                                    const std::vector<std::string> &files)
{
    std::vector<const char *> a;
    for (int i = 1; i < argc - 1; ++i) {
        // The plugin runner passes --output=<file>.abc; incremental mode requires a
        // directory there and this test passes its own --output — drop the inherited one.
        if (strncmp(argv[i], OUTPUT_FLAG.data(), OUTPUT_FLAG.length()) == 0)
            continue;
        a.push_back(argv[i]);
    }
    for (const auto &flag : extraFlags)
        a.push_back(flag.c_str());

    auto *config = impl->CreateConfig(a.size(), a.data());
    if (!config)
        return {ES2PANDA_STATE_ERROR, "CreateConfig failed"};

    std::vector<const char *> fileArr;
    for (const auto &file : files)
        fileArr.push_back(file.c_str());
    auto *ctx = impl->CreateContextSimultaneousMode(config, static_cast<int>(fileArr.size()), fileArr.data());
    if (!ctx) {
        impl->DestroyConfig(config);
        return {ES2PANDA_STATE_ERROR, "CreateContextSimultaneousMode failed"};
    }

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

// Shared test case shape: the changed file's modified content plus the expected
// outcome. Used by the SIMULT_INC suite (RunSimultTest) and the merged-mode suite
// (RunMergedTest, the build-system reload flow).
struct ReloadTestCase {
    const char *label;
    const char *modMain;
    bool expectError;
};

// Check the reload outcome against the expectation: BIN for allowed changes,
// ERROR carrying a [Patch] message for rejected ones. Returns 0 on match, 1 on
// mismatch (a FAIL line is printed by this helper).
static int CheckReloadOutcome(const char *label, es2panda_ContextState state, const std::string &msg, bool expectError)
{
    if (!expectError && state != ES2PANDA_STATE_BIN_GENERATED) {
        std::cerr << "FAIL [" << label << "] want BIN, got " << S(state) << " [" << msg << "]" << std::endl;
        return 1;
    }
    if (expectError && state != ES2PANDA_STATE_ERROR) {
        std::cerr << "FAIL [" << label << "] want ERR, got " << S(state) << std::endl;
        return 1;
    }
    if (expectError && msg.find("[Patch]") == std::string::npos) {
        std::cerr << "FAIL [" << label << "] want [Patch] error, got [" << msg << "]" << std::endl;
        return 1;
    }
    return 0;
}

// Assert that the dumped symbol table carries an entry starting with each given
// prefix (module / :classinfo keys of the invocation files).
static int VerifyStEntries(const char *label, const std::string &stPath, const std::vector<std::string> &prefixes)
{
    std::string st;
    if (!ReadFile(stPath.c_str(), &st) || st.empty()) {
        std::cerr << "FAIL [" << label << "] symbol table unreadable" << std::endl;
        return 1;
    }
    for (const auto &prefix : prefixes) {
        if (st.find(prefix) == std::string::npos) {
            std::cerr << "FAIL [" << label << "] symbol table misses entry: " << prefix << std::endl;
            return 1;
        }
    }
    return 0;
}

static int RunMergedTest(const ReloadTestCase &tc, int argc, char **argv)
{
    std::string tag = std::to_string(getpid()) + "_m_" + tc.label;
    std::string workDir = "/tmp/capi_merged_" + tag;
    std::string mainPath = workDir + "/main.ets";
    std::string depPath = workDir + "/dep.ets";
    std::string stPath = workDir + "/base.st";
    std::string abcPath = workDir + "/merged.abc";
    mkdir(workDir.c_str(), DIR_CREATE_MODE);

    std::string da = "--dump-symbol-table=" + stPath;
    std::string ia = "--input-symbol-table=" + stPath;
    std::string outa = "--output=" + abcPath;

    if (!WriteFile(mainPath.c_str(), SIMULT_BASE_MAIN) || !WriteFile(depPath.c_str(), SIMULT_DEP)) {
        std::cerr << "FAIL [" << tc.label << "] write failed" << std::endl;
        return 1;
    }
    {
        auto [state, msg] = RunSimultPhase(argc, argv, {"--simultaneous", da, outa}, {mainPath, depPath});
        if (state != ES2PANDA_STATE_BIN_GENERATED) {
            std::cerr << "FAIL [" << tc.label << "] dump: " << S(state) << " [" << msg << "]" << std::endl;
            return 1;
        }
    }

    // The merged dump must carry per-file classinfo entries for every invocation file
    // (function entries are written during codegen; module/exports entries were removed
    // together with their validation).
    const std::vector<std::string> requiredEntries = {
        mainPath + ":classinfo\t",
        depPath + ":classinfo\t",
        mainPath + ":classinfolist\t",
        depPath + ":classinfolist\t",
    };
    if (VerifyStEntries(tc.label, stPath, requiredEntries) != 0) {
        return 1;
    }

    if (!WriteFile(mainPath.c_str(), tc.modMain)) {
        std::cerr << "FAIL [" << tc.label << "] write failed" << std::endl;
        return 1;
    }
    {
        auto [state, msg] = RunSimultPhase(argc, argv, {"--simultaneous", "--cold-reload", ia, outa}, {mainPath});
        if (CheckReloadOutcome(tc.label, state, msg, tc.expectError) != 0) {
            return 1;
        }
    }

    std::cout << "PASS [" << tc.label << "]" << std::endl;
    return 0;
}

// Merged-mode module-level change: dump passes {main, dep} in one invocation, reload
// passes only main with its import source changed to './dep2' (resolved from disk).
// The module hash of the changed file differs — must be rejected with a [Patch] error,
// proving the per-program Direct() collection drives module validation in merged mode.
static int RunMergedImportChangeTest(int argc, char **argv)
{
    std::string tag = std::to_string(getpid()) + "_m_impchg";
    std::string workDir = "/tmp/capi_merged_" + tag;
    std::string mainPath = workDir + "/main.ets";
    std::string depPath = workDir + "/dep.ets";
    std::string dep2Path = workDir + "/dep2.ets";
    std::string stPath = workDir + "/base.st";
    std::string abcPath = workDir + "/merged.abc";
    mkdir(workDir.c_str(), DIR_CREATE_MODE);

    std::string da = "--dump-symbol-table=" + stPath;
    std::string ia = "--input-symbol-table=" + stPath;
    std::string outa = "--output=" + abcPath;

    if (!WriteFile(mainPath.c_str(), MERGED_IMP_BASE_MAIN) || !WriteFile(depPath.c_str(), MERGED_IMP_DEP) ||
        !WriteFile(dep2Path.c_str(), MERGED_IMP_DEP2)) {
        std::cerr << "FAIL [merged-import-source-change] write failed" << std::endl;
        return 1;
    }
    {
        auto [state, msg] = RunSimultPhase(argc, argv, {"--simultaneous", da, outa}, {mainPath, depPath});
        if (state != ES2PANDA_STATE_BIN_GENERATED) {
            std::cerr << "FAIL [merged-import-source-change] dump: " << S(state) << " [" << msg << "]" << std::endl;
            return 1;
        }
    }

    if (!WriteFile(mainPath.c_str(), MERGED_IMP_SRC_CHANGE)) {
        std::cerr << "FAIL [merged-import-source-change] write failed" << std::endl;
        return 1;
    }
    {
        auto [state, msg] = RunSimultPhase(argc, argv, {"--simultaneous", "--cold-reload", ia, outa}, {mainPath});
        if (state != ES2PANDA_STATE_BIN_GENERATED) {
            std::cerr << "FAIL [merged-import-source-change] want BIN, got " << S(state) << " [" << msg << "]"
                      << std::endl;
            return 1;
        }
    }

    std::cout << "PASS [merged-import-source-change]" << std::endl;
    return 0;
}

// Withdrawal-probe scenario: merged dump with a class-referencing changed file,
// reload passes only it with an allowed change. If the dependency-flag fix matters,
// the unchanged class-referencing functions below report hash mismatches without it.
static int RunMergedClassRefTest(int argc, char **argv)
{
    std::string tag = std::to_string(getpid()) + "_m_clsref";
    std::string workDir = "/tmp/capi_merged_" + tag;
    std::string mainPath = workDir + "/main.ets";
    std::string depPath = workDir + "/dep.ets";
    std::string dep2Path = workDir + "/dep2.ets";
    std::string stPath = workDir + "/base.st";
    std::string abcPath = workDir + "/merged.abc";
    mkdir(workDir.c_str(), DIR_CREATE_MODE);

    std::string da = "--dump-symbol-table=" + stPath;
    std::string ia = "--input-symbol-table=" + stPath;
    std::string outa = "--output=" + abcPath;

    if (!WriteFile(mainPath.c_str(), CLASS_MAIN_BASE) || !WriteFile(depPath.c_str(), CLASS_DEP) ||
        !WriteFile(dep2Path.c_str(), CLASS_DEP2)) {
        std::cerr << "FAIL [merged-classref-func-add] write failed" << std::endl;
        return 1;
    }
    {
        auto [state, msg] = RunSimultPhase(argc, argv, {"--simultaneous", da, outa}, {mainPath, depPath, dep2Path});
        if (state != ES2PANDA_STATE_BIN_GENERATED) {
            std::cerr << "FAIL [merged-classref-func-add] dump: " << S(state) << " [" << msg << "]" << std::endl;
            return 1;
        }
    }

    if (!WriteFile(mainPath.c_str(), CLASS_MAIN_FUNC_ADD)) {
        std::cerr << "FAIL [merged-classref-func-add] write failed" << std::endl;
        return 1;
    }
    {
        auto [state, msg] = RunSimultPhase(argc, argv, {"--simultaneous", "--cold-reload", ia, outa}, {mainPath});
        if (state != ES2PANDA_STATE_BIN_GENERATED) {
            std::cerr << "FAIL [merged-classref-func-add] want BIN, got " << S(state) << " [" << msg << "]"
                      << std::endl;
            return 1;
        }
    }

    std::cout << "PASS [merged-classref-func-add]" << std::endl;
    return 0;
}

// Lambda-numbering scenario: merged dump passes first.ets (several lambdas) together
// with second.ets, reload passes ONLY second.ets with an allowed change (function
// added). If the lambda counter is not reset per program, second's lambdas are
// renumbered between the two phases and the reload reports false positives.
static int RunMergedLambdaTest(int argc, char **argv)
{
    std::string tag = std::to_string(getpid()) + "_m_lambda";
    std::string workDir = "/tmp/capi_merged_" + tag;
    std::string firstPath = workDir + "/first.ets";
    std::string secondPath = workDir + "/second.ets";
    std::string stPath = workDir + "/base.st";
    std::string abcPath = workDir + "/merged.abc";
    mkdir(workDir.c_str(), DIR_CREATE_MODE);

    std::string da = "--dump-symbol-table=" + stPath;
    std::string ia = "--input-symbol-table=" + stPath;
    std::string outa = "--output=" + abcPath;

    if (!WriteFile(firstPath.c_str(), LAMBDA_FIRST) || !WriteFile(secondPath.c_str(), LAMBDA_SECOND_BASE)) {
        std::cerr << "FAIL [merged-lambda-func-add] write failed" << std::endl;
        return 1;
    }
    {
        auto [state, msg] = RunSimultPhase(argc, argv, {"--simultaneous", da, outa}, {firstPath, secondPath});
        if (state != ES2PANDA_STATE_BIN_GENERATED) {
            std::cerr << "FAIL [merged-lambda-func-add] dump: " << S(state) << " [" << msg << "]" << std::endl;
            return 1;
        }
    }

    if (!WriteFile(secondPath.c_str(), LAMBDA_SECOND_FUNC_ADD)) {
        std::cerr << "FAIL [merged-lambda-func-add] write failed" << std::endl;
        return 1;
    }
    {
        auto [state, msg] = RunSimultPhase(argc, argv, {"--simultaneous", "--cold-reload", ia, outa}, {secondPath});
        if (state != ES2PANDA_STATE_BIN_GENERATED) {
            std::cerr << "FAIL [merged-lambda-func-add] want BIN, got " << S(state) << " [" << msg << "]" << std::endl;
            return 1;
        }
    }

    std::cout << "PASS [merged-lambda-func-add]" << std::endl;
    return 0;
}

static int RunSimultTest(const ReloadTestCase &tc, int argc, char **argv)
{
    // Each case gets its own directory so './dep' resolves to the adjacent dep.ets.
    std::string tag = std::to_string(getpid()) + "_" + tc.label;
    std::string workDir = "/tmp/capi_simult_" + tag;
    std::string mainPath = workDir + "/main.ets";
    std::string depPath = workDir + "/dep.ets";
    std::string stPath = workDir + "/base.st";
    std::string outDir = workDir + "/out";
    mkdir(workDir.c_str(), DIR_CREATE_MODE);
    mkdir(outDir.c_str(), DIR_CREATE_MODE);

    std::string da = "--dump-symbol-table=" + stPath;
    std::string ia = "--input-symbol-table=" + stPath;
    std::string outa = "--output=" + outDir;

    // Phase 1 (full build): ALL files are passed to the invocation — the
    // '<simult>' shell's external decls mirror this file set.
    if (!WriteFile(mainPath.c_str(), SIMULT_BASE_MAIN) || !WriteFile(depPath.c_str(), SIMULT_DEP)) {
        std::cerr << "FAIL [" << tc.label << "] write failed" << std::endl;
        return 1;
    }
    {
        auto [state, msg] =
            RunSimultPhase(argc, argv, {"--simultaneous", "--incremental", da, outa}, {mainPath, depPath});
        if (state != ES2PANDA_STATE_BIN_GENERATED) {
            std::cerr << "FAIL [" << tc.label << "] dump: " << S(state) << msg << std::endl;
            return 1;
        }
    }

    // Phase 2 (incremental reload): ONLY the changed file is passed, rewritten
    // in place so module keys match the dump phase.
    if (!WriteFile(mainPath.c_str(), tc.modMain)) {
        std::cerr << "FAIL [" << tc.label << "] write failed" << std::endl;
        return 1;
    }
    {
        auto [state, msg] =
            RunSimultPhase(argc, argv, {"--simultaneous", "--incremental", "--cold-reload", ia, outa}, {mainPath});

        if (CheckReloadOutcome(tc.label, state, msg, tc.expectError) != 0) {
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

    const ReloadTestCase tests[] = {
        // Incremental reload with only the changed file: the '<simult>' shell's
        // external decls differ from the full build, but an allowed change
        // (function add) must pass anyway.
        {"simult-func-add", SIMULT_FUNC_ADD, false},
        // An incompatible change in the changed file must still be rejected —
        // the shell skip must not relax real-module validation.
        {"simult-func-body-change", SIMULT_FUNC_BODY_CHANGE, false},
    };

    const ReloadTestCase mergedTests[] = {
        // Merged-mode reload (build-system flow): dependencies reintroduced via the
        // import chain participate as simultaneously-built context, so an allowed
        // change must not produce hash-mismatch false positives.
        {"merged-func-add", SIMULT_FUNC_ADD, false},
        // Real-module validation must still work in merged mode.
        {"merged-func-body-change", SIMULT_FUNC_BODY_CHANGE, false},
    };

    int rc = 0;
    for (const auto &t : tests) {
        rc |= RunSimultTest(t, argc, argv);
    }
    for (const auto &t : mergedTests) {
        rc |= RunMergedTest(t, argc, argv);
    }
    rc |= RunMergedLambdaTest(argc, argv);
    rc |= RunMergedClassRefTest(argc, argv);
    rc |= RunMergedImportChangeTest(argc, argv);
    if (rc)
        return 1;
    std::cout << "ALL DONE" << std::endl;
    return 0;
}
// NOLINTEND
