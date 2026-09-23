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

#include <algorithm>
#include <iterator>
#include <string>
#include <vector>
#include <iostream>
#include <sys/wait.h>
#include <unistd.h>

#include "public/es2panda_lib.h"
#include "util/diagnostic.h"
#include "util/diagnosticEngine.h"
#include "util.h"

// NOLINTBEGIN

namespace {
es2panda_Impl *impl = nullptr;
es2panda_Context *ctx = nullptr;
es2panda_AstNode *annotatedFunction = nullptr;
es2panda_AstNode *testInterface = nullptr;

void FindAnnotatedFunction(es2panda_AstNode *node)
{
    if (annotatedFunction == nullptr && impl->IsScriptFunction(node)) {
        size_t count = 0;
        auto *annotations = impl->ScriptFunctionAnnotations(ctx, node, &count);
        if (count == 1 && annotations != nullptr) {
            annotatedFunction = node;
        }
    }
    if (testInterface == nullptr && impl->IsTSInterfaceDeclaration(node)) {
        testInterface = node;
    }
    impl->AstNodeIterateConst(ctx, node, FindAnnotatedFunction);
}

bool HasDiagnosticId(uint32_t id)
{
    const auto *opaque = impl->GetSemanticErrors(ctx);
    const auto *errors = reinterpret_cast<const ark::es2panda::util::DiagnosticStorage *>(opaque);
    return std::any_of(errors->begin(), errors->end(), [id](const auto &error) { return error->GetId() == id; });
}

bool HasMessage(const char *text)
{
    const auto *opaque = impl->GetSemanticErrors(ctx);
    const auto *errors = reinterpret_cast<const ark::es2panda::util::DiagnosticStorage *>(opaque);
    for (const auto &error : *errors) {
        if (error->Message().find(text) != std::string::npos) {
            return true;
        }
    }
    return false;
}

bool TestDetachedRecheck()
{
    impl->AstNodeRecheck(ctx, nullptr);
    return impl->ContextState(ctx) == ES2PANDA_STATE_ERROR && HasMessage("AstNodeRecheck requires a node attached");
}

bool TestDetachedRebind()
{
    impl->AstNodeRebind(ctx, nullptr);
    return impl->ContextState(ctx) == ES2PANDA_STATE_ERROR && HasMessage("AstNodeRebind requires a node attached");
}

bool TestDetachedNonNullRecheck()
{
    pid_t child = fork();
    if (child < 0) {
        return false;
    }
    if (child == 0) {
        auto *node = CreateIdentifierFromString(ctx, "detached");
        std::cerr << "RECHECK_DETACHED node=" << node << '\n';
        impl->AstNodeRecheck(ctx, node);
        _exit(impl->ContextState(ctx) == ES2PANDA_STATE_ERROR && HasMessage("AstNodeRecheck requires a node attached")
                  ? 0
                  : 1);
    }
    int status = 0;
    waitpid(child, &status, 0);
    if (WIFSIGNALED(status)) {
        std::cerr << "RECHECK_DETACHED signal=" << WTERMSIG(status) << '\n';
        return false;
    }
    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

bool TestDetachedNonNullRebind()
{
    pid_t child = fork();
    if (child < 0) {
        return false;
    }
    if (child == 0) {
        auto *node = CreateIdentifierFromString(ctx, "detached");
        std::cerr << "REBIND_DETACHED node=" << node << '\n';
        impl->AstNodeRebind(ctx, node);
        _exit(impl->ContextState(ctx) == ES2PANDA_STATE_ERROR && HasMessage("AstNodeRebind requires a node attached")
                  ? 0
                  : 1);
    }
    int status = 0;
    waitpid(child, &status, 0);
    if (WIFSIGNALED(status)) {
        std::cerr << "REBIND_DETACHED signal=" << WTERMSIG(status) << '\n';
        return false;
    }
    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

bool TestValueEarlyReturn()
{
    auto *program = impl->ContextProgram(ctx);
    auto *module = impl->ProgramAst(ctx, program);
    impl->AstNodeIterateConst(ctx, module, FindAnnotatedFunction);
    if (annotatedFunction == nullptr) {
        std::cerr << "Annotated ScriptFunction not found\n";
        return false;
    }
    size_t count = 0;
    auto **originalAnnotations = impl->ScriptFunctionAnnotations(ctx, annotatedFunction, &count);
    if (count != 1 || originalAnnotations == nullptr) {
        return false;
    }
    size_t propertyCount = 0;
    auto **properties = impl->AnnotationUsageIrProperties(ctx, originalAnnotations[0], &propertyCount);
    if (propertyCount != 1 || properties == nullptr) {
        std::cerr << "Expected one value property\n";
        return false;
    }
    auto *base = CreateIdentifierFromString(ctx, "Marker");
    impl->IdentifierSetAnnotationUsage(ctx, base);
    auto *usage = impl->CreateAnnotationUsageIr1(ctx, base, properties, propertyCount);
    impl->AstNodeSetParent(ctx, base, usage);
    es2panda_AstNode *replacement[] = {usage};
    impl->ScriptFunctionSetAnnotations(ctx, annotatedFunction, replacement, 1);
    if (impl->DeclarationFromIdentifier(ctx, base) != nullptr) {
        return false;
    }
    impl->ProceedToState(ctx, ES2PANDA_STATE_BIN_GENERATED);
    return impl->ContextState(ctx) == ES2PANDA_STATE_ERROR &&
           HasDiagnosticId(ark::es2panda::diagnostic::ANNOTATION_RESOLUTION_FAILED.Id());
}

bool TestRebindFailureState()
{
    auto *program = impl->ContextProgram(ctx);
    auto *module = impl->ProgramAst(ctx, program);
    impl->AstNodeIterateConst(ctx, module, FindAnnotatedFunction);
    if (testInterface == nullptr) {
        std::cerr << "Interface not found\n";
        return false;
    }
    auto *base = CreateIdentifierFromString(ctx, "MissingReviewAnnotation");
    impl->IdentifierSetAnnotationUsage(ctx, base);
    auto *usage = impl->CreateAnnotationUsageIr1(ctx, base, nullptr, 0);
    impl->AstNodeSetParent(ctx, base, usage);
    es2panda_AstNode *annotations[] = {usage};
    impl->TSInterfaceDeclarationSetAnnotations(ctx, testInterface, annotations, 1);
    impl->AstNodeRebind(ctx, testInterface);
    const bool hasError = impl->IsAnyError(ctx);
    const bool errorState = impl->ContextState(ctx) == ES2PANDA_STATE_ERROR;
    std::cerr << "REBIND_ERROR hasError=" << hasError << " errorState=" << errorState << '\n';
    return hasError && errorState;
}

using EdgeTest = bool (*)();
const EdgeTest EDGE_TESTS[] = {TestDetachedRecheck,    TestDetachedRebind,         TestValueEarlyReturn,
                               TestRebindFailureState, TestDetachedNonNullRecheck, TestDetachedNonNullRebind};
constexpr int EDGE_TEST_COUNT = static_cast<int>(std::size(EDGE_TESTS));
}  // namespace

int main(int argc, char **argv)
{
    if (argc < MIN_ARGC) {
        return INVALID_ARGC_ERROR_CODE;
    }
    impl = GetImpl();
    if (impl == nullptr) {
        return NULLPTR_IMPL_ERROR_CODE;
    }
    const std::string source = R"(
@Retention({policy: "SOURCE"})
@interface Marker { value: string }
@Marker("hello")
    function annotated(): void {}
    interface ReviewInterface {}
)";
    int failures = 0;
    for (int test = 0; test != EDGE_TEST_COUNT; ++test) {
        annotatedFunction = nullptr;
        testInterface = nullptr;
        auto *config = impl->CreateConfig(argc - 1, argv + 1);
        ctx = impl->CreateContextFromString(config, source.data(), argv[argc - 1]);
        if (ctx == nullptr) {
            return NULLPTR_CONTEXT_ERROR_CODE;
        }
        impl->ProceedToState(ctx, ES2PANDA_STATE_CHECKED);
        if (impl->ContextState(ctx) != ES2PANDA_STATE_CHECKED) {
            std::cerr << "Setup failed at CHECKED for test " << test << '\n';
            const auto *opaque = impl->GetSemanticErrors(ctx);
            const auto *errors = reinterpret_cast<const ark::es2panda::util::DiagnosticStorage *>(opaque);
            for (const auto &error : *errors) {
                std::cerr << error->Message() << '\n';
            }
            return TEST_ERROR_CODE;
        }
        const bool pass = EDGE_TESTS[test]();
        std::cerr << "EDGE " << test << " " << (pass ? "PASS" : "FAIL") << '\n';
        if (!pass) {
            ++failures;
        }
    }
    return failures == 0 ? 0 : TEST_ERROR_CODE;
}

// NOLINTEND
