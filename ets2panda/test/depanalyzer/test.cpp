/**
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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

#include "driver/dependency_analyzer/dep_analyzer.h"
#include "path_getter.h"

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

        depAnalyzer_.AnalyzeDeps(binPath, "", {testPath});
    }

    const auto &GetTestFileDirectDependencies() const
    {
        return depAnalyzer_.GetDirectDependencies();
    }

    const auto &GetTestFileDirectDependants() const
    {
        return depAnalyzer_.GetDirectDependants();
    }

    using DirectDepsMap = DepAnalyzer::FileDependenciesMap;

private:
    DepAnalyzer depAnalyzer_;
};

TEST_F(DepAnalyzerTest, Subtestv1)
{
    size_t testFolderNum = 1;
    RunDepAnalyzer(testFolderNum);
    DirectDepsMap dependenciesExpected;
    DirectDepsMap dependentsExpected;
    std::string file1 = test::utils::DepAnalyzerTestsPathGet(1, 1);
    ASSERT(GetTestFileDirectDependencies() == dependenciesExpected);
    ASSERT(GetTestFileDirectDependants() == dependentsExpected);
}

TEST_F(DepAnalyzerTest, Subtestv2)
{
    size_t testFolderNum = 2;
    RunDepAnalyzer(testFolderNum);
    DirectDepsMap dependenciesExpected;
    DirectDepsMap dependentsExpected;
    std::string file1 = test::utils::DepAnalyzerTestsPathGet(2, 1);
    std::string file2 = test::utils::DepAnalyzerTestsPathGet(2, 2);
    std::string file3 = test::utils::DepAnalyzerTestsPathGet(2, 3);
    std::string file4 = test::utils::DepAnalyzerTestsPathGet(2, 4);
    dependenciesExpected[file1] = {file2};
    dependenciesExpected[file2] = {file3};
    dependenciesExpected[file3] = {file2, file4};
    dependenciesExpected[file4] = {file2};
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
    DirectDepsMap dependenciesExpected;
    DirectDepsMap dependentsExpected;
    std::string file1 = test::utils::DepAnalyzerTestsPathGet(3, 1);
    std::string file2 = test::utils::DepAnalyzerTestsPathGet(3, 2);
    dependenciesExpected[file1] = {file2};
    dependentsExpected[file2] = {file1};
    ASSERT(GetTestFileDirectDependencies() == dependenciesExpected);
    ASSERT(GetTestFileDirectDependants() == dependentsExpected);
}

TEST_F(DepAnalyzerTest, Subtestv4)
{
    size_t testFolderNum = 4;
    RunDepAnalyzer(testFolderNum);
    DirectDepsMap dependenciesExpected;
    DirectDepsMap dependentsExpected;
    std::string file1 = test::utils::DepAnalyzerTestsPathGet(4, 1);
    std::string file2 = test::utils::DepAnalyzerTestsPathGet(4, 2);
    dependenciesExpected[file1] = {file2};
    dependentsExpected[file2] = {file1};
    ASSERT(GetTestFileDirectDependencies() == dependenciesExpected);
    ASSERT(GetTestFileDirectDependants() == dependentsExpected);
}

}  // namespace
