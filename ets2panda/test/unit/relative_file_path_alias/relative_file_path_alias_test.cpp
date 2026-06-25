/**
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

#include "test/utils/asm_test.h"
#include "relative_file_path_alias_paths.h"

#ifndef ES2PANDA_BIN_PATH
#error "ES2PANDA_BIN_PATH is not defined (pass it from CMakeLists.txt)"
#endif

namespace {

std::string ReadEntireFile(const std::string &path)
{
    std::ifstream ifs(path);
    std::stringstream buffer;
    buffer << ifs.rdbuf();
    return buffer.str();
}

bool IsUserDefined(std::string_view name)
{
    return name.find("std.") == std::string_view::npos && name.find("std:") == std::string_view::npos &&
           name.find("arkruntime") == std::string_view::npos;
}

bool EndsWith(std::string_view value, std::string_view suffix)
{
    return value.size() >= suffix.size() && value.substr(value.size() - suffix.size()) == suffix;
}

std::unique_ptr<ark::pandasm::Program> CompileEtsModuleFile(const std::string &absoluteInputPath,
                                                            const std::string &arktsconfigPath, std::string_view src)
{
    std::string optCfg = std::string("--arktsconfig=") + arktsconfigPath;
    std::vector<std::string> storage = {
        ES2PANDA_BIN_PATH, "--extension=ets", "--ets-module=true", "--opt-level=0", std::move(optCfg),
    };
    std::vector<const char *> argv;
    argv.reserve(storage.size());
    for (auto &s : storage) {
        argv.push_back(s.c_str());
    }
    return ::test::utils::AsmTest::GetProgram(ark::Span<const char *const>(argv.data(), argv.size()), absoluteInputPath,
                                              src);
}

}  // namespace

namespace ark::es2panda::compiler::test {

class RelativeFilePathAliasTest : public ::test::utils::AsmTest {};

TEST_F(RelativeFilePathAliasTest, DirectoryPrefixMapsToPathsAlias)
{
    const std::filesystem::path base = RFPA_DATA_DIR;
    const std::string entryPath = (base / "src" / "mod" / "entry.ets").string();
    const std::string cfgPath = (base / "arktsconfig.json").string();

    const std::string src = ReadEntireFile(entryPath);
    ASSERT_FALSE(src.empty()) << "Missing test data at " << entryPath;

    auto program = CompileEtsModuleFile(entryPath, cfgPath, src);
    ASSERT_NE(program, nullptr);

    for (const auto &[name, fn] : program->functionStaticTable) {
        if (!IsUserDefined(name)) {
            continue;
        }
        EXPECT_EQ(fn.sourceFile, "@ut_alias/entry.ets") << "function=" << name << " sourceFile=" << fn.sourceFile;
    }
}

TEST_F(RelativeFilePathAliasTest, ExactPathMapsToKeyWithPreservedExtension)
{
    const std::filesystem::path base = RFPA_DATA_DIR;
    const std::string flatPath = (base / "flat_file.ets").string();
    const std::string cfgPath = (base / "arktsconfig.json").string();

    const std::string src = ReadEntireFile(flatPath);
    ASSERT_FALSE(src.empty()) << "Missing test data at " << flatPath;

    auto program = CompileEtsModuleFile(flatPath, cfgPath, src);
    ASSERT_NE(program, nullptr);

    for (const auto &[name, fn] : program->functionStaticTable) {
        if (!IsUserDefined(name)) {
            continue;
        }
        EXPECT_EQ(fn.sourceFile, "@ut_file.ets") << "function=" << name << " sourceFile=" << fn.sourceFile;
    }
}

TEST_F(RelativeFilePathAliasTest, DeclarationSourceFileKeepsFileName)
{
    const std::filesystem::path base = RFPA_DATA_DIR;
    const std::string declPath = (base / "src" / "mod" / "decl_entry.d.ets").string();
    const std::string cfgPath = (base / "arktsconfig.json").string();

    const std::string src = ReadEntireFile(declPath);
    ASSERT_FALSE(src.empty()) << "Missing test data at " << declPath;

    auto program = CompileEtsModuleFile(declPath, cfgPath, src);
    ASSERT_NE(program, nullptr);

    bool found = false;
    for (const auto &[name, record] : program->recordTable) {
        if (!IsUserDefined(name)) {
            continue;
        }
        found = true;
        EXPECT_TRUE(EndsWith(record.sourceFile, "decl_entry.d.ets"))
            << "record=" << name << " sourceFile=" << record.sourceFile;
    }
    EXPECT_TRUE(found) << "Expected at least one user-defined record";
}

}  // namespace ark::es2panda::compiler::test
