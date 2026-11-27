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

#ifndef ES2PANDA_TEST_UNIT_MODULE_PROJECT_TEST_H
#define ES2PANDA_TEST_UNIT_MODULE_PROJECT_TEST_H

#include <gtest/gtest.h>

#include <chrono>
#include <fstream>
#include <sstream>

#include "util/arktsconfig.h"

namespace ark::es2panda::util::test {

class ModuleProjectTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        const auto tick = std::chrono::steady_clock::now().time_since_epoch().count();
        auto path = fs::temp_directory_path() / ("ets_imports_" + std::to_string(tick));
        ASSERT_TRUE(fs::create_directory(path));
        root_ = fs::canonical(path);
    }

    void TearDown() override
    {
        if (!root_.empty()) {
            fs::remove_all(root_);
        }
    }

    fs::path Path(std::string_view name) const
    {
        auto path = root_;
        path.append(name);
        path.make_preferred();
        return path;
    }

    void Write(std::string_view name, std::string_view contents = "export const value: int = 1;")
    {
        auto path = Path(name);
        fs::create_directories(path.parent_path());
        std::ofstream out(path, std::ios::binary);
        out << contents;
        out.close();
        ASSERT_TRUE(out) << path;
    }

    static std::string JsonPath(const fs::path &path)
    {
        std::ostringstream quoted;
        JsonEscape(quoted, path.string());
        return quoted.str();
    }

    fs::path root_;
};

}  // namespace ark::es2panda::util::test

#endif  // ES2PANDA_TEST_UNIT_MODULE_PROJECT_TEST_H
