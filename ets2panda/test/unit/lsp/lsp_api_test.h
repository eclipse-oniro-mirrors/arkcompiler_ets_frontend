/**
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

class LSPAPITests : public testing::Test {
public:
    LSPAPITests() = default;
    ~LSPAPITests() override
    {
        for (const auto &file : tempFiles_) {
            if (file.empty()) {
                continue;
            }
            if (remove(file.c_str()) != 0) {
                std::cerr << "Failed to delete file: " << file << std::endl;
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

    std::vector<std::string> CreateTempFile(std::vector<std::string> files, std::vector<std::string> texts)
    {
        std::vector<std::string> result = {};
        auto tempDir = testing::TempDir();
        for (size_t i = 0; i < files.size(); i++) {
            auto outPath = tempDir + files[i];
            std::ofstream outStream(outPath);
            if (outStream.fail()) {
                std::cerr << "Failed to open file: " << outPath << std::endl;
                return result;
            }
            outStream << texts[i];
            outStream.close();
            result.push_back(outPath);
        }
        tempFiles_.insert(tempFiles_.end(), result.begin(), result.end());
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
