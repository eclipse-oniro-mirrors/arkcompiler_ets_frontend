/**
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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
#include <vector>

#include "driver/dependency_analyzer/dep_analyzer.h"
#include "libarkbase/macros.h"
#include "path_getter.h"
#include "libarkbase/os/filesystem.h"

namespace {

class DepAnalyzerTest : public testing::Test {
public:
    DepAnalyzerTest() = default;
    ~DepAnalyzerTest() override = default;
    NO_COPY_SEMANTIC(DepAnalyzerTest);
    NO_MOVE_SEMANTIC(DepAnalyzerTest);

    void RunDepAnalyzer(size_t testFolderNum)
    {
        std::string binPath = test::utils::DepAnalyzerTestsBinPathGet();
        std::string testPath = test::utils::DepAnalyzerTestsPathGet(testFolderNum, 1);
        options_ = {binPath.c_str(), testPath.c_str()};

        depAnalyzer_.AnalyzeDeps(optionsSize_, options_.data());
    }

    void GetExpectedAns(std::vector<std::string> &expected, size_t testFolderNum, size_t testFileCounter)
    {
        for (size_t i = 1; i < testFileCounter + 1; ++i) {
            std::string expPath = test::utils::DepAnalyzerTestsPathGet(testFolderNum, i);
            expected.push_back(ark::os::GetAbsolutePath(expPath));
        }
    }

    const std::vector<std::string> &GetTestSourcePaths()
    {
        return depAnalyzer_.GetSourcePaths();
    }

    std::unordered_map<std::string, std::unordered_set<std::string>> &GetTestFileDirectDependencies()
    {
        return depAnalyzer_.GetFileDirectDependencies();
    }

    std::unordered_map<std::string, std::unordered_set<std::string>> &GetTestFileDirectDependants()
    {
        return depAnalyzer_.GetFileDirectDependants();
    }

private:
    DepAnalyzer depAnalyzer_;

    const size_t optionsSize_ = 2;
    std::vector<const char *> options_ = {};
};

TEST_F(DepAnalyzerTest, Subtestv1)
{
    size_t testFolderNum = 1;
    RunDepAnalyzer(testFolderNum);
    std::unordered_map<std::string, std::unordered_set<std::string>> dependenciesExpected;
    std::unordered_map<std::string, std::unordered_set<std::string>> dependentsExpected;
    std::string file1 = test::utils::DepAnalyzerTestsPathGet(1, 1);
    dependenciesExpected[file1] = {};
    dependentsExpected[file1] = {};
    ASSERT(GetTestFileDirectDependencies() == dependenciesExpected);
    ASSERT(GetTestFileDirectDependants() == dependentsExpected);
}

TEST_F(DepAnalyzerTest, Subtestv2)
{
    size_t testFolderNum = 2;
    RunDepAnalyzer(testFolderNum);
    std::unordered_map<std::string, std::unordered_set<std::string>> dependenciesExpected;
    std::unordered_map<std::string, std::unordered_set<std::string>> dependentsExpected;
    std::string file1 = test::utils::DepAnalyzerTestsPathGet(2, 1);
    std::string file2 = test::utils::DepAnalyzerTestsPathGet(2, 2);
    std::string file3 = test::utils::DepAnalyzerTestsPathGet(2, 3);
    std::string file4 = test::utils::DepAnalyzerTestsPathGet(2, 4);
    dependenciesExpected[file1] = {file2};
    dependenciesExpected[file2] = {file3};
    dependenciesExpected[file3] = {file2, file4};
    dependenciesExpected[file4] = {file2};
    dependentsExpected[file1] = {};
    dependentsExpected[file2] = {file3, file4, file1};
    dependentsExpected[file3] = {file2};
    dependentsExpected[file4] = {file3};
    ASSERT(GetTestFileDirectDependencies() == dependenciesExpected);
    ASSERT(GetTestFileDirectDependants() == dependentsExpected);
}

TEST_F(DepAnalyzerTest, Subtestv3)
{
    size_t testFolderNum = 3;
    RunDepAnalyzer(testFolderNum);
    std::unordered_map<std::string, std::unordered_set<std::string>> dependenciesExpected;
    std::unordered_map<std::string, std::unordered_set<std::string>> dependentsExpected;
    std::string file1 = test::utils::DepAnalyzerTestsPathGet(3, 1);
    std::string file2 = test::utils::DepAnalyzerTestsPathGet(3, 2);
    dependenciesExpected[file1] = {file2};
    dependentsExpected[file1] = {};
    dependentsExpected[file2] = {file1};
    ASSERT(GetTestFileDirectDependencies() == dependenciesExpected);
    ASSERT(GetTestFileDirectDependants() == dependentsExpected);
}

TEST_F(DepAnalyzerTest, Subtestv4)
{
    size_t testFolderNum = 4;
    RunDepAnalyzer(testFolderNum);
    std::unordered_map<std::string, std::unordered_set<std::string>> dependenciesExpected;
    std::unordered_map<std::string, std::unordered_set<std::string>> dependentsExpected;
    std::string file1 = test::utils::DepAnalyzerTestsPathGet(4, 1);
    std::string file2 = test::utils::DepAnalyzerTestsPathGet(4, 2);
    dependenciesExpected[file1] = {file2};
    dependentsExpected[file1] = {};
    dependentsExpected[file2] = {file1};
    ASSERT(GetTestFileDirectDependencies() == dependenciesExpected);
    ASSERT(GetTestFileDirectDependants() == dependentsExpected);
}

}  // namespace
