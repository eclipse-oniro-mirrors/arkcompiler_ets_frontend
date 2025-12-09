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

#include <gtest/gtest.h>
#include "libarkbase/macros.h"
#include "public/es2panda_lib.h"
#include "test/utils/panda_executable_path_getter.h"
#include "test/utils/ast_verifier_test.h"
#include "util/diagnostic.h"

namespace {

using Es2PandaLibTest = test::utils::AstVerifierTest;

TEST_F(Es2PandaLibTest, NoError)
{
    CONTEXT(ES2PANDA_STATE_ASM_GENERATED, "function main() {}", "no-error.ets") {}
}

TEST_F(Es2PandaLibTest, TypeError)
{
    CONTEXT(ES2PANDA_STATE_ASM_GENERATED, ES2PANDA_STATE_ERROR, "function main() { let x: int = \"\" }", "error.ets")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto diagnostics = GetImpl()->GetSemanticErrors(GetContext());
        ASSERT_EQ(reinterpret_cast<const ark::es2panda::util::DiagnosticStorage *>(diagnostics)->size(), 1);
    }
}

TEST_F(Es2PandaLibTest, ListIdentifiers)
{
    char const *text = R"XXX(
class C {
    n: string = "oh"
}

function main() {
    let c = new C
    console.log(c.n + 1) // type error, but not syntax error
}
)XXX";

    struct Arg {
        es2panda_Impl const *impl;
        es2panda_Context *ctx;
        std::vector<std::string> ids {};
    };

    auto func = [](es2panda_AstNode *ast, void *argp) {
        auto *a = reinterpret_cast<Arg *>(argp);
        if (a->impl->IsIdentifier(ast)) {
            a->ids.emplace_back(a->impl->IdentifierName(a->ctx, ast));
        }
    };
    CONTEXT(ES2PANDA_STATE_PARSED, text, "list-ids.ets")
    {
        Arg arg {GetImpl(), GetContext()};
        AstNodeForEach(func, &arg);

        std::vector<std::string> expected {"C", "n", "string",  "constructor", "constructor", "main",
                                           "c", "C", "console", "log",         "c",           "n"};
        ASSERT_EQ(arg.ids, expected);
    }
}

TEST_F(Es2PandaLibTest, LogDiagnostic)
{
    CONTEXT(ES2PANDA_STATE_ASM_GENERATED, "", "user-error.ets")
    {
        auto pos = GetImpl()->CreateSourcePosition(GetContext(), 3, 5);
        auto diagnostics = GetImpl()->GetPluginErrors(GetContext());
        auto diagnosticStorage = reinterpret_cast<const ark::es2panda::util::DiagnosticStorage *>(diagnostics);
        ASSERT_EQ(diagnosticStorage->size(), 0);

        auto kind = GetImpl()->CreateDiagnosticKind(GetContext(), "Test {}", ES2PANDA_PLUGIN_ERROR);
        const char **args = new const char *[1];
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        args[0] = "1";
        GetImpl()->LogDiagnostic(GetContext(), kind, args, 1, pos);
        delete[] args;
        ASSERT_EQ(diagnosticStorage->size(), 1);
        ASSERT_EQ((*diagnosticStorage)[0]->Type(), ark::es2panda::util::DiagnosticType::PLUGIN_ERROR);
        ASSERT_EQ((*diagnosticStorage)[0]->Message(), "Test 1");
    }
}

}  // namespace
