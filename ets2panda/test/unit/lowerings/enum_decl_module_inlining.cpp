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

#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <string>
#include <thread>

#include "lowering_test.h"

namespace ark::es2panda {

// An ambient `declare enum` is not compiled by the current build (no static
// initialization is generated for it). A member read must be inlined as a
// compile-time literal instead of a runtime valueOf() call (a static field
// load of the member): the field is never initialized, so the call would
// fail at runtime.
TEST_F(LoweringTest, TestAmbientEnumMemberIsInlined)
{
    char const *text = R"(
        declare enum PhotoKeys { URI = 'uri', LOCAL_ASSET_SIZE = 'local_asset_size' }
        function read(): string { return PhotoKeys.LOCAL_ASSET_SIZE; }
    )";

    CONTEXT(ES2PANDA_STATE_LOWERED, text)
    {
        const auto *const ast = GetAst();

        // The inlined literal is the only way 'local_asset_size' can appear
        // in the AST: a runtime valueOf() read leaves no such StringLiteral.
        ASSERT_TRUE(ast->IsAnyChild([](ir::AstNode *const node) {
            return node->IsStringLiteral() && node->AsStringLiteral()->Str().Is("local_asset_size");
        }));
    }
}

TEST_F(LoweringTest, TestEnumMemberFromDeclarationModuleIsInlined)
{
    // The declaration module and the importer have to exist on disk so that
    // the relative import resolves to a declaration-module program.
    const auto dir = std::filesystem::temp_directory_path() /
                     ("enum_decl_module_" + std::to_string(std::hash<std::thread::id> {}(std::this_thread::get_id())) +
                      "_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directory(dir);
    {
        std::ofstream {dir / "dep.d.ets"}
            << "export enum PhotoKeys { URI = 'uri', LOCAL_ASSET_SIZE = 'local_asset_size' }\n";
        std::ofstream {dir / "main.ets"} << "import { PhotoKeys } from './dep';\n"
                                            "function read(): string { return PhotoKeys.LOCAL_ASSET_SIZE; }\n";
    }

    const auto mainPath = (dir / "main.ets").string();

    CONTEXT_FROM_FILE(ES2PANDA_STATE_LOWERED, mainPath.c_str())
    {
        const auto *const ast = GetAst();

        EXPECT_TRUE(ast->IsAnyChild([](ir::AstNode *const node) {
            return node->IsStringLiteral() && node->AsStringLiteral()->Str().Is("local_asset_size");
        }));
    }

    std::error_code error;
    std::filesystem::remove_all(dir, error);
}

}  // namespace ark::es2panda
