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

#include "gtest/gtest.h"
#include "util/nameMangler.h"
#include "util/ustring.h"

using ark::es2panda::util::NameMangler;
using ark::es2panda::util::StringView;

class TestNameMangling : public ::testing::Test {
private:
    NameMangler *mangler_ = nullptr;

    void SetUp() override
    {
        mangler_ = NameMangler::GetInstance();
    }

public:
    NameMangler *GetMangler()
    {
        return mangler_;
    }
};

TEST_F(TestNameMangling, asyncNameGen)
{
    std::string mangledName =
        GetMangler()->CreateMangledNameByTypeAndName(NameMangler::LangFeatureType::ASYNC, "testFunc");
    std::string expectedResult = "%%async-testFunc";

    EXPECT_EQ(mangledName, expectedResult);
}

/* TEST_F(TestNameMangling, getterNameGen)
{
    std::string mangledName = GetMangler()->CreateMangledNameByTypeAndName(NameMangler::LangFeatureType::GET, "myProp");
    std::string expectedResult = "%%get-myProp";

    EXPECT_EQ(mangledName, expectedResult);
}

TEST_F(TestNameMangling, partialNameGen)
{
    std::string mangledName =
        GetMangler()->CreateMangledNameByTypeAndName(NameMangler::LangFeatureType::PARTIAL, "MyPartialClass");
    std::string expectedResult = "%%partial-MyPartialClass";

    EXPECT_EQ(mangledName, expectedResult);
}

TEST_F(TestNameMangling, propertyNameGen)
{
    std::string mangledName =
        GetMangler()->CreateMangledNameByTypeAndName(NameMangler::LangFeatureType::PROPERTY, "myProp");
    std::string expectedResult = "%%property-myProp";

    EXPECT_EQ(mangledName, expectedResult);
}

TEST_F(TestNameMangling, setterNameGen)
{
    std::string mangledName = GetMangler()->CreateMangledNameByTypeAndName(NameMangler::LangFeatureType::SET, "myProp");
    std::string expectedResult = "%%set-myProp";

    EXPECT_EQ(mangledName, expectedResult);
} */

TEST_F(TestNameMangling, lambdaInvokeNameGen)
{
    size_t counter = 0;
    std::string mangledName = GetMangler()->CreateMangledNameForLambdaInvoke(counter++);
    std::string expectedResult = "lambda_invoke-0";

    EXPECT_EQ(mangledName, expectedResult);

    mangledName = GetMangler()->CreateMangledNameForLambdaInvoke(counter++);
    expectedResult = "lambda_invoke-1";

    EXPECT_EQ(mangledName, expectedResult);
}

TEST_F(TestNameMangling, lambdaObjNameGen)
{
    size_t counter = 0;
    std::string lambdaInvokeName = GetMangler()->CreateMangledNameForLambdaInvoke(counter++);
    std::string mangledName = GetMangler()->CreateMangledNameForLambdaObject(StringView(lambdaInvokeName));
    std::string expectedResult = "%%lambda-lambda_invoke-0";

    EXPECT_EQ(mangledName, expectedResult);

    lambdaInvokeName = GetMangler()->CreateMangledNameForLambdaInvoke(counter++);
    mangledName = GetMangler()->CreateMangledNameForLambdaObject(StringView(lambdaInvokeName));
    expectedResult = "%%lambda-lambda_invoke-1";

    EXPECT_EQ(mangledName, expectedResult);
}

TEST_F(TestNameMangling, unionPropNameGen)
{
    std::string mangledName = GetMangler()->CreateMangledNameForUnionProperty("std.core.Double");
    std::string expectedResult = "%%union_prop-std_core_Double";

    EXPECT_EQ(mangledName, expectedResult);
}
