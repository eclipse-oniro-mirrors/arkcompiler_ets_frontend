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
#include "test/utils/ast_verifier_test.h"
#include "public/public.h"
#include <fstream>
#include <cstdio>
namespace ark::es2panda::public_lib::test {
using ApiLifecycleTest = ::test::utils::AstVerifierTest;
TEST_F(ApiLifecycleTest, WB08_ExternalResultsSurviveTheirContext)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class Host {}", "host.ets")
    {
        auto *host = reinterpret_cast<Context *>(GetContext());
        const std::string fileName = "external_result_" + std::to_string(reinterpret_cast<uintptr_t>(host)) + ".ets";
        {
            std::ofstream file(fileName);
            file << "class External { value: int = 1; }";
        }
        const char *args[] = {::test::utils::PandaExecutablePathGetter::Get()[0], fileName.c_str()};
        auto *config = GetImpl()->CreateConfig(2, args);
        ASSERT_NE(config, nullptr);
        const char *files[] = {fileName.c_str()};
        auto *global = GetImpl()->CreateGlobalContext(config, files, 1, false);
        ASSERT_NE(global, nullptr);
        auto *external = GetImpl()->CreateCacheContextFromString(config, "class External { value: int = 1; }", files[0],
                                                                 global, true, false);
        ASSERT_NE(external, nullptr);
        GetImpl()->ProceedToState(external, ES2PANDA_STATE_PARSED);
        EXPECT_EQ(GetImpl()->ContextState(external), ES2PANDA_STATE_PARSED);
        struct Search {
            es2panda_Impl const *api;
            es2panda_AstNode *node = nullptr;
        } search {GetImpl()};
        auto *root = GetImpl()->ProgramAst(external, GetImpl()->ContextProgram(external));
        GetImpl()->AstNodeForEach(
            root,
            [](es2panda_AstNode *node, void *arg) {
                auto &s = *static_cast<Search *>(arg);
                if (s.api->IsClassDefinition(node))
                    s.node = node;
            },
            &search);
        ASSERT_NE(search.node, nullptr);
        auto *id = GetImpl()->ClassDefinitionIdent(external, search.node);
        auto *name = GetImpl()->IdentifierNameConst(external, id);
        const std::string expected(name);
        size_t length;
        auto *body = GetImpl()->ClassDefinitionBodyConst(external, search.node, &length);
        const std::vector<es2panda_AstNode *> saved(body, body + length);
        GetImpl()->DestroyContext(external);
        EXPECT_STREQ(name, expected.c_str());
        EXPECT_TRUE(std::equal(saved.begin(), saved.end(), body));
        GetImpl()->DestroyGlobalContext(global);
        GetImpl()->DestroyConfig(config);
        std::remove(fileName.c_str());
    }
}
}  // namespace ark::es2panda::public_lib::test
