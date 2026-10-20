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
#include "public/es2panda_lib.h"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {
constexpr size_t FIELD_COUNT = 502;
constexpr size_t NAME_READS = 1000000;
constexpr size_t BODY_READS = 20000;
constexpr size_t DUMP_READS = 10000;
constexpr size_t DUMP_NAME_BYTES = 16U * 1024U;
constexpr size_t GROWTH_READS = 10000;
constexpr long BUDGET_BYTES = 8L * 1024 * 1024;
constexpr long BYTES_PER_KB = 1024;
constexpr int EXPECTED_ARGC = 3;
constexpr int MODE_ARG_INDEX = 2;
constexpr int COMPILER_ARG_INDEX = 1;
constexpr int EXIT_OK = 0;
constexpr int EXIT_FAILURE_CASE = 1;
constexpr int EXIT_USAGE = 2;
constexpr int EXIT_NO_API = 3;
}  // namespace

static const es2panda_Impl *api;
static const char *COMPILER_PATH;
static void Require(bool ok, const char *message)
{
    if (!ok) {
        throw std::runtime_error(message);
    }
}
struct Context {
    es2panda_Config *config;
    es2panda_Context *value;
    explicit Context(const std::string &source)
    {
        const char *args[] = {COMPILER_PATH};
        config = api->CreateConfig(1, args);
        Require(config != nullptr, "config");
        value = api->CreateContextFromString(config, source.c_str(), "capi_blackbox.ets");
        Require(value != nullptr, "context");
        Proceed(ES2PANDA_STATE_PARSED);
    }
    void Proceed(es2panda_ContextState state)
    {
        api->ProceedToState(value, state);
        Require(api->ContextState(value) == state, api->ContextErrorMessage(value));
    }
    es2panda_AstNode *Root() const
    {
        return api->ProgramAst(value, api->ContextProgram(value));
    }
    ~Context()
    {
        api->DestroyContext(value);
        api->DestroyConfig(config);
    }
};
static long Rss()
{
    std::ifstream in("/proc/self/status");
    std::string name;
    while (in >> name) {
        if (name == "VmRSS:") {
            long kb;
            in >> kb;
            return kb * BYTES_PER_KB;
        }
        std::string rest;
        std::getline(in, rest);
    }
    throw std::runtime_error("RSS unavailable");
}
static void Budget(long before, const char *kind)
{
    const long delta = Rss() - before;
    std::cout << "resource=" << kind << " rss_delta_bytes=" << delta << '\n';
    Require(delta <= BUDGET_BYTES, "repeated reads exceed 8 MiB budget");
}
static es2panda_AstNode *Identifier(Context &ctx, std::string name)
{
    auto *buffer = static_cast<char *>(api->AllocMemory(ctx.value, name.size() + 1, 1));
    const char *data = name.c_str();
    std::copy(data, data + name.size() + 1, buffer);
    return api->CreateIdentifier1(ctx.value, buffer);
}
static std::string LargeSource()
{
    std::string source = "class Component {";
    for (size_t i = 0; i < FIELD_COUNT; ++i) {
        source += "state" + std::to_string(i) + ": string = 'default';";
    }
    return source + "}";
}
static es2panda_AstNode *Class(Context &ctx)
{
    struct Search {
        Context *ctx;
        es2panda_AstNode *node = nullptr;
    } search {&ctx};
    api->AstNodeForEach(
        ctx.Root(),
        [](es2panda_AstNode *node, void *arg) {
            auto &s = *static_cast<Search *>(arg);
            if (api->IsClassDefinition(node)) {
                auto *id = api->ClassDefinitionIdent(s.ctx->value, node);
                if (id != nullptr && std::strcmp(api->IdentifierNameConst(s.ctx->value, id), "Component") == 0) {
                    s.node = node;
                }
            }
        },
        &search);
    Require(search.node != nullptr, "Component class not found");
    return search.node;
}
static std::vector<es2panda_AstNode *> Body(Context &ctx, es2panda_AstNode *node)
{
    size_t size;
    auto *result = api->ClassDefinitionBodyConst(ctx.value, node, &size);
    Require(result != nullptr || size == 0, "null nonempty body");
    return size == 0 ? std::vector<es2panda_AstNode *> {} : std::vector<es2panda_AstNode *>(result, result + size);
}
static void RunStringBytes(Context &ctx)
{
    std::vector<std::string> values {"", "name", std::string(4096, 'x'), "状态_é"};
    for (const auto &value : values) {
        std::string input = value;
        auto *literal = api->CreateStringLiteral1(ctx.value, input.data());
        auto *text = api->StringLiteralStrConst(ctx.value, literal);
        Require(std::string(text) == value && text[value.size()] == '\0', "literal bytes");
        if (value.empty()) {
            continue;
        }
        auto *id = Identifier(ctx, value);
        auto *old = api->IdentifierNameConst(ctx.value, id);
        Require(std::string(old) == value && old[value.size()] == '\0', "identifier bytes");
        std::string changed = "changed_" + value;
        api->IdentifierSetName(ctx.value, id, changed.data());
        Require(std::string(api->IdentifierNameConst(ctx.value, id)) == changed, "edited name");
        Require(std::string(old) == value, "old name overwritten");
    }
}
static void RunNameReuse(Context &ctx)
{
    auto *id = Identifier(ctx, "RepeatedIdentifierName");
    api->IdentifierNameConst(ctx.value, id);
    long before = Rss();
    for (size_t i = 0; i < NAME_READS; ++i) {
        Require(std::strcmp(api->IdentifierNameConst(ctx.value, id), "RepeatedIdentifierName") == 0, "name drift");
    }
    Budget(before, "name");
}
static void RunBodyReuse(Context &ctx)
{
    auto *node = Class(ctx);
    auto expected = Body(ctx, node);
    Require(expected.size() >= FIELD_COUNT, "large body");
    size_t fields = 0;
    for (auto *member : expected) {
        if (api->IsClassProperty(member)) {
            auto *key = api->ClassElementId(ctx.value, member);
            const std::string name = "state" + std::to_string(fields++);
            Require(std::string(api->IdentifierNameConst(ctx.value, key)) == name,
                    "body field order differs from source");
        }
    }
    Require(fields == FIELD_COUNT, "body field count differs from source");
    long before = Rss();
    for (size_t i = 0; i < BODY_READS; ++i) {
        size_t size;
        auto *result = api->ClassDefinitionBodyConst(ctx.value, node, &size);
        Require(size == expected.size() && std::equal(expected.begin(), expected.end(), result), "body drift");
    }
    Budget(before, "body");
}
static void RunBodyMutation(Context &ctx)
{
    auto *node = Class(ctx);
    size_t oldSize;
    auto *old = api->ClassDefinitionBodyConst(ctx.value, node, &oldSize);
    const std::vector<es2panda_AstNode *> original(old, old + oldSize);
    auto model = original;
    api->ClassDefinitionSetValueBody(ctx.value, node, original[1], 0);
    model[0] = original[1];
    Require(Body(ctx, node) == model, "replacement");
    api->ClassDefinitionEmplaceBody(ctx.value, node, original[0]);
    model.push_back(original[0]);
    Require(Body(ctx, node) == model, "append");
    size_t appendedSize;
    auto *appended = api->ClassDefinitionBodyConst(ctx.value, node, &appendedSize);
    api->ClassDefinitionClearBody(ctx.value, node);
    Require(Body(ctx, node).empty(), "clear");
    for (auto *item : original) {
        api->ClassDefinitionEmplaceBody(ctx.value, node, item);
    }
    Require(Body(ctx, node) == original, "restore");
    Require(std::equal(original.begin(), original.end(), old), "old array overwritten");
    Require(appendedSize == model.size() && std::equal(model.begin(), model.end(), appended),
            "appended array overwritten");
}
static void RunDumpReuse(Context &ctx)
{
    // Isolate returned-string copies from allocations made by dumping class bodies.
    auto *node = Identifier(ctx, std::string(DUMP_NAME_BYTES, 'x'));
    auto *old = api->AstNodeDumpEtsSrcConst(ctx.value, node);
    const std::string original(old);
    std::string name = "Renamed" + std::string(DUMP_NAME_BYTES, 'y');
    api->IdentifierSetName(ctx.value, node, name.data());
    const std::string changed(api->AstNodeDumpEtsSrcConst(ctx.value, node));
    Require(changed != original && changed.find(name) != std::string::npos, "stale dump");
    Require(std::string(old) == original, "old dump overwritten");
    long before = Rss();
    for (size_t i = 0; i < DUMP_READS; ++i) {
        Require(std::strcmp(api->AstNodeDumpEtsSrcConst(ctx.value, node), changed.c_str()) == 0, "dump drift");
    }
    Budget(before, "dump");
}
static void RunPhaseStability(Context &ctx)
{
    ctx.Proceed(ES2PANDA_STATE_CHECKED);
    auto *node = Class(ctx);
    auto *id = api->ClassDefinitionIdent(ctx.value, node);
    auto *name = api->IdentifierNameConst(ctx.value, id);
    size_t size;
    auto *body = api->ClassDefinitionBodyConst(ctx.value, node, &size);
    const std::vector<es2panda_AstNode *> saved(body, body + size);
    api->AstNodeRecheck(ctx.value, ctx.Root());
    Require(api->ContextState(ctx.value) != ES2PANDA_STATE_ERROR, "recheck failed");
    ctx.Proceed(ES2PANDA_STATE_ASM_GENERATED);
    Require(std::strcmp(name, "Component") == 0 && std::equal(saved.begin(), saved.end(), body),
            "phase invalidated result");
}
static void RunContextIsolation(Context &)
{
    auto first = std::make_unique<Context>("class First {}");
    Context second("class Second {}");
    auto *id1 = Identifier(*first, "shared_name");
    auto *id2 = Identifier(second, "shared_name");
    api->IdentifierNameConst(first->value, id1);
    auto *old = api->IdentifierNameConst(second.value, id2);
    first.reset();
    Require(std::strcmp(old, "shared_name") == 0, "context isolation");
    Require(std::strcmp(api->IdentifierNameConst(second.value, id2), "shared_name") == 0, "second context unusable");
}
static void RunGrowthPersistence(Context &ctx)
{
    auto *id = Identifier(ctx, "persistent_name");
    auto *old = api->IdentifierNameConst(ctx.value, id);
    for (size_t i = 0; i < GROWTH_READS; ++i) {
        std::string name = "unique_" + std::to_string(i);
        auto *next = Identifier(ctx, name);
        Require(std::string(api->IdentifierNameConst(ctx.value, next)) == name, "unique name");
    }
    Require(std::strcmp(old, "persistent_name") == 0, "growth invalidated name");
}
static void RunRecoveryAfterFailure(Context &)
{
    auto failed = std::make_unique<Context>("class Invalid { value: int = 'wrong'; }");
    auto *id = Identifier(*failed, "cached_before_failure");
    Require(std::strcmp(api->IdentifierNameConst(failed->value, id), "cached_before_failure") == 0, "failure setup");
    api->ProceedToState(failed->value, ES2PANDA_STATE_CHECKED);
    Require(api->ContextState(failed->value) == ES2PANDA_STATE_ERROR, "type error was not reported");
    failed.reset();
    Context recovered("class Valid { value: int = 1; }");
    auto *fresh = Identifier(recovered, "cached_before_failure");
    Require(std::strcmp(api->IdentifierNameConst(recovered.value, fresh), "cached_before_failure") == 0,
            "new context after failed checking");
    recovered.Proceed(ES2PANDA_STATE_ASM_GENERATED);
}
static void Run(const std::string &mode)
{
    using CaseFn = void (*)(Context &);
    static const std::unordered_map<std::string, CaseFn> CASES = {
        {"BB01", RunStringBytes},      {"BB02", RunNameReuse},         {"BB03", RunBodyReuse},
        {"BB04", RunBodyMutation},     {"BB05", RunDumpReuse},         {"BB06", RunPhaseStability},
        {"BB07", RunContextIsolation}, {"BB08", RunGrowthPersistence}, {"BB09", RunRecoveryAfterFailure},
    };
    auto found = CASES.find(mode);
    if (found == CASES.end()) {
        throw std::runtime_error("unknown mode");
    }
    const bool large = mode == "BB03" || mode == "BB04" || mode == "BB05";
    Context ctx(large ? LargeSource() : "class Component { value: int = 1; }");
    if (mode == "BB02" || mode == "BB03" || mode == "BB05") {
        ctx.Proceed(ES2PANDA_STATE_CHECKED);
    }
    found->second(ctx);
}
int main(int argc, char **argv)
{
    if (argc != EXPECTED_ARGC) {
        return EXIT_USAGE;
    }
    COMPILER_PATH = argv[COMPILER_ARG_INDEX];
    api = es2panda_GetImpl(ES2PANDA_LIB_VERSION);
    if (!api) {
        return EXIT_NO_API;
    }
    api->MemInitialize();
    try {
        Run(argv[MODE_ARG_INDEX]);
        api->MemFinalize();
        std::cout << argv[MODE_ARG_INDEX] << " PASS\n";
        return EXIT_OK;
    } catch (const std::exception &e) {
        std::cerr << argv[MODE_ARG_INDEX] << " FAIL " << e.what() << '\n';
        api->MemFinalize();
        return EXIT_FAILURE_CASE;
    }
}
