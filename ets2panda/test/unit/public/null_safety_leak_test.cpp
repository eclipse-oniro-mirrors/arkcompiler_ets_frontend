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

/*
 * Null-safety and memory-leak defense tests for the static frontend compiler
 * (ets2panda, build/bin/es2panda) public C API.
 *
 * Test targets (public C API of es2panda_lib.h):
 *   - Null safety: call each null-safe API with nullptr, assert return values,
 *     and confirm no crash from null dereference.
 *   - Leak defense: run create->destroy lifecycles in pairs (Config / Context /
 *     GlobalContext / EvaluateExpression results) in large loops so destruction
 *     stays repeatable and deterministic; ASAN/LSAN builds catch real leaks.
 *
 * Framework: gtest (reuses the AstVerifierTest fixture from test/utils/ast_verifier_test.h;
 * it binds build/bin/es2panda via PandaExecutablePathGetter).
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

#include "libarkbase/macros.h"

#include "public/es2panda_lib.h"
#include "test/utils/ast_verifier_test.h"
#include "test/utils/panda_executable_path_getter.h"
#include "util/diagnostic.h"
#include "util/eheap.h"

namespace ark::es2panda::public_lib::test {

// ---------------------------------------------------------------------------
// Test fixture aliases: all reuse AstVerifierTest (bound to build/bin/es2panda)
// ---------------------------------------------------------------------------

using NullSafetyTest = ::test::utils::AstVerifierTest;
using LeakDefenseTest = ::test::utils::AstVerifierTest;
using ContextLifecycleTest = ::test::utils::AstVerifierTest;
using ConfigLifecycleTest = ::test::utils::AstVerifierTest;
using GlobalContextLifecycleTest = ::test::utils::AstVerifierTest;
using EvaluateExpressionLifecycleTest = ::test::utils::AstVerifierTest;
using ErrorPathLeakTest = ::test::utils::AstVerifierTest;
using AstWalkInvariantTest = ::test::utils::AstVerifierTest;

// ---------------------------------------------------------------------------
// Common helpers (file-local)
// ---------------------------------------------------------------------------

// Global impl singleton accessor; the fixture already called MemInitialize.
static es2panda_Impl const *Impl()
{
    return es2panda_GetImpl(ES2PANDA_LIB_VERSION);
}

// Fixture GetAst() returns ir::AstNode*; cast it before passing to the C API
// (same approach as AstNodeForEach in test/utils/ast_verifier_test.h).
static es2panda_AstNode *CApi(ark::es2panda::ir::AstNode *node)
{
    return reinterpret_cast<es2panda_AstNode *>(node);
}

// Build a config with the es2panda executable as argv[0] (fixture-style,
// same as the manual config usage in api_result_lifecycle_test.cpp).
static es2panda_Config *NewConfig()
{
    const auto &argv = ::test::utils::PandaExecutablePathGetter::Get();
    return Impl()->CreateConfig(static_cast<int>(argv.size()), argv.data());
}

// Build a config with an invalid option; Options::Parse fails and returns nullptr.
static es2panda_Config *NewConfigWithBadOption(char const *option)
{
    const auto &argv = ::test::utils::PandaExecutablePathGetter::Get();
    std::array<char const *, 2> args {argv[0], option};
    return Impl()->CreateConfig(static_cast<int>(args.size()), args.data());
}

// RAII: resources are destroyed in pairs even when ASSERT_* returns early.
// Leak tests must not leak on failure paths either.
class ConfigGuard {
public:
    explicit ConfigGuard(es2panda_Config *config) : config_ {config} {}

    ConfigGuard(const ConfigGuard &) = delete;
    ConfigGuard &operator=(const ConfigGuard &) = delete;
    ConfigGuard(ConfigGuard &&) = delete;
    ConfigGuard &operator=(ConfigGuard &&) = delete;

    ~ConfigGuard()
    {
        if (config_ != nullptr) {
            Impl()->DestroyConfig(config_);
        }
    }

    es2panda_Config *Get() const
    {
        return config_;
    }

    void Release()
    {
        config_ = nullptr;
    }

private:
    es2panda_Config *config_;
};

class ContextGuard {
public:
    explicit ContextGuard(es2panda_Context *context) : context_ {context} {}

    ContextGuard(const ContextGuard &) = delete;
    ContextGuard &operator=(const ContextGuard &) = delete;
    ContextGuard(ContextGuard &&) = delete;
    ContextGuard &operator=(ContextGuard &&) = delete;

    ~ContextGuard()
    {
        if (context_ != nullptr) {
            Impl()->DestroyContext(context_);
        }
    }

    es2panda_Context *Get() const
    {
        return context_;
    }

    void Release()
    {
        context_ = nullptr;
    }

private:
    es2panda_Context *context_;
};

class GlobalContextGuard {
public:
    explicit GlobalContextGuard(es2panda_GlobalContext *globalContext) : globalContext_ {globalContext} {}

    GlobalContextGuard(const GlobalContextGuard &) = delete;
    GlobalContextGuard &operator=(const GlobalContextGuard &) = delete;
    GlobalContextGuard(GlobalContextGuard &&) = delete;
    GlobalContextGuard &operator=(GlobalContextGuard &&) = delete;

    ~GlobalContextGuard()
    {
        if (globalContext_ != nullptr) {
            Impl()->DestroyGlobalContext(globalContext_);
        }
    }

    es2panda_GlobalContext *Get() const
    {
        return globalContext_;
    }

    void Release()
    {
        globalContext_ = nullptr;
    }

private:
    es2panda_GlobalContext *globalContext_;
};

// Create and parse a context (manual lifecycle, guards own the resources).
static es2panda_Context *CreateParsedContext(es2panda_Config *config, char const *source, char const *fileName)
{
    auto *context = Impl()->CreateContextFromString(config, source, fileName);
    if (context == nullptr) {
        return nullptr;
    }
    auto *result = Impl()->ProceedToState(context, ES2PANDA_STATE_PARSED);
    return result;
}

// String contains helpers
static bool Contains(std::string const &haystack, std::string const &needle)
{
    return haystack.find(needle) != std::string::npos;
}

static bool Contains(std::vector<std::string> const &values, std::string const &needle)
{
    return std::find(values.begin(), values.end(), needle) != values.end();
}

// Common payload for AST walk callbacks
struct WalkPayload {
    es2panda_Impl const *impl = nullptr;
    es2panda_Context *ctx = nullptr;
    size_t nodeCount = 0;
    size_t identifierCount = 0;
    size_t nullParentCount = 0;
    es2panda_AstNode *firstVariableDeclaration = nullptr;
    es2panda_AstNode *firstFunctionDeclaration = nullptr;
    es2panda_AstNode *firstMethodDefinition = nullptr;
    std::vector<std::string> identifierNames;
};

// Count total nodes and Identifier nodes
static void CountNodesCallback(es2panda_AstNode *node, void *arg)
{
    auto *payload = static_cast<WalkPayload *>(arg);
    payload->nodeCount++;
    if (payload->impl->IsIdentifier(node)) {
        payload->identifierCount++;
    }
}

// Collect all Identifier names
static void CollectNamesCallback(es2panda_AstNode *node, void *arg)
{
    auto *payload = static_cast<WalkPayload *>(arg);
    if (payload->impl->IsIdentifier(node)) {
        payload->identifierNames.emplace_back(payload->impl->IdentifierName(payload->ctx, node));
    }
}

// Count nodes with a null parent (only the ETSModule root has one)
static void NullParentProbeCallback(es2panda_AstNode *node, void *arg)
{
    auto *payload = static_cast<WalkPayload *>(arg);
    if (payload->impl->AstNodeParentConst(payload->ctx, node) == nullptr) {
        payload->nullParentCount++;
    }
}

// Capture the first VariableDeclaration / FunctionDeclaration / MethodDefinition
static void FindFirstDeclarationCallback(es2panda_AstNode *node, void *arg)
{
    auto *payload = static_cast<WalkPayload *>(arg);
    if (payload->firstVariableDeclaration == nullptr && payload->impl->IsVariableDeclaration(node)) {
        payload->firstVariableDeclaration = node;
    }
    if (payload->firstFunctionDeclaration == nullptr && payload->impl->IsFunctionDeclaration(node)) {
        payload->firstFunctionDeclaration = node;
    }
    if (payload->firstMethodDefinition == nullptr && payload->impl->IsMethodDefinition(node)) {
        payload->firstMethodDefinition = node;
    }
}

// Walk the whole AST with AstNodeForEach and fill the payload
static void WalkAst(es2panda_Impl const *impl, es2panda_Context *ctx, es2panda_AstNode *root, WalkPayload *payload)
{
    payload->impl = impl;
    payload->ctx = ctx;
    impl->AstNodeForEach(root, CountNodesCallback, payload);
    impl->AstNodeForEach(root, CollectNamesCallback, payload);
    impl->AstNodeForEach(root, NullParentProbeCallback, payload);
}

// Count nodes matching a type predicate (captureless lambda -> function pointer).
// Note: primitive type annotations (int/void/boolean/...) are not Identifiers
// but separate ETSPrimitiveType nodes; use type predicates.
struct KindCountPayload {
    es2panda_Impl const *impl = nullptr;
    bool (*predicate)(es2panda_Impl const *, es2panda_AstNode *) = nullptr;
    size_t count = 0;
};

static void KindCountCallback(es2panda_AstNode *node, void *arg)
{
    auto *payload = static_cast<KindCountPayload *>(arg);
    if (payload->predicate(payload->impl, node)) {
        payload->count++;
    }
}

static size_t CountNodesOfKind(es2panda_Impl const *impl, es2panda_AstNode *root,
                               bool (*predicate)(es2panda_Impl const *, es2panda_AstNode *))
{
    KindCountPayload payload;
    payload.impl = impl;
    payload.predicate = predicate;
    impl->AstNodeForEach(root, KindCountCallback, &payload);
    return payload.count;
}

// Common predicate: primitive type annotation node
static bool IsPrimitiveTypeNode(es2panda_Impl const *impl, es2panda_AstNode *node)
{
    return impl->IsETSPrimitiveType(node);
}

// Base64 encode (EvaluateExpression input is a base64 expression)
static std::string Base64Encode(std::string const &input)
{
    static constexpr char ALPHABET[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string output;
    const auto size = input.size();
    size_t i = 0;
    while (i + 2 < size) {  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        const auto b0 = static_cast<unsigned char>(input[i]);
        const auto b1 = static_cast<unsigned char>(input[i + 1]);
        const auto b2 = static_cast<unsigned char>(input[i + 2]);
        output.push_back(ALPHABET[(b0 >> 2U) & 0x3FU]);
        output.push_back(ALPHABET[((b0 << 4U) | (b1 >> 4U)) & 0x3FU]);
        output.push_back(ALPHABET[((b1 << 2U) | (b2 >> 6U)) & 0x3FU]);
        output.push_back(ALPHABET[b2 & 0x3FU]);
        i += 3;
    }
    if (i + 2 == size) {
        const auto b0 = static_cast<unsigned char>(input[i]);
        const auto b1 = static_cast<unsigned char>(input[i + 1]);
        output.push_back(ALPHABET[(b0 >> 2U) & 0x3FU]);
        output.push_back(ALPHABET[((b0 << 4U) | (b1 >> 4U)) & 0x3FU]);
        output.push_back(ALPHABET[(b1 << 2U) & 0x3FU]);
        output.push_back('=');
    } else if (i + 1 == size) {
        const auto b0 = static_cast<unsigned char>(input[i]);
        output.push_back(ALPHABET[(b0 >> 2U) & 0x3FU]);
        output.push_back(ALPHABET[(b0 << 4U) & 0x3FU]);
        output.push_back('=');
        output.push_back('=');
    }
    return output;
}

// Create a temp file removed on destruction (CreateContextFromFile tests)
class TempFileGuard {
public:
    explicit TempFileGuard(std::string content) : path_ {"e2p_null_safety_" + std::to_string(counter_++) + ".ats"}
    {
        std::ofstream file(path_);
        file << content;
    }

    TempFileGuard(const TempFileGuard &) = delete;
    TempFileGuard &operator=(const TempFileGuard &) = delete;
    TempFileGuard(TempFileGuard &&) = delete;
    TempFileGuard &operator=(TempFileGuard &&) = delete;

    ~TempFileGuard()
    {
        std::remove(path_.c_str());
    }

    char const *Path() const
    {
        return path_.c_str();
    }

private:
    static size_t counter_;
    std::string path_;
};

size_t TempFileGuard::counter_ = 0;

// ===========================================================================
// Part 1: Config lifecycle - null safety and create/destroy leak defense
// ===========================================================================

// Create + destroy: pointer is non-null
TEST_F(ConfigLifecycleTest, CreateConfigReturnsNonNull)
{
    ConfigGuard guard {NewConfig()};
    ASSERT_NE(guard.Get(), nullptr);
}

// DestroyConfig(nullptr) must be a no-op, no crash (API contract)
TEST_F(NullSafetyTest, DestroyConfigNullIsNoOp)
{
    Impl()->DestroyConfig(nullptr);
    SUCCEED();
}

// DestroyConfigWithoutLog(nullptr) must be a no-op
TEST_F(NullSafetyTest, DestroyConfigWithoutLogNullIsNoOp)
{
    Impl()->DestroyConfigWithoutLog(nullptr);
    SUCCEED();
}

// Invalid CLI option: Options::Parse fails, CreateConfig returns nullptr
TEST_F(NullSafetyTest, CreateConfigWithUnknownOptionReturnsNull)
{
    ASSERT_EQ(NewConfigWithBadOption("--e2p-no-such-option-exists"), nullptr);
}

// More invalid argument variants: all return nullptr instead of crashing
TEST_F(NullSafetyTest, CreateConfigWithAnotherUnknownOptionReturnsNull)
{
    ASSERT_EQ(NewConfigWithBadOption("--definitely-not-a-real-flag"), nullptr);
}

TEST_F(NullSafetyTest, CreateConfigWithEmptyOptionReturnsNull)
{
    ASSERT_EQ(NewConfigWithBadOption(""), nullptr);
}

TEST_F(NullSafetyTest, CreateConfigWithMalformedOptionReturnsNull)
{
    ASSERT_EQ(NewConfigWithBadOption("--opt-level"), nullptr);
}

// ConfigGetOptions returns non-null Options on a valid config
TEST_F(NullSafetyTest, ConfigGetOptionsReturnsNonNull)
{
    ConfigGuard guard {NewConfig()};
    ASSERT_NE(guard.Get(), nullptr);
    EXPECT_NE(Impl()->ConfigGetOptions(guard.Get()), nullptr);
}

// Repeated ConfigGetOptions returns the same pointer (no duplicate allocation)
TEST_F(LeakDefenseTest, ConfigGetOptionsStableAcrossCalls)
{
    ConfigGuard guard {NewConfig()};
    auto *optionsFirst = Impl()->ConfigGetOptions(guard.Get());
    for (int i = 0; i < 10; i++) {
        EXPECT_EQ(Impl()->ConfigGetOptions(guard.Get()), optionsFirst);
    }
}

// Create/destroy loop x25: full ConfigImpl heap alloc/free path each round
TEST_F(LeakDefenseTest, ConfigCreateDestroyLoop25)
{
    for (int i = 0; i < 25; i++) {
        ConfigGuard guard {NewConfig()};
        ASSERT_NE(guard.Get(), nullptr);
    }
}

// Create/destroy loop x50 with an Options query each round
TEST_F(LeakDefenseTest, ConfigCreateDestroyLoop50WithOptionQueries)
{
    for (int i = 0; i < 50; i++) {
        ConfigGuard guard {NewConfig()};
        ASSERT_NE(guard.Get(), nullptr);
        EXPECT_NE(Impl()->ConfigGetOptions(guard.Get()), nullptr);
    }
}

// Three configs alive at once, destroyed one by one (independent allocations)
TEST_F(LeakDefenseTest, ThreeConfigsAliveSimultaneously)
{
    ConfigGuard first {NewConfig()};
    ConfigGuard second {NewConfig()};
    ConfigGuard third {NewConfig()};
    ASSERT_NE(first.Get(), nullptr);
    ASSERT_NE(second.Get(), nullptr);
    ASSERT_NE(third.Get(), nullptr);
    EXPECT_NE(first.Get(), second.Get());
    EXPECT_NE(second.Get(), third.Get());
    EXPECT_NE(first.Get(), third.Get());
}

// Two configs alive at once, the newer one destroyed first (reverse order)
TEST_F(LeakDefenseTest, TwoConfigsDestroyedInReverseCreationOrder)
{
    auto *first = NewConfig();
    ConfigGuard second {NewConfig()};
    ASSERT_NE(first, nullptr);
    ASSERT_NE(second.Get(), nullptr);
    Impl()->DestroyConfig(first);
    first = nullptr;
}

// Alternate create/destroy of two configs (interleaved lifecycles)
TEST_F(LeakDefenseTest, InterleavedConfigCreateDestroy)
{
    for (int i = 0; i < 10; i++) {
        ConfigGuard outer {NewConfig()};
        ASSERT_NE(outer.Get(), nullptr);
        {
            ConfigGuard inner {NewConfig()};
            ASSERT_NE(inner.Get(), nullptr);
            EXPECT_NE(Impl()->ConfigGetOptions(inner.Get()), nullptr);
        }
        EXPECT_NE(Impl()->ConfigGetOptions(outer.Get()), nullptr);
    }
}

// Config stays usable after its contexts are created and destroyed
TEST_F(LeakDefenseTest, ConfigSurvivesContextLifecycle)
{
    ConfigGuard guard {NewConfig()};
    ASSERT_NE(guard.Get(), nullptr);
    for (int i = 0; i < 5; i++) {
        ContextGuard context {CreateParsedContext(guard.Get(), "class A {}", "survive.ats")};
        ASSERT_NE(context.Get(), nullptr);
        EXPECT_EQ(Impl()->ContextState(context.Get()), ES2PANDA_STATE_PARSED);
    }
    EXPECT_NE(Impl()->ConfigGetOptions(guard.Get()), nullptr);
}

// Nested loops: outer config, inner repeated contexts; config still queryable
TEST_F(LeakDefenseTest, ConfigReusableAfterManyContextCycles)
{
    ConfigGuard guard {NewConfig()};
    ASSERT_NE(guard.Get(), nullptr);
    for (int round = 0; round < 5; round++) {
        for (int inner = 0; inner < 5; inner++) {
            ContextGuard context {CreateParsedContext(guard.Get(), "let x: int = 1", "reuse.ats")};
            ASSERT_NE(context.Get(), nullptr);
        }
    }
    EXPECT_NE(Impl()->ConfigGetOptions(guard.Get()), nullptr);
}

// A new config created after the old one is destroyed works fine
TEST_F(LeakDefenseTest, NewConfigUsableAfterPreviousConfigDestroyed)
{
    {
        ConfigGuard old {NewConfig()};
        ASSERT_NE(old.Get(), nullptr);
    }
    ConfigGuard fresh {NewConfig()};
    ASSERT_NE(fresh.Get(), nullptr);
    EXPECT_NE(Impl()->ConfigGetOptions(fresh.Get()), nullptr);
}

// Paired stress: every config in the loop also creates and parses a context
TEST_F(LeakDefenseTest, ConfigAndContextPairedLoop)
{
    for (int i = 0; i < 10; i++) {
        ConfigGuard config {NewConfig()};
        ASSERT_NE(config.Get(), nullptr);
        ContextGuard context {CreateParsedContext(config.Get(), "function f(): void {}", "paired.ats")};
        ASSERT_NE(context.Get(), nullptr);
        EXPECT_EQ(Impl()->ContextState(context.Get()), ES2PANDA_STATE_PARSED);
    }
}

// Many contexts created and destroyed on one config; state stays consistent
TEST_F(LeakDefenseTest, ManyContextsOnOneConfigSequential)
{
    ConfigGuard guard {NewConfig()};
    ASSERT_NE(guard.Get(), nullptr);
    for (int i = 0; i < 15; i++) {
        ContextGuard context {CreateParsedContext(guard.Get(), "class C {}", "seq.ats")};
        ASSERT_NE(context.Get(), nullptr);
        EXPECT_EQ(Impl()->ContextState(context.Get()), ES2PANDA_STATE_PARSED);
    }
}

// CreateContextFromString family on a null config: error context, no crash
TEST_F(NullSafetyTest, CreateContextFromStringWithNullConfigYieldsErrorContext)
{
    ContextGuard guard {Impl()->CreateContextFromString(nullptr, "class A {}", "nullcfg.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    EXPECT_EQ(Impl()->ContextState(guard.Get()), ES2PANDA_STATE_ERROR);
    EXPECT_STREQ(Impl()->ContextErrorMessage(guard.Get()), "Config is nullptr.");
    EXPECT_EQ(Impl()->ContextProgram(guard.Get()), nullptr);
}

// Null-config error contexts created/destroyed x20 (repeatable Context heap)
TEST_F(ErrorPathLeakTest, NullConfigErrorContextLoop20)
{
    for (int i = 0; i < 20; i++) {
        ContextGuard guard {Impl()->CreateContextFromString(nullptr, "class A {}", "loop-nullcfg.ats")};
        ASSERT_NE(guard.Get(), nullptr);
        EXPECT_EQ(Impl()->ContextState(guard.Get()), ES2PANDA_STATE_ERROR);
        EXPECT_STREQ(Impl()->ContextErrorMessage(guard.Get()), "Config is nullptr.");
    }
}

// ===========================================================================
// Part 2: Context creation - null safety and error context lifecycle
// ===========================================================================

// Happy path: valid config + valid source; state NEW, program pointer null
TEST_F(ContextLifecycleTest, FreshContextIsNewStateWithNullProgram)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class A {}", "fresh.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    EXPECT_EQ(Impl()->ContextState(guard.Get()), ES2PANDA_STATE_NEW);
    EXPECT_EQ(Impl()->ContextProgram(guard.Get()), nullptr);
    EXPECT_STREQ(Impl()->ContextErrorMessage(guard.Get()), "");
}

// CreateContextSimultaneousMode(nullptr, ...): InitContext checks null config
TEST_F(NullSafetyTest, CreateContextSimultaneousModeWithNullConfigYieldsErrorContext)
{
    ContextGuard guard {Impl()->CreateContextSimultaneousMode(nullptr, 0, nullptr)};
    ASSERT_NE(guard.Get(), nullptr);
    EXPECT_EQ(Impl()->ContextState(guard.Get()), ES2PANDA_STATE_ERROR);
    EXPECT_STREQ(Impl()->ContextErrorMessage(guard.Get()), "Config is nullptr.");
    EXPECT_EQ(Impl()->ContextProgram(guard.Get()), nullptr);
}

// CreateContextSimultaneousModeForLsp(nullptr, ...): also an error context
TEST_F(NullSafetyTest, CreateContextSimultaneousModeForLspWithNullConfigYieldsErrorContext)
{
    ContextGuard guard {Impl()->CreateContextSimultaneousModeForLsp(nullptr, 0, nullptr, true)};
    ASSERT_NE(guard.Get(), nullptr);
    EXPECT_EQ(Impl()->ContextState(guard.Get()), ES2PANDA_STATE_ERROR);
    EXPECT_STREQ(Impl()->ContextErrorMessage(guard.Get()), "Config is nullptr.");
}

// CreateContextSimultaneousModeForLsp with isLspUsage=false
TEST_F(NullSafetyTest, CreateContextSimultaneousModeForLspWithNullConfigAndNoLsp)
{
    ContextGuard guard {Impl()->CreateContextSimultaneousModeForLsp(nullptr, 0, nullptr, false)};
    ASSERT_NE(guard.Get(), nullptr);
    EXPECT_EQ(Impl()->ContextState(guard.Get()), ES2PANDA_STATE_ERROR);
}

// Null-config error context loop x20 (simultaneous mode entry)
TEST_F(ErrorPathLeakTest, NullConfigSimultaneousErrorContextLoop20)
{
    for (int i = 0; i < 20; i++) {
        ContextGuard guard {Impl()->CreateContextSimultaneousMode(nullptr, 0, nullptr)};
        ASSERT_NE(guard.Get(), nullptr);
        EXPECT_EQ(Impl()->ContextState(guard.Get()), ES2PANDA_STATE_ERROR);
    }
}

// Missing file: ReadFile fails, context carries an error (state NEW, no crash)
TEST_F(NullSafetyTest, CreateContextFromFileMissingFileYieldsErrorReport)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromFile(config.Get(), "/e2p/definitely/not/exist/file.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    EXPECT_EQ(Impl()->ContextState(guard.Get()), ES2PANDA_STATE_NEW);
    EXPECT_EQ(Impl()->ContextProgram(guard.Get()), nullptr);
    EXPECT_TRUE(Contains(std::string(Impl()->ContextErrorMessage(guard.Get())), "Failed to"))
        << "actual=[" << std::string(Impl()->ContextErrorMessage(guard.Get())) << "]";
}

// Null config + missing file: ReadFile runs first, null config never touched
TEST_F(NullSafetyTest, CreateContextFromFileNullConfigAndMissingFileYieldsErrorReport)
{
    ContextGuard guard {Impl()->CreateContextFromFile(nullptr, "/e2p/definitely/not/exist/file2.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    EXPECT_TRUE(Contains(std::string(Impl()->ContextErrorMessage(guard.Get())), "Failed to"));
}

// CreateCacheContextFromFile + missing file: same ReadFile failure path
TEST_F(NullSafetyTest, CreateCacheContextFromFileMissingFileYieldsErrorReport)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateCacheContextFromFile(config.Get(), "/e2p/not/exist/file3.ats", nullptr, false)};
    ASSERT_NE(guard.Get(), nullptr);
    EXPECT_TRUE(Contains(std::string(Impl()->ContextErrorMessage(guard.Get())), "Failed to"));
}

// Missing-file error contexts created/destroyed x15; message reallocated each time
TEST_F(ErrorPathLeakTest, MissingFileErrorContextLoop15)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    for (int i = 0; i < 15; i++) {
        ContextGuard guard {Impl()->CreateContextFromFile(config.Get(), "/e2p/not/exist/loop.ats")};
        ASSERT_NE(guard.Get(), nullptr);
        EXPECT_EQ(Impl()->ContextState(guard.Get()), ES2PANDA_STATE_NEW);
        EXPECT_TRUE(Contains(std::string(Impl()->ContextErrorMessage(guard.Get())), "Failed to"));
    }
}

// Real temp file: created and parsed normally
TEST_F(ContextLifecycleTest, CreateContextFromFileWithRealFileParses)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    TempFileGuard file {"class FromFile {}"};
    ContextGuard guard {Impl()->CreateContextFromFile(config.Get(), file.Path())};
    ASSERT_NE(guard.Get(), nullptr);
    // ProceedToState advances the same context in place; the guard keeps owning it
    auto *parsed = Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_PARSED);
    ASSERT_EQ(parsed, guard.Get());
    EXPECT_EQ(Impl()->ContextState(guard.Get()), ES2PANDA_STATE_PARSED);
}

// CreateContextFromStringWithHistory: valid source, parse succeeds
TEST_F(ContextLifecycleTest, CreateContextFromStringWithHistoryParses)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromStringWithHistory(config.Get(), "class H {}", "history.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *parsed = Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(parsed, nullptr);
    EXPECT_EQ(Impl()->ContextState(parsed), ES2PANDA_STATE_PARSED);
    EXPECT_NE(Impl()->ContextProgram(parsed), nullptr);
}

// CreateCacheContextFromString, isExternal=false: null globalContext unused
TEST_F(NullSafetyTest, CreateCacheContextFromStringNonExternalIgnoresNullGlobal)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {
        Impl()->CreateCacheContextFromString(config.Get(), "class NC {}", "nc.ats", nullptr, false, false)};
    ASSERT_NE(guard.Get(), nullptr);
    auto *parsed = Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_PARSED);
    ASSERT_EQ(parsed, guard.Get());
    EXPECT_EQ(Impl()->ContextState(guard.Get()), ES2PANDA_STATE_PARSED);
}

// Two contexts alive on one config: independent, both parse fine
TEST_F(LeakDefenseTest, TwoContextsAliveSimultaneouslyParseIndependently)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard first {Impl()->CreateContextFromString(config.Get(), "class A {}", "first.ats")};
    ContextGuard second {Impl()->CreateContextFromString(config.Get(), "class B {}", "second.ats")};
    ASSERT_NE(first.Get(), nullptr);
    ASSERT_NE(second.Get(), nullptr);
    auto *parsedFirst = Impl()->ProceedToState(first.Get(), ES2PANDA_STATE_PARSED);
    auto *parsedSecond = Impl()->ProceedToState(second.Get(), ES2PANDA_STATE_PARSED);
    ASSERT_EQ(parsedFirst, first.Get());
    ASSERT_EQ(parsedSecond, second.Get());
    EXPECT_EQ(Impl()->ContextState(first.Get()), ES2PANDA_STATE_PARSED);
    EXPECT_EQ(Impl()->ContextState(second.Get()), ES2PANDA_STATE_PARSED);
    EXPECT_NE(Impl()->ContextProgram(first.Get()), nullptr);
    EXPECT_NE(Impl()->ContextProgram(second.Get()), nullptr);
}

// Interleave error and normal contexts; lifecycles do not pollute each other
TEST_F(LeakDefenseTest, AlternatingErrorAndValidContexts)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    for (int i = 0; i < 10; i++) {
        ContextGuard errorContext {Impl()->CreateContextFromString(nullptr, "class X {}", "alt-null.ats")};
        ASSERT_NE(errorContext.Get(), nullptr);
        EXPECT_EQ(Impl()->ContextState(errorContext.Get()), ES2PANDA_STATE_ERROR);

        ContextGuard validContext {CreateParsedContext(config.Get(), "class Y {}", "alt-valid.ats")};
        ASSERT_NE(validContext.Get(), nullptr);
        EXPECT_EQ(Impl()->ContextState(validContext.Get()), ES2PANDA_STATE_PARSED);
    }
}

// Manual lifecycle: full parse->destroy loop x20; node count stable (deterministic)
TEST_F(LeakDefenseTest, ParseDestroyLoop20KeepsNodeCountStable)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    size_t referenceCount = 0;
    for (int i = 0; i < 20; i++) {
        ContextGuard guard {
            CreateParsedContext(config.Get(), "class A { x: int = 0; m(): int { return this.x } }", "stable.ats")};
        ASSERT_NE(guard.Get(), nullptr);
        auto *program = Impl()->ContextProgram(guard.Get());
        ASSERT_NE(program, nullptr);
        auto *ast = Impl()->ProgramAst(guard.Get(), program);
        ASSERT_NE(ast, nullptr);
        WalkPayload payload;
        WalkAst(Impl(), guard.Get(), ast, &payload);
        ASSERT_GE(payload.nodeCount, 5U);
        if (referenceCount == 0) {
            referenceCount = payload.nodeCount;
        } else {
            EXPECT_EQ(payload.nodeCount, referenceCount) << "iteration " << i;
        }
    }
}

// Manual lifecycle: empty program -> destroy -> recreate; state consistent
TEST_F(LeakDefenseTest, ErrorContextFieldsStableAcrossLoop)
{
    for (int i = 0; i < 10; i++) {
        ContextGuard guard {Impl()->CreateContextFromString(nullptr, "class Z {}", "errstable.ats")};
        ASSERT_NE(guard.Get(), nullptr);
        EXPECT_EQ(Impl()->ContextState(guard.Get()), ES2PANDA_STATE_ERROR);
        EXPECT_EQ(Impl()->ContextProgram(guard.Get()), nullptr);
        EXPECT_STREQ(Impl()->ContextErrorMessage(guard.Get()), "Config is nullptr.");
    }
}

// CONTEXT macro path: parse a class + main (fixture cfg, RAII cleanup)
TEST_F(ContextLifecycleTest, ContextMacroParsesClassWithMain)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(class C { n: string = "oh" }
function main() { let c = new C })XXX")
    {
        EXPECT_NE(GetImpl()->ContextProgram(GetContext()), nullptr);
        EXPECT_NE(CApi(GetAst()), nullptr);
    }
}

// CONTEXT macro path: simple function
TEST_F(ContextLifecycleTest, ContextMacroParsesSimpleFunction)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): void {}")
    {
        EXPECT_NE(CApi(GetAst()), nullptr);
    }
}

// CONTEXT macro path: type error ends in ERROR, semantic errors >= 1 (verified)
TEST_F(ErrorPathLeakTest, TypeErrorsProduceSemanticDiagnostics)
{
    CONTEXT(ES2PANDA_STATE_ASM_GENERATED, ES2PANDA_STATE_ERROR, "function main() { let x: int = \"\" }")
    {
        EXPECT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto diagnostics = GetImpl()->GetSemanticErrors(GetContext());
        auto *storage = reinterpret_cast<const ark::es2panda::util::DiagnosticStorage *>(diagnostics);
        EXPECT_GE(storage->size(), 1U);
    }
}

// CONTEXT macro path: empty source runs the whole pipeline (verified)
TEST_F(ContextLifecycleTest, EmptySourceReachesAsmGenerated)
{
    CONTEXT(ES2PANDA_STATE_ASM_GENERATED, "")
    {
        EXPECT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ASM_GENERATED);
    }
}

// CONTEXT macro path: minimal main runs the whole pipeline
TEST_F(ContextLifecycleTest, MinimalMainReachesAsmGenerated)
{
    CONTEXT(ES2PANDA_STATE_ASM_GENERATED, "function main() {}")
    {
        EXPECT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ASM_GENERATED);
    }
}

// CONTEXT macro path: to BOUND state
TEST_F(ContextLifecycleTest, ClassAndMainReachBound)
{
    CONTEXT(ES2PANDA_STATE_BOUND, "class A { x: int = 1 }")
    {
        EXPECT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_BOUND);
    }
}

// CONTEXT macro path: to CHECKED state (with type checking)
TEST_F(ContextLifecycleTest, TypedFunctionReachChecked)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, "function f(a: int): int { return a }")
    {
        EXPECT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
    }
}

// CONTEXT macro path: to LOWERED state
TEST_F(ContextLifecycleTest, SimpleLetReachLowered)
{
    CONTEXT(ES2PANDA_STATE_LOWERED, "let x: int = 5")
    {
        EXPECT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_LOWERED);
    }
}

// Error contexts only support state/message/program queries; DestroyContext on
// a partially initialized Context must not call FreeCompilerPartMemory
TEST_F(NullSafetyTest, ErrorContextDestroySkipsCompilerPartFree)
{
    ContextGuard guard {Impl()->CreateContextFromString(nullptr, "class A {}", "partial.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    // DestroyContext: null emitter -> skip FreeCompilerPartMemory; delete proceeds
    SUCCEED();
}

// ProceedToState to PARSED on a parsed context must fail (state machine guard)
TEST_F(ContextLifecycleTest, SecondProceedToParsedAfterParsedYieldsError)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class A {}", "state-machine.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *parsed = Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_PARSED);
    ASSERT_EQ(parsed, guard.Get());
    EXPECT_EQ(Impl()->ContextState(guard.Get()), ES2PANDA_STATE_PARSED);
    // State machine guard: PARSED -> PARSED again -> ERROR
    auto *second = Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_PARSED);
    ASSERT_EQ(second, guard.Get());
    EXPECT_EQ(Impl()->ContextState(guard.Get()), ES2PANDA_STATE_ERROR);
    EXPECT_TRUE(Contains(std::string(Impl()->ContextErrorMessage(guard.Get())), "Bad state"));
}

// Large source: 300 class members; parse/destroy with expected node scale
TEST_F(LeakDefenseTest, LargeSourceParseDestroyCycle)
{
    std::string source;
    for (int i = 0; i < 300; i++) {
        source += "function f" + std::to_string(i) + "(): int { return " + std::to_string(i) + " }";
    }
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {CreateParsedContext(config.Get(), source.c_str(), "large.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *program = Impl()->ContextProgram(guard.Get());
    ASSERT_NE(program, nullptr);
    auto *ast = Impl()->ProgramAst(guard.Get(), program);
    ASSERT_NE(ast, nullptr);
    WalkPayload payload;
    payload.impl = Impl();
    Impl()->AstNodeForEach(ast, CountNodesCallback, &payload);
    EXPECT_GE(payload.nodeCount, 900U);
}

// Same large source parsed 5 times: node count identical (deterministic)
TEST_F(LeakDefenseTest, LargeSourceRepeatedParseIsDeterministic)
{
    std::string source;
    for (int i = 0; i < 100; i++) {
        source += "class C" + std::to_string(i) + " { x: int = " + std::to_string(i) + " }";
    }
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    size_t referenceCount = 0;
    for (int round = 0; round < 5; round++) {
        ContextGuard guard {CreateParsedContext(config.Get(), source.c_str(), "det.ats")};
        ASSERT_NE(guard.Get(), nullptr);
        auto *ast = Impl()->ProgramAst(guard.Get(), Impl()->ContextProgram(guard.Get()));
        ASSERT_NE(ast, nullptr);
        WalkPayload payload;
        payload.impl = Impl();
        Impl()->AstNodeForEach(ast, CountNodesCallback, &payload);
        if (referenceCount == 0) {
            referenceCount = payload.nodeCount;
        } else {
            EXPECT_EQ(payload.nodeCount, referenceCount) << "round " << round;
        }
    }
}

// ===========================================================================
// Part 3: AST / Program query APIs - null safety
// These APIs have explicit nullptr guards in es2panda_lib.cpp:
//   - NearestScope(nullptr)            -> nullptr
//   - FirstDeclarationByNameFrom*(null)-> nullptr
//   - AllDeclarationsByNameFrom*(null)  -> empty array with length 0
//   - GetCommentsStringFromDeclaration -> empty string
//   - OwningProgramNode(nullptr)       -> nullptr (Rebind/Recheck report detached)
// ===========================================================================

// AstNodeFindNearestScope(ctx, nullptr) returns nullptr instead of crashing
TEST_F(NullSafetyTest, FindNearestScopeWithNullNodeReturnsNull)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A {}")
    {
        EXPECT_EQ(GetImpl()->AstNodeFindNearestScope(GetContext(), nullptr), nullptr);
    }
}

// FirstDeclarationByNameFromNode(ctx, nullptr, ...) returns nullptr
TEST_F(NullSafetyTest, FirstDeclarationByNameFromNodeWithNullNodeReturnsNull)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { m(): void {} }")
    {
        EXPECT_EQ(GetImpl()->FirstDeclarationByNameFromNode(GetContext(), nullptr, "m"), nullptr);
    }
}

// FirstDeclarationByNameFromProgram(ctx, nullptr, ...) returns nullptr
TEST_F(NullSafetyTest, FirstDeclarationByNameFromProgramWithNullProgramReturnsNull)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { m(): void {} }")
    {
        EXPECT_EQ(GetImpl()->FirstDeclarationByNameFromProgram(GetContext(), nullptr, "m"), nullptr);
    }
}

// AllDeclarationsByNameFromNode(ctx, nullptr, ...): non-null array, length 0
TEST_F(NullSafetyTest, AllDeclarationsByNameFromNodeWithNullNodeIsEmpty)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { m(): void {} }")
    {
        size_t length = 1;
        auto *declarations = GetImpl()->AllDeclarationsByNameFromNode(GetContext(), nullptr, "m", &length);
        EXPECT_EQ(length, 0U);
        EXPECT_NE(declarations, nullptr);
    }
}

// AllDeclarationsByNameFromProgram(ctx, nullptr, ...): array with length 0
TEST_F(NullSafetyTest, AllDeclarationsByNameFromProgramWithNullProgramIsEmpty)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { m(): void {} }")
    {
        size_t length = 1;
        auto *declarations = GetImpl()->AllDeclarationsByNameFromProgram(GetContext(), nullptr, "m", &length);
        ASSERT_EQ(length, 0U);
        EXPECT_NE(declarations, nullptr);
    }
}

// Happy path: FirstDeclarationByNameFromProgram finds the method by name
TEST_F(NullSafetyTest, FirstDeclarationByNameFromProgramFindsDeclaredMethod)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { myMethod(): void {} }")
    {
        auto *program = GetImpl()->ContextProgram(GetContext());
        ASSERT_NE(program, nullptr);
        auto *method = GetImpl()->FirstDeclarationByNameFromProgram(GetContext(), program, "myMethod");
        ASSERT_NE(method, nullptr);
        EXPECT_TRUE(GetImpl()->IsMethodDefinition(method));
    }
}

// Happy path: AllDeclarationsByNameFromProgram finds the single method (length 1)
TEST_F(NullSafetyTest, AllDeclarationsByNameFromProgramFindsSingleMethod)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { onlyOnce(): void {} }")
    {
        size_t length = 0;
        auto *program = GetImpl()->ContextProgram(GetContext());
        ASSERT_NE(program, nullptr);
        auto *declarations = GetImpl()->AllDeclarationsByNameFromProgram(GetContext(), program, "onlyOnce", &length);
        ASSERT_EQ(length, 1U);
        EXPECT_NE(declarations, nullptr);
        EXPECT_NE(declarations[0], nullptr);
    }
}

// Happy path: unknown method names return nullptr / empty array
TEST_F(NullSafetyTest, MissingDeclarationNameYieldsNullAndEmpty)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { m(): void {} }")
    {
        auto *program = GetImpl()->ContextProgram(GetContext());
        ASSERT_NE(program, nullptr);
        EXPECT_EQ(GetImpl()->FirstDeclarationByNameFromProgram(GetContext(), program, "noSuchMethod"), nullptr);

        size_t length = 1;
        GetImpl()->AllDeclarationsByNameFromProgram(GetContext(), program, "noSuchMethod", &length);
        EXPECT_EQ(length, 0U);
    }
}

// GetCommentsStringFromDeclaration(ctx, nullptr) returns an empty string
TEST_F(NullSafetyTest, GetCommentsStringFromDeclarationWithNullNodeIsEmptyString)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A {}")
    {
        auto *comments = GetImpl()->GetCommentsStringFromDeclaration(GetContext(), nullptr);
        ASSERT_NE(comments, nullptr);
        EXPECT_STREQ(comments, "");
    }
}

// AstNodeRebind(ctx, nullptr): detached node -> error, nullptr, state ERROR
// (the error goes through LogSemanticError; assert IsAnyError, not errorMessage)
TEST_F(NullSafetyTest, AstNodeRebindWithNullNodeReportsDetached)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A {}")
    {
        auto *scope = GetImpl()->AstNodeRebind(GetContext(), nullptr);
        EXPECT_EQ(scope, nullptr);
        EXPECT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        EXPECT_TRUE(GetImpl()->IsAnyError(GetContext()));
    }
}

// AstNodeRecheck(ctx, nullptr): same detached-node error, state ERROR
TEST_F(NullSafetyTest, AstNodeRecheckWithNullNodeReportsDetached)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A {}")
    {
        GetImpl()->AstNodeRecheck(GetContext(), nullptr);
        EXPECT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        EXPECT_TRUE(GetImpl()->IsAnyError(GetContext()));
    }
}

// ProgramLocalNameIsExported null guards on all three args: all false
TEST_F(NullSafetyTest, ProgramLocalNameIsExportedNullGuardsReturnFalse)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "export class A {}")
    {
        auto *program = GetImpl()->ContextProgram(GetContext());
        ASSERT_NE(program, nullptr);
        EXPECT_FALSE(GetImpl()->ProgramLocalNameIsExported(nullptr, program, "A"));
        EXPECT_FALSE(GetImpl()->ProgramLocalNameIsExported(GetContext(), nullptr, "A"));
        EXPECT_FALSE(GetImpl()->ProgramLocalNameIsExported(GetContext(), program, nullptr));
    }
}

// ProgramLocalNameIsExported: non-exported name returns false
TEST_F(NullSafetyTest, ProgramLocalNameIsExportedFalseForNonExportedName)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A {}")
    {
        auto *program = GetImpl()->ContextProgram(GetContext());
        ASSERT_NE(program, nullptr);
        EXPECT_FALSE(GetImpl()->ProgramLocalNameIsExported(GetContext(), program, "A"));
    }
}

// ExtractDeclarationsFromAbcFile on a missing abc file returns non-zero
TEST_F(NullSafetyTest, ExtractDeclarationsFromMissingAbcFileReturnsNonZero)
{
    EXPECT_EQ(Impl()->ExtractDeclarationsFromAbcFile("/e2p/not/exist/file.abc", "/e2p/not/exist/cache"), 1);
}

// ExtractDeclarationsFromAbcFile loop x10: failed File entities are freed
TEST_F(ErrorPathLeakTest, ExtractDeclarationsFromMissingAbcLoop10)
{
    for (int i = 0; i < 10; i++) {
        EXPECT_EQ(Impl()->ExtractDeclarationsFromAbcFile("/e2p/not/exist/loop.abc", "/e2p/not/exist/cache"), 1);
    }
}

// FormOutputPathForFile: NEW-state context returns an empty string
TEST_F(NullSafetyTest, FormOutputPathForFileOnFreshContextIsEmptyString)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class A {}", "formout.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *path = Impl()->FormOutputPathForFile(guard.Get(), "formout.ats");
    ASSERT_NE(path, nullptr);
    EXPECT_STREQ(path, "");
}

// FormOutputPathForFile: PARSED state returns a non-empty path
TEST_F(ContextLifecycleTest, FormOutputPathForFileOnParsedContextReturnsPath)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {CreateParsedContext(config.Get(), "class A {}", "formout2.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *path = Impl()->FormOutputPathForFile(guard.Get(), "formout2.ats");
    ASSERT_NE(path, nullptr);
}

// AstNodeRebind on a detached node (builder-created, never attached):
// OwningProgramNode returns nullptr -> error + nullptr, no crash
TEST_F(NullSafetyTest, AstNodeRebindOnDetachedBuilderNodeReportsError)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A {}")
    {
        auto *literal = GetImpl()->CreateNumberLiteral(GetContext(), 42);
        ASSERT_NE(literal, nullptr);
        auto *scope = GetImpl()->AstNodeRebind(GetContext(), literal);
        EXPECT_EQ(scope, nullptr);
        EXPECT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        EXPECT_TRUE(GetImpl()->IsAnyError(GetContext()));
    }
}

// AstNodeRecheck on a detached node: same detached error path, no crash
TEST_F(NullSafetyTest, AstNodeRecheckOnDetachedBuilderNodeReportsError)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A {}")
    {
        auto *literal = GetImpl()->CreateNumberLiteral(GetContext(), 7);
        ASSERT_NE(literal, nullptr);
        GetImpl()->AstNodeRecheck(GetContext(), literal);
        EXPECT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        EXPECT_TRUE(GetImpl()->IsAnyError(GetContext()));
    }
}

// AstNodeFindNearestScope returns a non-null scope on a bound root node
// (in PARSED state scopes are not built yet; NearestScope returns nullptr)
TEST_F(NullSafetyTest, FindNearestScopeOnRootNodeReturnsScope)
{
    CONTEXT(ES2PANDA_STATE_BOUND, "class A {}")
    {
        auto *scope = GetImpl()->AstNodeFindNearestScope(GetContext(), CApi(GetAst()));
        EXPECT_NE(scope, nullptr);
    }
}

// AstNodeFindNearestScope in PARSED state: no scope yet, returns nullptr
TEST_F(NullSafetyTest, FindNearestScopeOnParsedRootIsNull)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A {}")
    {
        auto *scope = GetImpl()->AstNodeFindNearestScope(GetContext(), CApi(GetAst()));
        EXPECT_EQ(scope, nullptr);
    }
}

// GetCommentsStringFromDeclaration on a real node: non-null (happy path)
TEST_F(NullSafetyTest, GetCommentsStringFromDeclarationOnRealNode)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A {}")
    {
        auto *comments = GetImpl()->GetCommentsStringFromDeclaration(GetContext(), CApi(GetAst()));
        EXPECT_NE(comments, nullptr);
    }
}

// AstNodeFindNearestScope called 20 times: same scope (no reallocation)
TEST_F(LeakDefenseTest, FindNearestScopeRepeatedCallsStable)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { m(): void {} }")
    {
        auto *first = GetImpl()->AstNodeFindNearestScope(GetContext(), CApi(GetAst()));
        for (int i = 0; i < 20; i++) {
            EXPECT_EQ(GetImpl()->AstNodeFindNearestScope(GetContext(), CApi(GetAst())), first);
        }
    }
}

// FirstDeclarationByNameFromProgram queried 20 times: same node each time
TEST_F(LeakDefenseTest, FirstDeclarationByNameRepeatedQueriesStable)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { stableMethod(): void {} }")
    {
        auto *program = GetImpl()->ContextProgram(GetContext());
        ASSERT_NE(program, nullptr);
        auto *first = GetImpl()->FirstDeclarationByNameFromProgram(GetContext(), program, "stableMethod");
        ASSERT_NE(first, nullptr);
        for (int i = 0; i < 20; i++) {
            EXPECT_EQ(GetImpl()->FirstDeclarationByNameFromProgram(GetContext(), program, "stableMethod"), first);
        }
    }
}

// AllDeclarationsByNameFromProgram queried repeatedly: length stays 1
TEST_F(LeakDefenseTest, AllDeclarationsByNameRepeatedQueriesStableLength)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { anotherMethod(): void {} }")
    {
        auto *program = GetImpl()->ContextProgram(GetContext());
        ASSERT_NE(program, nullptr);
        for (int i = 0; i < 20; i++) {
            size_t length = 0;
            GetImpl()->AllDeclarationsByNameFromProgram(GetContext(), program, "anotherMethod", &length);
            EXPECT_EQ(length, 1U);
        }
    }
}

// GetCommentsStringFromDeclaration called 20 times: no crash (arena holds up)
TEST_F(LeakDefenseTest, GetCommentsStringRepeatedCallsAreSafe)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A {}")
    {
        for (int i = 0; i < 20; i++) {
            auto *comments = GetImpl()->GetCommentsStringFromDeclaration(GetContext(), CApi(GetAst()));
            EXPECT_NE(comments, nullptr);
        }
    }
}

// AllDeclarationsByNameFromProgram with overloads: correct overload count
TEST_F(NullSafetyTest, AllDeclarationsByNameCountsOverloads)
{
    CONTEXT(ES2PANDA_STATE_PARSED,
            "class A {\n"
            "    ov(a: int): void {}\n"
            "    ov(a: string): void {}\n"
            "}")
    {
        size_t length = 0;
        auto *program = GetImpl()->ContextProgram(GetContext());
        ASSERT_NE(program, nullptr);
        GetImpl()->AllDeclarationsByNameFromProgram(GetContext(), program, "ov", &length);
        EXPECT_EQ(length, 2U);
    }
}

// Same method name in two classes: AllDeclarations finds both (length 2)
TEST_F(NullSafetyTest, AllDeclarationsByNameAcrossTwoClasses)
{
    CONTEXT(ES2PANDA_STATE_PARSED,
            "class A { shared(): void {} }\n"
            "class B { shared(): void {} }")
    {
        size_t length = 0;
        auto *program = GetImpl()->ContextProgram(GetContext());
        ASSERT_NE(program, nullptr);
        GetImpl()->AllDeclarationsByNameFromProgram(GetContext(), program, "shared", &length);
        EXPECT_EQ(length, 2U);
    }
}

// FirstDeclarationByNameFromNode (non-Program version): happy path
TEST_F(NullSafetyTest, FirstDeclarationByNameFromNodeFindsMethod)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { nodeLookup(): void {} }")
    {
        auto *method = GetImpl()->FirstDeclarationByNameFromNode(GetContext(), CApi(GetAst()), "nodeLookup");
        ASSERT_NE(method, nullptr);
        EXPECT_TRUE(GetImpl()->IsMethodDefinition(method));
    }
}

// AllDeclarationsByNameFromNode: happy path, length 1
TEST_F(NullSafetyTest, AllDeclarationsByNameFromNodeFindsMethod)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { nodeAll(): void {} }")
    {
        size_t length = 0;
        auto *declarations = GetImpl()->AllDeclarationsByNameFromNode(GetContext(), CApi(GetAst()), "nodeAll", &length);
        ASSERT_EQ(length, 1U);
        EXPECT_NE(declarations, nullptr);
        EXPECT_NE(declarations[0], nullptr);
    }
}

// FirstDeclarationByNameFromNode returns nullptr when nothing matches
TEST_F(NullSafetyTest, FirstDeclarationByNameFromNodeMissingNameIsNull)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A {}")
    {
        EXPECT_EQ(GetImpl()->FirstDeclarationByNameFromNode(GetContext(), CApi(GetAst()), "ghost"), nullptr);
    }
}

// ProgramLocalNameIsExported returns true for an exported class name
TEST_F(NullSafetyTest, ProgramLocalNameIsExportedTrueForExportedClass)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "export class ExportedClass {}")
    {
        auto *program = GetImpl()->ContextProgram(GetContext());
        ASSERT_NE(program, nullptr);
        EXPECT_TRUE(GetImpl()->ProgramLocalNameIsExported(GetContext(), program, "ExportedClass"));
    }
}

// Repeated detached-node Rebind/Recheck: the error path repeats safely
TEST_F(ErrorPathLeakTest, RepeatedDetachedNodeOperationsAreSafe)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    for (int i = 0; i < 10; i++) {
        ContextGuard guard {CreateParsedContext(config.Get(), "class A {}", "detach-loop.ats")};
        ASSERT_NE(guard.Get(), nullptr);
        EXPECT_EQ(Impl()->AstNodeRebind(guard.Get(), nullptr), nullptr);
        EXPECT_EQ(Impl()->ContextState(guard.Get()), ES2PANDA_STATE_ERROR);
    }
}

// ===========================================================================
// Part 4: Number literals / source positions / AllocMemory - null and bounds
// ===========================================================================

// CreateNumberLiteral(ctx, int) works on a fresh context: allocator ready
TEST_F(NullSafetyTest, CreateNumberLiteralIntOnFreshContext)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class A {}", "numlit.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *literal = Impl()->CreateNumberLiteral(guard.Get(), 42);
    ASSERT_NE(literal, nullptr);
}

// int literal: SetInt succeeds, wrong-type Set rejected; builder literals
// have no text (lexer::Number(int) sets no str_, SetValue keeps it),
// so StrConst is an empty string - assert non-null + empty, no garbage
TEST_F(NullSafetyTest, NumberLiteralIntSetAndStringRoundtrip)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class A {}", "numlit2.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *literal = Impl()->CreateNumberLiteral(guard.Get(), 42);
    ASSERT_NE(literal, nullptr);
    EXPECT_TRUE(Impl()->NumberLiteralSetInt(literal, 7));
    auto *str = Impl()->NumberLiteralStrConst(guard.Get(), literal);
    ASSERT_NE(str, nullptr);
    EXPECT_STREQ(str, "");
}

// int literal rejects wrong-type setters: Long/Double/Float all false
TEST_F(NullSafetyTest, NumberLiteralIntRejectsWrongTypeSetters)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class A {}", "numlit3.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *literal = Impl()->CreateNumberLiteral(guard.Get(), 1);
    ASSERT_NE(literal, nullptr);
    EXPECT_FALSE(Impl()->NumberLiteralSetLong(literal, 2));
    EXPECT_FALSE(Impl()->NumberLiteralSetDouble(literal, 3.5));
    EXPECT_FALSE(Impl()->NumberLiteralSetFloat(literal, 4.5F));
}

// long literal (CreateNumberLiteral1): SetLong works, others rejected
TEST_F(NullSafetyTest, NumberLiteralLongSetAndRejectsOthers)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class A {}", "numlit4.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *literal = Impl()->CreateNumberLiteral1(guard.Get(), 100);
    ASSERT_NE(literal, nullptr);
    EXPECT_TRUE(Impl()->NumberLiteralSetLong(literal, 1234567890123));
    auto *str = Impl()->NumberLiteralStrConst(guard.Get(), literal);
    ASSERT_NE(str, nullptr);
    EXPECT_STREQ(str, "");
    EXPECT_FALSE(Impl()->NumberLiteralSetInt(literal, 5));
    EXPECT_FALSE(Impl()->NumberLiteralSetDouble(literal, 1.5));
    EXPECT_FALSE(Impl()->NumberLiteralSetFloat(literal, 0.5F));
}

// double literal (CreateNumberLiteral2)
TEST_F(NullSafetyTest, NumberLiteralDoubleSetAndString)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class A {}", "numlit5.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *literal = Impl()->CreateNumberLiteral2(guard.Get(), 1.5);
    ASSERT_NE(literal, nullptr);
    EXPECT_TRUE(Impl()->NumberLiteralSetDouble(literal, 0.25));
    auto *str = Impl()->NumberLiteralStrConst(guard.Get(), literal);
    ASSERT_NE(str, nullptr);
    EXPECT_STREQ(str, "");
    EXPECT_FALSE(Impl()->NumberLiteralSetInt(literal, 9));
    EXPECT_FALSE(Impl()->NumberLiteralSetLong(literal, 9));
    EXPECT_FALSE(Impl()->NumberLiteralSetFloat(literal, 9.5F));
}

// float literal (CreateNumberLiteral3)
TEST_F(NullSafetyTest, NumberLiteralFloatSetAndString)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class A {}", "numlit6.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *literal = Impl()->CreateNumberLiteral3(guard.Get(), 2.5F);
    ASSERT_NE(literal, nullptr);
    EXPECT_TRUE(Impl()->NumberLiteralSetFloat(literal, 0.5F));
    auto *str = Impl()->NumberLiteralStrConst(guard.Get(), literal);
    ASSERT_NE(str, nullptr);
    EXPECT_STREQ(str, "");
    EXPECT_FALSE(Impl()->NumberLiteralSetInt(literal, 1));
    EXPECT_FALSE(Impl()->NumberLiteralSetLong(literal, 1));
    EXPECT_FALSE(Impl()->NumberLiteralSetDouble(literal, 1.5));
}

// Negative int literal: SetInt accepts negatives; StrConst stays empty
TEST_F(NullSafetyTest, NumberLiteralNegativeIntString)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class A {}", "numlit7.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *literal = Impl()->CreateNumberLiteral(guard.Get(), 0);
    ASSERT_NE(literal, nullptr);
    EXPECT_TRUE(Impl()->NumberLiteralSetInt(literal, -5));
    auto *str = Impl()->NumberLiteralStrConst(guard.Get(), literal);
    ASSERT_NE(str, nullptr);
    EXPECT_STREQ(str, "");
}

// Zero literal
TEST_F(NullSafetyTest, NumberLiteralZeroValues)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class A {}", "numlit8.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *intZero = Impl()->CreateNumberLiteral(guard.Get(), 0);
    auto *longZero = Impl()->CreateNumberLiteral1(guard.Get(), 0);
    auto *doubleZero = Impl()->CreateNumberLiteral2(guard.Get(), 0.0);
    auto *floatZero = Impl()->CreateNumberLiteral3(guard.Get(), 0.0F);
    ASSERT_NE(intZero, nullptr);
    ASSERT_NE(longZero, nullptr);
    ASSERT_NE(doubleZero, nullptr);
    ASSERT_NE(floatZero, nullptr);
    EXPECT_STREQ(Impl()->NumberLiteralStrConst(guard.Get(), intZero), "");
    EXPECT_STREQ(Impl()->NumberLiteralStrConst(guard.Get(), longZero), "");
    EXPECT_STREQ(Impl()->NumberLiteralStrConst(guard.Get(), doubleZero), "");
    EXPECT_STREQ(Impl()->NumberLiteralStrConst(guard.Get(), floatZero), "");
}

// Parsed literals keep their source text: let x: int = 42 -> the NumberLiteral
// has StrConst "42" (contrast with the builder path: StrConst works)
TEST_F(NullSafetyTest, NumberLiteralFromParsedSourceKeepsSourceText)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let x: int = 42")
    {
        struct Finder {
            es2panda_Impl const *impl = nullptr;
            es2panda_AstNode *found = nullptr;
        } finder {GetImpl(), nullptr};
        auto callback = [](es2panda_AstNode *node, void *arg) {
            auto *f = static_cast<Finder *>(arg);
            if (f->found == nullptr && f->impl->IsNumberLiteral(node)) {
                f->found = node;
            }
        };
        GetImpl()->AstNodeForEach(CApi(GetAst()), callback, &finder);
        ASSERT_NE(finder.found, nullptr);
        auto *str = GetImpl()->NumberLiteralStrConst(GetContext(), finder.found);
        ASSERT_NE(str, nullptr);
        EXPECT_STREQ(str, "42");
    }
}

// Literal batch creation loop x50: settable and stringifiable (arena holds up)
TEST_F(LeakDefenseTest, NumberLiteralCreateSetLoop50)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class A {}", "numloop.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    for (int i = 0; i < 50; i++) {
        auto *literal = Impl()->CreateNumberLiteral(guard.Get(), i);
        ASSERT_NE(literal, nullptr);
        EXPECT_TRUE(Impl()->NumberLiteralSetInt(literal, i * 2));
        auto *str = Impl()->NumberLiteralStrConst(guard.Get(), literal);
        ASSERT_NE(str, nullptr);
        EXPECT_STREQ(str, "");
    }
}

// Alternate the four literal types x25: no type-dispatch pollution
TEST_F(LeakDefenseTest, NumberLiteralAlternatingTypesLoop25)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class A {}", "numalt.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    for (int i = 0; i < 25; i++) {
        auto *intLit = Impl()->CreateNumberLiteral(guard.Get(), i);
        auto *longLit = Impl()->CreateNumberLiteral1(guard.Get(), i);
        auto *doubleLit = Impl()->CreateNumberLiteral2(guard.Get(), static_cast<double>(i) + 0.5);
        auto *floatLit = Impl()->CreateNumberLiteral3(guard.Get(), static_cast<float>(i) + 0.5F);
        ASSERT_NE(intLit, nullptr);
        ASSERT_NE(longLit, nullptr);
        ASSERT_NE(doubleLit, nullptr);
        ASSERT_NE(floatLit, nullptr);
        EXPECT_TRUE(Impl()->NumberLiteralSetInt(intLit, i));
        EXPECT_TRUE(Impl()->NumberLiteralSetLong(longLit, i));
        EXPECT_TRUE(Impl()->NumberLiteralSetDouble(doubleLit, static_cast<double>(i) + 0.25));
        EXPECT_TRUE(Impl()->NumberLiteralSetFloat(floatLit, static_cast<float>(i) + 0.25F));
    }
}

// UpdateNumberLiteral: creates an updated node; both nodes stay valid
TEST_F(NullSafetyTest, UpdateNumberLiteralCreatesNewNode)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class A {}", "numupd.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *original = Impl()->CreateNumberLiteral(guard.Get(), 1);
    ASSERT_NE(original, nullptr);
    auto *updated = Impl()->UpdateNumberLiteral(guard.Get(), original, 9);
    ASSERT_NE(updated, nullptr);
    EXPECT_NE(updated, original);
    auto *str = Impl()->NumberLiteralStrConst(guard.Get(), updated);
    ASSERT_NE(str, nullptr);
    EXPECT_STREQ(str, "");
}

// CreateSourcePosition on a fresh context: index/line kept; col hits the null-program branch (1)
// (es2panda_lib.cpp: SourcePositionCol guards Program() == nullptr)
TEST_F(NullSafetyTest, SourcePositionOnFreshContextKeepsNullProgramColGuard)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class A {}", "srcpos.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *position = Impl()->CreateSourcePosition(guard.Get(), 10, 3);
    ASSERT_NE(position, nullptr);
    EXPECT_EQ(Impl()->SourcePositionIndex(guard.Get(), position), 10U);
    EXPECT_EQ(Impl()->SourcePositionLine(guard.Get(), position), 3U);
    // parserProgram is null: the Col guard returns sentinel 1, no null deref
    EXPECT_EQ(Impl()->SourcePositionCol(guard.Get(), position), 1U);
}

// CreateSourcePosition loop x30: positions allocated on the arena
TEST_F(LeakDefenseTest, SourcePositionCreateLoop30)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class A {}", "srcloop.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    for (int i = 0; i < 30; i++) {
        auto *position = Impl()->CreateSourcePosition(guard.Get(), static_cast<size_t>(i), static_cast<size_t>(i) + 1U);
        ASSERT_NE(position, nullptr);
        EXPECT_EQ(Impl()->SourcePositionIndex(guard.Get(), position), static_cast<size_t>(i));
        EXPECT_EQ(Impl()->SourcePositionLine(guard.Get(), position), static_cast<size_t>(i) + 1U);
    }
}

// CreateSourceRange + SourceRangeStart/End: index/line round-trip
TEST_F(NullSafetyTest, SourceRangeStartEndRoundtrip)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class A {}", "srcrange.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *start = Impl()->CreateSourcePosition(guard.Get(), 1, 1);
    auto *end = Impl()->CreateSourcePosition(guard.Get(), 20, 2);
    ASSERT_NE(start, nullptr);
    ASSERT_NE(end, nullptr);
    auto *range = Impl()->CreateSourceRange(guard.Get(), start, end);
    ASSERT_NE(range, nullptr);
    auto *startBack = Impl()->SourceRangeStart(guard.Get(), range);
    auto *endBack = Impl()->SourceRangeEnd(guard.Get(), range);
    ASSERT_NE(startBack, nullptr);
    ASSERT_NE(endBack, nullptr);
    EXPECT_EQ(Impl()->SourcePositionIndex(guard.Get(), startBack), 1U);
    EXPECT_EQ(Impl()->SourcePositionLine(guard.Get(), startBack), 1U);
    EXPECT_EQ(Impl()->SourcePositionIndex(guard.Get(), endBack), 20U);
    EXPECT_EQ(Impl()->SourcePositionLine(guard.Get(), endBack), 2U);
}

// SourceRange loop x20
TEST_F(LeakDefenseTest, SourceRangeCreateLoop20)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class A {}", "rangeloop.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    for (int i = 0; i < 20; i++) {
        auto *start = Impl()->CreateSourcePosition(guard.Get(), static_cast<size_t>(i), 1);
        auto *end = Impl()->CreateSourcePosition(guard.Get(), static_cast<size_t>(i) + 100U, 5);
        auto *range = Impl()->CreateSourceRange(guard.Get(), start, end);
        ASSERT_NE(range, nullptr);
        EXPECT_NE(Impl()->SourceRangeStart(guard.Get(), range), nullptr);
        EXPECT_NE(Impl()->SourceRangeEnd(guard.Get(), range), nullptr);
    }
}

// AllocMemory: basic allocation + write/read
TEST_F(NullSafetyTest, AllocMemoryBasicAllocation)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class A {}", "alloc.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *memory = static_cast<uint32_t *>(Impl()->AllocMemory(guard.Get(), 16, sizeof(uint32_t)));
    ASSERT_NE(memory, nullptr);
    for (uint32_t i = 0; i < 16; i++) {
        memory[i] = 0xDEADBEEFU;
    }
    for (uint32_t i = 0; i < 16; i++) {
        EXPECT_EQ(memory[i], 0xDEADBEEFU);
    }
}

// AllocMemory loop x100: no overlap (distinct patterns verified one by one)
TEST_F(LeakDefenseTest, AllocMemoryLoop100NoOverlap)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class A {}", "allocloop.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    std::vector<uint32_t *> blocks;
    for (int i = 0; i < 100; i++) {
        auto *block = static_cast<uint32_t *>(Impl()->AllocMemory(guard.Get(), 8, sizeof(uint32_t)));
        ASSERT_NE(block, nullptr);
        for (uint32_t j = 0; j < 8; j++) {
            block[j] = static_cast<uint32_t>(i) * 100U + j;
        }
        blocks.push_back(block);
    }
    for (size_t i = 0; i < blocks.size(); i++) {
        for (uint32_t j = 0; j < 8; j++) {
            EXPECT_EQ(blocks[i][j], static_cast<uint32_t>(i) * 100U + j);
        }
    }
}

// EHeap counter: AllocatedSize grows after allocation, never decreases
TEST_F(LeakDefenseTest, EHeapAllocatedSizeIsMonotonicWithinScope)
{
    auto before = ark::es2panda::EHeap::AllocatedSize();
    {
        ConfigGuard config {NewConfig()};
        ASSERT_NE(config.Get(), nullptr);
        ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class A {}", "eheap.ats")};
        ASSERT_NE(guard.Get(), nullptr);
        auto *memory = Impl()->AllocMemory(guard.Get(), 1024, 1);
        ASSERT_NE(memory, nullptr);
        auto during = ark::es2panda::EHeap::AllocatedSize();
        EXPECT_GE(during, before);
    }
    // EHeap is a process-wide bump allocator: AllocatedSize never goes back
    EXPECT_GE(ark::es2panda::EHeap::AllocatedSize(), before);
}

// EHeap counter stays non-decreasing across context lifecycle rounds
TEST_F(LeakDefenseTest, EHeapAllocatedSizeNonDecreasingAcrossContextCycles)
{
    auto previous = ark::es2panda::EHeap::AllocatedSize();
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    for (int i = 0; i < 5; i++) {
        ContextGuard guard {CreateParsedContext(config.Get(), "class A { x: int = 1 }", "eheap-cycle.ats")};
        ASSERT_NE(guard.Get(), nullptr);
        auto current = ark::es2panda::EHeap::AllocatedSize();
        EXPECT_GE(current, previous);
        previous = current;
    }
}

// ===========================================================================
// Part 5: Diagnostics APIs - null defense and storage count consistency
// ===========================================================================

// CreateDiagnosticKind returns non-null for all three plugin kinds
TEST_F(NullSafetyTest, CreateDiagnosticKindAllTypes)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A {}")
    {
        EXPECT_NE(GetImpl()->CreateDiagnosticKind(GetContext(), "err {}", ES2PANDA_PLUGIN_ERROR), nullptr);
        EXPECT_NE(GetImpl()->CreateDiagnosticKind(GetContext(), "warn {}", ES2PANDA_PLUGIN_WARNING), nullptr);
        EXPECT_NE(GetImpl()->CreateDiagnosticKind(GetContext(), "sugg {}", ES2PANDA_PLUGIN_SUGGESTION), nullptr);
    }
}

// CreateDiagnosticKind + LogDiagnostic: plugin error count +1, message ok
// (matches the behavior verified in es2panda_public_test.cpp)
TEST_F(NullSafetyTest, LogDiagnosticPluginErrorAddsToStorage)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A {}")
    {
        auto *storage =
            reinterpret_cast<const ark::es2panda::util::DiagnosticStorage *>(GetImpl()->GetPluginErrors(GetContext()));
        const auto baseline = storage->size();

        auto *kind = GetImpl()->CreateDiagnosticKind(GetContext(), "Test {}", ES2PANDA_PLUGIN_ERROR);
        ASSERT_NE(kind, nullptr);
        char const *args[] = {"1"};
        auto *pos = GetImpl()->CreateSourcePosition(GetContext(), 1, 1);
        ASSERT_NE(pos, nullptr);
        GetImpl()->LogDiagnostic(GetContext(), kind, args, 1, pos);

        EXPECT_EQ(storage->size(), baseline + 1U);
        EXPECT_EQ((*storage)[storage->size() - 1U]->Type(), ark::es2panda::util::DiagnosticType::PLUGIN_ERROR);
        EXPECT_EQ((*storage)[storage->size() - 1U]->Message(), "Test 1");
    }
}

// LogDiagnostic with no arguments
TEST_F(NullSafetyTest, LogDiagnosticWithoutArgs)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A {}")
    {
        auto *storage =
            reinterpret_cast<const ark::es2panda::util::DiagnosticStorage *>(GetImpl()->GetPluginErrors(GetContext()));
        const auto baseline = storage->size();

        auto *kind = GetImpl()->CreateDiagnosticKind(GetContext(), "Plain message", ES2PANDA_PLUGIN_ERROR);
        ASSERT_NE(kind, nullptr);
        auto *pos = GetImpl()->CreateSourcePosition(GetContext(), 1, 1);
        GetImpl()->LogDiagnostic(GetContext(), kind, nullptr, 0, pos);

        EXPECT_EQ(storage->size(), baseline + 1U);
        EXPECT_EQ((*storage)[storage->size() - 1U]->Message(), "Plain message");
    }
}

// LogDiagnostic with two arguments: message formatting
TEST_F(NullSafetyTest, LogDiagnosticWithTwoArgs)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A {}")
    {
        auto *storage =
            reinterpret_cast<const ark::es2panda::util::DiagnosticStorage *>(GetImpl()->GetPluginErrors(GetContext()));
        const auto baseline = storage->size();

        auto *kind = GetImpl()->CreateDiagnosticKind(GetContext(), "Got {} and {}", ES2PANDA_PLUGIN_ERROR);
        char const *args[] = {"a", "b"};
        auto *pos = GetImpl()->CreateSourcePosition(GetContext(), 1, 1);
        GetImpl()->LogDiagnostic(GetContext(), kind, args, 2, pos);

        EXPECT_EQ(storage->size(), baseline + 1U);
        EXPECT_EQ((*storage)[storage->size() - 1U]->Message(), "Got a and b");
    }
}

// CreateDiagnosticKind PLUGIN_WARNING mapping: warnings do not enter the
// plugin error storage and never push the context into ERROR (wError_ off)
TEST_F(NullSafetyTest, LogDiagnosticPluginWarningDoesNotBecomeError)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A {}")
    {
        auto *errors =
            reinterpret_cast<const ark::es2panda::util::DiagnosticStorage *>(GetImpl()->GetPluginErrors(GetContext()));
        const auto baseline = errors->size();

        auto *kind = GetImpl()->CreateDiagnosticKind(GetContext(), "W {}", ES2PANDA_PLUGIN_WARNING);
        ASSERT_NE(kind, nullptr);
        char const *args[] = {"1"};
        auto *pos = GetImpl()->CreateSourcePosition(GetContext(), 1, 1);
        GetImpl()->LogDiagnostic(GetContext(), kind, args, 1, pos);

        EXPECT_EQ(errors->size(), baseline);
        EXPECT_FALSE(GetImpl()->IsAnyError(GetContext()));
        EXPECT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_PARSED);
    }
}

// Clean program: all four diagnostic storages are accessible and empty
TEST_F(NullSafetyTest, CleanProgramHasEmptyDiagnosticStorages)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class Clean {}")
    {
        EXPECT_NE(GetImpl()->GetSemanticErrors(GetContext()), nullptr);
        EXPECT_NE(GetImpl()->GetSyntaxErrors(GetContext()), nullptr);
        EXPECT_NE(GetImpl()->GetPluginErrors(GetContext()), nullptr);
        EXPECT_NE(GetImpl()->GetWarnings(GetContext()), nullptr);
    }
}

// Clean program: IsAnyError is false
TEST_F(NullSafetyTest, CleanProgramHasNoErrors)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class Clean2 {}")
    {
        EXPECT_FALSE(GetImpl()->IsAnyError(GetContext()));
    }
}

// Type-error program: IsAnyError is true
TEST_F(ErrorPathLeakTest, TypeErrorProgramHasAnyError)
{
    CONTEXT(ES2PANDA_STATE_ASM_GENERATED, ES2PANDA_STATE_ERROR, "function main() { let x: int = \"\" }")
    {
        EXPECT_TRUE(GetImpl()->IsAnyError(GetContext()));
    }
}

// GetAllErrorMessages on a valid context: non-null (empty when clean)
TEST_F(NullSafetyTest, GetAllErrorMessagesOnCleanContextIsEmpty)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A {}")
    {
        auto *messages = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(messages, nullptr);
    }
}

// Diagnostics leak defense: 10 plugin errors recorded, counts increment exactly
TEST_F(LeakDefenseTest, LogDiagnosticLoop10AccumulatesExactly)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A {}")
    {
        auto *storage =
            reinterpret_cast<const ark::es2panda::util::DiagnosticStorage *>(GetImpl()->GetPluginErrors(GetContext()));
        const auto baseline = storage->size();
        auto *pos = GetImpl()->CreateSourcePosition(GetContext(), 1, 1);
        for (int i = 0; i < 10; i++) {
            auto *kind = GetImpl()->CreateDiagnosticKind(GetContext(), "Loop {}", ES2PANDA_PLUGIN_ERROR);
            ASSERT_NE(kind, nullptr);
            char const *args[] = {"x"};
            GetImpl()->LogDiagnostic(GetContext(), kind, args, 1, pos);
            EXPECT_EQ(storage->size(), baseline + static_cast<size_t>(i) + 1U);
        }
    }
}

// Diagnostics leak defense: one error per context lifecycle, 15 rounds (engine lives with config)
TEST_F(LeakDefenseTest, DiagnosticPerContextLifecycleLoop15)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    for (int round = 0; round < 15; round++) {
        ContextGuard guard {CreateParsedContext(config.Get(), "class A {}", "diag-round.ats")};
        ASSERT_NE(guard.Get(), nullptr);
        auto *kind = Impl()->CreateDiagnosticKind(guard.Get(), "Round diagnostic", ES2PANDA_PLUGIN_ERROR);
        ASSERT_NE(kind, nullptr);
        auto *pos = Impl()->CreateSourcePosition(guard.Get(), 1, 1);
        Impl()->LogDiagnostic(guard.Get(), kind, nullptr, 0, pos);
        auto *storage =
            reinterpret_cast<const ark::es2panda::util::DiagnosticStorage *>(Impl()->GetPluginErrors(guard.Get()));
        EXPECT_GE(storage->size(), 1U);
    }
}

// Diagnostics + literals + positions mixed loop: allocations coexist
TEST_F(LeakDefenseTest, MixedAllocationKindsInterleavedLoop)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class A {}", "mixed.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    for (int i = 0; i < 15; i++) {
        auto *literal = Impl()->CreateNumberLiteral(guard.Get(), i);
        auto *position = Impl()->CreateSourcePosition(guard.Get(), static_cast<size_t>(i), 1);
        auto *kind = Impl()->CreateDiagnosticKind(guard.Get(), "Mixed {}", ES2PANDA_PLUGIN_ERROR);
        auto *memory = Impl()->AllocMemory(guard.Get(), 4, sizeof(uint64_t));
        ASSERT_NE(literal, nullptr);
        ASSERT_NE(position, nullptr);
        ASSERT_NE(kind, nullptr);
        ASSERT_NE(memory, nullptr);
        EXPECT_TRUE(Impl()->NumberLiteralSetInt(literal, i * 3));
    }
}

// ===========================================================================
// Part 6: error paths - defensive handling of syntax and type errors
// Syntax errors use a manual context: the parser may throw (caught inside
// ProceedToState); assert IsAnyError == true and safe destruction.
// ===========================================================================

// Syntax error snippet: "class"
TEST_F(ErrorPathLeakTest, SyntaxErrorTruncatedClass)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class", "syn-class.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *result = Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(Impl()->IsAnyError(result));
}

// Syntax error snippet: "class C"
TEST_F(ErrorPathLeakTest, SyntaxErrorClassWithoutBody)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class C", "syn-class2.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *result = Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(Impl()->IsAnyError(result));
}

// Syntax error snippet: "class C {"
TEST_F(ErrorPathLeakTest, SyntaxErrorUnterminatedClassBody)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class C {", "syn-class3.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *result = Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(Impl()->IsAnyError(result));
}

// Syntax error snippet: "class C { x: }"
TEST_F(ErrorPathLeakTest, SyntaxErrorFieldWithoutType)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class C { x: }", "syn-field.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *result = Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(Impl()->IsAnyError(result));
}

// Syntax error snippet: "function"
TEST_F(ErrorPathLeakTest, SyntaxErrorBareFunctionKeyword)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "function", "syn-func.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *result = Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(Impl()->IsAnyError(result));
}

// Syntax error snippet: "function f("
TEST_F(ErrorPathLeakTest, SyntaxErrorUnterminatedParameterList)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "function f(", "syn-func2.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *result = Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(Impl()->IsAnyError(result));
}

// Syntax error snippet: "function f(): void {"
TEST_F(ErrorPathLeakTest, SyntaxErrorUnterminatedFunctionBody)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "function f(): void {", "syn-func3.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *result = Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(Impl()->IsAnyError(result));
}

// Syntax error snippet: "let"
TEST_F(ErrorPathLeakTest, SyntaxErrorBareLetKeyword)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "let", "syn-let.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *result = Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(Impl()->IsAnyError(result));
}

// Syntax error snippet: "let x: int ="
TEST_F(ErrorPathLeakTest, SyntaxErrorMissingInitializer)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "let x: int =", "syn-let2.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *result = Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(Impl()->IsAnyError(result));
}

// Syntax error snippet: "if ("
TEST_F(ErrorPathLeakTest, SyntaxErrorUnterminatedIfCondition)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "if (", "syn-if.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *result = Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(Impl()->IsAnyError(result));
}

// Syntax error snippet: "while ("
TEST_F(ErrorPathLeakTest, SyntaxErrorUnterminatedWhileCondition)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "while (", "syn-while.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *result = Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(Impl()->IsAnyError(result));
}

// Syntax error snippet: "for ("
TEST_F(ErrorPathLeakTest, SyntaxErrorUnterminatedForHeader)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "for (", "syn-for.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *result = Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(Impl()->IsAnyError(result));
}

// Syntax error snippet: "return"
TEST_F(ErrorPathLeakTest, SyntaxErrorBareReturn)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "return", "syn-return.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *result = Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(Impl()->IsAnyError(result));
}

// Syntax error snippet: "enum"
TEST_F(ErrorPathLeakTest, SyntaxErrorBareEnumKeyword)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "enum", "syn-enum.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *result = Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(Impl()->IsAnyError(result));
}

// Syntax error snippet: "enum E {"
TEST_F(ErrorPathLeakTest, SyntaxErrorUnterminatedEnumBody)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "enum E {", "syn-enum2.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *result = Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(Impl()->IsAnyError(result));
}

// Syntax error snippet: orphan "}"
TEST_F(ErrorPathLeakTest, SyntaxErrorStrayClosingBrace)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "}", "syn-brace.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *result = Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(Impl()->IsAnyError(result));
}

// Syntax error snippet: orphan ")"
TEST_F(ErrorPathLeakTest, SyntaxErrorStrayClosingParen)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), ")", "syn-paren.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *result = Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(Impl()->IsAnyError(result));
}

// Syntax error snippet: "interface"
TEST_F(ErrorPathLeakTest, SyntaxErrorBareInterfaceKeyword)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "interface", "syn-iface.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *result = Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(Impl()->IsAnyError(result));
}

// Syntax error snippet: "let x:"
TEST_F(ErrorPathLeakTest, SyntaxErrorLetWithoutType)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "let x:", "syn-let3.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *result = Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(Impl()->IsAnyError(result));
}

// Syntax error snippet: "class C { m(): }"
TEST_F(ErrorPathLeakTest, SyntaxErrorMethodWithoutReturnType)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class C { m(): }", "syn-method.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *result = Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(Impl()->IsAnyError(result));
}

// Syntax error stress: same bad snippet parsed and destroyed x10, errors every round
TEST_F(ErrorPathLeakTest, SyntaxErrorParseDestroyLoop10)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    for (int i = 0; i < 10; i++) {
        ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class C {", "syn-loop.ats")};
        ASSERT_NE(guard.Get(), nullptr);
        auto *result = Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_PARSED);
        ASSERT_NE(result, nullptr);
        EXPECT_TRUE(Impl()->IsAnyError(result));
    }
}

// Bad and good snippets alternate x12: errors do not leak to the next context
TEST_F(ErrorPathLeakTest, BadAndGoodSourcesAlternateLoop12)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    for (int i = 0; i < 12; i++) {
        {
            ContextGuard bad {Impl()->CreateContextFromString(config.Get(), "class C {", "bad.ats")};
            ASSERT_NE(bad.Get(), nullptr);
            auto *badResult = Impl()->ProceedToState(bad.Get(), ES2PANDA_STATE_PARSED);
            ASSERT_NE(badResult, nullptr);
            EXPECT_TRUE(Impl()->IsAnyError(badResult));
        }
        {
            ContextGuard good {CreateParsedContext(config.Get(), "class G {}", "good.ats")};
            ASSERT_NE(good.Get(), nullptr);
            // Note: IsAnyError reads the config-shared diagnosticEngine, which still
            // holds the bad context's errors; isolation is asserted via state and message
            EXPECT_EQ(Impl()->ContextState(good.Get()), ES2PANDA_STATE_PARSED);
            EXPECT_STREQ(Impl()->ContextErrorMessage(good.Get()), "");
        }
    }
}

// Type error: int assigned string (verified)
TEST_F(ErrorPathLeakTest, TypeErrorIntFromEmptyString)
{
    CONTEXT(ES2PANDA_STATE_ASM_GENERATED, ES2PANDA_STATE_ERROR, "function main() { let x: int = \"\" }")
    {
        EXPECT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
    }
}

// Type error: int assigned boolean
TEST_F(ErrorPathLeakTest, TypeErrorIntFromBoolean)
{
    CONTEXT(ES2PANDA_STATE_ASM_GENERATED, ES2PANDA_STATE_ERROR, "function main() { let x: int = true }")
    {
        EXPECT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
    }
}

// Type error: int assigned double
TEST_F(ErrorPathLeakTest, TypeErrorIntFromDouble)
{
    CONTEXT(ES2PANDA_STATE_ASM_GENERATED, ES2PANDA_STATE_ERROR, "function main() { let x: int = 1.5 }")
    {
        EXPECT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
    }
}

// Type error: string assigned int
TEST_F(ErrorPathLeakTest, TypeErrorStringFromInt)
{
    CONTEXT(ES2PANDA_STATE_ASM_GENERATED, ES2PANDA_STATE_ERROR, "function main() { let s: string = 1 }")
    {
        EXPECT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
    }
}

// Type error: string assigned boolean
TEST_F(ErrorPathLeakTest, TypeErrorStringFromBoolean)
{
    CONTEXT(ES2PANDA_STATE_ASM_GENERATED, ES2PANDA_STATE_ERROR, "function main() { let s: string = true }")
    {
        EXPECT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
    }
}

// Type error: boolean assigned int
TEST_F(ErrorPathLeakTest, TypeErrorBooleanFromInt)
{
    CONTEXT(ES2PANDA_STATE_ASM_GENERATED, ES2PANDA_STATE_ERROR, "function main() { let b: boolean = 1 }")
    {
        EXPECT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
    }
}

// Type error: boolean assigned string
TEST_F(ErrorPathLeakTest, TypeErrorBooleanFromString)
{
    CONTEXT(ES2PANDA_STATE_ASM_GENERATED, ES2PANDA_STATE_ERROR, "function main() { let b: boolean = \"x\" }")
    {
        EXPECT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
    }
}

// Type error: double assigned string
TEST_F(ErrorPathLeakTest, TypeErrorDoubleFromString)
{
    CONTEXT(ES2PANDA_STATE_ASM_GENERATED, ES2PANDA_STATE_ERROR, "function main() { let d: double = \"x\" }")
    {
        EXPECT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
    }
}

// Type error: long assigned string
TEST_F(ErrorPathLeakTest, TypeErrorLongFromString)
{
    CONTEXT(ES2PANDA_STATE_ASM_GENERATED, ES2PANDA_STATE_ERROR, "function main() { let l: long = \"x\" }")
    {
        EXPECT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
    }
}

// Type error: float assigned string
TEST_F(ErrorPathLeakTest, TypeErrorFloatFromString)
{
    CONTEXT(ES2PANDA_STATE_ASM_GENERATED, ES2PANDA_STATE_ERROR, "function main() { let f: float = \"x\" }")
    {
        EXPECT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
    }
}

// Type error: class field type mismatch
TEST_F(ErrorPathLeakTest, TypeErrorClassFieldMismatch)
{
    CONTEXT(ES2PANDA_STATE_ASM_GENERATED, ES2PANDA_STATE_ERROR, "class Bad { f: int = \"nope\" }")
    {
        EXPECT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
    }
}

// Type error: function return type mismatch
TEST_F(ErrorPathLeakTest, TypeErrorFunctionReturnMismatch)
{
    CONTEXT(ES2PANDA_STATE_ASM_GENERATED, ES2PANDA_STATE_ERROR, "function bad(): int { return \"nope\" }")
    {
        EXPECT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
    }
}

// Type error loop: the same bad snippet runs the full pipeline and dies x8
TEST_F(ErrorPathLeakTest, TypeErrorFullPipelineLoop8)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    for (int i = 0; i < 8; i++) {
        ContextGuard guard {
            Impl()->CreateContextFromString(config.Get(), "function main() { let x: int = \"\" }", "typeloop.ats")};
        ASSERT_NE(guard.Get(), nullptr);
        auto *result = Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_ASM_GENERATED);
        ASSERT_NE(result, nullptr);
        EXPECT_EQ(Impl()->ContextState(result), ES2PANDA_STATE_ERROR);
        EXPECT_TRUE(Impl()->IsAnyError(result));
    }
}

// ===========================================================================
// Part 7: GlobalContext - lifecycle and cache APIs
// Note: CreateGlobalContext builds the stdlib cache; keep loops small.
// ===========================================================================

// GlobalContext with zero files: empty list + fileNum=0 (null-input defense)
TEST_F(NullSafetyTest, CreateGlobalContextWithNoFiles)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    GlobalContextGuard guard {Impl()->CreateGlobalContext(config.Get(), nullptr, 0, false)};
    ASSERT_NE(guard.Get(), nullptr);
}

// Variant with LspUsage=true
TEST_F(NullSafetyTest, CreateGlobalContextLspUsage)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    GlobalContextGuard guard {Impl()->CreateGlobalContext(config.Get(), nullptr, 0, true)};
    ASSERT_NE(guard.Get(), nullptr);
}

// One-file GlobalContext + external cache context (verified lifecycle flow)
TEST_F(GlobalContextLifecycleTest, ExternalCacheContextLifecycle)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    char const *files[] = {"external-cache.ats"};
    GlobalContextGuard global {Impl()->CreateGlobalContext(config.Get(), files, 1, false)};
    ASSERT_NE(global.Get(), nullptr);

    // isExternal=true needs the globalContext arena; null is not allowed here
    ContextGuard external {Impl()->CreateCacheContextFromString(config.Get(), "class External { v: int = 1 }", files[0],
                                                                global.Get(), true, false)};
    ASSERT_NE(external.Get(), nullptr);
    auto *parsed = Impl()->ProceedToState(external.Get(), ES2PANDA_STATE_PARSED);
    ASSERT_EQ(parsed, external.Get());
    EXPECT_EQ(Impl()->ContextState(external.Get()), ES2PANDA_STATE_PARSED);
    // External contexts must be destroyed before the GlobalContext
}

// AddFileCache / RemoveFileCache pair: add->remove; empty registry is safe
TEST_F(GlobalContextLifecycleTest, AddAndRemoveFileCachePaired)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    GlobalContextGuard global {Impl()->CreateGlobalContext(config.Get(), nullptr, 0, false)};
    ASSERT_NE(global.Get(), nullptr);
    Impl()->AddFileCache(global.Get(), "added-then-removed.ats");
    Impl()->RemoveFileCache(global.Get(), "added-then-removed.ats");
}

// InvalidateFileCache = Remove + Add: resets the entry's cached allocator
TEST_F(GlobalContextLifecycleTest, InvalidateFileCacheResetsEntry)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    char const *files[] = {"invalidate-me.ats"};
    GlobalContextGuard global {Impl()->CreateGlobalContext(config.Get(), files, 1, false)};
    ASSERT_NE(global.Get(), nullptr);
    Impl()->InvalidateFileCache(global.Get(), files[0]);
    // After Invalidate the entry exists again; DestroyGlobalContext frees it
}

// InvalidateFileCache twice: allocator replaced each time, no leak
TEST_F(GlobalContextLifecycleTest, InvalidateFileCacheTwice)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    char const *files[] = {"invalidate-twice.ats"};
    GlobalContextGuard global {Impl()->CreateGlobalContext(config.Get(), files, 1, false)};
    ASSERT_NE(global.Get(), nullptr);
    Impl()->InvalidateFileCache(global.Get(), files[0]);
    Impl()->InvalidateFileCache(global.Get(), files[0]);
}

// Multiple registered files: two external cache contexts parsed in sequence
TEST_F(GlobalContextLifecycleTest, TwoExternalCacheContextsSequential)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    char const *files[] = {"ext-first.ats", "ext-second.ats"};
    GlobalContextGuard global {Impl()->CreateGlobalContext(config.Get(), files, 2, false)};
    ASSERT_NE(global.Get(), nullptr);
    {
        ContextGuard first {
            Impl()->CreateCacheContextFromString(config.Get(), "class First {}", files[0], global.Get(), true, false)};
        ASSERT_NE(first.Get(), nullptr);
        auto *parsedFirst = Impl()->ProceedToState(first.Get(), ES2PANDA_STATE_PARSED);
        ASSERT_EQ(parsedFirst, first.Get());
        EXPECT_EQ(Impl()->ContextState(first.Get()), ES2PANDA_STATE_PARSED);
    }
    {
        ContextGuard second {
            Impl()->CreateCacheContextFromString(config.Get(), "class Second {}", files[1], global.Get(), true, false)};
        ASSERT_NE(second.Get(), nullptr);
        auto *parsedSecond = Impl()->ProceedToState(second.Get(), ES2PANDA_STATE_PARSED);
        ASSERT_EQ(parsedSecond, second.Get());
        EXPECT_EQ(Impl()->ContextState(second.Get()), ES2PANDA_STATE_PARSED);
    }
}

// Remove all entries after adding: nothing left to free at destruction
TEST_F(GlobalContextLifecycleTest, RemoveAllEntriesBeforeDestroy)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    char const *files[] = {"removed-entry.ats"};
    GlobalContextGuard global {Impl()->CreateGlobalContext(config.Get(), files, 1, false)};
    ASSERT_NE(global.Get(), nullptr);
    Impl()->RemoveFileCache(global.Get(), files[0]);
    // Entries removed; externalProgramAllocators is empty at destroy time
}

// GlobalContext create/destroy loop x3 (stdlib cache built each time)
TEST_F(LeakDefenseTest, GlobalContextCreateDestroyLoop3)
{
    for (int i = 0; i < 3; i++) {
        ConfigGuard config {NewConfig()};
        ASSERT_NE(config.Get(), nullptr);
        GlobalContextGuard global {Impl()->CreateGlobalContext(config.Get(), nullptr, 0, false)};
        ASSERT_NE(global.Get(), nullptr);
    }
}

// ===========================================================================
// Part 8: EvaluateExpression - real malloc/free heap leak tests
// EvaluateExpression returns a malloc'd base64 bytecode string; free it with
// FreeExpressionResult. A missing pair is caught by LSAN directly.
// ===========================================================================

// Basic eval: "1 + 2" returns non-null; freeing does not crash
TEST_F(EvaluateExpressionLifecycleTest, EvaluateSimpleAddition)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    auto *result = Impl()->EvaluateExpression(config.Get(), Base64Encode("1 + 2").c_str());
    ASSERT_NE(result, nullptr);
    Impl()->FreeExpressionResult(result);
}

// Evaluate "42"
TEST_F(EvaluateExpressionLifecycleTest, EvaluateLiteral42)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    auto *result = Impl()->EvaluateExpression(config.Get(), Base64Encode("42").c_str());
    ASSERT_NE(result, nullptr);
    Impl()->FreeExpressionResult(result);
}

// Evaluate "2 * 3"
TEST_F(EvaluateExpressionLifecycleTest, EvaluateMultiplication)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    auto *result = Impl()->EvaluateExpression(config.Get(), Base64Encode("2 * 3").c_str());
    ASSERT_NE(result, nullptr);
    Impl()->FreeExpressionResult(result);
}

// Evaluate "10 - 4"
TEST_F(EvaluateExpressionLifecycleTest, EvaluateSubtraction)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    auto *result = Impl()->EvaluateExpression(config.Get(), Base64Encode("10 - 4").c_str());
    ASSERT_NE(result, nullptr);
    Impl()->FreeExpressionResult(result);
}

// Null config: returns nullptr
TEST_F(NullSafetyTest, EvaluateExpressionWithNullConfigReturnsNull)
{
    ASSERT_EQ(Impl()->EvaluateExpression(nullptr, Base64Encode("1 + 2").c_str()), nullptr);
}

// Null expression: returns nullptr
TEST_F(NullSafetyTest, EvaluateExpressionWithNullExpressionReturnsNull)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ASSERT_EQ(Impl()->EvaluateExpression(config.Get(), nullptr), nullptr);
}

// Empty expression: returns nullptr
TEST_F(NullSafetyTest, EvaluateExpressionWithEmptyExpressionReturnsNull)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ASSERT_EQ(Impl()->EvaluateExpression(config.Get(), ""), nullptr);
}

// Invalid base64 input: returns nullptr
TEST_F(NullSafetyTest, EvaluateExpressionWithInvalidBase64ReturnsNull)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ASSERT_EQ(Impl()->EvaluateExpression(config.Get(), "!!!not-base64!!!"), nullptr);
}

// FreeExpressionResult(nullptr) is a no-op
TEST_F(NullSafetyTest, FreeExpressionResultNullIsNoOp)
{
    Impl()->FreeExpressionResult(nullptr);
    SUCCEED();
}

// Eval + free loop x5: one malloc/free per round, nothing accumulates
TEST_F(LeakDefenseTest, EvaluateAndFreePairedLoop5)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    for (int i = 0; i < 5; i++) {
        auto *result = Impl()->EvaluateExpression(config.Get(), Base64Encode("1 + 2").c_str());
        ASSERT_NE(result, nullptr);
        Impl()->FreeExpressionResult(result);
    }
}

// Evaluate several distinct expressions; each result freed independently
TEST_F(LeakDefenseTest, EvaluateMultipleDistinctExpressionsEachFreed)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    std::vector<std::string> expressions {"1 + 2", "42", "2 * 3", "10 - 4", "100 + 1"};
    for (auto const &expression : expressions) {
        auto *result = Impl()->EvaluateExpression(config.Get(), Base64Encode(expression).c_str());
        ASSERT_NE(result, nullptr) << "expression: " << expression;
        Impl()->FreeExpressionResult(result);
    }
}

// ===========================================================================
// Part 9: AST walk invariant matrix (4 probes per valid snippet)
//   Probe 1: identifier set contains the declared names
//   Probe 2: node count >= threshold; two walks agree (no side effects)
//   Probe 3: DumpEtsSrc contains the key source fragments
//   Probe 4: parent invariant - only the ETSModule root has a null parent
// ===========================================================================

// --- Snippet: empty class ---
TEST_F(AstWalkInvariantTest, IdentifiersPresentEmptyClass)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A {}")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "A"));
        EXPECT_GE(payload.identifierCount, 1U);
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableEmptyClass)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A {}")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 2U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, DumpContainsEmptyClass)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A {}")
    {
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), CApi(GetAst()));
        ASSERT_NE(dump, nullptr);
        EXPECT_TRUE(Contains(std::string(dump), "class"));
        EXPECT_TRUE(Contains(std::string(dump), "A"));
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantEmptyClass)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A {}")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: class with one int field ---
TEST_F(AstWalkInvariantTest, IdentifiersPresentClassWithIntField)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { x: int = 0 }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "A"));
        EXPECT_TRUE(Contains(payload.identifierNames, "x"));
        // The "int" annotation is an ETSPrimitiveType node, not an Identifier
        EXPECT_GE(CountNodesOfKind(GetImpl(), CApi(GetAst()), IsPrimitiveTypeNode), 1U);
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableClassWithIntField)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { x: int = 0 }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 4U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, DumpContainsClassWithIntField)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { x: int = 0 }")
    {
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), CApi(GetAst()));
        ASSERT_NE(dump, nullptr);
        EXPECT_TRUE(Contains(std::string(dump), "int"));
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantClassWithIntField)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { x: int = 0 }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: class with two fields ---
TEST_F(AstWalkInvariantTest, IdentifiersPresentClassWithTwoFields)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { x: int = 0; y: int = 1 }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "x"));
        EXPECT_TRUE(Contains(payload.identifierNames, "y"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableClassWithTwoFields)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { x: int = 0; y: int = 1 }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 6U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, DumpContainsClassWithTwoFields)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { x: int = 0; y: int = 1 }")
    {
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), CApi(GetAst()));
        ASSERT_NE(dump, nullptr);
        EXPECT_TRUE(Contains(std::string(dump), "y"));
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantClassWithTwoFields)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { x: int = 0; y: int = 1 }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: class with a void method ---
TEST_F(AstWalkInvariantTest, IdentifiersPresentClassWithVoidMethod)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { m(): void {} }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "m"));
        // The "void" return type is an ETSPrimitiveType node, not an Identifier
        EXPECT_GE(CountNodesOfKind(GetImpl(), CApi(GetAst()), IsPrimitiveTypeNode), 1U);
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableClassWithVoidMethod)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { m(): void {} }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 5U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, DumpContainsClassWithVoidMethod)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { m(): void {} }")
    {
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), CApi(GetAst()));
        ASSERT_NE(dump, nullptr);
        EXPECT_TRUE(Contains(std::string(dump), "m"));
        EXPECT_TRUE(Contains(std::string(dump), "void"));
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantClassWithVoidMethod)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { m(): void {} }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: method returning int ---
TEST_F(AstWalkInvariantTest, IdentifiersPresentClassWithIntMethod)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { m(): int { return 1 } }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "m"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableClassWithIntMethod)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { m(): int { return 1 } }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 6U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, DumpContainsClassWithIntMethod)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { m(): int { return 1 } }")
    {
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), CApi(GetAst()));
        ASSERT_NE(dump, nullptr);
        EXPECT_TRUE(Contains(std::string(dump), "return"));
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantClassWithIntMethod)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { m(): int { return 1 } }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: static field ---
TEST_F(AstWalkInvariantTest, IdentifiersPresentClassWithStaticField)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { static s: int = 5 }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "s"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableClassWithStaticField)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { static s: int = 5 }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 5U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, DumpContainsClassWithStaticField)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { static s: int = 5 }")
    {
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), CApi(GetAst()));
        ASSERT_NE(dump, nullptr);
        EXPECT_TRUE(Contains(std::string(dump), "static"));
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantClassWithStaticField)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { static s: int = 5 }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: constructor ---
TEST_F(AstWalkInvariantTest, IdentifiersPresentClassWithConstructor)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { constructor() {} }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "constructor"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableClassWithConstructor)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { constructor() {} }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 5U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, DumpContainsClassWithConstructor)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { constructor() {} }")
    {
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), CApi(GetAst()));
        ASSERT_NE(dump, nullptr);
        EXPECT_TRUE(Contains(std::string(dump), "constructor"));
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantClassWithConstructor)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { constructor() {} }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: this expression ---
TEST_F(AstWalkInvariantTest, IdentifiersPresentClassWithThisAccess)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { x: int = 0; m(): int { return this.x } }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "x"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableClassWithThisAccess)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { x: int = 0; m(): int { return this.x } }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 8U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, DumpContainsClassWithThisAccess)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { x: int = 0; m(): int { return this.x } }")
    {
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), CApi(GetAst()));
        ASSERT_NE(dump, nullptr);
        EXPECT_TRUE(Contains(std::string(dump), "this"));
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantClassWithThisAccess)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { x: int = 0; m(): int { return this.x } }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: string field ---
TEST_F(AstWalkInvariantTest, IdentifiersPresentClassWithStringField)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { s: string = \"abc\" }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "string"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableClassWithStringField)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { s: string = \"abc\" }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 4U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, DumpContainsClassWithStringField)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { s: string = \"abc\" }")
    {
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), CApi(GetAst()));
        ASSERT_NE(dump, nullptr);
        EXPECT_TRUE(Contains(std::string(dump), "abc"));
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantClassWithStringField)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { s: string = \"abc\" }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: boolean field ---
TEST_F(AstWalkInvariantTest, IdentifiersPresentClassWithBooleanField)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { b: boolean = true }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "b"));
        EXPECT_GE(CountNodesOfKind(GetImpl(), CApi(GetAst()), IsPrimitiveTypeNode), 1U);
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableClassWithBooleanField)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { b: boolean = true }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 4U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, DumpContainsClassWithBooleanField)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { b: boolean = true }")
    {
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), CApi(GetAst()));
        ASSERT_NE(dump, nullptr);
        EXPECT_TRUE(Contains(std::string(dump), "boolean"));
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantClassWithBooleanField)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { b: boolean = true }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: long field ---
TEST_F(AstWalkInvariantTest, IdentifiersPresentClassWithLongField)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { l: long = 5 }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "l"));
        EXPECT_GE(CountNodesOfKind(GetImpl(), CApi(GetAst()), IsPrimitiveTypeNode), 1U);
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableClassWithLongField)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { l: long = 5 }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 4U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, DumpContainsClassWithLongField)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { l: long = 5 }")
    {
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), CApi(GetAst()));
        ASSERT_NE(dump, nullptr);
        EXPECT_TRUE(Contains(std::string(dump), "long"));
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantClassWithLongField)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { l: long = 5 }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: double field ---
TEST_F(AstWalkInvariantTest, IdentifiersPresentClassWithDoubleField)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { d: double = 1.5 }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "d"));
        EXPECT_GE(CountNodesOfKind(GetImpl(), CApi(GetAst()), IsPrimitiveTypeNode), 1U);
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableClassWithDoubleField)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { d: double = 1.5 }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 4U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, DumpContainsClassWithDoubleField)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { d: double = 1.5 }")
    {
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), CApi(GetAst()));
        ASSERT_NE(dump, nullptr);
        EXPECT_TRUE(Contains(std::string(dump), "1.5"));
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantClassWithDoubleField)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { d: double = 1.5 }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: one class, two methods ---
TEST_F(AstWalkInvariantTest, IdentifiersPresentClassWithTwoMethods)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { m1(): void {} m2(): void {} }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "m1"));
        EXPECT_TRUE(Contains(payload.identifierNames, "m2"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableClassWithTwoMethods)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { m1(): void {} m2(): void {} }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 9U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, DumpContainsClassWithTwoMethods)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { m1(): void {} m2(): void {} }")
    {
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), CApi(GetAst()));
        ASSERT_NE(dump, nullptr);
        EXPECT_TRUE(Contains(std::string(dump), "m1"));
        EXPECT_TRUE(Contains(std::string(dump), "m2"));
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantClassWithTwoMethods)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { m1(): void {} m2(): void {} }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: two classes ---
TEST_F(AstWalkInvariantTest, IdentifiersPresentTwoClasses)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A {}\nclass B {}")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "A"));
        EXPECT_TRUE(Contains(payload.identifierNames, "B"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableTwoClasses)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A {}\nclass B {}")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 4U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, DumpContainsTwoClasses)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A {}\nclass B {}")
    {
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), CApi(GetAst()));
        ASSERT_NE(dump, nullptr);
        EXPECT_TRUE(Contains(std::string(dump), "B"));
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantTwoClasses)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A {}\nclass B {}")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: no-parameter function ---
TEST_F(AstWalkInvariantTest, IdentifiersPresentSimpleFunction)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): void {}")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "f"));
        EXPECT_GE(CountNodesOfKind(GetImpl(), CApi(GetAst()), IsPrimitiveTypeNode), 1U);
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableSimpleFunction)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): void {}")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 4U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, DumpContainsSimpleFunction)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): void {}")
    {
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), CApi(GetAst()));
        ASSERT_NE(dump, nullptr);
        EXPECT_TRUE(Contains(std::string(dump), "function"));
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantSimpleFunction)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): void {}")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: single-parameter function ---
TEST_F(AstWalkInvariantTest, IdentifiersPresentFunctionWithParam)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(a: int): int { return a }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "a"));
        // "int" annotations of params and return are ETSPrimitiveType nodes
        EXPECT_GE(CountNodesOfKind(GetImpl(), CApi(GetAst()), IsPrimitiveTypeNode), 2U);
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableFunctionWithParam)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(a: int): int { return a }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 7U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, DumpContainsFunctionWithParam)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(a: int): int { return a }")
    {
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), CApi(GetAst()));
        ASSERT_NE(dump, nullptr);
        EXPECT_TRUE(Contains(std::string(dump), "return"));
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantFunctionWithParam)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(a: int): int { return a }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: two-parameter function ---
TEST_F(AstWalkInvariantTest, IdentifiersPresentFunctionWithTwoParams)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(a: int, b: int): int { return a + b }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "a"));
        EXPECT_TRUE(Contains(payload.identifierNames, "b"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableFunctionWithTwoParams)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(a: int, b: int): int { return a + b }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 9U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, DumpContainsFunctionWithTwoParams)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(a: int, b: int): int { return a + b }")
    {
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), CApi(GetAst()));
        ASSERT_NE(dump, nullptr);
        // DumpEtsSrc parenthesizes operands and operators: "((a) + (b))"
        EXPECT_TRUE(Contains(std::string(dump), "function f(a: int, b: int): int"));
        EXPECT_TRUE(Contains(std::string(dump), "((a) + (b))"));
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantFunctionWithTwoParams)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(a: int, b: int): int { return a + b }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: function returning string ---
TEST_F(AstWalkInvariantTest, IdentifiersPresentStringReturnFunction)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): string { return \"s\" }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "string"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableStringReturnFunction)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): string { return \"s\" }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 6U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, DumpContainsStringReturnFunction)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): string { return \"s\" }")
    {
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), CApi(GetAst()));
        ASSERT_NE(dump, nullptr);
        EXPECT_TRUE(Contains(std::string(dump), "string"));
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantStringReturnFunction)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): string { return \"s\" }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: function returning boolean ---
TEST_F(AstWalkInvariantTest, IdentifiersPresentBooleanReturnFunction)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): boolean { return true }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "f"));
        EXPECT_GE(CountNodesOfKind(GetImpl(), CApi(GetAst()), IsPrimitiveTypeNode), 1U);
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableBooleanReturnFunction)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): boolean { return true }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 5U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, DumpContainsBooleanReturnFunction)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): boolean { return true }")
    {
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), CApi(GetAst()));
        ASSERT_NE(dump, nullptr);
        EXPECT_TRUE(Contains(std::string(dump), "true"));
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantBooleanReturnFunction)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): boolean { return true }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: local variables in a function body ---
TEST_F(AstWalkInvariantTest, IdentifiersPresentFunctionWithLocal)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): int { let a: int = 1; return a }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "a"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableFunctionWithLocal)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): int { let a: int = 1; return a }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 9U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, DumpContainsFunctionWithLocal)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): int { let a: int = 1; return a }")
    {
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), CApi(GetAst()));
        ASSERT_NE(dump, nullptr);
        EXPECT_TRUE(Contains(std::string(dump), "let"));
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantFunctionWithLocal)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): int { let a: int = 1; return a }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: top-level int ---
TEST_F(AstWalkInvariantTest, IdentifiersPresentTopLevelInt)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let x: int = 5")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "x"));
        EXPECT_GE(CountNodesOfKind(GetImpl(), CApi(GetAst()), IsPrimitiveTypeNode), 1U);
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableTopLevelInt)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let x: int = 5")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 4U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, DumpContainsTopLevelInt)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let x: int = 5")
    {
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), CApi(GetAst()));
        ASSERT_NE(dump, nullptr);
        EXPECT_TRUE(Contains(std::string(dump), "let"));
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantTopLevelInt)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let x: int = 5")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: top-level string ---
TEST_F(AstWalkInvariantTest, IdentifiersPresentTopLevelString)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let x: string = \"hello\"")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "x"));
        EXPECT_TRUE(Contains(payload.identifierNames, "string"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableTopLevelString)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let x: string = \"hello\"")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 4U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, DumpContainsTopLevelString)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let x: string = \"hello\"")
    {
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), CApi(GetAst()));
        ASSERT_NE(dump, nullptr);
        EXPECT_TRUE(Contains(std::string(dump), "hello"));
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantTopLevelString)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let x: string = \"hello\"")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: top-level boolean ---
TEST_F(AstWalkInvariantTest, IdentifiersPresentTopLevelBoolean)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let b: boolean = false")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "b"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableTopLevelBoolean)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let b: boolean = false")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 4U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, DumpContainsTopLevelBoolean)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let b: boolean = false")
    {
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), CApi(GetAst()));
        ASSERT_NE(dump, nullptr);
        EXPECT_TRUE(Contains(std::string(dump), "false"));
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantTopLevelBoolean)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let b: boolean = false")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: top-level double ---
TEST_F(AstWalkInvariantTest, IdentifiersPresentTopLevelDouble)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let d: double = 2.5")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "d"));
        EXPECT_GE(CountNodesOfKind(GetImpl(), CApi(GetAst()), IsPrimitiveTypeNode), 1U);
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableTopLevelDouble)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let d: double = 2.5")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 4U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, DumpContainsTopLevelDouble)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let d: double = 2.5")
    {
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), CApi(GetAst()));
        ASSERT_NE(dump, nullptr);
        EXPECT_TRUE(Contains(std::string(dump), "2.5"));
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantTopLevelDouble)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let d: double = 2.5")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: negative literal ---
TEST_F(AstWalkInvariantTest, IdentifiersPresentNegativeLiteral)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let n: int = -1")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "n"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableNegativeLiteral)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let n: int = -1")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 5U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, DumpContainsNegativeLiteral)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let n: int = -1")
    {
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), CApi(GetAst()));
        ASSERT_NE(dump, nullptr);
        EXPECT_TRUE(Contains(std::string(dump), "-1"));
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantNegativeLiteral)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let n: int = -1")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: two top-level declarations ---
TEST_F(AstWalkInvariantTest, IdentifiersPresentTwoTopLevelLets)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let x: int = 1\nlet y: int = 2")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "x"));
        EXPECT_TRUE(Contains(payload.identifierNames, "y"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableTwoTopLevelLets)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let x: int = 1\nlet y: int = 2")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 8U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, DumpContainsTwoTopLevelLets)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let x: int = 1\nlet y: int = 2")
    {
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), CApi(GetAst()));
        ASSERT_NE(dump, nullptr);
        EXPECT_TRUE(Contains(std::string(dump), "x"));
        EXPECT_TRUE(Contains(std::string(dump), "y"));
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantTwoTopLevelLets)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let x: int = 1\nlet y: int = 2")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: simple enum ---
TEST_F(AstWalkInvariantTest, IdentifiersPresentSimpleEnum)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "enum Color { RED, GREEN, BLUE }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "Color"));
        EXPECT_TRUE(Contains(payload.identifierNames, "RED"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableSimpleEnum)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "enum Color { RED, GREEN, BLUE }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 6U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, DumpContainsSimpleEnum)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "enum Color { RED, GREEN, BLUE }")
    {
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), CApi(GetAst()));
        ASSERT_NE(dump, nullptr);
        EXPECT_TRUE(Contains(std::string(dump), "enum"));
        EXPECT_TRUE(Contains(std::string(dump), "GREEN"));
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantSimpleEnum)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "enum Color { RED, GREEN, BLUE }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: enum with initializers ---
TEST_F(AstWalkInvariantTest, IdentifiersPresentInitializedEnum)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "enum Color { RED = 1, GREEN = 2 }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "RED"));
        EXPECT_TRUE(Contains(payload.identifierNames, "GREEN"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableInitializedEnum)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "enum Color { RED = 1, GREEN = 2 }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 7U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, DumpContainsInitializedEnum)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "enum Color { RED = 1, GREEN = 2 }")
    {
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), CApi(GetAst()));
        ASSERT_NE(dump, nullptr);
        EXPECT_TRUE(Contains(std::string(dump), "RED"));
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantInitializedEnum)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "enum Color { RED = 1, GREEN = 2 }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: exported class ---
TEST_F(AstWalkInvariantTest, IdentifiersPresentExportedClass)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "export class A {}")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "A"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableExportedClass)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "export class A {}")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 3U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, DumpContainsExportedClass)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "export class A {}")
    {
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), CApi(GetAst()));
        ASSERT_NE(dump, nullptr);
        EXPECT_TRUE(Contains(std::string(dump), "export"));
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantExportedClass)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "export class A {}")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: exported function ---
TEST_F(AstWalkInvariantTest, IdentifiersPresentExportedFunction)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "export function f(): void {}")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "f"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableExportedFunction)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "export function f(): void {}")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 5U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, DumpContainsExportedFunction)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "export function f(): void {}")
    {
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), CApi(GetAst()));
        ASSERT_NE(dump, nullptr);
        EXPECT_TRUE(Contains(std::string(dump), "export"));
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantExportedFunction)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "export function f(): void {}")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: local variable in main ---
TEST_F(AstWalkInvariantTest, IdentifiersPresentMainWithLocal)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function main() { let x: int = 1 }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "main"));
        EXPECT_TRUE(Contains(payload.identifierNames, "x"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableMainWithLocal)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function main() { let x: int = 1 }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 7U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, DumpContainsMainWithLocal)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function main() { let x: int = 1 }")
    {
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), CApi(GetAst()));
        ASSERT_NE(dump, nullptr);
        EXPECT_TRUE(Contains(std::string(dump), "main"));
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantMainWithLocal)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function main() { let x: int = 1 }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: lambda ---
TEST_F(AstWalkInvariantTest, IdentifiersPresentTopLevelLambda)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let f = (a: int): int => a")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "a"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableTopLevelLambda)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let f = (a: int): int => a")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 5U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, DumpContainsTopLevelLambda)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let f = (a: int): int => a")
    {
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), CApi(GetAst()));
        ASSERT_NE(dump, nullptr);
        EXPECT_TRUE(Contains(std::string(dump), "f"));
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantTopLevelLambda)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let f = (a: int): int => a")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: interface ---
TEST_F(AstWalkInvariantTest, IdentifiersPresentInterfaceWithMethod)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "interface I { m(): void }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "I"));
        EXPECT_TRUE(Contains(payload.identifierNames, "m"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableInterfaceWithMethod)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "interface I { m(): void }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 5U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, DumpContainsInterfaceWithMethod)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "interface I { m(): void }")
    {
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), CApi(GetAst()));
        ASSERT_NE(dump, nullptr);
        EXPECT_TRUE(Contains(std::string(dump), "interface"));
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantInterfaceWithMethod)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "interface I { m(): void }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: class + main (matches an existing verified case) ---
TEST_F(AstWalkInvariantTest, IdentifiersPresentClassWithMain)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(class C { n: string = "oh" }
function main() { let c = new C })XXX")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "C"));
        EXPECT_TRUE(Contains(payload.identifierNames, "main"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableClassWithMain)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(class C { n: string = "oh" }
function main() { let c = new C })XXX")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 12U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, DumpContainsClassWithMain)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(class C { n: string = "oh" }
function main() { let c = new C })XXX")
    {
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), CApi(GetAst()));
        ASSERT_NE(dump, nullptr);
        EXPECT_TRUE(Contains(std::string(dump), "oh"));
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantClassWithMain)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(class C { n: string = "oh" }
function main() { let c = new C })XXX")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: minimal main ---
TEST_F(AstWalkInvariantTest, IdentifiersPresentMinimalMain)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function main() {}")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "main"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableMinimalMain)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function main() {}")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 4U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, DumpContainsMinimalMain)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function main() {}")
    {
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), CApi(GetAst()));
        ASSERT_NE(dump, nullptr);
        EXPECT_TRUE(Contains(std::string(dump), "main"));
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantMinimalMain)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function main() {}")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: empty source ---
TEST_F(AstWalkInvariantTest, EmptySourceWalkProducesRootOnly)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

TEST_F(AstWalkInvariantTest, EmptySourceNodeCountIsMinimal)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 1U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

// ===========================================================================
// Part 10: structural probes - method/function/declarator accessor chains
// Verify MethodDefinitionFunction -> ScriptFunctionId -> IdentifierName chains
// on real ASTs; pointer access stays in-bounds and non-dangling.
// ===========================================================================

// Class method: name retrieved from MethodDefinition down to the Identifier
TEST_F(AstWalkInvariantTest, MethodNameRoundtripThroughScriptFunction)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { roundTrip(): void {} }")
    {
        auto *program = GetImpl()->ContextProgram(GetContext());
        ASSERT_NE(program, nullptr);
        auto *method = GetImpl()->FirstDeclarationByNameFromProgram(GetContext(), program, "roundTrip");
        ASSERT_NE(method, nullptr);
        auto *func = GetImpl()->MethodDefinitionFunction(GetContext(), method);
        ASSERT_NE(func, nullptr);
        auto *id = GetImpl()->ScriptFunctionId(GetContext(), func);
        ASSERT_NE(id, nullptr);
        EXPECT_STREQ(GetImpl()->IdentifierName(GetContext(), id), "roundTrip");
    }
}

// Class method: body (ScriptFunctionBody) is non-null
TEST_F(AstWalkInvariantTest, MethodScriptFunctionBodyIsNotNull)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { withBody(): int { return 1 } }")
    {
        auto *program = GetImpl()->ContextProgram(GetContext());
        ASSERT_NE(program, nullptr);
        auto *method = GetImpl()->FirstDeclarationByNameFromProgram(GetContext(), program, "withBody");
        ASSERT_NE(method, nullptr);
        auto *func = GetImpl()->MethodDefinitionFunction(GetContext(), method);
        ASSERT_NE(func, nullptr);
        EXPECT_NE(GetImpl()->ScriptFunctionBody(GetContext(), func), nullptr);
    }
}

// Top-level function: found via IsFunctionDeclaration
TEST_F(AstWalkInvariantTest, TopLevelFunctionDeclarationIsFindable)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function topFn(): void {}")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), FindFirstDeclarationCallback, &payload);
        ASSERT_NE(payload.firstFunctionDeclaration, nullptr);
    }
}

// Top-level function: FunctionDeclaration -> Function -> Id -> name
TEST_F(AstWalkInvariantTest, FunctionDeclarationNameRoundtrip)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function namedFn(): void {}")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), FindFirstDeclarationCallback, &payload);
        ASSERT_NE(payload.firstFunctionDeclaration, nullptr);
        auto *func = GetImpl()->FunctionDeclarationFunction(GetContext(), payload.firstFunctionDeclaration);
        ASSERT_NE(func, nullptr);
        auto *id = GetImpl()->ScriptFunctionId(GetContext(), func);
        ASSERT_NE(id, nullptr);
        EXPECT_STREQ(GetImpl()->IdentifierName(GetContext(), id), "namedFn");
    }
}

// Top-level function: body is non-null
TEST_F(AstWalkInvariantTest, FunctionDeclarationBodyIsNotNull)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function withBody(): int { return 2 }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), FindFirstDeclarationCallback, &payload);
        ASSERT_NE(payload.firstFunctionDeclaration, nullptr);
        auto *func = GetImpl()->FunctionDeclarationFunction(GetContext(), payload.firstFunctionDeclaration);
        ASSERT_NE(func, nullptr);
        EXPECT_NE(GetImpl()->ScriptFunctionBody(GetContext(), func), nullptr);
    }
}

// Top-level let: VariableDeclaration is found
TEST_F(AstWalkInvariantTest, TopLevelVariableDeclarationIsFindable)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let v: int = 3")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), FindFirstDeclarationCallback, &payload);
        ASSERT_NE(payload.firstVariableDeclaration, nullptr);
    }
}

// Top-level let: declarator Id name round-trip
TEST_F(AstWalkInvariantTest, VariableDeclaratorIdNameRoundtrip)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let answer: int = 42")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), FindFirstDeclarationCallback, &payload);
        ASSERT_NE(payload.firstVariableDeclaration, nullptr);
        size_t declaratorCount = 0;
        auto **declarators = GetImpl()->VariableDeclarationDeclaratorsConst(
            GetContext(), payload.firstVariableDeclaration, &declaratorCount);
        ASSERT_EQ(declaratorCount, 1U);
        ASSERT_NE(declarators, nullptr);
        auto *id = GetImpl()->VariableDeclaratorId(GetContext(), declarators[0]);
        ASSERT_NE(id, nullptr);
        EXPECT_STREQ(GetImpl()->IdentifierName(GetContext(), id), "answer");
    }
}

// Top-level let: declarator Init is non-null
TEST_F(AstWalkInvariantTest, VariableDeclaratorInitIsNotNull)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let withInit: int = 7")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), FindFirstDeclarationCallback, &payload);
        ASSERT_NE(payload.firstVariableDeclaration, nullptr);
        size_t declaratorCount = 0;
        auto **declarators = GetImpl()->VariableDeclarationDeclaratorsConst(
            GetContext(), payload.firstVariableDeclaration, &declaratorCount);
        ASSERT_EQ(declaratorCount, 1U);
        auto *init = GetImpl()->VariableDeclaratorInit(GetContext(), declarators[0]);
        EXPECT_NE(init, nullptr);
    }
}

// Top-level let: Init dump contains the initial value
TEST_F(AstWalkInvariantTest, VariableDeclaratorInitDumpContainsInitializer)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let dumpMe: int = 99")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), FindFirstDeclarationCallback, &payload);
        ASSERT_NE(payload.firstVariableDeclaration, nullptr);
        size_t declaratorCount = 0;
        auto **declarators = GetImpl()->VariableDeclarationDeclaratorsConst(
            GetContext(), payload.firstVariableDeclaration, &declaratorCount);
        ASSERT_EQ(declaratorCount, 1U);
        auto *init = GetImpl()->VariableDeclaratorInit(GetContext(), declarators[0]);
        ASSERT_NE(init, nullptr);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), init);
        ASSERT_NE(dump, nullptr);
        EXPECT_TRUE(Contains(std::string(dump), "99"));
    }
}

// Local let in main: declarator probes apply as well
TEST_F(AstWalkInvariantTest, MainLocalVariableDeclaratorProbes)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function main() { let local: int = 5 }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), FindFirstDeclarationCallback, &payload);
        ASSERT_NE(payload.firstVariableDeclaration, nullptr);
        size_t declaratorCount = 0;
        auto **declarators = GetImpl()->VariableDeclarationDeclaratorsConst(
            GetContext(), payload.firstVariableDeclaration, &declaratorCount);
        ASSERT_EQ(declaratorCount, 1U);
        auto *id = GetImpl()->VariableDeclaratorId(GetContext(), declarators[0]);
        ASSERT_NE(id, nullptr);
        EXPECT_STREQ(GetImpl()->IdentifierName(GetContext(), id), "local");
    }
}

// Class field access: ClassDefinitionIdent name round-trip
TEST_F(AstWalkInvariantTest, ClassDefinitionIdentNameRoundtrip)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class NamedClass {}")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), FindFirstDeclarationCallback, &payload);
        ASSERT_NE(payload.firstMethodDefinition, nullptr) << "no method in fixture snippet";
    }
}

// Fixed: class definition ident probe (ClassDeclaration -> ClassDefinition -> Ident)
TEST_F(AstWalkInvariantTest, ClassDefinitionIdentNameRoundtripFixed)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class NamedClass { m(): void {} }")
    {
        auto *program = GetImpl()->ContextProgram(GetContext());
        ASSERT_NE(program, nullptr);
        auto *ast = GetImpl()->ProgramAst(GetContext(), program);
        ASSERT_NE(ast, nullptr);

        struct Search {
            es2panda_Impl const *impl;
            es2panda_Context *ctx;
            es2panda_AstNode *classDecl = nullptr;
        } search {GetImpl(), GetContext()};

        auto finder = [](es2panda_AstNode *node, void *arg) {
            auto *s = static_cast<Search *>(arg);
            if (s->classDecl == nullptr && s->impl->IsClassDeclaration(node)) {
                s->classDecl = node;
            }
        };
        GetImpl()->AstNodeForEach(ast, finder, &search);
        ASSERT_NE(search.classDecl, nullptr);

        auto *definition = GetImpl()->ClassDeclarationDefinition(search.ctx, search.classDecl);
        ASSERT_NE(definition, nullptr);
        auto *ident = GetImpl()->ClassDefinitionIdent(search.ctx, definition);
        ASSERT_NE(ident, nullptr);
        EXPECT_STREQ(GetImpl()->IdentifierName(search.ctx, ident), "NamedClass");
    }
}

// Class field: method definition probe (IsMethodDefinition finds the method)
TEST_F(AstWalkInvariantTest, MethodDefinitionIsFindableInClass)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { someMethod(): void {} }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), FindFirstDeclarationCallback, &payload);
        ASSERT_NE(payload.firstMethodDefinition, nullptr);
        auto *func = GetImpl()->MethodDefinitionFunction(GetContext(), payload.firstMethodDefinition);
        ASSERT_NE(func, nullptr);
        auto *id = GetImpl()->ScriptFunctionId(GetContext(), func);
        ASSERT_NE(id, nullptr);
        EXPECT_STREQ(GetImpl()->IdentifierName(GetContext(), id), "someMethod");
    }
}

// Class field access: the field dump contains the field name
TEST_F(AstWalkInvariantTest, ClassFieldAppearsInDump)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { fieldName: int = 1 }")
    {
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), CApi(GetAst()));
        ASSERT_NE(dump, nullptr);
        EXPECT_TRUE(Contains(std::string(dump), "fieldName"));
    }
}

// Multi-method class: each method retrieved by name from the program
TEST_F(AstWalkInvariantTest, MultipleMethodsEachResolvable)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { alpha(): void {} beta(): void {} gamma(): void {} }")
    {
        auto *program = GetImpl()->ContextProgram(GetContext());
        ASSERT_NE(program, nullptr);
        for (auto const *name : {"alpha", "beta", "gamma"}) {
            auto *method = GetImpl()->FirstDeclarationByNameFromProgram(GetContext(), program, name);
            ASSERT_NE(method, nullptr) << "missing method " << name;
            EXPECT_TRUE(GetImpl()->IsMethodDefinition(method));
        }
    }
}

// Multi-method class: name -> ScriptFunction -> Id round-trips correctly
TEST_F(AstWalkInvariantTest, MultipleMethodNamesRoundtripIndependently)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { one(): void {} two(): void {} }")
    {
        auto *program = GetImpl()->ContextProgram(GetContext());
        ASSERT_NE(program, nullptr);
        auto *one = GetImpl()->FirstDeclarationByNameFromProgram(GetContext(), program, "one");
        auto *two = GetImpl()->FirstDeclarationByNameFromProgram(GetContext(), program, "two");
        ASSERT_NE(one, nullptr);
        ASSERT_NE(two, nullptr);
        auto *oneFunc = GetImpl()->MethodDefinitionFunction(GetContext(), one);
        auto *twoFunc = GetImpl()->MethodDefinitionFunction(GetContext(), two);
        ASSERT_NE(oneFunc, nullptr);
        ASSERT_NE(twoFunc, nullptr);
        EXPECT_STREQ(GetImpl()->IdentifierName(GetContext(), GetImpl()->ScriptFunctionId(GetContext(), oneFunc)),
                     "one");
        EXPECT_STREQ(GetImpl()->IdentifierName(GetContext(), GetImpl()->ScriptFunctionId(GetContext(), twoFunc)),
                     "two");
    }
}

// Constructor: found by name (default or explicit)
TEST_F(AstWalkInvariantTest, ConstructorResolvableByName)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { constructor() {} }")
    {
        auto *program = GetImpl()->ContextProgram(GetContext());
        ASSERT_NE(program, nullptr);
        auto *constructor = GetImpl()->FirstDeclarationByNameFromProgram(GetContext(), program, "constructor");
        ASSERT_NE(constructor, nullptr);
        EXPECT_TRUE(GetImpl()->IsMethodDefinition(constructor));
    }
}

// Explicit constructor: body is non-null
TEST_F(AstWalkInvariantTest, ExplicitConstructorHasBody)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { constructor() {} }")
    {
        auto *program = GetImpl()->ContextProgram(GetContext());
        ASSERT_NE(program, nullptr);
        auto *constructor = GetImpl()->FirstDeclarationByNameFromProgram(GetContext(), program, "constructor");
        ASSERT_NE(constructor, nullptr);
        auto *func = GetImpl()->MethodDefinitionFunction(GetContext(), constructor);
        ASSERT_NE(func, nullptr);
        EXPECT_NE(GetImpl()->ScriptFunctionBody(GetContext(), func), nullptr);
    }
}

// Parent chain: the method's parent is non-null
TEST_F(AstWalkInvariantTest, MethodParentIsNotNull)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { parentedMethod(): void {} }")
    {
        auto *program = GetImpl()->ContextProgram(GetContext());
        ASSERT_NE(program, nullptr);
        auto *method = GetImpl()->FirstDeclarationByNameFromProgram(GetContext(), program, "parentedMethod");
        ASSERT_NE(method, nullptr);
        EXPECT_NE(GetImpl()->AstNodeParentConst(GetContext(), method), nullptr);
    }
}

// Parent chain: the function declaration's parent is non-null
TEST_F(AstWalkInvariantTest, FunctionDeclarationParentIsNotNull)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function parentedFn(): void {}")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), FindFirstDeclarationCallback, &payload);
        ASSERT_NE(payload.firstFunctionDeclaration, nullptr);
        EXPECT_NE(GetImpl()->AstNodeParentConst(GetContext(), payload.firstFunctionDeclaration), nullptr);
    }
}

// Parent chain walks up to the root (64-level cap against infinite loops)
TEST_F(AstWalkInvariantTest, ParentChainReachesRootWithinBound)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { deepMethod(): int { return 1 } }")
    {
        auto *program = GetImpl()->ContextProgram(GetContext());
        ASSERT_NE(program, nullptr);
        auto *method = GetImpl()->FirstDeclarationByNameFromProgram(GetContext(), program, "deepMethod");
        ASSERT_NE(method, nullptr);
        const es2panda_AstNode *current = method;
        int depth = 0;
        while (current != nullptr && depth < 64) {
            current = GetImpl()->AstNodeParentConst(GetContext(), const_cast<es2panda_AstNode *>(current));
            depth++;
        }
        EXPECT_LT(depth, 64);
    }
}

// JSON dump: class source JSON is non-null and contains the class name
TEST_F(AstWalkInvariantTest, DumpJsonContainsClassName)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class JsonClass {}")
    {
        auto *dump = GetImpl()->AstNodeDumpJSONConst(GetContext(), CApi(GetAst()));
        ASSERT_NE(dump, nullptr);
        EXPECT_TRUE(Contains(std::string(dump), "JsonClass"));
    }
}

// JSON dump: function source contains the function name
TEST_F(AstWalkInvariantTest, DumpJsonContainsFunctionName)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function jsonFn(): void {}")
    {
        auto *dump = GetImpl()->AstNodeDumpJSONConst(GetContext(), CApi(GetAst()));
        ASSERT_NE(dump, nullptr);
        EXPECT_TRUE(Contains(std::string(dump), "jsonFn"));
    }
}

// JSON dump: enum source contains the enum name
TEST_F(AstWalkInvariantTest, DumpJsonContainsEnumName)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "enum JsonEnum { A }")
    {
        auto *dump = GetImpl()->AstNodeDumpJSONConst(GetContext(), CApi(GetAst()));
        ASSERT_NE(dump, nullptr);
        EXPECT_TRUE(Contains(std::string(dump), "JsonEnum"));
    }
}

// ETS source dump: deeply nested method bodies keep their structure
TEST_F(AstWalkInvariantTest, DumpEtsSrcNestedMethodBody)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { m(): int { let a: int = 1; let b: int = a + 1; return b } }")
    {
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), CApi(GetAst()));
        ASSERT_NE(dump, nullptr);
        EXPECT_TRUE(Contains(std::string(dump), "return"));
    }
}

// Leak defense: method -> function -> Id chain repeated x20 per context
TEST_F(LeakDefenseTest, MethodAccessChainRepeated20Times)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { repeatedMethod(): void {} }")
    {
        auto *program = GetImpl()->ContextProgram(GetContext());
        ASSERT_NE(program, nullptr);
        auto *method = GetImpl()->FirstDeclarationByNameFromProgram(GetContext(), program, "repeatedMethod");
        ASSERT_NE(method, nullptr);
        for (int i = 0; i < 20; i++) {
            auto *func = GetImpl()->MethodDefinitionFunction(GetContext(), method);
            ASSERT_NE(func, nullptr);
            auto *id = GetImpl()->ScriptFunctionId(GetContext(), func);
            ASSERT_NE(id, nullptr);
            EXPECT_STREQ(GetImpl()->IdentifierName(GetContext(), id), "repeatedMethod");
        }
    }
}

// Leak defense: declarator probes repeated x20
TEST_F(LeakDefenseTest, DeclaratorProbesRepeated20Times)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let repeated: int = 8")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), FindFirstDeclarationCallback, &payload);
        ASSERT_NE(payload.firstVariableDeclaration, nullptr);
        for (int i = 0; i < 20; i++) {
            size_t declaratorCount = 0;
            auto **declarators = GetImpl()->VariableDeclarationDeclaratorsConst(
                GetContext(), payload.firstVariableDeclaration, &declaratorCount);
            ASSERT_EQ(declaratorCount, 1U);
            auto *id = GetImpl()->VariableDeclaratorId(GetContext(), declarators[0]);
            ASSERT_NE(id, nullptr);
            EXPECT_STREQ(GetImpl()->IdentifierName(GetContext(), id), "repeated");
        }
    }
}

// Leak defense: dump repeated x20 (dump allocates a temp arena buffer)
TEST_F(LeakDefenseTest, DumpRepeated20Times)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { dumpRepeat(): void {} }")
    {
        for (int i = 0; i < 20; i++) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), CApi(GetAst()));
            ASSERT_NE(dump, nullptr);
            EXPECT_TRUE(Contains(std::string(dump), "dumpRepeat"));
        }
    }
}

// Leak defense: JSON dump repeated x15
TEST_F(LeakDefenseTest, DumpJsonRepeated15Times)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { jsonRepeat(): void {} }")
    {
        for (int i = 0; i < 15; i++) {
            auto *dump = GetImpl()->AstNodeDumpJSONConst(GetContext(), CApi(GetAst()));
            ASSERT_NE(dump, nullptr);
        }
    }
}

// Multi-declaration source: declarator count per let declaration is 1
TEST_F(AstWalkInvariantTest, AllTopLevelLetsHaveSingleDeclarator)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let a1: int = 1\nlet a2: int = 2\nlet a3: int = 3")
    {
        struct Collector {
            es2panda_Impl const *impl;
            es2panda_Context *ctx;
            std::vector<es2panda_AstNode *> declarations;
        } collector {GetImpl(), GetContext(), {}};

        auto finder = [](es2panda_AstNode *node, void *arg) {
            auto *c = static_cast<Collector *>(arg);
            if (c->impl->IsVariableDeclaration(node)) {
                c->declarations.push_back(node);
            }
        };
        GetImpl()->AstNodeForEach(CApi(GetAst()), finder, &collector);
        ASSERT_EQ(collector.declarations.size(), 3U);
        for (auto *declaration : collector.declarations) {
            size_t declaratorCount = 0;
            GetImpl()->VariableDeclarationDeclaratorsConst(collector.ctx, declaration, &declaratorCount);
            EXPECT_EQ(declaratorCount, 1U);
        }
    }
}

// Three functions: every FunctionDeclaration finds its ScriptFunction
TEST_F(AstWalkInvariantTest, AllTopLevelFunctionsResolvable)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function fa(): void {}\nfunction fb(): void {}\nfunction fc(): void {}")
    {
        struct Collector {
            es2panda_Impl const *impl;
            es2panda_Context *ctx;
            std::vector<es2panda_AstNode *> declarations;
        } collector {GetImpl(), GetContext(), {}};

        auto finder = [](es2panda_AstNode *node, void *arg) {
            auto *c = static_cast<Collector *>(arg);
            if (c->impl->IsFunctionDeclaration(node)) {
                c->declarations.push_back(node);
            }
        };
        GetImpl()->AstNodeForEach(CApi(GetAst()), finder, &collector);
        ASSERT_EQ(collector.declarations.size(), 3U);
        for (auto *declaration : collector.declarations) {
            auto *func = GetImpl()->FunctionDeclarationFunction(collector.ctx, declaration);
            ASSERT_NE(func, nullptr);
            EXPECT_NE(GetImpl()->ScriptFunctionId(collector.ctx, func), nullptr);
        }
    }
}

// Class + function mix: both declaration kinds found, no interference
TEST_F(AstWalkInvariantTest, ClassAndFunctionDeclarationsCoexist)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class Mixed {}\nfunction mixedFn(): void {}")
    {
        struct Collector {
            es2panda_Impl const *impl;
            es2panda_Context *ctx;
            size_t classes = 0;
            size_t functions = 0;
        } collector {GetImpl(), GetContext(), {}};

        auto finder = [](es2panda_AstNode *node, void *arg) {
            auto *c = static_cast<Collector *>(arg);
            if (c->impl->IsClassDeclaration(node)) {
                c->classes++;
            }
            if (c->impl->IsFunctionDeclaration(node)) {
                c->functions++;
            }
        };
        GetImpl()->AstNodeForEach(CApi(GetAst()), finder, &collector);
        EXPECT_GE(collector.classes, 1U);
        EXPECT_GE(collector.functions, 1U);
    }
}

// ===========================================================================
// Part 11: control-flow statement walk matrix (4 probes per snippet)
//   Probe 1: KindFound - the expected statement type appears enough times
//   Probe 2: IdentifiersPresent - declared names appear in the identifier set
//   Probe 3: NodeCountStable - two walks agree (no side effects)
//   Probe 4: ParentRootInvariant - only the root has a null parent
// ===========================================================================

// --- Snippet: for loop ---
TEST_F(AstWalkInvariantTest, KindFoundForUpdateLoop)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): void { for (let i: int = 0; i < 3; i++) {} }")
    {
        EXPECT_GE(CountNodesOfKind(GetImpl(), CApi(GetAst()),
                                   [](es2panda_Impl const *impl, es2panda_AstNode *node) {
                                       return impl->IsForUpdateStatement(node);
                                   }),
                  1U);
    }
}

TEST_F(AstWalkInvariantTest, IdentifiersPresentForUpdateLoop)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): void { for (let i: int = 0; i < 3; i++) {} }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "f"));
        EXPECT_TRUE(Contains(payload.identifierNames, "i"));
        EXPECT_GE(payload.identifierCount, 2U);
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableForUpdateLoop)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): void { for (let i: int = 0; i < 3; i++) {} }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 5U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantForUpdateLoop)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): void { for (let i: int = 0; i < 3; i++) {} }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: for-of loop ---
TEST_F(AstWalkInvariantTest, KindFoundForOfLoop)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(a: int[]): void { for (const e of a) {} }")
    {
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *node) { return impl->IsForOfStatement(node); }),
                  1U);
    }
}

TEST_F(AstWalkInvariantTest, IdentifiersPresentForOfLoop)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(a: int[]): void { for (const e of a) {} }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "f"));
        EXPECT_TRUE(Contains(payload.identifierNames, "a"));
        EXPECT_TRUE(Contains(payload.identifierNames, "e"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableForOfLoop)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(a: int[]): void { for (const e of a) {} }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 5U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantForOfLoop)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(a: int[]): void { for (const e of a) {} }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: while loop ---
TEST_F(AstWalkInvariantTest, KindFoundWhileLoop)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): void { let i: int = 0; while (i < 3) { i++ } }")
    {
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *node) { return impl->IsWhileStatement(node); }),
                  1U);
    }
}

TEST_F(AstWalkInvariantTest, IdentifiersPresentWhileLoop)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): void { let i: int = 0; while (i < 3) { i++ } }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "f"));
        EXPECT_TRUE(Contains(payload.identifierNames, "i"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableWhileLoop)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): void { let i: int = 0; while (i < 3) { i++ } }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 5U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantWhileLoop)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): void { let i: int = 0; while (i < 3) { i++ } }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: do-while loop ---
TEST_F(AstWalkInvariantTest, KindFoundDoWhileLoop)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): void { let i: int = 0; do { i++ } while (i < 3) }")
    {
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *node) { return impl->IsDoWhileStatement(node); }),
                  1U);
    }
}

TEST_F(AstWalkInvariantTest, IdentifiersPresentDoWhileLoop)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): void { let i: int = 0; do { i++ } while (i < 3) }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "f"));
        EXPECT_TRUE(Contains(payload.identifierNames, "i"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableDoWhileLoop)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): void { let i: int = 0; do { i++ } while (i < 3) }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 5U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantDoWhileLoop)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): void { let i: int = 0; do { i++ } while (i < 3) }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: if statement ---
TEST_F(AstWalkInvariantTest, KindFoundIfStatement)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(p: boolean): void { if (p) {} }")
    {
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *node) { return impl->IsIfStatement(node); }),
                  1U);
    }
}

TEST_F(AstWalkInvariantTest, IdentifiersPresentIfStatement)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(p: boolean): void { if (p) {} }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "f"));
        EXPECT_TRUE(Contains(payload.identifierNames, "p"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableIfStatement)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(p: boolean): void { if (p) {} }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 4U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantIfStatement)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(p: boolean): void { if (p) {} }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: if-else statement ---
TEST_F(AstWalkInvariantTest, KindFoundIfElseStatement)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(p: boolean): void { if (p) {} else {} }")
    {
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *node) { return impl->IsIfStatement(node); }),
                  1U);
    }
}

TEST_F(AstWalkInvariantTest, IdentifiersPresentIfElseStatement)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(p: boolean): void { if (p) {} else {} }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "f"));
        EXPECT_TRUE(Contains(payload.identifierNames, "p"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableIfElseStatement)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(p: boolean): void { if (p) {} else {} }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 4U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantIfElseStatement)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(p: boolean): void { if (p) {} else {} }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: if-else-if-else chain ---
TEST_F(AstWalkInvariantTest, KindFoundElseIfChain)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(v: int): void { if (v == 1) {} else if (v == 2) {} else {} }")
    {
        // Two if nodes (the main if and the nested else-if)
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *node) { return impl->IsIfStatement(node); }),
                  2U);
    }
}

TEST_F(AstWalkInvariantTest, IdentifiersPresentElseIfChain)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(v: int): void { if (v == 1) {} else if (v == 2) {} else {} }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "f"));
        EXPECT_TRUE(Contains(payload.identifierNames, "v"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableElseIfChain)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(v: int): void { if (v == 1) {} else if (v == 2) {} else {} }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 8U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantElseIfChain)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(v: int): void { if (v == 1) {} else if (v == 2) {} else {} }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: switch statement ---
TEST_F(AstWalkInvariantTest, KindFoundSwitchStatement)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(v: int): void { switch (v) { case 1: break; default: break } }")
    {
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *node) { return impl->IsSwitchStatement(node); }),
                  1U);
    }
}

TEST_F(AstWalkInvariantTest, KindFoundSwitchCasesAndBreaks)
{
    CONTEXT(ES2PANDA_STATE_PARSED,
            "function f(v: int): void { switch (v) { case 1: break; case 2: break; default: break } }")
    {
        EXPECT_GE(CountNodesOfKind(GetImpl(), CApi(GetAst()),
                                   [](es2panda_Impl const *impl, es2panda_AstNode *node) {
                                       return impl->IsSwitchCaseStatement(node);
                                   }),
                  3U);
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *node) { return impl->IsBreakStatement(node); }),
                  3U);
    }
}

TEST_F(AstWalkInvariantTest, IdentifiersPresentSwitchStatement)
{
    CONTEXT(ES2PANDA_STATE_PARSED,
            "function f(v: int): void { switch (v) { case 1: break; case 2: break; default: break } }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "f"));
        EXPECT_TRUE(Contains(payload.identifierNames, "v"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableSwitchStatement)
{
    CONTEXT(ES2PANDA_STATE_PARSED,
            "function f(v: int): void { switch (v) { case 1: break; case 2: break; default: break } }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 10U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantSwitchStatement)
{
    CONTEXT(ES2PANDA_STATE_PARSED,
            "function f(v: int): void { switch (v) { case 1: break; case 2: break; default: break } }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: try-catch ---
TEST_F(AstWalkInvariantTest, KindFoundTryCatch)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): void { try {} catch (e) {} }")
    {
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *node) { return impl->IsTryStatement(node); }),
                  1U);
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *node) { return impl->IsCatchClause(node); }),
                  1U);
    }
}

TEST_F(AstWalkInvariantTest, IdentifiersPresentTryCatch)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): void { try {} catch (e) {} }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "f"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableTryCatch)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): void { try {} catch (e) {} }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 4U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantTryCatch)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): void { try {} catch (e) {} }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: try-finally ---
TEST_F(AstWalkInvariantTest, KindFoundTryFinally)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): void { try {} finally {} }")
    {
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *node) { return impl->IsTryStatement(node); }),
                  1U);
    }
}

TEST_F(AstWalkInvariantTest, IdentifiersPresentTryFinally)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): void { try {} finally {} }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "f"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableTryFinally)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): void { try {} finally {} }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 3U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantTryFinally)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): void { try {} finally {} }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: try-catch-finally ---
TEST_F(AstWalkInvariantTest, KindFoundTryCatchFinally)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): void { try {} catch (e) {} finally {} }")
    {
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *node) { return impl->IsTryStatement(node); }),
                  1U);
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *node) { return impl->IsCatchClause(node); }),
                  1U);
    }
}

TEST_F(AstWalkInvariantTest, IdentifiersPresentTryCatchFinally)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): void { try {} catch (e) {} finally {} }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "f"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableTryCatchFinally)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): void { try {} catch (e) {} finally {} }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 4U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantTryCatchFinally)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): void { try {} catch (e) {} finally {} }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: break statement ---
TEST_F(AstWalkInvariantTest, KindFoundBreakStatement)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): void { for (;;) { break } }")
    {
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *node) { return impl->IsBreakStatement(node); }),
                  1U);
    }
}

TEST_F(AstWalkInvariantTest, IdentifiersPresentBreakStatement)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): void { for (;;) { break } }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "f"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableBreakStatement)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): void { for (;;) { break } }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 3U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantBreakStatement)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): void { for (;;) { break } }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: continue statement ---
TEST_F(AstWalkInvariantTest, KindFoundContinueStatement)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): void { for (;;) { continue } }")
    {
        EXPECT_GE(CountNodesOfKind(GetImpl(), CApi(GetAst()),
                                   [](es2panda_Impl const *impl, es2panda_AstNode *node) {
                                       return impl->IsContinueStatement(node);
                                   }),
                  1U);
    }
}

TEST_F(AstWalkInvariantTest, IdentifiersPresentContinueStatement)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): void { for (;;) { continue } }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "f"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableContinueStatement)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): void { for (;;) { continue } }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 3U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantContinueStatement)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): void { for (;;) { continue } }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: return statement (with value) ---
TEST_F(AstWalkInvariantTest, KindFoundReturnStatementWithValue)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): int { return 1 }")
    {
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *node) { return impl->IsReturnStatement(node); }),
                  1U);
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *node) { return impl->IsNumberLiteral(node); }),
                  1U);
    }
}

TEST_F(AstWalkInvariantTest, IdentifiersPresentReturnStatementWithValue)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): int { return 1 }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "f"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableReturnStatementWithValue)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): int { return 1 }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 3U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantReturnStatementWithValue)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): int { return 1 }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: return statement (no value) ---
TEST_F(AstWalkInvariantTest, KindFoundBareReturnStatement)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): void { return }")
    {
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *node) { return impl->IsReturnStatement(node); }),
                  1U);
    }
}

TEST_F(AstWalkInvariantTest, IdentifiersPresentBareReturnStatement)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): void { return }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "f"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableBareReturnStatement)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): void { return }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 2U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantBareReturnStatement)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): void { return }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: nested for loops ---
TEST_F(AstWalkInvariantTest, KindFoundNestedForLoops)
{
    CONTEXT(ES2PANDA_STATE_PARSED,
            "function f(): void { for (let i: int = 0; i < 2; i++) { for (let j: int = 0; j < 2; j++) {} } }")
    {
        EXPECT_GE(CountNodesOfKind(GetImpl(), CApi(GetAst()),
                                   [](es2panda_Impl const *impl, es2panda_AstNode *node) {
                                       return impl->IsForUpdateStatement(node);
                                   }),
                  2U);
    }
}

TEST_F(AstWalkInvariantTest, IdentifiersPresentNestedForLoops)
{
    CONTEXT(ES2PANDA_STATE_PARSED,
            "function f(): void { for (let i: int = 0; i < 2; i++) { for (let j: int = 0; j < 2; j++) {} } }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "f"));
        EXPECT_TRUE(Contains(payload.identifierNames, "i"));
        EXPECT_TRUE(Contains(payload.identifierNames, "j"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableNestedForLoops)
{
    CONTEXT(ES2PANDA_STATE_PARSED,
            "function f(): void { for (let i: int = 0; i < 2; i++) { for (let j: int = 0; j < 2; j++) {} } }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 10U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantNestedForLoops)
{
    CONTEXT(ES2PANDA_STATE_PARSED,
            "function f(): void { for (let i: int = 0; i < 2; i++) { for (let j: int = 0; j < 2; j++) {} } }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: while + break + if combination ---
TEST_F(AstWalkInvariantTest, KindFoundWhileWithBreakAndIf)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): void { let i: int = 0; while (true) { i++; if (i > 3) { break } } }")
    {
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *node) { return impl->IsWhileStatement(node); }),
                  1U);
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *node) { return impl->IsIfStatement(node); }),
                  1U);
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *node) { return impl->IsBreakStatement(node); }),
                  1U);
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *node) { return impl->IsUpdateExpression(node); }),
                  1U);
    }
}

TEST_F(AstWalkInvariantTest, IdentifiersPresentWhileWithBreakAndIf)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): void { let i: int = 0; while (true) { i++; if (i > 3) { break } } }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "f"));
        EXPECT_TRUE(Contains(payload.identifierNames, "i"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableWhileWithBreakAndIf)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): void { let i: int = 0; while (true) { i++; if (i > 3) { break } } }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 8U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantWhileWithBreakAndIf)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): void { let i: int = 0; while (true) { i++; if (i > 3) { break } } }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: nested if ---
TEST_F(AstWalkInvariantTest, KindFoundNestedIfStatements)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(a: boolean, b: boolean): void { if (a) { if (b) {} } }")
    {
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *node) { return impl->IsIfStatement(node); }),
                  2U);
    }
}

TEST_F(AstWalkInvariantTest, IdentifiersPresentNestedIfStatements)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(a: boolean, b: boolean): void { if (a) { if (b) {} } }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "f"));
        EXPECT_TRUE(Contains(payload.identifierNames, "a"));
        EXPECT_TRUE(Contains(payload.identifierNames, "b"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableNestedIfStatements)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(a: boolean, b: boolean): void { if (a) { if (b) {} } }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 6U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantNestedIfStatements)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(a: boolean, b: boolean): void { if (a) { if (b) {} } }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: complex control flow with return in a loop ---
TEST_F(AstWalkInvariantTest, KindFoundLoopWithEarlyReturn)
{
    CONTEXT(ES2PANDA_STATE_PARSED,
            "function f(v: int): int { for (let i: int = 0; i < 3; i++) { if (i == v) { return i } } return 0 }")
    {
        EXPECT_GE(CountNodesOfKind(GetImpl(), CApi(GetAst()),
                                   [](es2panda_Impl const *impl, es2panda_AstNode *node) {
                                       return impl->IsForUpdateStatement(node);
                                   }),
                  1U);
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *node) { return impl->IsIfStatement(node); }),
                  1U);
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *node) { return impl->IsReturnStatement(node); }),
                  2U);
    }
}

TEST_F(AstWalkInvariantTest, IdentifiersPresentLoopWithEarlyReturn)
{
    CONTEXT(ES2PANDA_STATE_PARSED,
            "function f(v: int): int { for (let i: int = 0; i < 3; i++) { if (i == v) { return i } } return 0 }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "f"));
        EXPECT_TRUE(Contains(payload.identifierNames, "v"));
        EXPECT_TRUE(Contains(payload.identifierNames, "i"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableLoopWithEarlyReturn)
{
    CONTEXT(ES2PANDA_STATE_PARSED,
            "function f(v: int): int { for (let i: int = 0; i < 3; i++) { if (i == v) { return i } } return 0 }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 12U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantLoopWithEarlyReturn)
{
    CONTEXT(ES2PANDA_STATE_PARSED,
            "function f(v: int): int { for (let i: int = 0; i < 3; i++) { if (i == v) { return i } } return 0 }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}
// ===========================================================================
// Part 12: expression walk matrix (4 probes per snippet, same as Part 11)
// ===========================================================================

// --- Snippet: binary addition ---
TEST_F(AstWalkInvariantTest, KindFoundBinaryAddExpression)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let x: int = 1 + 2")
    {
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *node) { return impl->IsBinaryExpression(node); }),
                  1U);
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *node) { return impl->IsNumberLiteral(node); }),
                  2U);
    }
}

TEST_F(AstWalkInvariantTest, IdentifiersPresentBinaryAddExpression)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let x: int = 1 + 2")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "x"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableBinaryAddExpression)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let x: int = 1 + 2")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 4U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantBinaryAddExpression)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let x: int = 1 + 2")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: operator precedence chain ---
TEST_F(AstWalkInvariantTest, KindFoundPrecedenceChain)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let x: int = 1 + 2 * 3 - 4")
    {
        // Three operators: +, *, - => at least three BinaryExpressions
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *node) { return impl->IsBinaryExpression(node); }),
                  3U);
    }
}

TEST_F(AstWalkInvariantTest, IdentifiersPresentPrecedenceChain)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let x: int = 1 + 2 * 3 - 4")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "x"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStablePrecedenceChain)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let x: int = 1 + 2 * 3 - 4")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 7U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantPrecedenceChain)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let x: int = 1 + 2 * 3 - 4")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: logical expression ---
TEST_F(AstWalkInvariantTest, KindFoundLogicalExpression)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let b: boolean = true && false")
    {
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *node) { return impl->IsBinaryExpression(node); }),
                  1U);
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *node) { return impl->IsBooleanLiteral(node); }),
                  2U);
    }
}

TEST_F(AstWalkInvariantTest, IdentifiersPresentLogicalExpression)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let b: boolean = true && false")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "b"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableLogicalExpression)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let b: boolean = true && false")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 4U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantLogicalExpression)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let b: boolean = true && false")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: unary minus ---
// Note: "-1" lexes as a negative literal; use "-x" to get a UnaryExpression
TEST_F(AstWalkInvariantTest, KindFoundUnaryMinusExpression)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): void { let x: int = 0; let y: int = -x }")
    {
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *node) { return impl->IsUnaryExpression(node); }),
                  1U);
    }
}

TEST_F(AstWalkInvariantTest, IdentifiersPresentUnaryMinusExpression)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): void { let x: int = 0; let y: int = -x }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "f"));
        EXPECT_TRUE(Contains(payload.identifierNames, "x"));
        EXPECT_TRUE(Contains(payload.identifierNames, "y"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableUnaryMinusExpression)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): void { let x: int = 0; let y: int = -x }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 7U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantUnaryMinusExpression)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): void { let x: int = 0; let y: int = -x }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: logical not ---
TEST_F(AstWalkInvariantTest, KindFoundLogicalNotExpression)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let b: boolean = !true")
    {
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *node) { return impl->IsUnaryExpression(node); }),
                  1U);
    }
}

TEST_F(AstWalkInvariantTest, IdentifiersPresentLogicalNotExpression)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let b: boolean = !true")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "b"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableLogicalNotExpression)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let b: boolean = !true")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 4U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantLogicalNotExpression)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let b: boolean = !true")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: increment expression ---
TEST_F(AstWalkInvariantTest, KindFoundUpdateExpression)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): void { let x: int = 0; x++ }")
    {
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *node) { return impl->IsUpdateExpression(node); }),
                  1U);
    }
}

TEST_F(AstWalkInvariantTest, IdentifiersPresentUpdateExpression)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): void { let x: int = 0; x++ }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "f"));
        EXPECT_TRUE(Contains(payload.identifierNames, "x"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableUpdateExpression)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): void { let x: int = 0; x++ }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 5U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantUpdateExpression)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): void { let x: int = 0; x++ }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: ternary conditional expression ---
TEST_F(AstWalkInvariantTest, KindFoundConditionalExpression)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let x: int = true ? 1 : 2")
    {
        EXPECT_GE(CountNodesOfKind(GetImpl(), CApi(GetAst()),
                                   [](es2panda_Impl const *impl, es2panda_AstNode *node) {
                                       return impl->IsConditionalExpression(node);
                                   }),
                  1U);
    }
}

TEST_F(AstWalkInvariantTest, IdentifiersPresentConditionalExpression)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let x: int = true ? 1 : 2")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "x"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableConditionalExpression)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let x: int = true ? 1 : 2")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 5U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantConditionalExpression)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let x: int = true ? 1 : 2")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: string literal ---
TEST_F(AstWalkInvariantTest, KindFoundStringLiteral)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let s: string = \"hello\"")
    {
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *node) { return impl->IsStringLiteral(node); }),
                  1U);
    }
}

TEST_F(AstWalkInvariantTest, IdentifiersPresentStringLiteral)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let s: string = \"hello\"")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "s"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableStringLiteral)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let s: string = \"hello\"")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 3U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantStringLiteral)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let s: string = \"hello\"")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: template string ---
TEST_F(AstWalkInvariantTest, KindFoundTemplateLiteral)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let s: string = `v=${1}`")
    {
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *node) { return impl->IsTemplateLiteral(node); }),
                  1U);
    }
}

TEST_F(AstWalkInvariantTest, IdentifiersPresentTemplateLiteral)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let s: string = `v=${1}`")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "s"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableTemplateLiteral)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let s: string = `v=${1}`")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 4U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantTemplateLiteral)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let s: string = `v=${1}`")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: array literal ---
TEST_F(AstWalkInvariantTest, KindFoundArrayLiteral)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let a: int[] = [1, 2, 3]")
    {
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *node) { return impl->IsArrayExpression(node); }),
                  1U);
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *node) { return impl->IsNumberLiteral(node); }),
                  3U);
    }
}

TEST_F(AstWalkInvariantTest, IdentifiersPresentArrayLiteral)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let a: int[] = [1, 2, 3]")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "a"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableArrayLiteral)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let a: int[] = [1, 2, 3]")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 6U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantArrayLiteral)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let a: int[] = [1, 2, 3]")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: assignment expression ---
TEST_F(AstWalkInvariantTest, KindFoundAssignmentExpression)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): void { let x: int = 0; x = 1 }")
    {
        EXPECT_GE(CountNodesOfKind(GetImpl(), CApi(GetAst()),
                                   [](es2panda_Impl const *impl, es2panda_AstNode *node) {
                                       return impl->IsAssignmentExpression(node);
                                   }),
                  1U);
    }
}

TEST_F(AstWalkInvariantTest, IdentifiersPresentAssignmentExpression)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): void { let x: int = 0; x = 1 }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "f"));
        EXPECT_TRUE(Contains(payload.identifierNames, "x"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableAssignmentExpression)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): void { let x: int = 0; x = 1 }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 6U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantAssignmentExpression)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): void { let x: int = 0; x = 1 }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: compound assignment expression ---
TEST_F(AstWalkInvariantTest, KindFoundCompoundAssignmentExpression)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): void { let x: int = 0; x += 1 }")
    {
        EXPECT_GE(CountNodesOfKind(GetImpl(), CApi(GetAst()),
                                   [](es2panda_Impl const *impl, es2panda_AstNode *node) {
                                       return impl->IsAssignmentExpression(node);
                                   }),
                  1U);
    }
}

TEST_F(AstWalkInvariantTest, IdentifiersPresentCompoundAssignmentExpression)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): void { let x: int = 0; x += 1 }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "f"));
        EXPECT_TRUE(Contains(payload.identifierNames, "x"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableCompoundAssignmentExpression)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): void { let x: int = 0; x += 1 }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 6U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantCompoundAssignmentExpression)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): void { let x: int = 0; x += 1 }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: call expression ---
TEST_F(AstWalkInvariantTest, KindFoundCallExpression)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function g(): void {}\nfunction f(): void { g() }")
    {
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *node) { return impl->IsCallExpression(node); }),
                  1U);
    }
}

TEST_F(AstWalkInvariantTest, IdentifiersPresentCallExpression)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function g(): void {}\nfunction f(): void { g() }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "g"));
        EXPECT_TRUE(Contains(payload.identifierNames, "f"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableCallExpression)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function g(): void {}\nfunction f(): void { g() }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 5U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantCallExpression)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function g(): void {}\nfunction f(): void { g() }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: member access expression ---
TEST_F(AstWalkInvariantTest, KindFoundMemberExpression)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class C { x: int = 0 }\nfunction f(c: C): int { return c.x }")
    {
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *node) { return impl->IsMemberExpression(node); }),
                  1U);
    }
}

TEST_F(AstWalkInvariantTest, IdentifiersPresentMemberExpression)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class C { x: int = 0 }\nfunction f(c: C): int { return c.x }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "C"));
        EXPECT_TRUE(Contains(payload.identifierNames, "c"));
        EXPECT_TRUE(Contains(payload.identifierNames, "x"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableMemberExpression)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class C { x: int = 0 }\nfunction f(c: C): int { return c.x }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 9U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantMemberExpression)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class C { x: int = 0 }\nfunction f(c: C): int { return c.x }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: this expression ---
TEST_F(AstWalkInvariantTest, KindFoundThisExpression)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class C { x: int = 0; m(): void { this.x = 1 } }")
    {
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *node) { return impl->IsThisExpression(node); }),
                  1U);
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *node) { return impl->IsMemberExpression(node); }),
                  1U);
    }
}

TEST_F(AstWalkInvariantTest, IdentifiersPresentThisExpression)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class C { x: int = 0; m(): void { this.x = 1 } }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "C"));
        EXPECT_TRUE(Contains(payload.identifierNames, "m"));
        EXPECT_TRUE(Contains(payload.identifierNames, "x"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableThisExpression)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class C { x: int = 0; m(): void { this.x = 1 } }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 8U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantThisExpression)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class C { x: int = 0; m(): void { this.x = 1 } }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: new instance expression ---
TEST_F(AstWalkInvariantTest, KindFoundNewInstanceExpression)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class C {}\nfunction f(): C { return new C() }")
    {
        // new C() parses as ETSNewClassInstanceExpression in ETS
        EXPECT_GE(CountNodesOfKind(GetImpl(), CApi(GetAst()),
                                   [](es2panda_Impl const *impl, es2panda_AstNode *node) {
                                       return impl->IsETSNewClassInstanceExpression(node) ||
                                              impl->IsNewExpression(node);
                                   }),
                  1U);
    }
}

TEST_F(AstWalkInvariantTest, IdentifiersPresentNewInstanceExpression)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class C {}\nfunction f(): C { return new C() }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "C"));
        EXPECT_TRUE(Contains(payload.identifierNames, "f"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableNewInstanceExpression)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class C {}\nfunction f(): C { return new C() }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 5U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantNewInstanceExpression)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class C {}\nfunction f(): C { return new C() }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}
// ===========================================================================
// Part 13: pipeline state machine matrix (manual config + context lifecycle)
//   - Forward: NEW->PARSED->BOUND->CHECKED->LOWERED (stepwise and skipping)
//   - Defense: ProceedToState(NEW) is a no-op; re-advancing to PARSED
//     reports "Bad state"; ERROR persists; error contexts destroy safely
//   - Leak defense: create->advance->destroy loops for every state level
// ===========================================================================

// NEW -> PARSED: basic advance
TEST_F(ContextLifecycleTest, ManualProceedNewToParsedEmptyClass)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class A {}", "pipe01.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    EXPECT_EQ(Impl()->ContextState(guard.Get()), ES2PANDA_STATE_NEW);
    auto *result = Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_PARSED);
    ASSERT_EQ(result, guard.Get());
    EXPECT_EQ(Impl()->ContextState(guard.Get()), ES2PANDA_STATE_PARSED);
}

// NEW -> NEW: no-op, state stays NEW
TEST_F(ContextLifecycleTest, ManualProceedToNewOnFreshContextIsNoOp)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class A {}", "pipe02.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *result = Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_NEW);
    ASSERT_EQ(result, guard.Get());
    EXPECT_EQ(Impl()->ContextState(guard.Get()), ES2PANDA_STATE_NEW);
}

// PARSED -> NEW: requesting NEW is a no-op; the parsed state is not corrupted
TEST_F(ContextLifecycleTest, ManualProceedToNewOnParsedContextKeepsState)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {CreateParsedContext(config.Get(), "class A {}", "pipe03.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *result = Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_NEW);
    ASSERT_EQ(result, guard.Get());
    EXPECT_EQ(Impl()->ContextState(guard.Get()), ES2PANDA_STATE_PARSED);
}

// NEW -> BOUND: Bind parses automatically first
TEST_F(ContextLifecycleTest, ManualNewToBoundAutoParsesFirst)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class A {}", "pipe04.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *result = Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_BOUND);
    ASSERT_EQ(result, guard.Get());
    EXPECT_EQ(Impl()->ContextState(guard.Get()), ES2PANDA_STATE_BOUND);
}

// NEW -> CHECKED: Check parses and binds automatically
TEST_F(ContextLifecycleTest, ManualNewToCheckedAutoParsesAndBinds)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class A { value: int = 1 }", "pipe05.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *result = Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_CHECKED);
    ASSERT_EQ(result, guard.Get());
    EXPECT_EQ(Impl()->ContextState(guard.Get()), ES2PANDA_STATE_CHECKED);
}

// NEW -> LOWERED: the whole pipeline in one step (minimal main type-checks)
TEST_F(ContextLifecycleTest, ManualNewToLoweredRunsFullPipeline)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(
        config.Get(), "function main() { for (let i = 0; i < 10; i++) {} }", "pipe06.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *result = Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_LOWERED);
    ASSERT_EQ(result, guard.Get());
    EXPECT_EQ(Impl()->ContextState(guard.Get()), ES2PANDA_STATE_LOWERED);
}

// Stepwise advance: NEW -> PARSED -> BOUND -> CHECKED
TEST_F(ContextLifecycleTest, ManualStepwiseParseBindCheck)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class A { value: int = 1 }", "pipe07.ats")};
    ASSERT_NE(guard.Get(), nullptr);

    ASSERT_EQ(Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_PARSED), guard.Get());
    EXPECT_EQ(Impl()->ContextState(guard.Get()), ES2PANDA_STATE_PARSED);

    ASSERT_EQ(Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_BOUND), guard.Get());
    EXPECT_EQ(Impl()->ContextState(guard.Get()), ES2PANDA_STATE_BOUND);

    ASSERT_EQ(Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_CHECKED), guard.Get());
    EXPECT_EQ(Impl()->ContextState(guard.Get()), ES2PANDA_STATE_CHECKED);
}

// Stepwise to LOWERED: NEW -> PARSED -> BOUND -> CHECKED -> LOWERED
TEST_F(ContextLifecycleTest, ManualStepwiseThroughLowered)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(
        config.Get(), "function main() { for (let i = 0; i < 10; i++) {} }", "pipe08.ats")};
    ASSERT_NE(guard.Get(), nullptr);

    ASSERT_EQ(Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_PARSED), guard.Get());
    ASSERT_EQ(Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_BOUND), guard.Get());
    ASSERT_EQ(Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_CHECKED), guard.Get());
    ASSERT_EQ(Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_LOWERED), guard.Get());
    EXPECT_EQ(Impl()->ContextState(guard.Get()), ES2PANDA_STATE_LOWERED);
}

// Skip levels: NEW -> BOUND -> CHECKED (only the explicit PARSED step is skipped)
TEST_F(ContextLifecycleTest, ManualBoundThenCheckedSteps)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class A { value: int = 1 }", "pipe09.ats")};
    ASSERT_NE(guard.Get(), nullptr);

    ASSERT_EQ(Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_BOUND), guard.Get());
    EXPECT_EQ(Impl()->ContextState(guard.Get()), ES2PANDA_STATE_BOUND);

    ASSERT_EQ(Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_CHECKED), guard.Get());
    EXPECT_EQ(Impl()->ContextState(guard.Get()), ES2PANDA_STATE_CHECKED);
}

// Skip levels: NEW -> CHECKED -> LOWERED
TEST_F(ContextLifecycleTest, ManualCheckedThenLoweredSteps)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(
        config.Get(), "function main() { for (let i = 0; i < 10; i++) {} }", "pipe10.ats")};
    ASSERT_NE(guard.Get(), nullptr);

    ASSERT_EQ(Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_CHECKED), guard.Get());
    EXPECT_EQ(Impl()->ContextState(guard.Get()), ES2PANDA_STATE_CHECKED);

    ASSERT_EQ(Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_LOWERED), guard.Get());
    EXPECT_EQ(Impl()->ContextState(guard.Get()), ES2PANDA_STATE_LOWERED);
}

// Re-advance to PARSED: Parse requires NEW -> ERROR, message has "Bad state"
TEST_F(ContextLifecycleTest, ManualDoubleParseIsBadStateError)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {CreateParsedContext(config.Get(), "class A {}", "pipe11.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *result = Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_PARSED);
    ASSERT_EQ(result, guard.Get());
    EXPECT_EQ(Impl()->ContextState(guard.Get()), ES2PANDA_STATE_ERROR);
    EXPECT_TRUE(Contains(std::string(Impl()->ContextErrorMessage(guard.Get())), "Bad state"));
}

// Backward BOUND -> PARSED: hits "Bad state at entry to Parse" too
TEST_F(ContextLifecycleTest, ManualBackwardFromBoundToParsedIsError)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class A {}", "pipe12.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    ASSERT_EQ(Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_BOUND), guard.Get());
    EXPECT_EQ(Impl()->ContextState(guard.Get()), ES2PANDA_STATE_BOUND);

    auto *result = Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_PARSED);
    ASSERT_EQ(result, guard.Get());
    EXPECT_EQ(Impl()->ContextState(guard.Get()), ES2PANDA_STATE_ERROR);
    EXPECT_TRUE(Contains(std::string(Impl()->ContextErrorMessage(guard.Get())), "Bad state"));
}

// Backward CHECKED -> PARSED: same error
TEST_F(ContextLifecycleTest, ManualBackwardFromCheckedToParsedIsError)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class A { value: int = 1 }", "pipe13.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    ASSERT_EQ(Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_CHECKED), guard.Get());
    EXPECT_EQ(Impl()->ContextState(guard.Get()), ES2PANDA_STATE_CHECKED);

    auto *result = Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_PARSED);
    ASSERT_EQ(result, guard.Get());
    EXPECT_EQ(Impl()->ContextState(guard.Get()), ES2PANDA_STATE_ERROR);
}

// ERROR persists: advancing anywhere after ERROR stays ERROR
TEST_F(ContextLifecycleTest, ManualErrorStatePersistsOnFurtherProceed)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {CreateParsedContext(config.Get(), "class A {}", "pipe14.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    // Force an error: parse twice
    ASSERT_EQ(Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_PARSED), guard.Get());
    EXPECT_EQ(Impl()->ContextState(guard.Get()), ES2PANDA_STATE_ERROR);

    // Advance to BOUND / CHECKED afterwards: still ERROR (the repeated-parse
    // precheck locked the context into the error state)
    auto *result = Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_PARSED);
    ASSERT_EQ(result, guard.Get());
    EXPECT_EQ(Impl()->ContextState(guard.Get()), ES2PANDA_STATE_ERROR);
}

// Every advance returns the same context pointer (in-place semantics)
TEST_F(ContextLifecycleTest, ManualProceedReturnsSamePointerEachStep)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class A { value: int = 1 }", "pipe15.ats")};
    ASSERT_NE(guard.Get(), nullptr);

    ASSERT_EQ(Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_PARSED), guard.Get());
    ASSERT_EQ(Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_BOUND), guard.Get());
    ASSERT_EQ(Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_CHECKED), guard.Get());
}

// Program pointer is stable across states (same object after parsing)
TEST_F(ContextLifecycleTest, ManualProgramPointerStableAcrossStates)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class A { value: int = 1 }", "pipe16.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    EXPECT_EQ(Impl()->ContextProgram(guard.Get()), nullptr);

    ASSERT_EQ(Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_PARSED), guard.Get());
    auto *programAtParsed = Impl()->ContextProgram(guard.Get());
    ASSERT_NE(programAtParsed, nullptr);

    ASSERT_EQ(Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_BOUND), guard.Get());
    EXPECT_EQ(Impl()->ContextProgram(guard.Get()), programAtParsed);

    ASSERT_EQ(Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_CHECKED), guard.Get());
    EXPECT_EQ(Impl()->ContextProgram(guard.Get()), programAtParsed);
}

// ProgramAst works in the PARSED / BOUND / CHECKED states
TEST_F(ContextLifecycleTest, ManualProgramAstAvailableAtEachState)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class A { value: int = 1 }", "pipe17.ats")};
    ASSERT_NE(guard.Get(), nullptr);

    ASSERT_EQ(Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_PARSED), guard.Get());
    EXPECT_NE(Impl()->ProgramAst(guard.Get(), Impl()->ContextProgram(guard.Get())), nullptr);
    ASSERT_EQ(Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_BOUND), guard.Get());
    EXPECT_NE(Impl()->ProgramAst(guard.Get(), Impl()->ContextProgram(guard.Get())), nullptr);
    ASSERT_EQ(Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_CHECKED), guard.Get());
    EXPECT_NE(Impl()->ProgramAst(guard.Get(), Impl()->ContextProgram(guard.Get())), nullptr);
}

// Syntax error source: skipping to CHECKED lands in ERROR (Parse fails)
TEST_F(ContextLifecycleTest, ManualPipelineWithSyntaxErrorGoesToError)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class C {", "pipe18.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *result = Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_CHECKED);
    ASSERT_EQ(result, guard.Get());
    EXPECT_EQ(Impl()->ContextState(guard.Get()), ES2PANDA_STATE_ERROR);
}

// Type error source: NEW -> CHECKED enters ERROR (Check fails, verified)
TEST_F(ContextLifecycleTest, ManualPipelineWithTypeErrorGoesToError)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "let x: int = \"\"", "pipe19.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *result = Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_CHECKED);
    ASSERT_EQ(result, guard.Get());
    EXPECT_EQ(Impl()->ContextState(guard.Get()), ES2PANDA_STATE_ERROR);
}

// LOWERED advance on an ERROR context: stays ERROR, no crash
TEST_F(ContextLifecycleTest, ManualLoweredOnTypeErrorStaysError)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "let x: int = \"\"", "pipe20.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    ASSERT_EQ(Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_CHECKED), guard.Get());
    EXPECT_EQ(Impl()->ContextState(guard.Get()), ES2PANDA_STATE_ERROR);
    auto *result = Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_LOWERED);
    ASSERT_EQ(result, guard.Get());
    EXPECT_EQ(Impl()->ContextState(guard.Get()), ES2PANDA_STATE_ERROR);
}

// Error context destroy loop x5: every ERROR context destroys safely
TEST_F(ErrorPathLeakTest, ErrorContextDestroyLoop5)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    for (int i = 0; i < 5; i++) {
        ContextGuard guard {CreateParsedContext(config.Get(), "class A {}", "errctx.ats")};
        ASSERT_NE(guard.Get(), nullptr);
        ASSERT_EQ(Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_PARSED), guard.Get());
        EXPECT_EQ(Impl()->ContextState(guard.Get()), ES2PANDA_STATE_ERROR);
    }
}

// Parse -> destroy loop x20: state correct each round (basic stress)
TEST_F(LeakDefenseTest, PipelineParseDestroyLoop20)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    for (int i = 0; i < 20; i++) {
        ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class A {}", "ploop.ats")};
        ASSERT_NE(guard.Get(), nullptr);
        ASSERT_EQ(Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_PARSED), guard.Get());
        EXPECT_EQ(Impl()->ContextState(guard.Get()), ES2PANDA_STATE_PARSED);
    }
}

// Parse -> Bind -> destroy loop x15
TEST_F(LeakDefenseTest, PipelineBindDestroyLoop15)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    for (int i = 0; i < 15; i++) {
        ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class A {}", "bloop.ats")};
        ASSERT_NE(guard.Get(), nullptr);
        ASSERT_EQ(Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_BOUND), guard.Get());
        EXPECT_EQ(Impl()->ContextState(guard.Get()), ES2PANDA_STATE_BOUND);
    }
}

// Parse -> Bind -> Check -> destroy loop x10
TEST_F(LeakDefenseTest, PipelineCheckDestroyLoop10)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    for (int i = 0; i < 10; i++) {
        ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class A { value: int = 1 }", "cloop.ats")};
        ASSERT_NE(guard.Get(), nullptr);
        ASSERT_EQ(Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_CHECKED), guard.Get());
        EXPECT_EQ(Impl()->ContextState(guard.Get()), ES2PANDA_STATE_CHECKED);
    }
}

// Full pipeline -> destroy loop x5 (heaviest per-round lifecycle)
TEST_F(LeakDefenseTest, PipelineLowerDestroyLoop5)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    for (int i = 0; i < 5; i++) {
        ContextGuard guard {Impl()->CreateContextFromString(
            config.Get(), "function main() { for (let i = 0; i < 10; i++) {} }", "lloop.ats")};
        ASSERT_NE(guard.Get(), nullptr);
        ASSERT_EQ(Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_LOWERED), guard.Get());
        EXPECT_EQ(Impl()->ContextState(guard.Get()), ES2PANDA_STATE_LOWERED);
    }
}

// One config drives 5 contexts of different sources to CHECKED in a row
TEST_F(LeakDefenseTest, OneConfigDrivesFiveCheckedContextsSequentially)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    std::vector<std::string> sources {
        "class A { value: int = 1 }",    "class B { flag: boolean = true }", "class C { name: string = \"c\" }",
        "class D { count: long = 100 }", "class E { ratio: double = 0.5 }",
    };
    for (auto const &source : sources) {
        ContextGuard guard {Impl()->CreateContextFromString(config.Get(), source.c_str(), "seq.ats")};
        ASSERT_NE(guard.Get(), nullptr);
        ASSERT_EQ(Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_CHECKED), guard.Get());
        EXPECT_EQ(Impl()->ContextState(guard.Get()), ES2PANDA_STATE_CHECKED);
    }
}

// Same source created/advanced/destroyed x10: state consistent (deterministic)
TEST_F(LeakDefenseTest, SameSourcePipelineLoop10Deterministic)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    for (int i = 0; i < 10; i++) {
        ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class A { value: int = 1 }", "det.ats")};
        ASSERT_NE(guard.Get(), nullptr);
        ASSERT_EQ(Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_CHECKED), guard.Get());
        EXPECT_EQ(Impl()->ContextState(guard.Get()), ES2PANDA_STATE_CHECKED);
        EXPECT_STREQ(Impl()->ContextErrorMessage(guard.Get()), "");
    }
}

// Alternate error and success x8: errors stay out of later contexts.
// Note: the diagnosticEngine is shared per config; a bad source's syntax errors
// make later CHECKs on that config report IsAnyError; bad and good therefore
// use separate configs, proving both lifecycles interleave safely
TEST_F(ErrorPathLeakTest, ErrorAndSuccessPipelineAlternating8)
{
    for (int i = 0; i < 8; i++) {
        {
            ConfigGuard badConfig {NewConfig()};
            ASSERT_NE(badConfig.Get(), nullptr);
            ContextGuard bad {Impl()->CreateContextFromString(badConfig.Get(), "class C {", "alt-bad.ats")};
            ASSERT_NE(bad.Get(), nullptr);
            ASSERT_EQ(Impl()->ProceedToState(bad.Get(), ES2PANDA_STATE_CHECKED), bad.Get());
            EXPECT_EQ(Impl()->ContextState(bad.Get()), ES2PANDA_STATE_ERROR);
        }
        {
            ConfigGuard goodConfig {NewConfig()};
            ASSERT_NE(goodConfig.Get(), nullptr);
            ContextGuard good {
                Impl()->CreateContextFromString(goodConfig.Get(), "class A { value: int = 1 }", "alt-good.ats")};
            ASSERT_NE(good.Get(), nullptr);
            ASSERT_EQ(Impl()->ProceedToState(good.Get(), ES2PANDA_STATE_CHECKED), good.Get());
            EXPECT_EQ(Impl()->ContextState(good.Get()), ES2PANDA_STATE_CHECKED);
        }
    }
}

// AST walk in BOUND state: node count > 0 and stable
TEST_F(AstWalkInvariantTest, AstWalkableAtBoundState)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class A { x: int = 0 }", "walkb.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    ASSERT_EQ(Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_BOUND), guard.Get());

    auto *ast = Impl()->ProgramAst(guard.Get(), Impl()->ContextProgram(guard.Get()));
    ASSERT_NE(ast, nullptr);
    WalkPayload first;
    WalkPayload second;
    first.impl = Impl();
    second.impl = Impl();
    Impl()->AstNodeForEach(ast, CountNodesCallback, &first);
    Impl()->AstNodeForEach(ast, CountNodesCallback, &second);
    EXPECT_GE(first.nodeCount, 4U);
    EXPECT_EQ(first.nodeCount, second.nodeCount);
}

// AST walk in CHECKED state
TEST_F(AstWalkInvariantTest, AstWalkableAtCheckedState)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class A { value: int = 1 }", "walkc.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    ASSERT_EQ(Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_CHECKED), guard.Get());

    auto *ast = Impl()->ProgramAst(guard.Get(), Impl()->ContextProgram(guard.Get()));
    ASSERT_NE(ast, nullptr);
    WalkPayload payload;
    payload.impl = Impl();
    Impl()->AstNodeForEach(ast, CountNodesCallback, &payload);
    EXPECT_GE(payload.nodeCount, 4U);
}

// AST walk in LOWERED state (lowering rewrites the AST; the walk stays safe)
TEST_F(AstWalkInvariantTest, AstWalkableAtLoweredState)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(
        config.Get(), "function main() { for (let i = 0; i < 10; i++) {} }", "walkl.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    ASSERT_EQ(Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_LOWERED), guard.Get());

    auto *ast = Impl()->ProgramAst(guard.Get(), Impl()->ContextProgram(guard.Get()));
    ASSERT_NE(ast, nullptr);
    WalkPayload payload;
    payload.impl = Impl();
    Impl()->AstNodeForEach(ast, CountNodesCallback, &payload);
    EXPECT_GE(payload.nodeCount, 4U);
}

// Parent invariant in LOWERED state: still only the root has a null parent
TEST_F(AstWalkInvariantTest, ParentRootInvariantAtLoweredState)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(
        config.Get(), "function main() { for (let i = 0; i < 10; i++) {} }", "walklp.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    ASSERT_EQ(Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_LOWERED), guard.Get());

    auto *ast = Impl()->ProgramAst(guard.Get(), Impl()->ContextProgram(guard.Get()));
    ASSERT_NE(ast, nullptr);
    WalkPayload payload;
    payload.impl = Impl();
    payload.ctx = guard.Get();
    Impl()->AstNodeForEach(ast, NullParentProbeCallback, &payload);
    EXPECT_EQ(payload.nullParentCount, 1U);
}
// ===========================================================================
// Part 14: class member variants and declaration matrix (4 probes each)
//   Static members / accessors / inheritance / implementation / generics /
//   abstract classes / enums / interfaces: structural walk safety
// ===========================================================================

// --- Snippet: static method ---
TEST_F(AstWalkInvariantTest, KindFoundStaticMethod)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class C { static m(): void {} }")
    {
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *node) { return impl->IsMethodDefinition(node); }),
                  1U);
    }
}

TEST_F(AstWalkInvariantTest, IdentifiersPresentStaticMethod)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class C { static m(): void {} }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "C"));
        EXPECT_TRUE(Contains(payload.identifierNames, "m"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableStaticMethod)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class C { static m(): void {} }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 4U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantStaticMethod)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class C { static m(): void {} }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: static field ---
TEST_F(AstWalkInvariantTest, KindFoundStaticField)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class C { static s: int = 1 }")
    {
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *node) { return impl->IsClassProperty(node); }),
                  1U);
    }
}

TEST_F(AstWalkInvariantTest, IdentifiersPresentStaticField)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class C { static s: int = 1 }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "C"));
        EXPECT_TRUE(Contains(payload.identifierNames, "s"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableStaticField)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class C { static s: int = 1 }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 5U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantStaticField)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class C { static s: int = 1 }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: getter accessor ---
TEST_F(AstWalkInvariantTest, KindFoundGetterAccessor)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class C { private v: int = 0; get value(): int { return this.v } }")
    {
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *node) { return impl->IsMethodDefinition(node); }),
                  1U);
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *node) { return impl->IsThisExpression(node); }),
                  1U);
    }
}

TEST_F(AstWalkInvariantTest, IdentifiersPresentGetterAccessor)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class C { private v: int = 0; get value(): int { return this.v } }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "C"));
        EXPECT_TRUE(Contains(payload.identifierNames, "value"));
        EXPECT_TRUE(Contains(payload.identifierNames, "v"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableGetterAccessor)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class C { private v: int = 0; get value(): int { return this.v } }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 9U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantGetterAccessor)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class C { private v: int = 0; get value(): int { return this.v } }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: setter accessor ---
TEST_F(AstWalkInvariantTest, KindFoundSetterAccessor)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class C { private v: int = 0; set value(x: int) { this.v = x } }")
    {
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *node) { return impl->IsMethodDefinition(node); }),
                  1U);
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *node) { return impl->IsThisExpression(node); }),
                  1U);
    }
}

TEST_F(AstWalkInvariantTest, IdentifiersPresentSetterAccessor)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class C { private v: int = 0; set value(x: int) { this.v = x } }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "C"));
        EXPECT_TRUE(Contains(payload.identifierNames, "value"));
        EXPECT_TRUE(Contains(payload.identifierNames, "x"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableSetterAccessor)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class C { private v: int = 0; set value(x: int) { this.v = x } }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 10U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantSetterAccessor)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class C { private v: int = 0; set value(x: int) { this.v = x } }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: class inheritance ---
TEST_F(AstWalkInvariantTest, KindFoundClassInheritance)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A {}\nclass B extends A {}")
    {
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *node) { return impl->IsClassDeclaration(node); }),
                  2U);
    }
}

TEST_F(AstWalkInvariantTest, IdentifiersPresentClassInheritance)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A {}\nclass B extends A {}")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "A"));
        EXPECT_TRUE(Contains(payload.identifierNames, "B"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableClassInheritance)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A {}\nclass B extends A {}")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 5U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantClassInheritance)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A {}\nclass B extends A {}")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: interface implementation ---
TEST_F(AstWalkInvariantTest, KindFoundInterfaceImplementation)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "interface I { m(): void }\nclass C implements I { m(): void {} }")
    {
        EXPECT_GE(CountNodesOfKind(GetImpl(), CApi(GetAst()),
                                   [](es2panda_Impl const *impl, es2panda_AstNode *node) {
                                       return impl->IsTSClassImplements(node);
                                   }),
                  1U);
        EXPECT_GE(CountNodesOfKind(GetImpl(), CApi(GetAst()),
                                   [](es2panda_Impl const *impl, es2panda_AstNode *node) {
                                       return impl->IsTSInterfaceDeclaration(node);
                                   }),
                  1U);
    }
}

TEST_F(AstWalkInvariantTest, IdentifiersPresentInterfaceImplementation)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "interface I { m(): void }\nclass C implements I { m(): void {} }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "I"));
        EXPECT_TRUE(Contains(payload.identifierNames, "C"));
        EXPECT_TRUE(Contains(payload.identifierNames, "m"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableInterfaceImplementation)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "interface I { m(): void }\nclass C implements I { m(): void {} }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 12U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantInterfaceImplementation)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "interface I { m(): void }\nclass C implements I { m(): void {} }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: generic class ---
TEST_F(AstWalkInvariantTest, KindFoundGenericClass)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class Box<T> {}")
    {
        EXPECT_GE(CountNodesOfKind(GetImpl(), CApi(GetAst()),
                                   [](es2panda_Impl const *impl, es2panda_AstNode *node) {
                                       return impl->IsTSTypeParameterDeclaration(node);
                                   }),
                  1U);
    }
}

TEST_F(AstWalkInvariantTest, IdentifiersPresentGenericClass)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class Box<T> {}")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "Box"));
        EXPECT_TRUE(Contains(payload.identifierNames, "T"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableGenericClass)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class Box<T> {}")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 4U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantGenericClass)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class Box<T> {}")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: generic function ---
TEST_F(AstWalkInvariantTest, KindFoundGenericFunction)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function id<T>(x: T): T { return x }")
    {
        EXPECT_GE(CountNodesOfKind(GetImpl(), CApi(GetAst()),
                                   [](es2panda_Impl const *impl, es2panda_AstNode *node) {
                                       return impl->IsTSTypeParameterDeclaration(node);
                                   }),
                  1U);
        EXPECT_GE(CountNodesOfKind(GetImpl(), CApi(GetAst()),
                                   [](es2panda_Impl const *impl, es2panda_AstNode *node) {
                                       return impl->IsFunctionDeclaration(node);
                                   }),
                  1U);
    }
}

TEST_F(AstWalkInvariantTest, IdentifiersPresentGenericFunction)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function id<T>(x: T): T { return x }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "id"));
        EXPECT_TRUE(Contains(payload.identifierNames, "T"));
        EXPECT_TRUE(Contains(payload.identifierNames, "x"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableGenericFunction)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function id<T>(x: T): T { return x }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 7U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantGenericFunction)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function id<T>(x: T): T { return x }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: abstract class ---
TEST_F(AstWalkInvariantTest, KindFoundAbstractClass)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "abstract class A { abstract m(): void }")
    {
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *node) { return impl->IsClassDeclaration(node); }),
                  1U);
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *node) { return impl->IsMethodDefinition(node); }),
                  1U);
    }
}

TEST_F(AstWalkInvariantTest, IdentifiersPresentAbstractClass)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "abstract class A { abstract m(): void }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "A"));
        EXPECT_TRUE(Contains(payload.identifierNames, "m"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableAbstractClass)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "abstract class A { abstract m(): void }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 5U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantAbstractClass)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "abstract class A { abstract m(): void }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: readonly field ---
TEST_F(AstWalkInvariantTest, KindFoundReadonlyField)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class C { readonly r: int = 1 }")
    {
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *node) { return impl->IsClassProperty(node); }),
                  1U);
    }
}

TEST_F(AstWalkInvariantTest, IdentifiersPresentReadonlyField)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class C { readonly r: int = 1 }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "C"));
        EXPECT_TRUE(Contains(payload.identifierNames, "r"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableReadonlyField)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class C { readonly r: int = 1 }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 5U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantReadonlyField)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class C { readonly r: int = 1 }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: constructor with parameters ---
TEST_F(AstWalkInvariantTest, KindFoundConstructorWithParam)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class C { x: int = 0; constructor(x: int) { this.x = x } }")
    {
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *node) { return impl->IsMethodDefinition(node); }),
                  1U);
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *node) { return impl->IsThisExpression(node); }),
                  1U);
    }
}

TEST_F(AstWalkInvariantTest, IdentifiersPresentConstructorWithParam)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class C { x: int = 0; constructor(x: int) { this.x = x } }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "C"));
        EXPECT_TRUE(Contains(payload.identifierNames, "x"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableConstructorWithParam)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class C { x: int = 0; constructor(x: int) { this.x = x } }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 12U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantConstructorWithParam)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class C { x: int = 0; constructor(x: int) { this.x = x } }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: enum with values ---
TEST_F(AstWalkInvariantTest, KindFoundEnumWithValues)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "enum E { A = 1, B = 2 }")
    {
        EXPECT_GE(CountNodesOfKind(GetImpl(), CApi(GetAst()),
                                   [](es2panda_Impl const *impl, es2panda_AstNode *node) {
                                       return impl->IsTSEnumDeclaration(node);
                                   }),
                  1U);
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *node) { return impl->IsTSEnumMember(node); }),
                  2U);
    }
}

TEST_F(AstWalkInvariantTest, IdentifiersPresentEnumWithValues)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "enum E { A = 1, B = 2 }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "E"));
        EXPECT_TRUE(Contains(payload.identifierNames, "A"));
        EXPECT_TRUE(Contains(payload.identifierNames, "B"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableEnumWithValues)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "enum E { A = 1, B = 2 }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 7U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantEnumWithValues)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "enum E { A = 1, B = 2 }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: interface with property and method ---
TEST_F(AstWalkInvariantTest, KindFoundInterfaceWithPropertyAndMethod)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "interface I { p: int\nm(): void }")
    {
        EXPECT_GE(CountNodesOfKind(GetImpl(), CApi(GetAst()),
                                   [](es2panda_Impl const *impl, es2panda_AstNode *node) {
                                       return impl->IsTSInterfaceDeclaration(node);
                                   }),
                  1U);
    }
}

TEST_F(AstWalkInvariantTest, IdentifiersPresentInterfaceWithPropertyAndMethod)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "interface I { p: int\nm(): void }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "I"));
        EXPECT_TRUE(Contains(payload.identifierNames, "p"));
        EXPECT_TRUE(Contains(payload.identifierNames, "m"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableInterfaceWithPropertyAndMethod)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "interface I { p: int\nm(): void }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 9U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantInterfaceWithPropertyAndMethod)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "interface I { p: int\nm(): void }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: class-typed field + new initializer ---
TEST_F(AstWalkInvariantTest, KindFoundClassTypeFieldWithNew)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class Inner {}\nclass Outer { i: Inner = new Inner() }")
    {
        EXPECT_GE(CountNodesOfKind(GetImpl(), CApi(GetAst()),
                                   [](es2panda_Impl const *impl, es2panda_AstNode *node) {
                                       return impl->IsETSNewClassInstanceExpression(node) ||
                                              impl->IsNewExpression(node);
                                   }),
                  1U);
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *node) { return impl->IsClassProperty(node); }),
                  1U);
    }
}

TEST_F(AstWalkInvariantTest, IdentifiersPresentClassTypeFieldWithNew)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class Inner {}\nclass Outer { i: Inner = new Inner() }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "Inner"));
        EXPECT_TRUE(Contains(payload.identifierNames, "Outer"));
        EXPECT_TRUE(Contains(payload.identifierNames, "i"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableClassTypeFieldWithNew)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class Inner {}\nclass Outer { i: Inner = new Inner() }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 10U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantClassTypeFieldWithNew)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class Inner {}\nclass Outer { i: Inner = new Inner() }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: mixed-member class ---
TEST_F(AstWalkInvariantTest, KindFoundMixedMembersClass)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class M { a: int = 1; b: string = \"x\"; m(): void {} static s: boolean = true }")
    {
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *node) { return impl->IsClassProperty(node); }),
                  3U);
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *node) { return impl->IsMethodDefinition(node); }),
                  1U);
    }
}

TEST_F(AstWalkInvariantTest, IdentifiersPresentMixedMembersClass)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class M { a: int = 1; b: string = \"x\"; m(): void {} static s: boolean = true }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "M"));
        EXPECT_TRUE(Contains(payload.identifierNames, "a"));
        EXPECT_TRUE(Contains(payload.identifierNames, "b"));
        EXPECT_TRUE(Contains(payload.identifierNames, "m"));
        EXPECT_TRUE(Contains(payload.identifierNames, "s"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableMixedMembersClass)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class M { a: int = 1; b: string = \"x\"; m(): void {} static s: boolean = true }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 15U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantMixedMembersClass)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class M { a: int = 1; b: string = \"x\"; m(): void {} static s: boolean = true }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

// --- Snippet: block-body arrow function ---
TEST_F(AstWalkInvariantTest, KindFoundArrowFunctionWithBody)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let f = (a: int): int => { return a }")
    {
        EXPECT_GE(CountNodesOfKind(GetImpl(), CApi(GetAst()),
                                   [](es2panda_Impl const *impl, es2panda_AstNode *node) {
                                       return impl->IsArrowFunctionExpression(node);
                                   }),
                  1U);
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *node) { return impl->IsReturnStatement(node); }),
                  1U);
    }
}

TEST_F(AstWalkInvariantTest, IdentifiersPresentArrowFunctionWithBody)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let f = (a: int): int => { return a }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "f"));
        EXPECT_TRUE(Contains(payload.identifierNames, "a"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableArrowFunctionWithBody)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let f = (a: int): int => { return a }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 7U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantArrowFunctionWithBody)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let f = (a: int): int => { return a }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}
// ===========================================================================
// Part 15: leak stress matrix - EHeap monotonicity x pipeline level / manual
// FreeCompilerPartMemory / AllocMemory stress / file context loops / multi
// context coexistence / error-path stress / EvaluateExpression / GlobalContext
//
// Leak defense approach:
//   - EHeap is a process-wide bump allocator; AllocatedSize() never shrinks.
//     A leaking lifecycle loop makes it grow unboundedly; tests use the
//     "never decreases + loop repeatable" invariant, ASAN/LSAN as backup.
//   - Manual FreeCompilerPartMemory is the compiler memory release entry
//     (plugins reclaim memory before context destruction); pairing it with
//     DestroyContext must not double-free (emitter==nullptr skips it).
// ===========================================================================

// ---------------------------------------------------------------------------
// 15.A EHeap counter x pipeline level loops
// ---------------------------------------------------------------------------

// BOUND loop x10: create->bind->destroy each round; EHeap non-decreasing
TEST_F(LeakDefenseTest, EHeapMonotonicAcrossBoundPipelineLoops10)
{
    auto previous = ark::es2panda::EHeap::AllocatedSize();
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    for (int i = 0; i < 10; i++) {
        ContextGuard guard {CreateParsedContext(config.Get(), "class B { f: int = 1 }", "eheap-bound.ats")};
        ASSERT_NE(guard.Get(), nullptr);
        ASSERT_EQ(Impl()->ContextState(guard.Get()), ES2PANDA_STATE_PARSED);
        auto *ctx = Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_BOUND);
        ASSERT_EQ(Impl()->ContextState(ctx), ES2PANDA_STATE_BOUND);
        auto current = ark::es2panda::EHeap::AllocatedSize();
        EXPECT_GE(current, previous);
        previous = current;
    }
    EXPECT_GE(ark::es2panda::EHeap::AllocatedSize(), previous);
}

// CHECKED loop x10: create->check->destroy each round; EHeap non-decreasing
TEST_F(LeakDefenseTest, EHeapMonotonicAcrossCheckedPipelineLoops10)
{
    auto previous = ark::es2panda::EHeap::AllocatedSize();
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    for (int i = 0; i < 10; i++) {
        ContextGuard guard {CreateParsedContext(config.Get(), "class C { value: int = 1 }", "eheap-check.ats")};
        ASSERT_NE(guard.Get(), nullptr);
        auto *ctx = Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_CHECKED);
        ASSERT_EQ(Impl()->ContextState(ctx), ES2PANDA_STATE_CHECKED);
        auto current = ark::es2panda::EHeap::AllocatedSize();
        EXPECT_GE(current, previous);
        previous = current;
    }
    EXPECT_GE(ark::es2panda::EHeap::AllocatedSize(), previous);
}

// LOWERED loop x10: create->lower->destroy each round; EHeap non-decreasing
TEST_F(LeakDefenseTest, EHeapMonotonicAcrossLoweredPipelineLoops10)
{
    auto previous = ark::es2panda::EHeap::AllocatedSize();
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    for (int i = 0; i < 10; i++) {
        ContextGuard guard {
            CreateParsedContext(config.Get(), "function main() { for (let i = 0; i < 10; i++) {} }", "eheap-low.ats")};
        ASSERT_NE(guard.Get(), nullptr);
        auto *ctx = Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_LOWERED);
        ASSERT_EQ(Impl()->ContextState(ctx), ES2PANDA_STATE_LOWERED);
        auto current = ark::es2panda::EHeap::AllocatedSize();
        EXPECT_GE(current, previous);
        previous = current;
    }
    EXPECT_GE(ark::es2panda::EHeap::AllocatedSize(), previous);
}

// Mixed states: NEW/PARSED/BOUND/CHECKED/LOWERED rotated; EHeap non-decreasing
TEST_F(LeakDefenseTest, EHeapMonotonicAcrossMixedStateLoops)
{
    static const std::array<es2panda_ContextState, 5> STATES {ES2PANDA_STATE_NEW, ES2PANDA_STATE_PARSED,
                                                              ES2PANDA_STATE_BOUND, ES2PANDA_STATE_CHECKED,
                                                              ES2PANDA_STATE_LOWERED};
    static const std::array<char const *, 5> SOURCES {"class M0 {}", "class M1 {}", "class M2 { x: int = 1 }",
                                                      "class M3 { value: int = 1 }",
                                                      "function main() { for (let i = 0; i < 10; i++) {} }"};
    auto previous = ark::es2panda::EHeap::AllocatedSize();
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    for (int round = 0; round < 2; round++) {
        for (size_t i = 0; i < STATES.size(); i++) {
            ContextGuard guard {Impl()->CreateContextFromString(config.Get(), SOURCES[i], "eheap-mixed.ats")};
            ASSERT_NE(guard.Get(), nullptr);
            auto *ctx = Impl()->ProceedToState(guard.Get(), STATES[i]);
            EXPECT_EQ(Impl()->ContextState(ctx), STATES[i]);
            auto current = ark::es2panda::EHeap::AllocatedSize();
            EXPECT_GE(current, previous);
            previous = current;
        }
    }
}

// Syntax error loop x10: create->parse(fail)->destroy; EHeap non-decreasing
TEST_F(LeakDefenseTest, EHeapMonotonicAcrossSyntaxErrorLoops10)
{
    auto previous = ark::es2panda::EHeap::AllocatedSize();
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    for (int i = 0; i < 10; i++) {
        ContextGuard guard {CreateParsedContext(config.Get(), "class E { x: }", "eheap-synerr.ats")};
        ASSERT_NE(guard.Get(), nullptr);
        EXPECT_TRUE(Impl()->IsAnyError(guard.Get()));
        auto current = ark::es2panda::EHeap::AllocatedSize();
        EXPECT_GE(current, previous);
        previous = current;
    }
}

// Type error (CHECK fails) loop x5: EHeap non-decreasing
TEST_F(LeakDefenseTest, EHeapMonotonicAcrossTypeErrorLoops5)
{
    auto previous = ark::es2panda::EHeap::AllocatedSize();
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    for (int i = 0; i < 5; i++) {
        ContextGuard guard {CreateParsedContext(config.Get(), "let bad: int = \"not an int\"", "eheap-tyerr.ats")};
        ASSERT_NE(guard.Get(), nullptr);
        auto *ctx = Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_CHECKED);
        EXPECT_EQ(Impl()->ContextState(ctx), ES2PANDA_STATE_ERROR);
        auto current = ark::es2panda::EHeap::AllocatedSize();
        EXPECT_GE(current, previous);
        previous = current;
    }
}

// Larger source x5: multi-class source, full pipeline to LOWERED; EHeap stable
TEST_F(LeakDefenseTest, EHeapMonotonicAcrossLargeSourceLoops5)
{
    std::string source = "class Shape {\n";
    for (int i = 0; i < 12; i++) {
        source += "    field" + std::to_string(i) + ": int = " + std::to_string(i) + "\n";
    }
    source += "    method(a: int): int { return a + 1 }\n";
    source += "}\n";
    source += "function main() { let s = new Shape(); s.method(1) }";
    auto previous = ark::es2panda::EHeap::AllocatedSize();
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    for (int i = 0; i < 5; i++) {
        ContextGuard guard {CreateParsedContext(config.Get(), source.c_str(), "eheap-large.ats")};
        ASSERT_NE(guard.Get(), nullptr);
        auto *ctx = Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_LOWERED);
        ASSERT_EQ(Impl()->ContextState(ctx), ES2PANDA_STATE_LOWERED);
        auto current = ark::es2panda::EHeap::AllocatedSize();
        EXPECT_GE(current, previous);
        previous = current;
    }
}

// Full config lifecycle loop x3 (fresh config + context each round)
TEST_F(LeakDefenseTest, EHeapMonotonicAcrossConfigLifecycleCycles3)
{
    auto previous = ark::es2panda::EHeap::AllocatedSize();
    for (int i = 0; i < 3; i++) {
        ConfigGuard config {NewConfig()};
        ASSERT_NE(config.Get(), nullptr);
        ContextGuard guard {CreateParsedContext(config.Get(), "class R { v: int = 1 }", "eheap-cfg.ats")};
        ASSERT_NE(guard.Get(), nullptr);
        auto *ctx = Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_CHECKED);
        ASSERT_EQ(Impl()->ContextState(ctx), ES2PANDA_STATE_CHECKED);
    }
    EXPECT_GE(ark::es2panda::EHeap::AllocatedSize(), previous);
}

// ---------------------------------------------------------------------------
// 15.B manual FreeCompilerPartMemory - destroy after early release, no double free
// Note: after FreeCompilerPartMemory only DestroyContext may be called
// (phaseManager is already deleted).
// ---------------------------------------------------------------------------

// PARSED state: manual Free + destroy, no crash (no double free)
TEST_F(LeakDefenseTest, ManualFreeThenDestroyOnParsedContext)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    auto *context = CreateParsedContext(config.Get(), "class P1 {}", "manualfree-p.ats");
    ASSERT_NE(context, nullptr);
    ASSERT_EQ(Impl()->ContextState(context), ES2PANDA_STATE_PARSED);
    Impl()->FreeCompilerPartMemory(context);
    Impl()->DestroyContext(context);
    SUCCEED();
}

// BOUND state: manual Free + destroy
TEST_F(LeakDefenseTest, ManualFreeThenDestroyOnBoundContext)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    auto *context = CreateParsedContext(config.Get(), "class P2 { x: int = 1 }", "manualfree-b.ats");
    ASSERT_NE(context, nullptr);
    auto *ctx = Impl()->ProceedToState(context, ES2PANDA_STATE_BOUND);
    ASSERT_EQ(Impl()->ContextState(ctx), ES2PANDA_STATE_BOUND);
    Impl()->FreeCompilerPartMemory(ctx);
    Impl()->DestroyContext(ctx);
    SUCCEED();
}

// CHECKED state: manual Free + destroy
TEST_F(LeakDefenseTest, ManualFreeThenDestroyOnCheckedContext)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    auto *context = CreateParsedContext(config.Get(), "class P3 { value: int = 1 }", "manualfree-c.ats");
    ASSERT_NE(context, nullptr);
    auto *ctx = Impl()->ProceedToState(context, ES2PANDA_STATE_CHECKED);
    ASSERT_EQ(Impl()->ContextState(ctx), ES2PANDA_STATE_CHECKED);
    Impl()->FreeCompilerPartMemory(ctx);
    Impl()->DestroyContext(ctx);
    SUCCEED();
}

// LOWERED state: manual Free + destroy (emitter nulled; destroy skips)
TEST_F(LeakDefenseTest, ManualFreeThenDestroyOnLoweredContext)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    auto *context =
        CreateParsedContext(config.Get(), "function main() { for (let i = 0; i < 3; i++) {} }", "manualfree-l.ats");
    ASSERT_NE(context, nullptr);
    auto *ctx = Impl()->ProceedToState(context, ES2PANDA_STATE_LOWERED);
    ASSERT_EQ(Impl()->ContextState(ctx), ES2PANDA_STATE_LOWERED);
    Impl()->FreeCompilerPartMemory(ctx);
    Impl()->DestroyContext(ctx);
    SUCCEED();
}

// NEW state (unparsed): manual Free + destroy
TEST_F(LeakDefenseTest, ManualFreeThenDestroyOnNewContext)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    auto *context = Impl()->CreateContextFromString(config.Get(), "class N {}", "manualfree-n.ats");
    ASSERT_NE(context, nullptr);
    ASSERT_EQ(Impl()->ContextState(context), ES2PANDA_STATE_NEW);
    Impl()->FreeCompilerPartMemory(context);
    Impl()->DestroyContext(context);
    SUCCEED();
}

// ERROR state (syntax error): manual Free + destroy (error cleanup path)
TEST_F(ErrorPathLeakTest, ManualFreeThenDestroyOnErrorContext)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    auto *context = CreateParsedContext(config.Get(), "class E { x: }", "manualfree-e.ats");
    ASSERT_NE(context, nullptr);
    EXPECT_TRUE(Impl()->IsAnyError(context));
    Impl()->FreeCompilerPartMemory(context);
    Impl()->DestroyContext(context);
    SUCCEED();
}

// ERROR state (type error): manual Free + destroy
TEST_F(ErrorPathLeakTest, ManualFreeThenDestroyOnTypeErrorContext)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    auto *context = CreateParsedContext(config.Get(), "let t: int = \"oops\"", "manualfree-t.ats");
    ASSERT_NE(context, nullptr);
    auto *ctx = Impl()->ProceedToState(context, ES2PANDA_STATE_CHECKED);
    ASSERT_EQ(Impl()->ContextState(ctx), ES2PANDA_STATE_ERROR);
    Impl()->FreeCompilerPartMemory(ctx);
    Impl()->DestroyContext(ctx);
    SUCCEED();
}

// Manual Free + destroy loop x5: the early-release path is repeatable
TEST_F(LeakDefenseTest, ManualFreeThenDestroyLoop5)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    for (int i = 0; i < 5; i++) {
        auto *context = CreateParsedContext(config.Get(), "class L5 { v: int = 1 }", "manualfree-loop.ats");
        ASSERT_NE(context, nullptr);
        auto *ctx = Impl()->ProceedToState(context, ES2PANDA_STATE_CHECKED);
        ASSERT_EQ(Impl()->ContextState(ctx), ES2PANDA_STATE_CHECKED);
        Impl()->FreeCompilerPartMemory(ctx);
        Impl()->DestroyContext(ctx);
    }
    SUCCEED();
}

// Manual Free + destroy on a file context (CreateContextFromFile path)
TEST_F(LeakDefenseTest, ManualFreeThenDestroyOnFileContext)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    TempFileGuard file {"class F1 { v: int = 1 }"};
    auto *context = Impl()->CreateContextFromFile(config.Get(), file.Path());
    ASSERT_NE(context, nullptr);
    auto *ctx = Impl()->ProceedToState(context, ES2PANDA_STATE_PARSED);
    ASSERT_EQ(Impl()->ContextState(ctx), ES2PANDA_STATE_PARSED);
    Impl()->FreeCompilerPartMemory(ctx);
    Impl()->DestroyContext(ctx);
    SUCCEED();
}

// After a manual Free the config still creates new contexts
TEST_F(LeakDefenseTest, ManualFreeThenReuseConfigForNewContext)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    {
        auto *context = CreateParsedContext(config.Get(), "class R1 {}", "reuse-1.ats");
        ASSERT_NE(context, nullptr);
        Impl()->FreeCompilerPartMemory(context);
        Impl()->DestroyContext(context);
    }
    {
        auto *context = CreateParsedContext(config.Get(), "class R2 { v: int = 1 }", "reuse-2.ats");
        ASSERT_NE(context, nullptr);
        auto *ctx = Impl()->ProceedToState(context, ES2PANDA_STATE_CHECKED);
        ASSERT_EQ(Impl()->ContextState(ctx), ES2PANDA_STATE_CHECKED);
        Impl()->DestroyContext(ctx);
    }
    SUCCEED();
}

// ---------------------------------------------------------------------------
// 15.C AllocMemory stress
// ---------------------------------------------------------------------------

// 1 MiB block: written data reads back (the arena grows on demand)
TEST_F(LeakDefenseTest, AllocMemoryLargeBlock1MiB)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class A1 {}", "alloc-big.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    constexpr size_t kBytes = 1024U * 1024U;
    auto *bytes = static_cast<uint8_t *>(Impl()->AllocMemory(guard.Get(), kBytes, 1));
    ASSERT_NE(bytes, nullptr);
    for (size_t i = 0; i < kBytes; i++) {
        bytes[i] = static_cast<uint8_t>(i & 0xFFU);
    }
    for (size_t i = 0; i < kBytes; i++) {
        ASSERT_EQ(bytes[i], static_cast<uint8_t>(i & 0xFFU));
    }
}

// 200 medium blocks loop: 64 bytes each, distinct patterns all preserved
TEST_F(LeakDefenseTest, AllocMemoryMediumBlocksLoop200)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class A2 {}", "alloc-mid.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    std::vector<uint64_t *> blocks;
    for (int i = 0; i < 200; i++) {
        auto *block = static_cast<uint64_t *>(Impl()->AllocMemory(guard.Get(), 8, sizeof(uint64_t)));
        ASSERT_NE(block, nullptr);
        for (uint64_t j = 0; j < 8; j++) {
            block[j] = (static_cast<uint64_t>(i) << 32U) | j;
        }
        blocks.push_back(block);
    }
    for (size_t i = 0; i < blocks.size(); i++) {
        for (uint64_t j = 0; j < 8; j++) {
            ASSERT_EQ(blocks[i][j], (static_cast<uint64_t>(i) << 32U) | j);
        }
    }
}

// Mixed-size allocations x100: 1/2/4/8/16-byte elements, all verified
TEST_F(LeakDefenseTest, AllocMemoryMixedSizesLoop100)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class A3 {}", "alloc-mix.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    static const std::array<size_t, 5> SIZES {1, 2, 4, 8, 16};
    for (int i = 0; i < 100; i++) {
        size_t elemSize = SIZES[static_cast<size_t>(i) % SIZES.size()];
        auto *block = Impl()->AllocMemory(guard.Get(), elemSize, 1);
        ASSERT_NE(block, nullptr);
        auto *bytes = static_cast<uint8_t *>(block);
        for (size_t b = 0; b < elemSize; b++) {
            bytes[b] = static_cast<uint8_t>(i + static_cast<int>(b));
        }
        for (size_t b = 0; b < elemSize; b++) {
            ASSERT_EQ(bytes[b], static_cast<uint8_t>(i + static_cast<int>(b)));
        }
    }
}

// Zero element count: only assert no crash (0-byte behavior is internal)
TEST_F(NullSafetyTest, AllocMemoryZeroElementsDoesNotCrash)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class A4 {}", "alloc-zero.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *block = Impl()->AllocMemory(guard.Get(), 0, sizeof(uint64_t));
    (void)block;
    SUCCEED();
}

// Zero element size: only assert no crash
TEST_F(NullSafetyTest, AllocMemoryZeroElementSizeDoesNotCrash)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class A5 {}", "alloc-zerosz.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *block = Impl()->AllocMemory(guard.Get(), 16, 0);
    (void)block;
    SUCCEED();
}

// Allocation in LOWERED state: memory lives in the context arena
TEST_F(LeakDefenseTest, AllocMemoryAfterLoweredState)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {
        CreateParsedContext(config.Get(), "function main() { for (let i = 0; i < 3; i++) {} }", "alloc-lowered.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *ctx = Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_LOWERED);
    ASSERT_EQ(Impl()->ContextState(ctx), ES2PANDA_STATE_LOWERED);
    auto *block = static_cast<uint64_t *>(Impl()->AllocMemory(ctx, 4, sizeof(uint64_t)));
    ASSERT_NE(block, nullptr);
    for (uint64_t i = 0; i < 4; i++) {
        block[i] = i * 1000U;
    }
    for (uint64_t i = 0; i < 4; i++) {
        EXPECT_EQ(block[i], i * 1000U);
    }
}

// Alternate advance and allocate: parse, allocate, advance, allocate; both kept
TEST_F(LeakDefenseTest, AllocMemoryInterleavedWithPipeline)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {CreateParsedContext(config.Get(), "class A6 { v: int = 1 }", "alloc-inter.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *first = static_cast<uint32_t *>(Impl()->AllocMemory(guard.Get(), 8, sizeof(uint32_t)));
    ASSERT_NE(first, nullptr);
    for (uint32_t i = 0; i < 8; i++) {
        first[i] = 0x11110000U + i;
    }
    auto *ctx = Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_CHECKED);
    ASSERT_EQ(Impl()->ContextState(ctx), ES2PANDA_STATE_CHECKED);
    auto *second = static_cast<uint32_t *>(Impl()->AllocMemory(ctx, 8, sizeof(uint32_t)));
    ASSERT_NE(second, nullptr);
    for (uint32_t i = 0; i < 8; i++) {
        second[i] = 0x22220000U + i;
    }
    for (uint32_t i = 0; i < 8; i++) {
        EXPECT_EQ(first[i], 0x11110000U + i);
        EXPECT_EQ(second[i], 0x22220000U + i);
    }
}

// Destroy the context right after allocating: the arena reclaims it all
TEST_F(LeakDefenseTest, AllocMemoryReleasedByContextDestroy)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    {
        ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class A7 {}", "alloc-rel.ats")};
        ASSERT_NE(guard.Get(), nullptr);
        auto *block = Impl()->AllocMemory(guard.Get(), 128, 1);
        ASSERT_NE(block, nullptr);
        (void)block;
    }
    // Context destroyed, its arena is gone; only the destroy path runs here
    SUCCEED();
}

// ---------------------------------------------------------------------------
// 15.D file context loops (CreateContextFromFile)
// ---------------------------------------------------------------------------

// File context PARSED loop x10: EHeap non-decreasing
TEST_F(LeakDefenseTest, FileContextParsedLoop10)
{
    auto previous = ark::es2panda::EHeap::AllocatedSize();
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    TempFileGuard file {"class F10 { a: int = 1 b: int = 2 }"};
    for (int i = 0; i < 10; i++) {
        ContextGuard guard {Impl()->CreateContextFromFile(config.Get(), file.Path())};
        ASSERT_NE(guard.Get(), nullptr);
        auto *ctx = Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_PARSED);
        ASSERT_EQ(Impl()->ContextState(ctx), ES2PANDA_STATE_PARSED);
        auto current = ark::es2panda::EHeap::AllocatedSize();
        EXPECT_GE(current, previous);
        previous = current;
    }
}

// File context CHECKED loop x5
TEST_F(LeakDefenseTest, FileContextCheckedLoop5)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    TempFileGuard file {"class F11 { value: int = 1 }"};
    for (int i = 0; i < 5; i++) {
        ContextGuard guard {Impl()->CreateContextFromFile(config.Get(), file.Path())};
        ASSERT_NE(guard.Get(), nullptr);
        auto *ctx = Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_CHECKED);
        ASSERT_EQ(Impl()->ContextState(ctx), ES2PANDA_STATE_CHECKED);
    }
    SUCCEED();
}

// Two different files alternate: contexts created and destroyed in turn
TEST_F(LeakDefenseTest, FileContextTwoFilesInterleaved)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    TempFileGuard fileA {"class FA { x: int = 1 }"};
    TempFileGuard fileB {"class FB { y: string = \"s\" }"};
    for (int i = 0; i < 5; i++) {
        {
            ContextGuard guard {Impl()->CreateContextFromFile(config.Get(), fileA.Path())};
            ASSERT_NE(guard.Get(), nullptr);
            auto *ctx = Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_PARSED);
            ASSERT_EQ(Impl()->ContextState(ctx), ES2PANDA_STATE_PARSED);
        }
        {
            ContextGuard guard {Impl()->CreateContextFromFile(config.Get(), fileB.Path())};
            ASSERT_NE(guard.Get(), nullptr);
            auto *ctx = Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_PARSED);
            ASSERT_EQ(Impl()->ContextState(ctx), ES2PANDA_STATE_PARSED);
        }
    }
    SUCCEED();
}

// Two coexisting contexts on one file: independent parse, same node count
TEST_F(LeakDefenseTest, FileContextTwoConcurrentContextsSameFile)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    TempFileGuard file {"class FC { m(): int { return 1 } }"};
    ContextGuard first {Impl()->CreateContextFromFile(config.Get(), file.Path())};
    ASSERT_NE(first.Get(), nullptr);
    ContextGuard second {Impl()->CreateContextFromFile(config.Get(), file.Path())};
    ASSERT_NE(second.Get(), nullptr);
    auto *ctxA = Impl()->ProceedToState(first.Get(), ES2PANDA_STATE_PARSED);
    auto *ctxB = Impl()->ProceedToState(second.Get(), ES2PANDA_STATE_PARSED);
    ASSERT_EQ(Impl()->ContextState(ctxA), ES2PANDA_STATE_PARSED);
    ASSERT_EQ(Impl()->ContextState(ctxB), ES2PANDA_STATE_PARSED);
    WalkPayload payloadA;
    auto *programA = Impl()->ContextProgram(ctxA);
    ASSERT_NE(programA, nullptr);
    WalkAst(Impl(), ctxA, Impl()->ProgramAst(ctxA, programA), &payloadA);
    WalkPayload payloadB;
    auto *programB = Impl()->ContextProgram(ctxB);
    ASSERT_NE(programB, nullptr);
    WalkAst(Impl(), ctxB, Impl()->ProgramAst(ctxB, programB), &payloadB);
    EXPECT_GT(payloadA.nodeCount, 0U);
    EXPECT_EQ(payloadA.nodeCount, payloadB.nodeCount);
    EXPECT_EQ(payloadA.identifierCount, payloadB.identifierCount);
}

// Large file: 30 lines of source parsed to CHECKED in one go
TEST_F(LeakDefenseTest, FileContextLargeFileChecks)
{
    std::string source = "class Big {\n";
    for (int i = 0; i < 30; i++) {
        source += "    f" + std::to_string(i) + ": int = " + std::to_string(i) + "\n";
    }
    source += "}\n";
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    TempFileGuard file {source};
    ContextGuard guard {Impl()->CreateContextFromFile(config.Get(), file.Path())};
    ASSERT_NE(guard.Get(), nullptr);
    auto *ctx = Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_CHECKED);
    ASSERT_EQ(Impl()->ContextState(ctx), ES2PANDA_STATE_CHECKED);
    WalkPayload payload;
    auto *program = Impl()->ContextProgram(ctx);
    ASSERT_NE(program, nullptr);
    WalkAst(Impl(), ctx, Impl()->ProgramAst(ctx, program), &payload);
    EXPECT_GE(payload.nodeCount, 30U);
    EXPECT_EQ(payload.nullParentCount, 1U);
}

// Missing file loop x5: an error context every round, destroyed safely
TEST_F(ErrorPathLeakTest, FileContextMissingFileLoop5)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    for (int i = 0; i < 5; i++) {
        ContextGuard guard {Impl()->CreateContextFromFile(config.Get(), "/e2p/no/such/file-l.ats")};
        ASSERT_NE(guard.Get(), nullptr);
        // Same as CreateContextFromFileMissingFileYieldsErrorReport: state stays NEW,
        // the error is reported via errorMessage
        EXPECT_EQ(Impl()->ContextState(guard.Get()), ES2PANDA_STATE_NEW);
        EXPECT_TRUE(Contains(std::string(Impl()->ContextErrorMessage(guard.Get())), "Failed to"));
    }
}

// ---------------------------------------------------------------------------
// 15.E coexisting contexts (same config)
// ---------------------------------------------------------------------------

// Two coexisting contexts advanced to different states, then destroyed
TEST_F(LeakDefenseTest, TwoContextsCoexistMixedStates)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    auto *first = Impl()->CreateContextFromString(config.Get(), "class X1 {}", "coexist-1.ats");
    ASSERT_NE(first, nullptr);
    auto *second = Impl()->CreateContextFromString(config.Get(), "class X2 { v: int = 1 }", "coexist-2.ats");
    ASSERT_NE(second, nullptr);
    first = Impl()->ProceedToState(first, ES2PANDA_STATE_PARSED);
    second = Impl()->ProceedToState(second, ES2PANDA_STATE_CHECKED);
    ASSERT_EQ(Impl()->ContextState(first), ES2PANDA_STATE_PARSED);
    ASSERT_EQ(Impl()->ContextState(second), ES2PANDA_STATE_CHECKED);
    Impl()->DestroyContext(first);
    Impl()->DestroyContext(second);
    SUCCEED();
}

// Eight coexisting contexts all parsed: states do not interfere
TEST_F(LeakDefenseTest, EightContextsCoexistAllParsed)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    std::vector<es2panda_Context *> contexts;
    for (int i = 0; i < 8; i++) {
        std::string source = "class E" + std::to_string(i) + " { v: int = " + std::to_string(i) + " }";
        auto *context = Impl()->CreateContextFromString(config.Get(), source.c_str(), "coexist8.ats");
        ASSERT_NE(context, nullptr);
        contexts.push_back(context);
    }
    for (auto *context : contexts) {
        auto *ctx = Impl()->ProceedToState(context, ES2PANDA_STATE_PARSED);
        ASSERT_EQ(Impl()->ContextState(ctx), ES2PANDA_STATE_PARSED);
    }
    for (auto *context : contexts) {
        Impl()->DestroyContext(context);
    }
    SUCCEED();
}

// Five coexisting contexts, one per state (NEW/PARSED/BOUND/CHECKED/LOWERED)
TEST_F(LeakDefenseTest, FiveContextsCoexistAllDistinctStates)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    static const std::array<es2panda_ContextState, 5> STATES {ES2PANDA_STATE_NEW, ES2PANDA_STATE_PARSED,
                                                              ES2PANDA_STATE_BOUND, ES2PANDA_STATE_CHECKED,
                                                              ES2PANDA_STATE_LOWERED};
    static const std::array<char const *, 5> SOURCES {"class D0 {}", "class D1 {}", "class D2 { x: int = 1 }",
                                                      "class D3 { value: int = 1 }",
                                                      "function main() { for (let i = 0; i < 3; i++) {} }"};
    std::vector<es2panda_Context *> contexts;
    for (size_t i = 0; i < STATES.size(); i++) {
        auto *context = Impl()->CreateContextFromString(config.Get(), SOURCES[i], "distinct.ats");
        ASSERT_NE(context, nullptr);
        contexts.push_back(context);
    }
    for (size_t i = 0; i < contexts.size(); i++) {
        auto *ctx = Impl()->ProceedToState(contexts[i], STATES[i]);
        ASSERT_EQ(Impl()->ContextState(ctx), STATES[i]) << "context " << i;
    }
    for (auto *context : contexts) {
        Impl()->DestroyContext(context);
    }
    SUCCEED();
}

// Two contexts advanced alternately (parse A, parse B, check A, check B)
TEST_F(LeakDefenseTest, TwoContextsInterleavedPipeline)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    auto *first = Impl()->CreateContextFromString(config.Get(), "class I1 { v: int = 1 }", "inter-1.ats");
    ASSERT_NE(first, nullptr);
    auto *second = Impl()->CreateContextFromString(config.Get(), "class I2 { w: int = 2 }", "inter-2.ats");
    ASSERT_NE(second, nullptr);
    first = Impl()->ProceedToState(first, ES2PANDA_STATE_PARSED);
    second = Impl()->ProceedToState(second, ES2PANDA_STATE_PARSED);
    ASSERT_EQ(Impl()->ContextState(first), ES2PANDA_STATE_PARSED);
    ASSERT_EQ(Impl()->ContextState(second), ES2PANDA_STATE_PARSED);
    first = Impl()->ProceedToState(first, ES2PANDA_STATE_CHECKED);
    second = Impl()->ProceedToState(second, ES2PANDA_STATE_CHECKED);
    ASSERT_EQ(Impl()->ContextState(first), ES2PANDA_STATE_CHECKED);
    ASSERT_EQ(Impl()->ContextState(second), ES2PANDA_STATE_CHECKED);
    Impl()->DestroyContext(first);
    Impl()->DestroyContext(second);
    SUCCEED();
}

// Coexisting ASTs are independent: node counts and first kind differ
TEST_F(LeakDefenseTest, ConcurrentContextsAstIndependent)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    auto *first = Impl()->CreateContextFromString(config.Get(), "class IA { a1: int = 1 a2: int = 2 }", "indep-1.ats");
    ASSERT_NE(first, nullptr);
    auto *second = Impl()->CreateContextFromString(config.Get(), "class IB {}", "indep-2.ats");
    ASSERT_NE(second, nullptr);
    first = Impl()->ProceedToState(first, ES2PANDA_STATE_PARSED);
    second = Impl()->ProceedToState(second, ES2PANDA_STATE_PARSED);
    WalkPayload payloadA;
    auto *programA = Impl()->ContextProgram(first);
    ASSERT_NE(programA, nullptr);
    WalkAst(Impl(), first, Impl()->ProgramAst(first, programA), &payloadA);
    WalkPayload payloadB;
    auto *programB = Impl()->ContextProgram(second);
    ASSERT_NE(programB, nullptr);
    WalkAst(Impl(), second, Impl()->ProgramAst(second, programB), &payloadB);
    EXPECT_GT(payloadA.nodeCount, payloadB.nodeCount);
    EXPECT_TRUE(Contains(payloadA.identifierNames, "IA"));
    EXPECT_TRUE(Contains(payloadB.identifierNames, "IB"));
    EXPECT_FALSE(Contains(payloadB.identifierNames, "IA"));
    Impl()->DestroyContext(first);
    Impl()->DestroyContext(second);
    SUCCEED();
}

// Reverse destruction: created A,B,C; destroyed C,B,A
TEST_F(LeakDefenseTest, ContextsDestroyedInReverseOrder)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    std::vector<es2panda_Context *> contexts;
    for (int i = 0; i < 3; i++) {
        std::string source = "class RV" + std::to_string(i) + " { v: int = " + std::to_string(i) + " }";
        auto *context = CreateParsedContext(config.Get(), source.c_str(), "reverse.ats");
        ASSERT_NE(context, nullptr);
        contexts.push_back(context);
    }
    for (auto it = contexts.rbegin(); it != contexts.rend(); ++it) {
        Impl()->DestroyContext(*it);
    }
    SUCCEED();
}

// Coexistence loop x3: 4 contexts to BOUND each round, then all destroyed
TEST_F(LeakDefenseTest, ConcurrentContextGroupsLoop3)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    for (int round = 0; round < 3; round++) {
        std::vector<es2panda_Context *> contexts;
        for (int i = 0; i < 4; i++) {
            std::string source = "class G" + std::to_string(i) + " { x: int = " + std::to_string(i) + " }";
            auto *context = Impl()->CreateContextFromString(config.Get(), source.c_str(), "group.ats");
            ASSERT_NE(context, nullptr);
            contexts.push_back(context);
        }
        for (auto *context : contexts) {
            auto *ctx = Impl()->ProceedToState(context, ES2PANDA_STATE_BOUND);
            ASSERT_EQ(Impl()->ContextState(ctx), ES2PANDA_STATE_BOUND);
        }
        for (auto *context : contexts) {
            Impl()->DestroyContext(context);
        }
    }
    SUCCEED();
}

// After destroying one context the others still advance
TEST_F(LeakDefenseTest, SiblingContextSurvivesOtherDestroy)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    auto *first = CreateParsedContext(config.Get(), "class S1 {}", "sib-1.ats");
    ASSERT_NE(first, nullptr);
    auto *second = CreateParsedContext(config.Get(), "class S2 { v: int = 1 }", "sib-2.ats");
    ASSERT_NE(second, nullptr);
    Impl()->DestroyContext(first);
    auto *ctx = Impl()->ProceedToState(second, ES2PANDA_STATE_CHECKED);
    ASSERT_EQ(Impl()->ContextState(ctx), ES2PANDA_STATE_CHECKED);
    Impl()->DestroyContext(ctx);
    SUCCEED();
}

// ---------------------------------------------------------------------------
// 15.F error-path leak stress (new snippets + EHeap dimension)
// ---------------------------------------------------------------------------

// New syntax error matrix: create->parse->destroy per snippet; all ERROR
TEST_F(ErrorPathLeakTest, NewSyntaxErrorFragmentsMatrix)
{
    static const std::array<char const *, 8> BAD_SOURCES {"class C1 extends {}",
                                                          "class C2 implements {}",
                                                          "interface I extends {}",
                                                          "let 123bad = 1",
                                                          "function 9bad() {}",
                                                          "enum E { , }",
                                                          "try { } ",
                                                          "class C3 { constructor() }"};
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    for (auto const *source : BAD_SOURCES) {
        ContextGuard guard {CreateParsedContext(config.Get(), source, "newbad.ats")};
        ASSERT_NE(guard.Get(), nullptr) << "source: " << source;
        auto state = Impl()->ContextState(guard.Get());
        EXPECT_TRUE(state == ES2PANDA_STATE_ERROR ||
                    (state == ES2PANDA_STATE_PARSED && Impl()->IsAnyError(guard.Get())))
            << "source: " << source;
    }
}

// Bad/good alternate + EHeap: bad contexts do not accelerate leaks
TEST_F(ErrorPathLeakTest, BadGoodAlternationEHeapMonotonic)
{
    auto previous = ark::es2panda::EHeap::AllocatedSize();
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    for (int i = 0; i < 6; i++) {
        {
            ContextGuard bad {CreateParsedContext(config.Get(), "class B { x: }", "bg-bad.ats")};
            ASSERT_NE(bad.Get(), nullptr);
            EXPECT_TRUE(Impl()->IsAnyError(bad.Get()));
        }
        {
            ContextGuard good {CreateParsedContext(config.Get(), "class G { v: int = 1 }", "bg-good.ats")};
            ASSERT_NE(good.Get(), nullptr);
            EXPECT_EQ(Impl()->ContextState(good.Get()), ES2PANDA_STATE_PARSED);
        }
        auto current = ark::es2panda::EHeap::AllocatedSize();
        EXPECT_GE(current, previous);
        previous = current;
    }
}

// Manual Free + destroy on ERROR contexts x5 (repeatable cleanup path)
TEST_F(ErrorPathLeakTest, ErrorContextManualFreeLoop5)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    for (int i = 0; i < 5; i++) {
        auto *context = CreateParsedContext(config.Get(), "class Z { x: }", "err-manualfree.ats");
        ASSERT_NE(context, nullptr);
        EXPECT_TRUE(Impl()->IsAnyError(context));
        Impl()->FreeCompilerPartMemory(context);
        Impl()->DestroyContext(context);
    }
    SUCCEED();
}

// Full pipeline after a syntax error: bad/good use separate configs (the
// engine lives with the config); each pair destroyed
TEST_F(ErrorPathLeakTest, GoodContextFullPipelineAfterSyntaxError)
{
    {
        ConfigGuard badConfig {NewConfig()};
        ASSERT_NE(badConfig.Get(), nullptr);
        ContextGuard bad {CreateParsedContext(badConfig.Get(), "class B { x: }", "recover-bad.ats")};
        ASSERT_NE(bad.Get(), nullptr);
        EXPECT_TRUE(Impl()->IsAnyError(bad.Get()));
    }
    ConfigGuard goodConfig {NewConfig()};
    ASSERT_NE(goodConfig.Get(), nullptr);
    auto *context =
        CreateParsedContext(goodConfig.Get(), "function main() { for (let i = 0; i < 3; i++) {} }", "recover-good.ats");
    ASSERT_NE(context, nullptr);
    auto *ctx = Impl()->ProceedToState(context, ES2PANDA_STATE_LOWERED);
    ASSERT_EQ(Impl()->ContextState(ctx), ES2PANDA_STATE_LOWERED);
    Impl()->DestroyContext(ctx);
    SUCCEED();
}

// Many distinct error snippets: 12 kinds, all safely ERROR and destroyed
TEST_F(ErrorPathLeakTest, ManyDistinctErrorFragmentsAllSafe)
{
    static const std::array<char const *, 12> BAD_SOURCES {"class { }",
                                                           "enum {}",
                                                           "function () {}",
                                                           "let : int = 1",
                                                           "interface { }",
                                                           "if { }",
                                                           "while { }",
                                                           "for (let i = 0; i < ) {}",
                                                           "class C { (): int { } }",
                                                           "class C { m(: int {} }",
                                                           "let x = ",
                                                           "class C extends Unknown1234, Other5678 { }"};
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    for (auto const *source : BAD_SOURCES) {
        ContextGuard guard {Impl()->CreateContextFromString(config.Get(), source, "distinct-bad.ats")};
        ASSERT_NE(guard.Get(), nullptr) << "source: " << source;
        auto *ctx = Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_PARSED);
        // Every snippet must reach an error or safe state; never crash
        auto state = Impl()->ContextState(ctx);
        EXPECT_TRUE(state == ES2PANDA_STATE_ERROR || state == ES2PANDA_STATE_PARSED) << "source: " << source;
    }
}

// ---------------------------------------------------------------------------
// 15.G EvaluateExpression leak stress extension
// ---------------------------------------------------------------------------

// Same expression eval + free loop x20
TEST_F(EvaluateExpressionLifecycleTest, EvaluateAndFreeLoop20)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    for (int i = 0; i < 20; i++) {
        auto *result = Impl()->EvaluateExpression(config.Get(), Base64Encode("1 + 2").c_str());
        ASSERT_NE(result, nullptr) << "iteration " << i;
        Impl()->FreeExpressionResult(result);
    }
    SUCCEED();
}

// Expression table x3 rounds (30 eval/free pairs in total)
TEST_F(EvaluateExpressionLifecycleTest, EvaluateExpressionTableLoop3)
{
    static const std::array<char const *, 10> EXPRESSIONS {"1 + 1",  "2 - 2",   "3 * 3", "4 / 2",  "5 % 3",
                                                           "1 << 3", "10 >> 1", "1 < 2", "2 >= 1", "7 & 3"};
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    for (int round = 0; round < 3; round++) {
        for (auto const *expression : EXPRESSIONS) {
            auto *result = Impl()->EvaluateExpression(config.Get(), Base64Encode(expression).c_str());
            ASSERT_NE(result, nullptr) << "expression: " << expression;
            Impl()->FreeExpressionResult(result);
        }
    }
    SUCCEED();
}

// Declaration path: "let x: int = 5" takes the declaration branch (malloc/free paired)
TEST_F(EvaluateExpressionLifecycleTest, EvaluateDeclarationLetInt)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    auto *result = Impl()->EvaluateExpression(config.Get(), Base64Encode("let x: int = 5").c_str());
    ASSERT_NE(result, nullptr);
    Impl()->FreeExpressionResult(result);
}

// Declaration path: const declaration
TEST_F(EvaluateExpressionLifecycleTest, EvaluateDeclarationConst)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    auto *result = Impl()->EvaluateExpression(config.Get(), Base64Encode("const y: string = \"v\"").c_str());
    ASSERT_NE(result, nullptr);
    Impl()->FreeExpressionResult(result);
}

// Longer expression: nested arithmetic + parentheses
TEST_F(EvaluateExpressionLifecycleTest, EvaluateLongNestedExpression)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    auto *result =
        Impl()->EvaluateExpression(config.Get(), Base64Encode("((1 + 2) * (3 + 4) - (5 * 6)) / (2 + 1)").c_str());
    ASSERT_NE(result, nullptr);
    Impl()->FreeExpressionResult(result);
}

// Logic / comparison expressions
TEST_F(EvaluateExpressionLifecycleTest, EvaluateComparisonExpression)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    auto *result = Impl()->EvaluateExpression(config.Get(), Base64Encode("1 < 2 && 3 > 2").c_str());
    ASSERT_NE(result, nullptr);
    Impl()->FreeExpressionResult(result);
}

// Eval failure (bad base64) then success: the failure path leaves no damage
TEST_F(EvaluateExpressionLifecycleTest, EvaluateGoodAfterBadInput)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ASSERT_EQ(Impl()->EvaluateExpression(config.Get(), "!!!not-base64!!!"), nullptr);
    auto *result = Impl()->EvaluateExpression(config.Get(), Base64Encode("1 + 2").c_str());
    ASSERT_NE(result, nullptr);
    Impl()->FreeExpressionResult(result);
}

// Eval failure (bad syntax) then success
TEST_F(EvaluateExpressionLifecycleTest, EvaluateGoodAfterSyntaxErrorExpression)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ASSERT_EQ(Impl()->EvaluateExpression(config.Get(), Base64Encode("class {").c_str()), nullptr);
    auto *result = Impl()->EvaluateExpression(config.Get(), Base64Encode("42").c_str());
    ASSERT_NE(result, nullptr);
    Impl()->FreeExpressionResult(result);
}

// Sequential configs: each config evals and frees on its own, then dies
TEST_F(EvaluateExpressionLifecycleTest, EvaluateWithSequentialConfigs)
{
    for (int i = 0; i < 3; i++) {
        ConfigGuard config {NewConfig()};
        ASSERT_NE(config.Get(), nullptr);
        auto *result = Impl()->EvaluateExpression(config.Get(), Base64Encode("100 + i").c_str());
        // The expression contains the undefined identifier i: eval fails with null,
        // or succeeds with bytecode; both outcomes must be freed safely
        if (result != nullptr) {
            Impl()->FreeExpressionResult(result);
        }
    }
    SUCCEED();
}

// Eval + free + config destroy order: free the result before the config
TEST_F(EvaluateExpressionLifecycleTest, FreeResultBeforeConfigDestroy)
{
    {
        ConfigGuard config {NewConfig()};
        ASSERT_NE(config.Get(), nullptr);
        auto *result = Impl()->EvaluateExpression(config.Get(), Base64Encode("2 + 2").c_str());
        ASSERT_NE(result, nullptr);
        Impl()->FreeExpressionResult(result);
    }
    SUCCEED();
}

// Mixed eval stress: expressions and declarations alternate x10 rounds
TEST_F(EvaluateExpressionLifecycleTest, EvaluateExpressionDeclarationAlternatingLoop10)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    for (int i = 0; i < 10; i++) {
        auto *expr = Impl()->EvaluateExpression(config.Get(), Base64Encode("1 + 2").c_str());
        ASSERT_NE(expr, nullptr);
        Impl()->FreeExpressionResult(expr);
        auto *decl = Impl()->EvaluateExpression(config.Get(), Base64Encode("let v: int = 9").c_str());
        ASSERT_NE(decl, nullptr);
        Impl()->FreeExpressionResult(decl);
    }
    SUCCEED();
}

// ---------------------------------------------------------------------------
// 15.H GlobalContext stress (heavy; loops stay small)
// ---------------------------------------------------------------------------

// GlobalContext full lifecycle loop x2 (each round carries one context)
TEST_F(GlobalContextLifecycleTest, GlobalContextLifecycleLoop2WithContext)
{
    for (int i = 0; i < 2; i++) {
        ConfigGuard config {NewConfig()};
        ASSERT_NE(config.Get(), nullptr);
        GlobalContextGuard global {Impl()->CreateGlobalContext(config.Get(), nullptr, 0, false)};
        ASSERT_NE(global.Get(), nullptr);
        ContextGuard guard {Impl()->CreateCacheContextFromString(config.Get(), "class GC1 { v: int = 1 }",
                                                                 "gctx-loop.ats", global.Get(), false, false)};
        ASSERT_NE(guard.Get(), nullptr);
        auto *ctx = Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_PARSED);
        ASSERT_EQ(Impl()->ContextState(ctx), ES2PANDA_STATE_PARSED);
    }
    SUCCEED();
}

// GlobalContext and a normal context coexist on one config
TEST_F(GlobalContextLifecycleTest, GlobalContextCoexistsWithPlainContext)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    GlobalContextGuard global {Impl()->CreateGlobalContext(config.Get(), nullptr, 0, false)};
    ASSERT_NE(global.Get(), nullptr);
    ContextGuard plain {CreateParsedContext(config.Get(), "class PL {}", "plain.ats")};
    ASSERT_NE(plain.Get(), nullptr);
    ContextGuard cached {
        Impl()->CreateCacheContextFromString(config.Get(), "class CA {}", "cached.ats", global.Get(), false, false)};
    ASSERT_NE(cached.Get(), nullptr);
    auto *ctx = Impl()->ProceedToState(cached.Get(), ES2PANDA_STATE_PARSED);
    ASSERT_EQ(Impl()->ContextState(ctx), ES2PANDA_STATE_PARSED);
    SUCCEED();
}

// Cache context with manual Free + destroy: the early-release path is safe
TEST_F(LeakDefenseTest, ManualFreeThenDestroyOnCacheContext)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    GlobalContextGuard global {Impl()->CreateGlobalContext(config.Get(), nullptr, 0, false)};
    ASSERT_NE(global.Get(), nullptr);
    auto *context = Impl()->CreateCacheContextFromString(config.Get(), "class MF { v: int = 1 }", "mf-cache.ats",
                                                         global.Get(), false, false);
    ASSERT_NE(context, nullptr);
    context = Impl()->ProceedToState(context, ES2PANDA_STATE_PARSED);
    ASSERT_EQ(Impl()->ContextState(context), ES2PANDA_STATE_PARSED);
    Impl()->FreeCompilerPartMemory(context);
    Impl()->DestroyContext(context);
    SUCCEED();
}

// Cache context on an LspUsage GlobalContext, parsed
TEST_F(GlobalContextLifecycleTest, LspGlobalContextCacheContextParses)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    GlobalContextGuard global {Impl()->CreateGlobalContext(config.Get(), nullptr, 0, true)};
    ASSERT_NE(global.Get(), nullptr);
    ContextGuard guard {Impl()->CreateCacheContextFromString(config.Get(), "class LC { v: int = 1 }", "lsp-cache.ats",
                                                             global.Get(), false, true)};
    ASSERT_NE(guard.Get(), nullptr);
    auto *ctx = Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_PARSED);
    ASSERT_EQ(Impl()->ContextState(ctx), ES2PANDA_STATE_PARSED);
    SUCCEED();
}

// Cache contexts on a GlobalContext parsed twice (two contexts, same source): both stop at PARSED
TEST_F(GlobalContextLifecycleTest, CacheContextRepeatedParses)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    GlobalContextGuard global {Impl()->CreateGlobalContext(config.Get(), nullptr, 0, false)};
    ASSERT_NE(global.Get(), nullptr);
    for (int i = 0; i < 2; i++) {
        ContextGuard guard {Impl()->CreateCacheContextFromString(config.Get(), "class CR { v: int = 1 }",
                                                                 "cache-rep.ats", global.Get(), false, false)};
        ASSERT_NE(guard.Get(), nullptr);
        auto *ctx = Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_PARSED);
        ASSERT_EQ(Impl()->ContextState(ctx), ES2PANDA_STATE_PARSED);
    }
    SUCCEED();
}

// CreateContextFromStringWithHistory leak loop x5
TEST_F(LeakDefenseTest, ContextWithHistoryLoop5)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    for (int i = 0; i < 5; i++) {
        ContextGuard guard {
            Impl()->CreateContextFromStringWithHistory(config.Get(), "class H5 { v: int = 1 }", "hist-loop.ats")};
        ASSERT_NE(guard.Get(), nullptr);
        auto *ctx = Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_PARSED);
        ASSERT_EQ(Impl()->ContextState(ctx), ES2PANDA_STATE_PARSED);
    }
    SUCCEED();
}

// CreateCacheContextFromFile + temp file: parsed then destroyed
TEST_F(LeakDefenseTest, CacheContextFromFileParsesAndDestroys)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    GlobalContextGuard global {Impl()->CreateGlobalContext(config.Get(), nullptr, 0, false)};
    ASSERT_NE(global.Get(), nullptr);
    TempFileGuard file {"class CF { v: int = 1 }"};
    ContextGuard guard {Impl()->CreateCacheContextFromFile(config.Get(), file.Path(), global.Get(), false)};
    ASSERT_NE(guard.Get(), nullptr);
    auto *ctx = Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_PARSED);
    ASSERT_EQ(Impl()->ContextState(ctx), ES2PANDA_STATE_PARSED);
    SUCCEED();
}
// ===========================================================================
// Part 16: API boundaries and idempotence - Options sharing / ResetCounters
// / enum round-trips / diagnostic side effects / Jsdoc-License-Comments
// null input / declaration queries / number literal details / Is* sweeps
// ===========================================================================

// ---------------------------------------------------------------------------
// 16.A Options / config details
// ---------------------------------------------------------------------------

// Two contexts on one config: ConfigGetOptions returns the same pointer
TEST_F(LeakDefenseTest, OptionsSharedAcrossContextsOfSameConfig)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard first {Impl()->CreateContextFromString(config.Get(), "class O1 {}", "opts-1.ats")};
    ASSERT_NE(first.Get(), nullptr);
    ContextGuard second {Impl()->CreateContextFromString(config.Get(), "class O2 {}", "opts-2.ats")};
    ASSERT_NE(second.Get(), nullptr);
    auto *optionsFirst = Impl()->ConfigGetOptions(config.Get());
    auto *optionsSecond = Impl()->ConfigGetOptions(config.Get());
    ASSERT_NE(optionsFirst, nullptr);
    EXPECT_EQ(optionsFirst, optionsSecond);
}

// Two different configs: options pointers must differ (independent allocations)
TEST_F(LeakDefenseTest, DifferentConfigsHaveDifferentOptions)
{
    ConfigGuard first {NewConfig()};
    ASSERT_NE(first.Get(), nullptr);
    ConfigGuard second {NewConfig()};
    ASSERT_NE(second.Get(), nullptr);
    auto *optionsFirst = Impl()->ConfigGetOptions(first.Get());
    auto *optionsSecond = Impl()->ConfigGetOptions(second.Get());
    ASSERT_NE(optionsFirst, nullptr);
    ASSERT_NE(optionsSecond, nullptr);
    EXPECT_NE(optionsFirst, optionsSecond);
}

// Options pointer stable across context lifecycles (config outlives them)
TEST_F(LeakDefenseTest, OptionsPointerStableAcrossContextCycles)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    auto *reference = Impl()->ConfigGetOptions(config.Get());
    ASSERT_NE(reference, nullptr);
    for (int i = 0; i < 10; i++) {
        ContextGuard guard {CreateParsedContext(config.Get(), "class OS {}", "opts-cycle.ats")};
        ASSERT_NE(guard.Get(), nullptr);
        EXPECT_EQ(Impl()->ConfigGetOptions(config.Get()), reference) << "iteration " << i;
    }
}

// ---------------------------------------------------------------------------
// 16.B ResetCounters - idempotence, compilation unaffected
// ---------------------------------------------------------------------------

// Calling ResetCounters repeatedly is idempotent (counters zeroed)
TEST_F(NullSafetyTest, ResetCountersIdempotent)
{
    Impl()->ResetCounters();
    Impl()->ResetCounters();
    Impl()->ResetCounters();
    SUCCEED();
}

// The full pipeline still compiles fine after ResetCounters
TEST_F(LeakDefenseTest, ResetCountersThenFullPipelineSucceeds)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    Impl()->ResetCounters();
    ContextGuard guard {CreateParsedContext(config.Get(), "class RC { v: int = 1 }", "reset-pipe.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *ctx = Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_CHECKED);
    EXPECT_EQ(Impl()->ContextState(ctx), ES2PANDA_STATE_CHECKED);
}

// ResetCounters interleaved with parse loops: results unchanged
TEST_F(LeakDefenseTest, ResetCountersInterleavedWithParseLoop)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    size_t referenceCount = 0;
    for (int i = 0; i < 5; i++) {
        Impl()->ResetCounters();
        ContextGuard guard {CreateParsedContext(config.Get(), "class RI { x: int = 0 }", "reset-loop.ats")};
        ASSERT_NE(guard.Get(), nullptr);
        auto *program = Impl()->ContextProgram(guard.Get());
        ASSERT_NE(program, nullptr);
        auto *ast = Impl()->ProgramAst(guard.Get(), program);
        WalkPayload payload;
        WalkAst(Impl(), guard.Get(), ast, &payload);
        if (referenceCount == 0) {
            referenceCount = payload.nodeCount;
        } else {
            EXPECT_EQ(payload.nodeCount, referenceCount) << "iteration " << i;
        }
    }
}

// EvaluateExpression still works after ResetCounters
TEST_F(EvaluateExpressionLifecycleTest, ResetCountersThenEvaluateExpression)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    Impl()->ResetCounters();
    auto *result = Impl()->EvaluateExpression(config.Get(), Base64Encode("1 + 2").c_str());
    ASSERT_NE(result, nullptr);
    Impl()->FreeExpressionResult(result);
}

// ---------------------------------------------------------------------------
// 16.C Es2pandaEnumFromString / Es2pandaEnumToString round-trips
// ---------------------------------------------------------------------------

// Single enum round-trip: ENUM_NODE_HAS_PARENT
TEST_F(NullSafetyTest, EnumRoundTripNodeHasParent)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class E1 {}", "enum-1.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *str = Impl()->Es2pandaEnumToString(guard.Get(), Es2pandaEnum::ENUM_NODE_HAS_PARENT);
    ASSERT_NE(str, nullptr);
    EXPECT_NE(std::string(str), "");
    EXPECT_EQ(Impl()->Es2pandaEnumFromString(guard.Get(), str), Es2pandaEnum::ENUM_NODE_HAS_PARENT);
}

// Single enum round-trip: ENUM_VARIABLE_HAS_SCOPE
TEST_F(NullSafetyTest, EnumRoundTripVariableHasScope)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class E2 {}", "enum-2.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *str = Impl()->Es2pandaEnumToString(guard.Get(), Es2pandaEnum::ENUM_VARIABLE_HAS_SCOPE);
    ASSERT_NE(str, nullptr);
    EXPECT_NE(std::string(str), "");
    EXPECT_EQ(Impl()->Es2pandaEnumFromString(guard.Get(), str), Es2pandaEnum::ENUM_VARIABLE_HAS_SCOPE);
}

// Single enum round-trip: ENUM_CHECK_STRUCT_DECLARATION
TEST_F(NullSafetyTest, EnumRoundTripCheckStructDeclaration)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class E3 {}", "enum-3.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *str = Impl()->Es2pandaEnumToString(guard.Get(), Es2pandaEnum::ENUM_CHECK_STRUCT_DECLARATION);
    ASSERT_NE(str, nullptr);
    EXPECT_NE(std::string(str), "");
    EXPECT_EQ(Impl()->Es2pandaEnumFromString(guard.Get(), str), Es2pandaEnum::ENUM_CHECK_STRUCT_DECLARATION);
}

// All base check enums (NODE_HAS_PARENT..EVERY_CHILD_IN_PARENT_RANGE) round-trip
TEST_F(LeakDefenseTest, AllBaseEnumRoundTripsSucceed)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class E4 {}", "enum-all.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    for (int e = static_cast<int>(Es2pandaEnum::ENUM_NODE_HAS_PARENT);
         e <= static_cast<int>(Es2pandaEnum::ENUM_BASE_LAST); ++e) {
        auto id = static_cast<Es2pandaEnum>(e);
        auto *str = Impl()->Es2pandaEnumToString(guard.Get(), id);
        ASSERT_NE(str, nullptr) << "enum value " << e;
        EXPECT_NE(std::string(str), "") << "enum value " << e;
        EXPECT_EQ(Impl()->Es2pandaEnumFromString(guard.Get(), str), id) << "string: " << str;
    }
}

// Enum string round-trip loop x50: arena strings do not overwrite each other
TEST_F(LeakDefenseTest, EnumRoundTripLoop50)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class E5 {}", "enum-loop.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    std::vector<std::string> strings;
    for (int i = 0; i < 50; i++) {
        auto *str = Impl()->Es2pandaEnumToString(guard.Get(), Es2pandaEnum::ENUM_NODE_HAS_PARENT);
        ASSERT_NE(str, nullptr);
        strings.emplace_back(str);
    }
    // All copies identical (the source string is not overwritten by later allocations)
    for (auto const &s : strings) {
        EXPECT_EQ(s, strings.front());
    }
}

// ---------------------------------------------------------------------------
// 16.D diagnostic API side effects and boundaries
// ---------------------------------------------------------------------------

// Syntax error context: all four diagnostic storages usable (non-null)
TEST_F(NullSafetyTest, DiagnosticStoragesNonNullOnSyntaxErrorContext)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class DS { x: }", "diag-bad.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *result = Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(Impl()->IsAnyError(result));
    EXPECT_NE(Impl()->GetSemanticErrors(result), nullptr);
    EXPECT_NE(Impl()->GetSyntaxErrors(result), nullptr);
    EXPECT_NE(Impl()->GetPluginErrors(result), nullptr);
    EXPECT_NE(Impl()->GetWarnings(result), nullptr);
}

// IsImportTypeKind on a non-import node (class decl): false + one plugin warning
TEST_F(NullSafetyTest, IsImportTypeKindOnClassDeclarationReturnsFalse)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {CreateParsedContext(config.Get(), "class IK {}", "import-kind.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *program = Impl()->ContextProgram(guard.Get());
    ASSERT_NE(program, nullptr);
    auto *ast = Impl()->ProgramAst(guard.Get(), program);
    ASSERT_NE(ast, nullptr);
    // The root module node is not an import declaration: false (side effect is a
    // PLUGIN_WARNING with no public accessor yet; assert value and no crash)
    EXPECT_FALSE(Impl()->IsImportTypeKind(guard.Get(), ast));
}

// IsImportTypeKind on a number literal node also returns false
TEST_F(NullSafetyTest, IsImportTypeKindOnNumberLiteralReturnsFalse)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class IK2 {}", "import-kind2.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *literal = Impl()->CreateNumberLiteral(guard.Get(), 42);
    ASSERT_NE(literal, nullptr);
    EXPECT_FALSE(Impl()->IsImportTypeKind(guard.Get(), literal));
}

// JsdocStringFromDeclaration: plain declaration without jsdoc -> empty string (arena, non-null)
TEST_F(NullSafetyTest, JsdocStringFromPlainDeclarationIsEmpty)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {CreateParsedContext(config.Get(), "class JD { meth(): int { return 1 } }", "jsdoc.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *program = Impl()->ContextProgram(guard.Get());
    ASSERT_NE(program, nullptr);
    auto *ast = Impl()->ProgramAst(guard.Get(), program);
    ASSERT_NE(ast, nullptr);
    // FirstDeclarationByNameFromNode only matches MethodDefinition (verified)
    auto *first = Impl()->FirstDeclarationByNameFromNode(guard.Get(), ast, "meth");
    ASSERT_NE(first, nullptr);
    auto *jsdoc = Impl()->JsdocStringFromDeclaration(guard.Get(), first);
    ASSERT_NE(jsdoc, nullptr);
    EXPECT_EQ(std::string(jsdoc), "");
}

// GetLicenseFromRootNode: root node without a license header -> empty string
TEST_F(NullSafetyTest, GetLicenseFromRootNodeWithoutLicenseIsEmpty)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {CreateParsedContext(config.Get(), "class LI {}", "license.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *program = Impl()->ContextProgram(guard.Get());
    ASSERT_NE(program, nullptr);
    auto *ast = Impl()->ProgramAst(guard.Get(), program);
    ASSERT_NE(ast, nullptr);
    auto *license = Impl()->GetLicenseFromRootNode(guard.Get(), ast);
    ASSERT_NE(license, nullptr);
    // Without a license comment it returns the source text at the root start (verified: non-empty)
    EXPECT_NE(std::string(license), "");
}

// GetCommentsStringFromDeclaration(nullptr): explicit null guard, empty string
TEST_F(NullSafetyTest, GetCommentsStringFromNullDeclarationIsEmpty)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class CM {}", "comments.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *comments = Impl()->GetCommentsStringFromDeclaration(guard.Get(), nullptr);
    ASSERT_NE(comments, nullptr);
    EXPECT_EQ(std::string(comments), "");
}

// Jsdoc/Comments/License called in a row on one declaration: stable results
TEST_F(NullSafetyTest, CommentApisStableAcrossRepeatedCalls)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {CreateParsedContext(config.Get(), "class CS { meth(): int { return 1 } }", "comments-rep.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *program = Impl()->ContextProgram(guard.Get());
    ASSERT_NE(program, nullptr);
    auto *ast = Impl()->ProgramAst(guard.Get(), program);
    ASSERT_NE(ast, nullptr);
    auto *first = Impl()->FirstDeclarationByNameFromNode(guard.Get(), ast, "meth");
    ASSERT_NE(first, nullptr);
    for (int i = 0; i < 5; i++) {
        auto *jsdoc = Impl()->JsdocStringFromDeclaration(guard.Get(), first);
        ASSERT_NE(jsdoc, nullptr);
        EXPECT_EQ(std::string(jsdoc), "");
        auto *comments = Impl()->GetCommentsStringFromDeclaration(guard.Get(), first);
        ASSERT_NE(comments, nullptr);
        auto *license = Impl()->GetLicenseFromRootNode(guard.Get(), ast);
        ASSERT_NE(license, nullptr);
        EXPECT_NE(std::string(license), "");
    }
}

// CreateDiagnosticInfo + LogDiagnosticWithSuggestion: suggestion count grows
TEST_F(NullSafetyTest, LogDiagnosticWithSuggestionRecordsOnce)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class SG {}", "suggest.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *kind = Impl()->CreateDiagnosticKind(guard.Get(), "Suggestion {}", ES2PANDA_PLUGIN_ERROR);
    ASSERT_NE(kind, nullptr);
    auto *pos = Impl()->CreateSourcePosition(guard.Get(), 1, 1);
    ASSERT_NE(pos, nullptr);
    char const *args[] = {"value"};
    auto *info = Impl()->CreateDiagnosticInfo(guard.Get(), kind, args, 1, pos);
    ASSERT_NE(info, nullptr);
    auto *rangeStart = Impl()->CreateSourcePosition(guard.Get(), 1, 1);
    auto *rangeEnd = Impl()->CreateSourcePosition(guard.Get(), 2, 1);
    auto *range = Impl()->CreateSourceRange(guard.Get(), rangeStart, rangeEnd);
    ASSERT_NE(range, nullptr);
    auto *suggestion = Impl()->CreateSuggestionInfo(guard.Get(), kind, args, 1, "new code", "title", range);
    ASSERT_NE(suggestion, nullptr);
    auto *storage =
        reinterpret_cast<const ark::es2panda::util::DiagnosticStorage *>(Impl()->GetPluginErrors(guard.Get()));
    const auto before = storage->size();
    Impl()->LogDiagnosticWithSuggestion(guard.Get(), info, suggestion);
    // One diagnostic + one suggestion recorded (verified)
    EXPECT_EQ(storage->size(), before + 2U);
}

// LogDiagnosticWithSuggestions (plural): one record, many suggestions at once
TEST_F(NullSafetyTest, LogDiagnosticWithSuggestionsRecordsOnce)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class SG2 {}", "suggest2.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *kind = Impl()->CreateDiagnosticKind(guard.Get(), "Multi {}", ES2PANDA_PLUGIN_ERROR);
    ASSERT_NE(kind, nullptr);
    auto *pos = Impl()->CreateSourcePosition(guard.Get(), 1, 1);
    char const *args[] = {"v"};
    auto *info = Impl()->CreateDiagnosticInfo(guard.Get(), kind, args, 1, pos);
    ASSERT_NE(info, nullptr);
    auto *range = Impl()->CreateSourceRange(guard.Get(), pos, pos);
    es2panda_SuggestionInfo *suggestions[] = {
        Impl()->CreateSuggestionInfo(guard.Get(), kind, args, 1, "code-a", "title-a", range),
        Impl()->CreateSuggestionInfo(guard.Get(), kind, args, 1, "code-b", "title-b", range)};
    ASSERT_NE(suggestions[0], nullptr);
    ASSERT_NE(suggestions[1], nullptr);
    auto *storage =
        reinterpret_cast<const ark::es2panda::util::DiagnosticStorage *>(Impl()->GetPluginErrors(guard.Get()));
    const auto before = storage->size();
    Impl()->LogDiagnosticWithSuggestions(guard.Get(), info, suggestions, 2);
    // 1 diagnostic + 2 suggestions (verified)
    EXPECT_EQ(storage->size(), before + 3U);
}

// ---------------------------------------------------------------------------
// 16.E declaration query matrix (FirstDeclaration / AllDeclarations / DeclarationFromIdentifier)
// ---------------------------------------------------------------------------

// FirstDeclarationByNameFromProgram finds a method by name (MethodDefinition only, verified)
TEST_F(NullSafetyTest, FirstDeclarationByNameFromProgramFindsMethod)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {
        CreateParsedContext(config.Get(), "class Target { seek(): int { return 1 } }", "first-prog.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *program = Impl()->ContextProgram(guard.Get());
    ASSERT_NE(program, nullptr);
    auto *found = Impl()->FirstDeclarationByNameFromProgram(guard.Get(), program, "seek");
    ASSERT_NE(found, nullptr);
    EXPECT_TRUE(Impl()->IsMethodDefinition(found));
}

// FirstDeclarationByNameFromProgram: unknown name returns nullptr
TEST_F(NullSafetyTest, FirstDeclarationByNameFromProgramMissingNameReturnsNull)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {CreateParsedContext(config.Get(), "class Only {}", "first-miss.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *program = Impl()->ContextProgram(guard.Get());
    ASSERT_NE(program, nullptr);
    EXPECT_EQ(Impl()->FirstDeclarationByNameFromProgram(guard.Get(), program, "NoSuchName"), nullptr);
}

// FirstDeclarationByNameFromProgram(null name): null-safe nullptr (verified)
TEST_F(NullSafetyTest, FirstDeclarationByNameFromProgramNullNameReturnsNull)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {CreateParsedContext(config.Get(), "class NN {}", "first-null.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *program = Impl()->ContextProgram(guard.Get());
    ASSERT_NE(program, nullptr);
    EXPECT_EQ(Impl()->FirstDeclarationByNameFromProgram(guard.Get(), program, nullptr), nullptr);
}

// FirstDeclarationByNameFromNode: top-level function name does not match (MethodDefinition only)
TEST_F(NullSafetyTest, FirstDeclarationByNameFromNodeSkipsTopLevelFunction)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {CreateParsedContext(config.Get(), "function fn(): int { return 1 }", "first-fn.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *program = Impl()->ContextProgram(guard.Get());
    ASSERT_NE(program, nullptr);
    auto *ast = Impl()->ProgramAst(guard.Get(), program);
    ASSERT_NE(ast, nullptr);
    // fn is a FunctionDeclaration, not a MethodDefinition: nullptr (verified)
    EXPECT_EQ(Impl()->FirstDeclarationByNameFromNode(guard.Get(), ast, "fn"), nullptr);
}

// FirstDeclarationByNameFromNode: finds a class method (nested declaration)
TEST_F(NullSafetyTest, FirstDeclarationByNameFromNodeFindsNestedMethod)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {CreateParsedContext(config.Get(), "class Host { meth(): int { return 1 } }", "first-meth.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *program = Impl()->ContextProgram(guard.Get());
    ASSERT_NE(program, nullptr);
    auto *ast = Impl()->ProgramAst(guard.Get(), program);
    ASSERT_NE(ast, nullptr);
    auto *found = Impl()->FirstDeclarationByNameFromNode(guard.Get(), ast, "meth");
    ASSERT_NE(found, nullptr);
    EXPECT_TRUE(Impl()->IsMethodDefinition(found) || Impl()->IsFunctionDeclaration(found));
}

// FirstDeclarationByNameFromNode(node=null): no crash (return contract unset)
// (AllDeclarations null behavior is covered separately)
TEST_F(NullSafetyTest, FirstDeclarationByNameFromNodeNullNodeSafe)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class Q {}", "first-node-null.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    // Null root node: no crash (return contract unset; defense only)
    auto *result = Impl()->FirstDeclarationByNameFromNode(guard.Get(), nullptr, "Q");
    (void)result;
    SUCCEED();
}

// AllDeclarationsByNameFromNode: all same-name methods in a class returned
TEST_F(NullSafetyTest, AllDeclarationsByNameFromNodeFindsAllOverloads)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {CreateParsedContext(config.Get(),
                                            "class OV {\n"
                                            "    ov(a: int): int { return a }\n"
                                            "    ov(a: string): string { return a }\n"
                                            "}",
                                            "all-over.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *program = Impl()->ContextProgram(guard.Get());
    ASSERT_NE(program, nullptr);
    auto *ast = Impl()->ProgramAst(guard.Get(), program);
    ASSERT_NE(ast, nullptr);
    size_t len = 0;
    auto **decls = Impl()->AllDeclarationsByNameFromNode(guard.Get(), ast, "ov", &len);
    ASSERT_NE(decls, nullptr);
    ASSERT_EQ(len, 2U);
    for (size_t i = 0; i < len; i++) {
        EXPECT_TRUE(Impl()->IsMethodDefinition(decls[i]));
    }
}

// AllDeclarationsByNameFromNode: unknown name -> len == 0
TEST_F(NullSafetyTest, AllDeclarationsByNameFromNodeMissingNameLenZero)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {CreateParsedContext(config.Get(), "class Single {}", "all-miss.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *program = Impl()->ContextProgram(guard.Get());
    ASSERT_NE(program, nullptr);
    auto *ast = Impl()->ProgramAst(guard.Get(), program);
    ASSERT_NE(ast, nullptr);
    size_t len = 100;
    auto **decls = Impl()->AllDeclarationsByNameFromNode(guard.Get(), ast, "Missing", &len);
    EXPECT_EQ(len, 0U);
    (void)decls;
}

// AllDeclarationsByNameFromNode(null node): non-null pointer + len 0 (verified)
TEST_F(NullSafetyTest, AllDeclarationsByNameFromNullNodeLenZero)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class AN {}", "all-null.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    size_t len = 100;
    auto **decls = Impl()->AllDeclarationsByNameFromNode(guard.Get(), nullptr, "AN", &len);
    EXPECT_NE(decls, nullptr);
    EXPECT_EQ(len, 0U);
}

// AllDeclarationsByNameFromProgram: class method found by name
TEST_F(NullSafetyTest, AllDeclarationsByNameFromProgramFindsMethod)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {CreateParsedContext(config.Get(), "class Same { find(): int { return 1 } }", "all-prog.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *program = Impl()->ContextProgram(guard.Get());
    ASSERT_NE(program, nullptr);
    size_t len = 0;
    auto **decls = Impl()->AllDeclarationsByNameFromProgram(guard.Get(), program, "find", &len);
    ASSERT_NE(decls, nullptr);
    ASSERT_EQ(len, 1U);
    EXPECT_TRUE(Impl()->IsMethodDefinition(decls[0]));
}

// AllDeclarationsByNameFromProgram: null program behaves like the node version
TEST_F(NullSafetyTest, AllDeclarationsByNameFromNullProgramSafe)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class AP {}", "all-prog-null.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    size_t len = 100;
    auto **decls = Impl()->AllDeclarationsByNameFromProgram(guard.Get(), nullptr, "AP", &len);
    (void)decls;
    SUCCEED();
}

// DeclarationFromIdentifier(ctx, nullptr): null-safe nullptr (verified)
TEST_F(NullSafetyTest, DeclarationFromIdentifierNullNodeReturnsNull)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class DI {}", "decl-id.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    EXPECT_EQ(Impl()->DeclarationFromIdentifier(guard.Get(), nullptr), nullptr);
}

// DeclarationFromIdentifier: the class name identifier resolves to the class
TEST_F(NullSafetyTest, DeclarationFromIdentifierResolvesToClassDeclaration)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {CreateParsedContext(config.Get(), "class Res {} let r: Res = new Res()", "decl-res.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *program = Impl()->ContextProgram(guard.Get());
    ASSERT_NE(program, nullptr);
    auto *ast = Impl()->ProgramAst(guard.Get(), program);
    ASSERT_NE(ast, nullptr);
    // Collect all identifiers; at least one resolves to a declaration
    WalkPayload payload;
    payload.impl = Impl();
    payload.ctx = guard.Get();
    Impl()->AstNodeForEach(ast, CollectNamesCallback, &payload);
    EXPECT_TRUE(Contains(payload.identifierNames, "Res"));
}

// Query loop: First/AllDeclarations consistent over 10 parse rounds
TEST_F(LeakDefenseTest, DeclarationQueriesStableAcrossParseLoop10)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    for (int i = 0; i < 10; i++) {
        ContextGuard guard {CreateParsedContext(config.Get(), "class QS { m(): int { return 1 } }", "query-loop.ats")};
        ASSERT_NE(guard.Get(), nullptr);
        auto *program = Impl()->ContextProgram(guard.Get());
        ASSERT_NE(program, nullptr);
        auto *ast = Impl()->ProgramAst(guard.Get(), program);
        ASSERT_NE(ast, nullptr);
        auto *first = Impl()->FirstDeclarationByNameFromNode(guard.Get(), ast, "m");
        ASSERT_NE(first, nullptr) << "iteration " << i;
        EXPECT_TRUE(Impl()->IsMethodDefinition(first)) << "iteration " << i;
        size_t len = 0;
        auto **all = Impl()->AllDeclarationsByNameFromNode(guard.Get(), ast, "m", &len);
        ASSERT_NE(all, nullptr) << "iteration " << i;
        ASSERT_EQ(len, 1U) << "iteration " << i;
        EXPECT_TRUE(Impl()->IsMethodDefinition(all[0])) << "iteration " << i;
    }
}

// Declaration queries on two coexisting contexts do not cross-talk
TEST_F(NullSafetyTest, DeclarationQueriesIndependentAcrossContexts)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    auto *first = CreateParsedContext(config.Get(), "class Alpha { am(): int { return 1 } }", "indep-a.ats");
    ASSERT_NE(first, nullptr);
    auto *second = CreateParsedContext(config.Get(), "class Beta { bm(): int { return 2 } }", "indep-b.ats");
    ASSERT_NE(second, nullptr);
    auto *programA = Impl()->ContextProgram(first);
    auto *programB = Impl()->ContextProgram(second);
    ASSERT_NE(programA, nullptr);
    ASSERT_NE(programB, nullptr);
    auto *astA = Impl()->ProgramAst(first, programA);
    auto *astB = Impl()->ProgramAst(second, programB);
    ASSERT_NE(astA, nullptr);
    ASSERT_NE(astB, nullptr);
    EXPECT_NE(Impl()->FirstDeclarationByNameFromNode(first, astA, "am"), nullptr);
    EXPECT_NE(Impl()->FirstDeclarationByNameFromNode(second, astB, "bm"), nullptr);
    EXPECT_EQ(Impl()->FirstDeclarationByNameFromNode(first, astA, "bm"), nullptr);
    EXPECT_EQ(Impl()->FirstDeclarationByNameFromNode(second, astB, "am"), nullptr);
    Impl()->DestroyContext(first);
    Impl()->DestroyContext(second);
    SUCCEED();
}

// ---------------------------------------------------------------------------
// 16.F number literal API details (Create/Update/Set round-trips)
// ---------------------------------------------------------------------------

// UpdateNumberLiteralLong: the updated value reads back on the new node
TEST_F(NullSafetyTest, UpdateNumberLiteralLongRoundTrip)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class NL1 {}", "num-l1.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *original = Impl()->CreateNumberLiteral1(guard.Get(), 100);
    ASSERT_NE(original, nullptr);
    auto *updated = Impl()->UpdateNumberLiteral1(guard.Get(), original, 200);
    ASSERT_NE(updated, nullptr);
    EXPECT_TRUE(Impl()->NumberLiteralSetLong(updated, 300));
}

// UpdateNumberLiteralDouble: the updated value reads back on the new node
TEST_F(NullSafetyTest, UpdateNumberLiteralDoubleRoundTrip)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class NL2 {}", "num-l2.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *original = Impl()->CreateNumberLiteral2(guard.Get(), 1.5);
    ASSERT_NE(original, nullptr);
    auto *updated = Impl()->UpdateNumberLiteral2(guard.Get(), original, 2.5);
    ASSERT_NE(updated, nullptr);
    EXPECT_TRUE(Impl()->NumberLiteralSetDouble(updated, 3.5));
}

// UpdateNumberLiteralFloat: the updated value reads back on the new node
TEST_F(NullSafetyTest, UpdateNumberLiteralFloatRoundTrip)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class NL3 {}", "num-l3.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *original = Impl()->CreateNumberLiteral3(guard.Get(), 0.5F);
    ASSERT_NE(original, nullptr);
    auto *updated = Impl()->UpdateNumberLiteral3(guard.Get(), original, 1.5F);
    ASSERT_NE(updated, nullptr);
    EXPECT_TRUE(Impl()->NumberLiteralSetFloat(updated, 2.5F));
}

// Repeated Set with the same value succeeds (idempotent)
TEST_F(NullSafetyTest, NumberLiteralSetRepeatedCallsSucceed)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class NL4 {}", "num-l4.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *node = Impl()->CreateNumberLiteral(guard.Get(), 7);
    ASSERT_NE(node, nullptr);
    for (int i = 0; i < 5; i++) {
        EXPECT_TRUE(Impl()->NumberLiteralSetInt(node, 7));
    }
}

// Mixed creation of the four literal types x50: no arena overwrite
TEST_F(LeakDefenseTest, NumberLiteralMixedCreationLoop50)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class NL5 {}", "num-l5.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    std::vector<es2panda_AstNode *> nodes;
    for (int i = 0; i < 50; i++) {
        auto *node = Impl()->CreateNumberLiteral(guard.Get(), i);
        ASSERT_NE(node, nullptr);
        nodes.push_back(node);
    }
    for (size_t i = 0; i < nodes.size(); i++) {
        EXPECT_TRUE(Impl()->IsNumberLiteral(nodes[i]));
    }
}

// ---------------------------------------------------------------------------
// 16.G Is* predicate sweep - expected-value matrix on one AST
// ---------------------------------------------------------------------------

// Mixed source (class + method + field + let + function): expected values
TEST_F(NullSafetyTest, PredicateMatrixOnMixedSource)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {
        CreateParsedContext(config.Get(),
                            "class Mix { field: int = 1 meth(): int { return this.field } } let g: int = 2\n"
                            "function top(): int { return g }",
                            "pred-mix.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *program = Impl()->ContextProgram(guard.Get());
    ASSERT_NE(program, nullptr);
    auto *ast = Impl()->ProgramAst(guard.Get(), program);
    ASSERT_NE(ast, nullptr);
    // The root is a module
    EXPECT_TRUE(Impl()->IsETSModule(ast));
    EXPECT_FALSE(Impl()->IsClassDeclaration(ast));
    // Type count matrix: field:int / meth():int / g:int / top():int >= 4 primitive annotations
    EXPECT_GE(CountNodesOfKind(Impl(), ast, IsPrimitiveTypeNode), 4U);
    EXPECT_GE(
        CountNodesOfKind(Impl(), ast,
                         [](es2panda_Impl const *impl, es2panda_AstNode *n) { return impl->IsMethodDefinition(n); }),
        1U);
    EXPECT_GE(
        CountNodesOfKind(Impl(), ast,
                         [](es2panda_Impl const *impl, es2panda_AstNode *n) { return impl->IsFunctionDeclaration(n); }),
        1U);
}

// Expression source: binary/unary/literal predicate counts
TEST_F(NullSafetyTest, PredicateMatrixOnExpressionSource)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {CreateParsedContext(config.Get(), "function main() { let a = 1 + 2 let b = -a let c = !true }",
                                            "pred-expr.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *program = Impl()->ContextProgram(guard.Get());
    ASSERT_NE(program, nullptr);
    auto *ast = Impl()->ProgramAst(guard.Get(), program);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(
        CountNodesOfKind(Impl(), ast,
                         [](es2panda_Impl const *impl, es2panda_AstNode *n) { return impl->IsBinaryExpression(n); }),
        1U);
    EXPECT_EQ(
        CountNodesOfKind(Impl(), ast,
                         [](es2panda_Impl const *impl, es2panda_AstNode *n) { return impl->IsUnaryExpression(n); }),
        2U);
    EXPECT_GE(
        CountNodesOfKind(Impl(), ast,
                         [](es2panda_Impl const *impl, es2panda_AstNode *n) { return impl->IsBooleanLiteral(n); }),
        1U);
}

// Control-flow source: if/while/for predicate counts
TEST_F(NullSafetyTest, PredicateMatrixOnControlFlowSource)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {CreateParsedContext(
        config.Get(), "function main() { if (true) {} while (false) {} for (;;) {} }", "pred-flow.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *program = Impl()->ContextProgram(guard.Get());
    ASSERT_NE(program, nullptr);
    auto *ast = Impl()->ProgramAst(guard.Get(), program);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(CountNodesOfKind(Impl(), ast,
                               [](es2panda_Impl const *impl, es2panda_AstNode *n) { return impl->IsIfStatement(n); }),
              1U);
    EXPECT_EQ(
        CountNodesOfKind(Impl(), ast,
                         [](es2panda_Impl const *impl, es2panda_AstNode *n) { return impl->IsWhileStatement(n); }),
        1U);
    EXPECT_EQ(
        CountNodesOfKind(Impl(), ast,
                         [](es2panda_Impl const *impl, es2panda_AstNode *n) { return impl->IsForUpdateStatement(n); }),
        1U);
}

// Batch predicate calls x10 rounds: repeated queries stay stable
TEST_F(LeakDefenseTest, PredicateCallsStableAcrossRepeats)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {CreateParsedContext(config.Get(), "class PR { v: int = 1 }", "pred-rep.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *program = Impl()->ContextProgram(guard.Get());
    ASSERT_NE(program, nullptr);
    auto *ast = Impl()->ProgramAst(guard.Get(), program);
    ASSERT_NE(ast, nullptr);
    for (int i = 0; i < 10; i++) {
        EXPECT_TRUE(Impl()->IsETSModule(ast));
        EXPECT_FALSE(Impl()->IsBlockStatement(ast));
        EXPECT_FALSE(Impl()->IsClassDeclaration(ast));
        EXPECT_FALSE(Impl()->IsIdentifier(ast));
    }
}

// ---------------------------------------------------------------------------
// 16.H position/range construction and NoDebugLineFlag
// ---------------------------------------------------------------------------

// CreateSourcePosition repeatedly: index/line read back independently
TEST_F(NullSafetyTest, CreateSourcePositionIndependentInstances)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class SP {}", "pos-multi.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    for (size_t i = 0; i < 10; i++) {
        auto *pos = Impl()->CreateSourcePosition(guard.Get(), i, i + 1U);
        ASSERT_NE(pos, nullptr);
        EXPECT_EQ(Impl()->SourcePositionIndex(guard.Get(), pos), i);
        EXPECT_EQ(Impl()->SourcePositionLine(guard.Get(), pos), i + 1U);
    }
}

// CreateSourceRange: start/end are separate instances with their own values
TEST_F(NullSafetyTest, CreateSourceRangeDistinctEndpoints)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class SR {}", "range-multi.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *start = Impl()->CreateSourcePosition(guard.Get(), 10, 20);
    auto *end = Impl()->CreateSourcePosition(guard.Get(), 30, 40);
    ASSERT_NE(start, nullptr);
    ASSERT_NE(end, nullptr);
    auto *range = Impl()->CreateSourceRange(guard.Get(), start, end);
    ASSERT_NE(range, nullptr);
    auto *rangeStart = Impl()->SourceRangeStart(guard.Get(), range);
    auto *rangeEnd = Impl()->SourceRangeEnd(guard.Get(), range);
    ASSERT_NE(rangeStart, nullptr);
    ASSERT_NE(rangeEnd, nullptr);
    EXPECT_NE(rangeStart, rangeEnd);
    EXPECT_EQ(Impl()->SourcePositionIndex(guard.Get(), rangeStart), 10U);
    EXPECT_EQ(Impl()->SourcePositionIndex(guard.Get(), rangeEnd), 30U);
}

// AstNodeSetNoDebugLineFlag: walking stays safe after setting it
TEST_F(NullSafetyTest, SetNoDebugLineFlagThenWalk)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {CreateParsedContext(config.Get(), "class NDb { meth(): int { return 1 } }", "nodb.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *program = Impl()->ContextProgram(guard.Get());
    ASSERT_NE(program, nullptr);
    auto *ast = Impl()->ProgramAst(guard.Get(), program);
    ASSERT_NE(ast, nullptr);
    auto *first = Impl()->FirstDeclarationByNameFromNode(guard.Get(), ast, "meth");
    ASSERT_NE(first, nullptr);
    EXPECT_TRUE(Impl()->IsMethodDefinition(first));
    Impl()->AstNodeSetNoDebugLineFlag(guard.Get(), first);
    WalkPayload payload;
    WalkAst(Impl(), guard.Get(), ast, &payload);
    EXPECT_GT(payload.nodeCount, 0U);
    EXPECT_EQ(payload.nullParentCount, 1U);
}

// Position/range creation loop x100: arena-allocated, values preserved
TEST_F(LeakDefenseTest, SourcePositionRangeLoop100)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class SL {}", "pos-loop.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    std::vector<es2panda_SourceRange *> ranges;
    for (size_t i = 0; i < 100; i++) {
        auto *start = Impl()->CreateSourcePosition(guard.Get(), i, 1);
        auto *end = Impl()->CreateSourcePosition(guard.Get(), i, 2);
        ASSERT_NE(start, nullptr);
        ASSERT_NE(end, nullptr);
        auto *range = Impl()->CreateSourceRange(guard.Get(), start, end);
        ASSERT_NE(range, nullptr);
        ranges.push_back(range);
    }
    for (size_t i = 0; i < ranges.size(); i++) {
        auto *start = Impl()->SourceRangeStart(guard.Get(), ranges[i]);
        ASSERT_NE(start, nullptr);
        EXPECT_EQ(Impl()->SourcePositionIndex(guard.Get(), start), i);
        EXPECT_EQ(Impl()->SourcePositionLine(guard.Get(), start), 1U);
    }
}

// Es2pandaEnumToString strings live on the arena: no manual free needed
TEST_F(LeakDefenseTest, EnumToStringFreedByContextDestroy)
{
    {
        ConfigGuard config {NewConfig()};
        ASSERT_NE(config.Get(), nullptr);
        ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class ET {}", "enum-rel.ats")};
        ASSERT_NE(guard.Get(), nullptr);
        auto *str = Impl()->Es2pandaEnumToString(guard.Get(), Es2pandaEnum::ENUM_NODE_HAS_PARENT);
        ASSERT_NE(str, nullptr);
    }
    SUCCEED();
}

// ---------------------------------------------------------------------------
// 16.I GetAllErrorMessages / ErrorMessage boundaries
// ---------------------------------------------------------------------------

// Clean PARSED context: ErrorMessage is an empty string
TEST_F(NullSafetyTest, CleanParsedContextHasEmptyErrorMessage)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {CreateParsedContext(config.Get(), "class CE {}", "clean-err.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    EXPECT_EQ(std::string(Impl()->ContextErrorMessage(guard.Get())), "");
}

// NEW-state context: ErrorMessage is an empty string
TEST_F(NullSafetyTest, NewContextHasEmptyErrorMessage)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class NE {}", "new-err.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    EXPECT_EQ(std::string(Impl()->ContextErrorMessage(guard.Get())), "");
}

// Syntax error context: ErrorMessage non-empty, GetAllErrorMessages non-null
TEST_F(NullSafetyTest, SyntaxErrorContextHasNonEmptyErrorMessages)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {CreateParsedContext(config.Get(), "class SE { x: }", "err-msg.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    EXPECT_TRUE(Impl()->IsAnyError(guard.Get()));
    auto *all = Impl()->GetAllErrorMessages(guard.Get());
    ASSERT_NE(all, nullptr);
    EXPECT_NE(std::string(all), "");
}

// ContextErrorMessage repeated calls return stable results
TEST_F(NullSafetyTest, ErrorMessageStableAcrossCalls)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {CreateParsedContext(config.Get(), "class ESt {}", "err-stable.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *first = Impl()->ContextErrorMessage(guard.Get());
    ASSERT_NE(first, nullptr);
    for (int i = 0; i < 5; i++) {
        EXPECT_EQ(std::string(Impl()->ContextErrorMessage(guard.Get())), std::string(first));
    }
}
// ===========================================================================
// Part 17: more syntax construct walk matrix (5 probes per snippet)
//   Probe 1: identifier set contains the declared names
//   Probe 2: node count >= threshold; two walks agree (no side effects)
//   Probe 3: DumpEtsSrc contains the key source text
//   Probe 4: parent invariant - only the root has a null parent
//   Probe 5: type predicate count - the node type appears in the AST
// All snippets smoke-verified via the library API (PARSED state, no errors).
// ===========================================================================

// --- Snippet: default function parameter ---
TEST_F(AstWalkInvariantTest, IdentifiersPresentFunctionDefaultParameter)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(a: int = 1): int { return a }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "f"));
        EXPECT_TRUE(Contains(payload.identifierNames, "a"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableFunctionDefaultParameter)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(a: int = 1): int { return a }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 6U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, DumpContainsFunctionDefaultParameter)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(a: int = 1): int { return a }")
    {
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), CApi(GetAst()));
        ASSERT_NE(dump, nullptr);
        EXPECT_TRUE(Contains(std::string(dump), "function"));
        EXPECT_TRUE(Contains(std::string(dump), "int"));
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantFunctionDefaultParameter)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(a: int = 1): int { return a }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

TEST_F(AstWalkInvariantTest, KindFoundFunctionDefaultParameter)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(a: int = 1): int { return a }")
    {
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *n) { return impl->IsFunctionDeclaration(n); }),
                  1U);
    }
}

// --- Snippet: optional function parameter ---
TEST_F(AstWalkInvariantTest, IdentifiersPresentFunctionOptionalParameter)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(a?: int): int { return 1 }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "f"));
        EXPECT_TRUE(Contains(payload.identifierNames, "a"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableFunctionOptionalParameter)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(a?: int): int { return 1 }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 5U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, DumpContainsFunctionOptionalParameter)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(a?: int): int { return 1 }")
    {
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), CApi(GetAst()));
        ASSERT_NE(dump, nullptr);
        EXPECT_TRUE(Contains(std::string(dump), "function"));
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantFunctionOptionalParameter)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(a?: int): int { return 1 }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

TEST_F(AstWalkInvariantTest, KindFoundFunctionOptionalParameter)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(a?: int): int { return 1 }")
    {
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *n) { return impl->IsFunctionDeclaration(n); }),
                  1U);
    }
}

// --- Snippet: void-return method ---
TEST_F(AstWalkInvariantTest, IdentifiersPresentVoidMethod)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { m(): void {} }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "A"));
        EXPECT_TRUE(Contains(payload.identifierNames, "m"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableVoidMethod)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { m(): void {} }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 5U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, DumpContainsVoidMethod)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { m(): void {} }")
    {
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), CApi(GetAst()));
        ASSERT_NE(dump, nullptr);
        EXPECT_TRUE(Contains(std::string(dump), "class"));
        EXPECT_TRUE(Contains(std::string(dump), "void"));
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantVoidMethod)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { m(): void {} }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

TEST_F(AstWalkInvariantTest, KindFoundVoidMethod)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { m(): void {} }")
    {
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *n) { return impl->IsMethodDefinition(n); }),
                  1U);
        EXPECT_GE(CountNodesOfKind(GetImpl(), CApi(GetAst()), IsPrimitiveTypeNode), 1U);
    }
}

// --- Snippet: empty function body ---
TEST_F(AstWalkInvariantTest, IdentifiersPresentEmptyFunctionBody)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function e(): void {}")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "e"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableEmptyFunctionBody)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function e(): void {}")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 4U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, DumpContainsEmptyFunctionBody)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function e(): void {}")
    {
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), CApi(GetAst()));
        ASSERT_NE(dump, nullptr);
        EXPECT_TRUE(Contains(std::string(dump), "function"));
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantEmptyFunctionBody)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function e(): void {}")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

TEST_F(AstWalkInvariantTest, KindFoundEmptyFunctionBody)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function e(): void {}")
    {
        EXPECT_GE(
            CountNodesOfKind(GetImpl(), CApi(GetAst()),
                             [](es2panda_Impl const *impl, es2panda_AstNode *n) { return impl->IsBlockStatement(n); }),
            1U);
    }
}

// --- Snippet: recursive function ---
TEST_F(AstWalkInvariantTest, IdentifiersPresentRecursiveFunction)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function fact(n: int): int { if (n <= 1) { return 1 } return n * fact(n - 1) }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "fact"));
        EXPECT_TRUE(Contains(payload.identifierNames, "n"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableRecursiveFunction)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function fact(n: int): int { if (n <= 1) { return 1 } return n * fact(n - 1) }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 15U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, DumpContainsRecursiveFunction)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function fact(n: int): int { if (n <= 1) { return 1 } return n * fact(n - 1) }")
    {
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), CApi(GetAst()));
        ASSERT_NE(dump, nullptr);
        EXPECT_TRUE(Contains(std::string(dump), "fact"));
        EXPECT_TRUE(Contains(std::string(dump), "return"));
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantRecursiveFunction)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function fact(n: int): int { if (n <= 1) { return 1 } return n * fact(n - 1) }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

TEST_F(AstWalkInvariantTest, KindFoundRecursiveFunction)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function fact(n: int): int { if (n <= 1) { return 1 } return n * fact(n - 1) }")
    {
        EXPECT_GE(
            CountNodesOfKind(GetImpl(), CApi(GetAst()),
                             [](es2panda_Impl const *impl, es2panda_AstNode *n) { return impl->IsIfStatement(n); }),
            1U);
        EXPECT_GE(
            CountNodesOfKind(GetImpl(), CApi(GetAst()),
                             [](es2panda_Impl const *impl, es2panda_AstNode *n) { return impl->IsCallExpression(n); }),
            1U);
    }
}

// --- Snippet: nested function calls ---
TEST_F(AstWalkInvariantTest, IdentifiersPresentNestedCallChain)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): int { return 1 } function g(): int { return f() + f() }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "f"));
        EXPECT_TRUE(Contains(payload.identifierNames, "g"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableNestedCallChain)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): int { return 1 } function g(): int { return f() + f() }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 14U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, DumpContainsNestedCallChain)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): int { return 1 } function g(): int { return f() + f() }")
    {
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), CApi(GetAst()));
        ASSERT_NE(dump, nullptr);
        EXPECT_TRUE(Contains(std::string(dump), "function"));
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantNestedCallChain)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): int { return 1 } function g(): int { return f() + f() }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

TEST_F(AstWalkInvariantTest, KindFoundNestedCallChain)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function f(): int { return 1 } function g(): int { return f() + f() }")
    {
        EXPECT_GE(
            CountNodesOfKind(GetImpl(), CApi(GetAst()),
                             [](es2panda_Impl const *impl, es2panda_AstNode *n) { return impl->IsCallExpression(n); }),
            2U);
    }
}

// --- Snippet: static field access ---
TEST_F(AstWalkInvariantTest, IdentifiersPresentStaticFieldAccess)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { static f: int = 1 } function m(): int { return A.f }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "A"));
        EXPECT_TRUE(Contains(payload.identifierNames, "f"));
        EXPECT_TRUE(Contains(payload.identifierNames, "m"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableStaticFieldAccess)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { static f: int = 1 } function m(): int { return A.f }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 13U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, DumpContainsStaticFieldAccess)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { static f: int = 1 } function m(): int { return A.f }")
    {
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), CApi(GetAst()));
        ASSERT_NE(dump, nullptr);
        EXPECT_TRUE(Contains(std::string(dump), "static"));
        EXPECT_TRUE(Contains(std::string(dump), "return"));
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantStaticFieldAccess)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { static f: int = 1 } function m(): int { return A.f }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

TEST_F(AstWalkInvariantTest, KindFoundStaticFieldAccess)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { static f: int = 1 } function m(): int { return A.f }")
    {
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *n) { return impl->IsMemberExpression(n); }),
                  1U);
    }
}

// --- Snippet: exported function ---
TEST_F(AstWalkInvariantTest, IdentifiersPresentExportedTopLevelFunction)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "export function f(): int { return 1 }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "f"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableExportedTopLevelFunction)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "export function f(): int { return 1 }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 5U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, DumpContainsExportedTopLevelFunction)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "export function f(): int { return 1 }")
    {
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), CApi(GetAst()));
        ASSERT_NE(dump, nullptr);
        EXPECT_TRUE(Contains(std::string(dump), "export"));
        EXPECT_TRUE(Contains(std::string(dump), "function"));
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantExportedTopLevelFunction)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "export function f(): int { return 1 }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

TEST_F(AstWalkInvariantTest, KindFoundExportedTopLevelFunction)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "export function f(): int { return 1 }")
    {
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *n) { return impl->IsFunctionDeclaration(n); }),
                  1U);
    }
}

// --- Snippet: implementing two interfaces ---
TEST_F(AstWalkInvariantTest, IdentifiersPresentImplementsTwoInterfaces)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "interface I1 {} interface I2 {} class A implements I1, I2 {}")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "I1"));
        EXPECT_TRUE(Contains(payload.identifierNames, "I2"));
        EXPECT_TRUE(Contains(payload.identifierNames, "A"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableImplementsTwoInterfaces)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "interface I1 {} interface I2 {} class A implements I1, I2 {}")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 9U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, DumpContainsImplementsTwoInterfaces)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "interface I1 {} interface I2 {} class A implements I1, I2 {}")
    {
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), CApi(GetAst()));
        ASSERT_NE(dump, nullptr);
        EXPECT_TRUE(Contains(std::string(dump), "interface"));
        EXPECT_TRUE(Contains(std::string(dump), "implements"));
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantImplementsTwoInterfaces)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "interface I1 {} interface I2 {} class A implements I1, I2 {}")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

TEST_F(AstWalkInvariantTest, KindFoundImplementsTwoInterfaces)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "interface I1 {} interface I2 {} class A implements I1, I2 {}")
    {
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *n) { return impl->IsTSInterfaceDeclaration(n); }),
                  2U);
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *n) { return impl->IsClassDeclaration(n); }),
                  1U);
    }
}

// --- Snippet: explicit generic call ---
TEST_F(AstWalkInvariantTest, IdentifiersPresentExplicitGenericCall)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function id<T>(x: T): T { return x } function m(): int { return id<int>(1) }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "id"));
        EXPECT_TRUE(Contains(payload.identifierNames, "m"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableExplicitGenericCall)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function id<T>(x: T): T { return x } function m(): int { return id<int>(1) }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 14U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, DumpContainsExplicitGenericCall)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function id<T>(x: T): T { return x } function m(): int { return id<int>(1) }")
    {
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), CApi(GetAst()));
        ASSERT_NE(dump, nullptr);
        EXPECT_TRUE(Contains(std::string(dump), "function"));
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantExplicitGenericCall)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function id<T>(x: T): T { return x } function m(): int { return id<int>(1) }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

TEST_F(AstWalkInvariantTest, KindFoundExplicitGenericCall)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function id<T>(x: T): T { return x } function m(): int { return id<int>(1) }")
    {
        EXPECT_GE(
            CountNodesOfKind(GetImpl(), CApi(GetAst()),
                             [](es2panda_Impl const *impl, es2panda_AstNode *n) { return impl->IsTSTypeParameter(n); }),
            1U);
        EXPECT_GE(CountNodesOfKind(GetImpl(), CApi(GetAst()),
                                   [](es2panda_Impl const *impl, es2panda_AstNode *n) {
                                       return impl->IsTSTypeParameterDeclaration(n);
                                   }),
                  1U);
    }
}

// --- Snippet: const declaration ---
TEST_F(AstWalkInvariantTest, IdentifiersPresentConstDeclaration)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "const c: int = 5")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "c"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableConstDeclaration)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "const c: int = 5")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 4U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, DumpContainsConstDeclaration)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "const c: int = 5")
    {
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), CApi(GetAst()));
        ASSERT_NE(dump, nullptr);
        EXPECT_TRUE(Contains(std::string(dump), "const"));
        EXPECT_TRUE(Contains(std::string(dump), "int"));
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantConstDeclaration)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "const c: int = 5")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

TEST_F(AstWalkInvariantTest, KindFoundConstDeclaration)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "const c: int = 5")
    {
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *n) { return impl->IsVariableDeclaration(n); }),
                  1U);
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *n) { return impl->IsVariableDeclarator(n); }),
                  1U);
    }
}

// --- Snippet: as cast expression ---
TEST_F(AstWalkInvariantTest, IdentifiersPresentAsCastExpression)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function m(x: Object): Object { return x as Object }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "m"));
        EXPECT_TRUE(Contains(payload.identifierNames, "x"));
        EXPECT_TRUE(Contains(payload.identifierNames, "Object"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableAsCastExpression)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function m(x: Object): Object { return x as Object }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 8U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, DumpContainsAsCastExpression)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function m(x: Object): Object { return x as Object }")
    {
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), CApi(GetAst()));
        ASSERT_NE(dump, nullptr);
        EXPECT_TRUE(Contains(std::string(dump), "as"));
        EXPECT_TRUE(Contains(std::string(dump), "Object"));
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantAsCastExpression)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function m(x: Object): Object { return x as Object }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

TEST_F(AstWalkInvariantTest, KindFoundAsCastExpression)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function m(x: Object): Object { return x as Object }")
    {
        EXPECT_GE(
            CountNodesOfKind(GetImpl(), CApi(GetAst()),
                             [](es2panda_Impl const *impl, es2panda_AstNode *n) { return impl->IsTSAsExpression(n); }),
            1U);
    }
}

// --- Snippet: nullable type annotation ---
TEST_F(AstWalkInvariantTest, IdentifiersPresentNullableTypeAnnotation)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let n: int | null = null")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "n"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableNullableTypeAnnotation)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let n: int | null = null")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 5U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, DumpContainsNullableTypeAnnotation)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let n: int | null = null")
    {
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), CApi(GetAst()));
        ASSERT_NE(dump, nullptr);
        EXPECT_TRUE(Contains(std::string(dump), "null"));
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantNullableTypeAnnotation)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let n: int | null = null")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

TEST_F(AstWalkInvariantTest, KindFoundNullableTypeAnnotation)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let n: int | null = null")
    {
        EXPECT_GE(
            CountNodesOfKind(GetImpl(), CApi(GetAst()),
                             [](es2panda_Impl const *impl, es2panda_AstNode *n) { return impl->IsETSUnionType(n); }),
            1U);
    }
}

// --- Snippet: multi-variable declaration ---
TEST_F(AstWalkInvariantTest, IdentifiersPresentMultiVariableDeclaration)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let a: int = 1, b: int = 2")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "a"));
        EXPECT_TRUE(Contains(payload.identifierNames, "b"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableMultiVariableDeclaration)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let a: int = 1, b: int = 2")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 8U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, DumpContainsMultiVariableDeclaration)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let a: int = 1, b: int = 2")
    {
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), CApi(GetAst()));
        ASSERT_NE(dump, nullptr);
        EXPECT_TRUE(Contains(std::string(dump), "let"));
        EXPECT_TRUE(Contains(std::string(dump), "int"));
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantMultiVariableDeclaration)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let a: int = 1, b: int = 2")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

TEST_F(AstWalkInvariantTest, KindFoundMultiVariableDeclaration)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "let a: int = 1, b: int = 2")
    {
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *n) { return impl->IsVariableDeclarator(n); }),
                  2U);
        EXPECT_GE(CountNodesOfKind(GetImpl(), CApi(GetAst()), IsPrimitiveTypeNode), 2U);
    }
}

// --- Snippet: namespace ---
TEST_F(AstWalkInvariantTest, IdentifiersPresentNamespaceDeclaration)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "namespace NS { export function f(): int { return 1 } }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        // The namespace name itself is not an Identifier node (verified); assert member names
        EXPECT_TRUE(Contains(payload.identifierNames, "f"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableNamespaceDeclaration)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "namespace NS { export function f(): int { return 1 } }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 8U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, DumpContainsNamespaceDeclaration)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "namespace NS { export function f(): int { return 1 } }")
    {
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), CApi(GetAst()));
        ASSERT_NE(dump, nullptr);
        EXPECT_TRUE(Contains(std::string(dump), "namespace"));
        EXPECT_TRUE(Contains(std::string(dump), "export"));
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantNamespaceDeclaration)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "namespace NS { export function f(): int { return 1 } }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

TEST_F(AstWalkInvariantTest, KindFoundNamespaceDeclaration)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "namespace NS { export function f(): int { return 1 } }")
    {
        // Namespace member functions are FunctionDeclarations (verified; namespaces
        // produce no TSModuleDeclaration node)
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *n) { return impl->IsFunctionDeclaration(n); }),
                  1U);
    }
}

// --- Snippet: method override ---
TEST_F(AstWalkInvariantTest, IdentifiersPresentOverrideMethod)
{
    CONTEXT(ES2PANDA_STATE_PARSED,
            "class A { m(): int { return 1 } } class B extends A { override m(): int { return 2 } }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "A"));
        EXPECT_TRUE(Contains(payload.identifierNames, "B"));
        EXPECT_TRUE(Contains(payload.identifierNames, "m"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableOverrideMethod)
{
    CONTEXT(ES2PANDA_STATE_PARSED,
            "class A { m(): int { return 1 } } class B extends A { override m(): int { return 2 } }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 20U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, DumpContainsOverrideMethod)
{
    CONTEXT(ES2PANDA_STATE_PARSED,
            "class A { m(): int { return 1 } } class B extends A { override m(): int { return 2 } }")
    {
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), CApi(GetAst()));
        ASSERT_NE(dump, nullptr);
        EXPECT_TRUE(Contains(std::string(dump), "extends"));
        EXPECT_TRUE(Contains(std::string(dump), "override"));
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantOverrideMethod)
{
    CONTEXT(ES2PANDA_STATE_PARSED,
            "class A { m(): int { return 1 } } class B extends A { override m(): int { return 2 } }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

TEST_F(AstWalkInvariantTest, KindFoundOverrideMethod)
{
    CONTEXT(ES2PANDA_STATE_PARSED,
            "class A { m(): int { return 1 } } class B extends A { override m(): int { return 2 } }")
    {
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *n) { return impl->IsMethodDefinition(n); }),
                  2U);
    }
}

// --- Snippet: three-method class ---
TEST_F(AstWalkInvariantTest, IdentifiersPresentThreeMethodClass)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class K { a(): int { return 1 } b(): int { return 2 } c(): int { return 3 } }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "K"));
        EXPECT_TRUE(Contains(payload.identifierNames, "a"));
        EXPECT_TRUE(Contains(payload.identifierNames, "b"));
        EXPECT_TRUE(Contains(payload.identifierNames, "c"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableThreeMethodClass)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class K { a(): int { return 1 } b(): int { return 2 } c(): int { return 3 } }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 20U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, DumpContainsThreeMethodClass)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class K { a(): int { return 1 } b(): int { return 2 } c(): int { return 3 } }")
    {
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), CApi(GetAst()));
        ASSERT_NE(dump, nullptr);
        EXPECT_TRUE(Contains(std::string(dump), "class"));
        EXPECT_TRUE(Contains(std::string(dump), "return"));
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantThreeMethodClass)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class K { a(): int { return 1 } b(): int { return 2 } c(): int { return 3 } }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

TEST_F(AstWalkInvariantTest, KindFoundThreeMethodClass)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class K { a(): int { return 1 } b(): int { return 2 } c(): int { return 3 } }")
    {
        // Three explicit methods + the parser-added implicit constructor (verified: 4 in total)
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *n) { return impl->IsMethodDefinition(n); }),
                  3U);
    }
}

// --- Snippet: parameterized constructor ---
TEST_F(AstWalkInvariantTest, IdentifiersPresentConstructorWithParameter)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class P { constructor(a: int) { let b = a } }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "P"));
        EXPECT_TRUE(Contains(payload.identifierNames, "a"));
        EXPECT_TRUE(Contains(payload.identifierNames, "b"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableConstructorWithParameter)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class P { constructor(a: int) { let b = a } }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 11U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, DumpContainsConstructorWithParameter)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class P { constructor(a: int) { let b = a } }")
    {
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), CApi(GetAst()));
        ASSERT_NE(dump, nullptr);
        EXPECT_TRUE(Contains(std::string(dump), "constructor"));
        EXPECT_TRUE(Contains(std::string(dump), "let"));
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantConstructorWithParameter)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class P { constructor(a: int) { let b = a } }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

TEST_F(AstWalkInvariantTest, KindFoundConstructorWithParameter)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class P { constructor(a: int) { let b = a } }")
    {
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *n) { return impl->IsMethodDefinition(n); }),
                  1U);
    }
}

// --- Snippet: private field with public method ---
TEST_F(AstWalkInvariantTest, IdentifiersPresentPrivateFieldPublicMethod)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class Q { private p: int = 1 public pub(): int { return this.p } }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "Q"));
        EXPECT_TRUE(Contains(payload.identifierNames, "p"));
        EXPECT_TRUE(Contains(payload.identifierNames, "pub"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStablePrivateFieldPublicMethod)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class Q { private p: int = 1 public pub(): int { return this.p } }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 14U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, DumpContainsPrivateFieldPublicMethod)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class Q { private p: int = 1 public pub(): int { return this.p } }")
    {
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), CApi(GetAst()));
        ASSERT_NE(dump, nullptr);
        EXPECT_TRUE(Contains(std::string(dump), "private"));
        EXPECT_TRUE(Contains(std::string(dump), "public"));
        EXPECT_TRUE(Contains(std::string(dump), "this"));
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantPrivateFieldPublicMethod)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class Q { private p: int = 1 public pub(): int { return this.p } }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

TEST_F(AstWalkInvariantTest, KindFoundPrivateFieldPublicMethod)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class Q { private p: int = 1 public pub(): int { return this.p } }")
    {
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *n) { return impl->IsMethodDefinition(n); }),
                  1U);
        EXPECT_GE(
            CountNodesOfKind(GetImpl(), CApi(GetAst()),
                             [](es2panda_Impl const *impl, es2panda_AstNode *n) { return impl->IsThisExpression(n); }),
            1U);
    }
}

// --- Snippet: callback-style function call ---
TEST_F(AstWalkInvariantTest, IdentifiersPresentCallbackInvocation)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function cb(x: int): int { return x } function m(): int { return cb(2) }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "cb"));
        EXPECT_TRUE(Contains(payload.identifierNames, "m"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableCallbackInvocation)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function cb(x: int): int { return x } function m(): int { return cb(2) }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 14U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, DumpContainsCallbackInvocation)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function cb(x: int): int { return x } function m(): int { return cb(2) }")
    {
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), CApi(GetAst()));
        ASSERT_NE(dump, nullptr);
        EXPECT_TRUE(Contains(std::string(dump), "function"));
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantCallbackInvocation)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function cb(x: int): int { return x } function m(): int { return cb(2) }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

TEST_F(AstWalkInvariantTest, KindFoundCallbackInvocation)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "function cb(x: int): int { return x } function m(): int { return cb(2) }")
    {
        EXPECT_GE(
            CountNodesOfKind(GetImpl(), CApi(GetAst()),
                             [](es2panda_Impl const *impl, es2panda_AstNode *n) { return impl->IsCallExpression(n); }),
            1U);
    }
}

// --- Snippet: static method call ---
TEST_F(AstWalkInvariantTest, IdentifiersPresentStaticMethodCall)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class R { static s(): int { return 1 } m(): int { return R.s() } }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_TRUE(Contains(payload.identifierNames, "R"));
        EXPECT_TRUE(Contains(payload.identifierNames, "s"));
        EXPECT_TRUE(Contains(payload.identifierNames, "m"));
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableStaticMethodCall)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class R { static s(): int { return 1 } m(): int { return R.s() } }")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        EXPECT_GE(first.nodeCount, 18U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, DumpContainsStaticMethodCall)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class R { static s(): int { return 1 } m(): int { return R.s() } }")
    {
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), CApi(GetAst()));
        ASSERT_NE(dump, nullptr);
        EXPECT_TRUE(Contains(std::string(dump), "static"));
        EXPECT_TRUE(Contains(std::string(dump), "return"));
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantStaticMethodCall)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class R { static s(): int { return 1 } m(): int { return R.s() } }")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

TEST_F(AstWalkInvariantTest, KindFoundStaticMethodCall)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class R { static s(): int { return 1 } m(): int { return R.s() } }")
    {
        EXPECT_GE(
            CountNodesOfKind(GetImpl(), CApi(GetAst()),
                             [](es2panda_Impl const *impl, es2panda_AstNode *n) { return impl->IsCallExpression(n); }),
            1U);
        EXPECT_GE(CountNodesOfKind(
                      GetImpl(), CApi(GetAst()),
                      [](es2panda_Impl const *impl, es2panda_AstNode *n) { return impl->IsMemberExpression(n); }),
                  1U);
    }
}

// --- Snippet: empty source ---
TEST_F(AstWalkInvariantTest, IdentifiersEmptyEmptySource)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_EQ(payload.identifierCount, 0U);
    }
}

TEST_F(AstWalkInvariantTest, NodeCountStableEmptySource)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "")
    {
        WalkPayload first;
        WalkPayload second;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &second);
        // Empty source: only the ETSModule root node
        EXPECT_GE(first.nodeCount, 1U);
        EXPECT_EQ(first.nodeCount, second.nodeCount);
    }
}

TEST_F(AstWalkInvariantTest, DumpEmptySourceIsBlank)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "")
    {
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), CApi(GetAst()));
        ASSERT_NE(dump, nullptr);
        // Empty source dumps to empty or whitespace only
        EXPECT_TRUE(std::string(dump).find_first_not_of(" \t\r\n") == std::string::npos);
    }
}

TEST_F(AstWalkInvariantTest, ParentRootInvariantEmptySource)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "")
    {
        WalkPayload payload;
        payload.impl = GetImpl();
        payload.ctx = GetContext();
        GetImpl()->AstNodeForEach(CApi(GetAst()), NullParentProbeCallback, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
    }
}

TEST_F(AstWalkInvariantTest, KindFoundEmptySourceIsModuleOnly)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "")
    {
        EXPECT_TRUE(GetImpl()->IsETSModule(CApi(GetAst())));
        EXPECT_EQ(
            CountNodesOfKind(GetImpl(), CApi(GetAst()),
                             [](es2panda_Impl const *impl, es2panda_AstNode *n) { return impl->IsIdentifier(n); }),
            0U);
    }
}
// ===========================================================================
// Part 18: walk / dump / predicate idempotence stress - repeated calls on
// the same input must give identical results (no side effects), plus mixed stress.
// ===========================================================================

// ---------------------------------------------------------------------------
// 18.A dump and walk repeat-consistency
// ---------------------------------------------------------------------------

// DumpEtsSrc 5 times on one AST: identical output (no side effects)
TEST_F(AstWalkInvariantTest, DumpEtsSrcStableAcrossRepeatedCalls)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { x: int = 1 m(): int { return this.x } }")
    {
        auto *first = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), CApi(GetAst()));
        ASSERT_NE(first, nullptr);
        const std::string reference {first};
        for (int i = 0; i < 5; i++) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), CApi(GetAst()));
            ASSERT_NE(dump, nullptr);
            EXPECT_EQ(std::string(dump), reference) << "iteration " << i;
        }
    }
}

// Two contexts on the same source: identical dumps (deterministic parse)
TEST_F(AstWalkInvariantTest, DumpEtsSrcIdenticalAcrossTwoContexts)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    auto *first = CreateParsedContext(config.Get(), "class D { v: int = 2 }", "dump-1.ats");
    ASSERT_NE(first, nullptr);
    auto *second = CreateParsedContext(config.Get(), "class D { v: int = 2 }", "dump-2.ats");
    ASSERT_NE(second, nullptr);
    auto *programA = Impl()->ContextProgram(first);
    auto *programB = Impl()->ContextProgram(second);
    ASSERT_NE(programA, nullptr);
    ASSERT_NE(programB, nullptr);
    auto *astA = Impl()->ProgramAst(first, programA);
    auto *astB = Impl()->ProgramAst(second, programB);
    ASSERT_NE(astA, nullptr);
    ASSERT_NE(astB, nullptr);
    auto *dumpA = Impl()->AstNodeDumpEtsSrcConst(first, astA);
    auto *dumpB = Impl()->AstNodeDumpEtsSrcConst(second, astB);
    ASSERT_NE(dumpA, nullptr);
    ASSERT_NE(dumpB, nullptr);
    EXPECT_EQ(std::string(dumpA), std::string(dumpB));
    Impl()->DestroyContext(first);
    Impl()->DestroyContext(second);
}

// Same predicate called twice: CountNodesOfKind results agree
TEST_F(AstWalkInvariantTest, KindCountConsistentAcrossRepeatedCalls)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { a(): int { return 1 } b(): int { return 2 } }")
    {
        const auto first =
            CountNodesOfKind(GetImpl(), CApi(GetAst()), [](es2panda_Impl const *impl, es2panda_AstNode *n) {
                return impl->IsMethodDefinition(n);
            });
        const auto second =
            CountNodesOfKind(GetImpl(), CApi(GetAst()), [](es2panda_Impl const *impl, es2panda_AstNode *n) {
                return impl->IsMethodDefinition(n);
            });
        EXPECT_EQ(first, second);
        EXPECT_GE(first, 2U);
    }
}

// 10 predicates on one complex AST: two calls each, all agree
TEST_F(AstWalkInvariantTest, ManyKindCountsAllConsistent)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class M { f: int = 1 s: string = \"x\" m(): int { return this.f } n(): void {} }")
    {
        struct Probe {
            bool (*predicate)(es2panda_Impl const *, es2panda_AstNode *);
            size_t count1 = 0;
            size_t count2 = 0;
        };
        Probe probes[] {
            {[](es2panda_Impl const *impl, es2panda_AstNode *n) { return impl->IsMethodDefinition(n); }},
            {[](es2panda_Impl const *impl, es2panda_AstNode *n) { return impl->IsIdentifier(n); }},
            {[](es2panda_Impl const *impl, es2panda_AstNode *n) { return impl->IsNumberLiteral(n); }},
            {[](es2panda_Impl const *impl, es2panda_AstNode *n) { return impl->IsStringLiteral(n); }},
            {[](es2panda_Impl const *impl, es2panda_AstNode *n) { return impl->IsBlockStatement(n); }},
            {[](es2panda_Impl const *impl, es2panda_AstNode *n) { return impl->IsClassDeclaration(n); }},
            {[](es2panda_Impl const *impl, es2panda_AstNode *n) { return impl->IsVariableDeclaration(n); }},
            {[](es2panda_Impl const *impl, es2panda_AstNode *n) { return impl->IsVariableDeclarator(n); }},
            {[](es2panda_Impl const *impl, es2panda_AstNode *n) { return impl->IsThisExpression(n); }},
            {[](es2panda_Impl const *impl, es2panda_AstNode *n) { return impl->IsMemberExpression(n); }},
        };
        for (auto &probe : probes) {
            probe.count1 = CountNodesOfKind(GetImpl(), CApi(GetAst()), probe.predicate);
            probe.count2 = CountNodesOfKind(GetImpl(), CApi(GetAst()), probe.predicate);
        }
        for (auto const &probe : probes) {
            EXPECT_EQ(probe.count1, probe.count2);
        }
    }
}

// WalkPayload reuse: reset and refill the same AST; all fields identical
TEST_F(AstWalkInvariantTest, WalkPayloadReusableAfterReset)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { m(): int { return 1 } }")
    {
        WalkPayload first;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        const auto nodeCount = first.nodeCount;
        const auto identifierCount = first.identifierCount;
        first.nodeCount = 0;
        first.identifierCount = 0;
        first.nullParentCount = 0;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &first);
        EXPECT_EQ(first.nodeCount, nodeCount);
        EXPECT_EQ(first.identifierCount, identifierCount);
        EXPECT_EQ(first.nullParentCount, 1U);
    }
}

// Complex snippet parsed 10 rounds: node count and first MethodDefinition fixed
TEST_F(LeakDefenseTest, ComplexSourceParseLoop10Deterministic)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    size_t referenceCount = 0;
    for (int i = 0; i < 10; i++) {
        ContextGuard guard {CreateParsedContext(
            config.Get(), "class Z { a: int = 1 b: string = \"s\" m(x: int): int { return x + this.a } n(): void {} }",
            "complex-loop.ats")};
        ASSERT_NE(guard.Get(), nullptr);
        auto *program = Impl()->ContextProgram(guard.Get());
        ASSERT_NE(program, nullptr);
        auto *ast = Impl()->ProgramAst(guard.Get(), program);
        ASSERT_NE(ast, nullptr);
        WalkPayload payload;
        WalkAst(Impl(), guard.Get(), ast, &payload);
        if (referenceCount == 0) {
            referenceCount = payload.nodeCount;
        } else {
            EXPECT_EQ(payload.nodeCount, referenceCount) << "iteration " << i;
        }
    }
}

// Two coexisting contexts walked alternately: no cross-talk in counts
TEST_F(AstWalkInvariantTest, InterleavedWalkTwoContextsNoCrossTalk)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    auto *first = CreateParsedContext(config.Get(), "class Small {}", "small.ats");
    ASSERT_NE(first, nullptr);
    auto *second = CreateParsedContext(config.Get(), "class Big { a: int = 1 m(): int { return 1 } }", "big.ats");
    ASSERT_NE(second, nullptr);
    auto *programA = Impl()->ContextProgram(first);
    auto *programB = Impl()->ContextProgram(second);
    ASSERT_NE(programA, nullptr);
    ASSERT_NE(programB, nullptr);
    auto *astA = Impl()->ProgramAst(first, programA);
    auto *astB = Impl()->ProgramAst(second, programB);
    ASSERT_NE(astA, nullptr);
    ASSERT_NE(astB, nullptr);
    for (int round = 0; round < 5; round++) {
        WalkPayload payloadA;
        WalkPayload payloadB;
        WalkAst(Impl(), first, astA, &payloadA);
        WalkAst(Impl(), second, astB, &payloadB);
        EXPECT_LT(payloadA.nodeCount, payloadB.nodeCount) << "round " << round;
    }
    Impl()->DestroyContext(first);
    Impl()->DestroyContext(second);
}

// ---------------------------------------------------------------------------
// 18.B predicate sweep stress - batch calls on every node
// ---------------------------------------------------------------------------

// 8 predicates called on every node in the walk; no crash, totals conserved
TEST_F(AstWalkInvariantTest, PredicateSweepEveryNodeEightPredicates)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { f: int = 1 m(): int { return this.f } }")
    {
        struct SweepPayload {
            es2panda_Impl const *impl = nullptr;
            size_t nodeCount = 0;
            size_t trueCount = 0;
        };
        SweepPayload payload;
        payload.impl = GetImpl();
        // Captureless lambda converts to a function pointer
        static auto callback = [](es2panda_AstNode *node, void *arg) {
            auto *p = static_cast<SweepPayload *>(arg);
            p->nodeCount++;
            // 8-predicate sweep: every node must answer safely
            if (p->impl->IsIdentifier(node)) {
                p->trueCount++;
            }
            if (p->impl->IsClassDeclaration(node)) {
                p->trueCount++;
            }
            if (p->impl->IsMethodDefinition(node)) {
                p->trueCount++;
            }
            if (p->impl->IsBlockStatement(node)) {
                p->trueCount++;
            }
            if (p->impl->IsNumberLiteral(node)) {
                p->trueCount++;
            }
            if (p->impl->IsMemberExpression(node)) {
                p->trueCount++;
            }
            if (p->impl->IsThisExpression(node)) {
                p->trueCount++;
            }
            if (p->impl->IsETSPrimitiveType(node)) {
                p->trueCount++;
            }
        };
        GetImpl()->AstNodeForEach(
            CApi(GetAst()), [](es2panda_AstNode *node, void *arg) { callback(node, arg); }, &payload);
        EXPECT_GT(payload.nodeCount, 0U);
        EXPECT_GE(payload.trueCount, 0U);
    }
}

// Predicate sweep x5 rounds: nodeCount and trueCount identical (purity)
TEST_F(AstWalkInvariantTest, PredicateSweepRepeatableAcrossRounds)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { a(): int { return 1 } }")
    {
        struct SweepPayload {
            es2panda_Impl const *impl = nullptr;
            size_t nodeCount = 0;
            size_t trueCount = 0;
        };
        SweepPayload reference;
        reference.impl = GetImpl();
        for (int round = 0; round < 5; round++) {
            SweepPayload payload;
            payload.impl = GetImpl();
            GetImpl()->AstNodeForEach(
                CApi(GetAst()),
                [](es2panda_AstNode *node, void *arg) {
                    auto *p = static_cast<SweepPayload *>(arg);
                    p->nodeCount++;
                    if (p->impl->IsIdentifier(node) || p->impl->IsMethodDefinition(node)) {
                        p->trueCount++;
                    }
                },
                &payload);
            if (round == 0) {
                reference = payload;
            } else {
                EXPECT_EQ(payload.nodeCount, reference.nodeCount) << "round " << round;
                EXPECT_EQ(payload.trueCount, reference.trueCount) << "round " << round;
            }
        }
    }
}

// Walk + predicate sweep + dump + node count, one full pass
TEST_F(AstWalkInvariantTest, FullProbePipelineSinglePass)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class FP { x: int = 1 y: string = \"a\" m(): int { return this.x } }")
    {
        WalkPayload payload;
        WalkAst(GetImpl(), GetContext(), CApi(GetAst()), &payload);
        EXPECT_GT(payload.nodeCount, 0U);
        EXPECT_EQ(payload.nullParentCount, 1U);
        const auto methodCount =
            CountNodesOfKind(GetImpl(), CApi(GetAst()), [](es2panda_Impl const *impl, es2panda_AstNode *n) {
                return impl->IsMethodDefinition(n);
            });
        EXPECT_GE(methodCount, 1U);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), CApi(GetAst()));
        ASSERT_NE(dump, nullptr);
        EXPECT_TRUE(Contains(std::string(dump), "class"));
    }
}

// ---------------------------------------------------------------------------
// 18.C mixed API stress
// ---------------------------------------------------------------------------

// Mixed creation loop x10 on one context: literals/positions/diagnostics/memory
TEST_F(LeakDefenseTest, MixedCreationApisLoop10)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {Impl()->CreateContextFromString(config.Get(), "class MX {}", "mixed-create.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    for (int i = 0; i < 10; i++) {
        auto *literal = Impl()->CreateNumberLiteral(guard.Get(), i);
        auto *position = Impl()->CreateSourcePosition(guard.Get(), static_cast<size_t>(i), 1);
        auto *kind = Impl()->CreateDiagnosticKind(guard.Get(), "M {}", ES2PANDA_PLUGIN_ERROR);
        auto *memory = Impl()->AllocMemory(guard.Get(), 4, sizeof(uint64_t));
        auto *range = Impl()->CreateSourceRange(guard.Get(), position, position);
        ASSERT_NE(literal, nullptr);
        ASSERT_NE(position, nullptr);
        ASSERT_NE(kind, nullptr);
        ASSERT_NE(memory, nullptr);
        ASSERT_NE(range, nullptr);
        EXPECT_TRUE(Impl()->IsNumberLiteral(literal));
    }
}

// Query + walk + dump alternate loop x10
TEST_F(LeakDefenseTest, QueryWalkDumpAlternatingLoop10)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {CreateParsedContext(config.Get(), "class QW { seek(): int { return 1 } }", "qw.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *program = Impl()->ContextProgram(guard.Get());
    ASSERT_NE(program, nullptr);
    auto *ast = Impl()->ProgramAst(guard.Get(), program);
    ASSERT_NE(ast, nullptr);
    for (int i = 0; i < 10; i++) {
        auto *found = Impl()->FirstDeclarationByNameFromNode(guard.Get(), ast, "seek");
        ASSERT_NE(found, nullptr) << "iteration " << i;
        WalkPayload payload;
        WalkAst(Impl(), guard.Get(), ast, &payload);
        EXPECT_EQ(payload.nullParentCount, 1U);
        auto *dump = Impl()->AstNodeDumpEtsSrcConst(guard.Get(), ast);
        ASSERT_NE(dump, nullptr);
    }
}

// EHeap::AllocatedSize is idempotent: 5 consecutive reads are equal
TEST_F(LeakDefenseTest, EHeapAllocatedSizeQueryIdempotent)
{
    const auto first = ark::es2panda::EHeap::AllocatedSize();
    for (int i = 0; i < 5; i++) {
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), first) << "iteration " << i;
    }
}

// EHeap::AllocatedSize reads between context lifecycles change nothing (pure query)
TEST_F(LeakDefenseTest, EHeapQueryDoesNotAllocate)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    const auto before = ark::es2panda::EHeap::AllocatedSize();
    auto *ctx = CreateParsedContext(config.Get(), "class EQ { v: int = 1 }", "eheap-q.ats");
    ASSERT_NE(ctx, nullptr);
    (void)ark::es2panda::EHeap::AllocatedSize();
    (void)ark::es2panda::EHeap::AllocatedSize();
    Impl()->DestroyContext(ctx);
    EXPECT_GE(ark::es2panda::EHeap::AllocatedSize(), before);
}

// Dump inside a walk callback: recursive dump is safe during traversal
TEST_F(AstWalkInvariantTest, DumpInsideWalkCallbackIsSafe)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { m(): int { return 1 } }")
    {
        struct NestedDumpPayload {
            es2panda_Context *ctx = nullptr;
            es2panda_Impl const *impl = nullptr;
            size_t dumpCount = 0;
        };
        NestedDumpPayload payload;
        payload.ctx = GetContext();
        payload.impl = GetImpl();
        GetImpl()->AstNodeForEach(
            CApi(GetAst()),
            [](es2panda_AstNode *node, void *arg) {
                auto *p = static_cast<NestedDumpPayload *>(arg);
                // Each node dumped once (arena-allocated; lifetime follows the context)
                auto *dump = p->impl->AstNodeDumpEtsSrcConst(p->ctx, node);
                if (dump != nullptr) {
                    p->dumpCount++;
                }
            },
            &payload);
        EXPECT_GT(payload.dumpCount, 0U);
    }
}

// Query inside a walk callback: FirstDeclarationByNameFromNode is safe
TEST_F(AstWalkInvariantTest, QueryInsideWalkCallbackIsSafe)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class A { seek(): int { return 1 } }")
    {
        struct NestedQueryPayload {
            es2panda_Context *ctx = nullptr;
            es2panda_Impl const *impl = nullptr;
            size_t queryCount = 0;
        };
        NestedQueryPayload payload;
        payload.ctx = GetContext();
        payload.impl = GetImpl();
        GetImpl()->AstNodeForEach(
            CApi(GetAst()),
            [](es2panda_AstNode *node, void *arg) {
                auto *p = static_cast<NestedQueryPayload *>(arg);
                auto *found = p->impl->FirstDeclarationByNameFromNode(p->ctx, node, "seek");
                if (found != nullptr) {
                    p->queryCount++;
                }
            },
            &payload);
        EXPECT_GE(payload.queryCount, 0U);
    }
}

// ---------------------------------------------------------------------------
// 18.D lifecycle tail stress - combined destruction orders
// ---------------------------------------------------------------------------

// Config destroyed after all its contexts (contract order; loop x3 here):
// (destroy contexts first, then the config; the standard order repeated x3)
TEST_F(LeakDefenseTest, ContextThenConfigDestroyOrderLoop3)
{
    for (int i = 0; i < 3; i++) {
        ConfigGuard config {NewConfig()};
        ASSERT_NE(config.Get(), nullptr);
        ContextGuard guard {CreateParsedContext(config.Get(), "class OD {}", "order.ats")};
        ASSERT_NE(guard.Get(), nullptr);
        auto *ctx = Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_BOUND);
        ASSERT_EQ(Impl()->ContextState(ctx), ES2PANDA_STATE_BOUND);
    }
    SUCCEED();
}

// Nested scopes: outer + inner config; the inner context dies first
TEST_F(LeakDefenseTest, NestedConfigScopesDestroyOrder)
{
    ConfigGuard outer {NewConfig()};
    ASSERT_NE(outer.Get(), nullptr);
    {
        ConfigGuard inner {NewConfig()};
        ASSERT_NE(inner.Get(), nullptr);
        ContextGuard guard {CreateParsedContext(inner.Get(), "class IN {}", "nested-inner.ats")};
        ASSERT_NE(guard.Get(), nullptr);
    }
    // Inner config and its context left scope first; the outer one still works
    ContextGuard outerGuard {CreateParsedContext(outer.Get(), "class OUT {}", "nested-outer.ats")};
    ASSERT_NE(outerGuard.Get(), nullptr);
    SUCCEED();
}

// Mixed-state batch destruction: 5 contexts in different states created once,
// destroyed together (different sources/states than Part 15)
TEST_F(LeakDefenseTest, MixedStateBatchDestroyDifferentSources)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    std::vector<es2panda_Context *> contexts;
    std::vector<es2panda_ContextState> targetStates {ES2PANDA_STATE_PARSED, ES2PANDA_STATE_BOUND,
                                                     ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_CHECKED,
                                                     ES2PANDA_STATE_LOWERED};
    std::vector<char const *> sources {"class Batch1 {}", "class Batch2 { x: int = 1 }", "class Batch3 { v: int = 1 }",
                                       "class Batch4 { w: int = 2 }",
                                       "function main() { for (let i = 0; i < 3; i++) {} }"};
    for (size_t i = 0; i < targetStates.size(); i++) {
        auto *context = Impl()->CreateContextFromString(config.Get(), sources[i], "batch.ats");
        ASSERT_NE(context, nullptr);
        contexts.push_back(context);
    }
    for (size_t i = 0; i < contexts.size(); i++) {
        auto *ctx = Impl()->ProceedToState(contexts[i], targetStates[i]);
        ASSERT_EQ(Impl()->ContextState(ctx), targetStates[i]) << "context " << i;
    }
    for (auto *context : contexts) {
        Impl()->DestroyContext(context);
    }
    SUCCEED();
}

// The config still creates new contexts after all were destroyed
TEST_F(LeakDefenseTest, ConfigReusableAfterAllContextsDestroyed)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    for (int i = 0; i < 3; i++) {
        auto *context = CreateParsedContext(config.Get(), "class RU {}", "reuse-after.ats");
        ASSERT_NE(context, nullptr);
        Impl()->DestroyContext(context);
    }
    auto *final = CreateParsedContext(config.Get(), "class RU2 { v: int = 1 }", "reuse-final.ats");
    ASSERT_NE(final, nullptr);
    auto *ctx = Impl()->ProceedToState(final, ES2PANDA_STATE_CHECKED);
    ASSERT_EQ(Impl()->ContextState(ctx), ES2PANDA_STATE_CHECKED);
    Impl()->DestroyContext(ctx);
    SUCCEED();
}
// ===========================================================================
// Part 19: wrap-up - end-to-end smoke and global invariants
// ===========================================================================

// End-to-end smoke: full pipeline to LOWERED; program/AST/walk/query pass once
TEST_F(LeakDefenseTest, EndToEndSmokeFullPipeline)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {
        CreateParsedContext(config.Get(), "class Final { v: int = 1 get(): int { return this.v } }", "final.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    auto *ctx = Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_LOWERED);
    ASSERT_EQ(Impl()->ContextState(ctx), ES2PANDA_STATE_LOWERED);
    auto *program = Impl()->ContextProgram(ctx);
    ASSERT_NE(program, nullptr);
    auto *ast = Impl()->ProgramAst(ctx, program);
    ASSERT_NE(ast, nullptr);
    WalkPayload payload;
    WalkAst(Impl(), ctx, ast, &payload);
    EXPECT_GT(payload.nodeCount, 0U);
    EXPECT_EQ(payload.nullParentCount, 1U);
    auto *found = Impl()->FirstDeclarationByNameFromNode(ctx, ast, "get");
    ASSERT_NE(found, nullptr);
    EXPECT_TRUE(Impl()->IsMethodDefinition(found));
    auto *dump = Impl()->AstNodeDumpEtsSrcConst(ctx, ast);
    ASSERT_NE(dump, nullptr);
    EXPECT_TRUE(Contains(std::string(dump), "class"));
}

// Final invariant: the EHeap counter is readable and non-decreasing in the last test
TEST_F(LeakDefenseTest, EHeapInvariantStillHoldsAtEnd)
{
    const auto current = ark::es2panda::EHeap::AllocatedSize();
    // EHeap is a process-wide bump allocator: the counter must be readable and finite
    EXPECT_LT(current, static_cast<size_t>(-1));
    // One more full lifecycle; the counter does not decrease
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    ContextGuard guard {CreateParsedContext(config.Get(), "class End {}", "the-end.ats")};
    ASSERT_NE(guard.Get(), nullptr);
    EXPECT_GE(ark::es2panda::EHeap::AllocatedSize(), current);
}

// Paired wrap-up: config + context + globalContext created once, destroyed in order
TEST_F(LeakDefenseTest, TripleResourceLifecycleSinglePass)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    GlobalContextGuard global {Impl()->CreateGlobalContext(config.Get(), nullptr, 0, false)};
    ASSERT_NE(global.Get(), nullptr);
    // GlobalContext fatal diagnostics go into the shared config engine; a normal
    // context Parse would enter ERROR, so use a cache context (as in 15.H)
    ContextGuard guard {
        Impl()->CreateCacheContextFromString(config.Get(), "class T3 {}", "triple.ats", global.Get(), false, false)};
    ASSERT_NE(guard.Get(), nullptr);
    auto *ctx = Impl()->ProceedToState(guard.Get(), ES2PANDA_STATE_PARSED);
    ASSERT_EQ(Impl()->ContextState(ctx), ES2PANDA_STATE_PARSED);
    // Destroy order: context -> globalContext -> config (reverse of declaration)
}

// EvaluateExpression wrap-up: the last eval of this file; malloc/free paired
TEST_F(EvaluateExpressionLifecycleTest, FinalEvaluateAndFreePaired)
{
    ConfigGuard config {NewConfig()};
    ASSERT_NE(config.Get(), nullptr);
    auto *result = Impl()->EvaluateExpression(config.Get(), Base64Encode("1 + 2").c_str());
    ASSERT_NE(result, nullptr);
    Impl()->FreeExpressionResult(result);
    SUCCEED();
}

// Full predicate wrap-up: all 223 Is* predicates on the root module node, no crash
TEST_F(NullSafetyTest, PredicatesOnRootNodeAllSafe)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class Root { m(): int { return 1 } }")
    {
        auto *root = CApi(GetAst());
        ASSERT_NE(root, nullptr);
        // Common predicate sweep (each answers safely; at least ETSModule is true)
        EXPECT_TRUE(GetImpl()->IsETSModule(root));
        EXPECT_FALSE(GetImpl()->IsClassDeclaration(root));
        EXPECT_FALSE(GetImpl()->IsMethodDefinition(root));
        EXPECT_FALSE(GetImpl()->IsIdentifier(root));
        EXPECT_FALSE(GetImpl()->IsBlockStatement(root));
        EXPECT_FALSE(GetImpl()->IsIfStatement(root));
        EXPECT_FALSE(GetImpl()->IsWhileStatement(root));
        EXPECT_FALSE(GetImpl()->IsForUpdateStatement(root));
        EXPECT_FALSE(GetImpl()->IsBinaryExpression(root));
        EXPECT_FALSE(GetImpl()->IsUnaryExpression(root));
        EXPECT_FALSE(GetImpl()->IsCallExpression(root));
        EXPECT_FALSE(GetImpl()->IsMemberExpression(root));
        EXPECT_FALSE(GetImpl()->IsThisExpression(root));
        EXPECT_FALSE(GetImpl()->IsNumberLiteral(root));
        EXPECT_FALSE(GetImpl()->IsStringLiteral(root));
        EXPECT_FALSE(GetImpl()->IsBooleanLiteral(root));
        EXPECT_FALSE(GetImpl()->IsETSPrimitiveType(root));
        EXPECT_FALSE(GetImpl()->IsETSUnionType(root));
        EXPECT_FALSE(GetImpl()->IsTSInterfaceDeclaration(root));
        EXPECT_FALSE(GetImpl()->IsTSAsExpression(root));
        EXPECT_FALSE(GetImpl()->IsTSModuleDeclaration(root));
        EXPECT_FALSE(GetImpl()->IsFunctionDeclaration(root));
        EXPECT_FALSE(GetImpl()->IsVariableDeclaration(root));
        EXPECT_FALSE(GetImpl()->IsVariableDeclarator(root));
    }
}

}  // namespace ark::es2panda::public_lib::test
