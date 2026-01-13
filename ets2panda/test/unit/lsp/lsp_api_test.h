/**
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

#ifndef ES2PANDA_TEST_LSP_API_TEST_H
#define ES2PANDA_TEST_LSP_API_TEST_H

#include "lsp/include/api.h"
#include "lsp/include/internal_api.h"
#include "public/public.h"
#include <gtest/gtest.h>
#include <cstdio>
#include <unistd.h>

class LSPAPITests : public testing::Test {
public:
    LSPAPITests() = default;
    ~LSPAPITests() override
    {
        for (const auto &file : tempFiles_) {
            if (file.empty()) {
                continue;
            }

            std::filesystem::path path = file;

            if (!std::filesystem::exists(path)) {
                std::cerr << "Path does not exist: " << file << std::endl;
                continue;
            }

            try {
                if (std::filesystem::is_directory(path)) {
                    std::filesystem::remove_all(path);
                } else {
                    std::filesystem::remove(path);
                }
            } catch (const std::filesystem::filesystem_error &e) {
                std::cerr << "Failed to delete " << file << ": " << e.what() << std::endl;
            }
        }
    }

    NO_COPY_SEMANTIC(LSPAPITests);
    NO_MOVE_SEMANTIC(LSPAPITests);

    es2panda_ContextState ContextState(es2panda_Context *context)
    {
        auto *s = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
        return s->state;
    }

    template <typename Ast>
    Ast *GetAstFromContext(es2panda_Context *context)
    {
        auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
        auto ast = reinterpret_cast<Ast *>(ctx->parserProgram->Ast());
        return ast;
    }

    std::string GetExecutableName()
    {
        char exe_path[PATH_MAX] = {0};
        ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
        if (len == -1) {
            return "unknown_exe";
        }
        return std::filesystem::path(std::string(exe_path, len)).filename().string();
    }

    std::vector<std::string> CreateTempFile(std::vector<std::string> files, std::vector<std::string> texts)
    {
        std::vector<std::string> result = {};
        std::filesystem::path tempDir = testing::TempDir();
        tempDir.append(GetExecutableName());
        std::filesystem::create_directory(tempDir);
        for (size_t i = 0; i < files.size(); i++) {
            std::filesystem::path outPath = tempDir;
            std::ofstream outStream(outPath.append(files[i]));
            if (outStream.fail()) {
                std::cerr << "Failed to open file: " << outPath << std::endl;
                return result;
            }
            outStream << texts[i];
            outStream.close();
            result.push_back(outPath);
        }
        tempFiles_.push_back(tempDir);
        return result;
    }

    Range CreateTestRange()
    {
        int const endPos = 10;
        Position start(1, 0);
        Position end(1, endPos);
        return Range(start, end);
    }

    void SetUp() override
    {
        range_ = CreateTestRange();
        message_ = R"(Test message)";
    }

protected:
    // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
    Range range_;
    std::string message_;
    std::vector<std::string> tempFiles_;
    // NOLINTEND(misc-non-private-member-variables-in-classes)
};

#endif
