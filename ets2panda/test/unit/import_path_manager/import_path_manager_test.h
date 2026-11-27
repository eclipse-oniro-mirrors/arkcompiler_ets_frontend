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

#ifndef ES2PANDA_TEST_UNIT_IMPORT_PATH_MANAGER_TEST_H
#define ES2PANDA_TEST_UNIT_IMPORT_PATH_MANAGER_TEST_H

#include "module_project_test.h"

#include <algorithm>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

#include "ir/expressions/literals/stringLiteral.h"
#include "public/public.h"
#include "util/diagnosticEngine.h"
#include "util/importPathManager.h"

namespace ark::es2panda::util::test {

class ImportPathManagerTest : public ModuleProjectTest {
protected:
    void SetUp() override
    {
        ASSERT_NO_FATAL_FAILURE(ModuleProjectTest::SetUp());
        Write("Main.ats", "export function main(): void {}");
    }

    void TearDown() override
    {
        Close();
        ModuleProjectTest::TearDown();
    }

    std::string CacheDirectory() const
    {
        return R"("cacheDir":)" + JsonPath(Path("cache"));
    }

    void Configure(std::string_view fields, const std::vector<std::string> &options = {},
                   std::string_view baseUrl = ".")
    {
        std::string json =
            R"({"compilerOptions":{"baseUrl":)" + JsonPath(fs::path(baseUrl)) + R"(,"rootDir":)" + JsonPath(root_);
        if (!fields.empty()) {
            json += ",";
            json += fields;
        }
        Write("arktsconfig.json", json + "}}");
        const auto configArg = "--arktsconfig=" + Path("arktsconfig.json").string();
        std::vector<const char *> args {
            ES2PANDA_BIN_PATH,
            "--extension=ats",
            "--stdlib=" IPM_TEST_STDLIB,
            configArg.c_str(),
        };
        for (const auto &option : options) {
            args.push_back(option.c_str());
        }
        config_ = impl_->CreateConfig(static_cast<int>(args.size()), args.data());
        ASSERT_NE(config_, nullptr);
    }

    void OpenFile(std::string_view name)
    {
        handle_ = impl_->CreateContextFromFile(config_, Path(name).string().c_str());
        ASSERT_NE(handle_, nullptr);
        ASSERT_EQ(impl_->ContextState(handle_), ES2PANDA_STATE_NEW) << impl_->ContextErrorMessage(handle_);
    }

    void Start(std::string_view fields = {}, const std::vector<std::string> &options = {})
    {
        ASSERT_NO_FATAL_FAILURE(Configure(fields, options));
        ASSERT_NO_FATAL_FAILURE(OpenFile("Main.ats"));
        Manager()->SetupGlobalProgram();
        ASSERT_NE(Main(), nullptr);
    }

    void Close()
    {
        if (handle_ != nullptr) {
            auto *engine = Context()->diagnosticEngine;
            if (engine != nullptr && engine->IsAnyError()) {
                ADD_FAILURE() << "Unexpected compiler diagnostic: " << engine->GetAnyError().Message();
            }
            impl_->DestroyContext(handle_);
            handle_ = nullptr;
        }
        if (config_ != nullptr) {
            // ImportCache uses process-wide logical module keys, including across compiler contexts.
            parser::ImportCache<parser::CacheType::SOURCES>::ClearAll();
            parser::ImportCache<parser::CacheType::METADATA>::ClearAll();
            impl_->DestroyConfigWithoutLog(config_);
            config_ = nullptr;
            impl_->MemFinalize();
        }
    }

    public_lib::Context *Context() const
    {
        return reinterpret_cast<public_lib::Context *>(handle_);
    }

    ImportPathManager *Manager() const
    {
        return Context()->parser->GetImportPathManager();
    }

    parser::Program *Main() const
    {
        return Context()->parserProgram;
    }

    ir::StringLiteral *Literal(parser::Program *importer, std::string_view path)
    {
        auto *literal = Context()->Allocator()->New<ir::StringLiteral>(StringView {path});
        literal->SetStart(lexer::SourcePosition(importer));
        return literal;
    }

    parser::Program *Import(std::string_view path, parser::Program *importer = nullptr)
    {
        if (importer == nullptr) {
            importer = Main();
        }
        return Manager()->GatherImportInfo(importer, Literal(importer, path));
    }

    fs::path Resolve(std::string_view path)
    {
        return std::string(Manager()->ResolvePathAPI(Main(), Literal(Main(), path)).Utf8());
    }

    void ExpectDiagnostic(const diagnostic::DiagnosticKind &expected)
    {
        const auto &entries = Context()->diagnosticEngine->GetDiagnosticStorage(expected.Type());
        EXPECT_TRUE(std::any_of(entries.begin(), entries.end(), [&expected](const auto &entry) {
            return entry->GetId() == expected.Id();
        })) << expected.Message();
    }

    const es2panda_Impl *impl_ = es2panda_GetImpl(ES2PANDA_LIB_VERSION);
    es2panda_Config *config_ = nullptr;
    es2panda_Context *handle_ = nullptr;
};
}  // namespace ark::es2panda::util::test

#endif  // ES2PANDA_TEST_UNIT_IMPORT_PATH_MANAGER_TEST_H
