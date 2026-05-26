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

#include <gtest/gtest.h>
#include "ir/astNode.h"
#include "ir/ets/etsIntrinsicNode.h"
#include "public/es2panda_lib.h"
#include "test/utils/panda_executable_path_getter.h"

#include <array>

#if defined(__has_feature)
#if __has_feature(address_sanitizer)
#define ES2PANDA_TEST_WITH_ASAN 1
#endif
#endif

#if defined(__SANITIZE_ADDRESS__)
#define ES2PANDA_TEST_WITH_ASAN 1
#endif

namespace ark::es2panda {

class RelaxedAnyLoweringTest : public testing::Test {
public:
    RelaxedAnyLoweringTest() = default;

    void SetUp() override
    {
#if defined(ES2PANDA_TEST_WITH_ASAN)
        GTEST_SKIP() << "Skip relaxed-any lowering CAPI checks under ASan to avoid leak noise.";
#endif

        impl_ = es2panda_GetImpl(ES2PANDA_LIB_VERSION);
        impl_->MemInitialize();

        auto es2pandaPath = test::utils::PandaExecutablePathGetter::Get()[0];
        std::array argv = {es2pandaPath, "--permit-relaxed-any"};
        cfg_ = impl_->CreateConfig(argv.size(), argv.data());
    }

    ~RelaxedAnyLoweringTest() override
    {
        if (impl_ == nullptr) {
            return;
        }

        impl_->DestroyConfig(cfg_);
        impl_->MemFinalize();
    }

    NO_COPY_SEMANTIC(RelaxedAnyLoweringTest);
    NO_MOVE_SEMANTIC(RelaxedAnyLoweringTest);

protected:
    es2panda_Impl const *impl_ {};
    es2panda_Config *cfg_ {};
};

ir::ETSIntrinsicNode *FindStoreIntrinsic(es2panda_Impl const *impl, es2panda_Context *ctx, char const *intrinsicName)
{
    auto *ast = reinterpret_cast<ir::AstNode *>(impl->ProgramAst(ctx, impl->ContextProgram(ctx)));
    auto *storeIntrin = ast->FindChild([intrinsicName](ir::AstNode *child) {
        return child->IsETSIntrinsicNode() && child->AsETSIntrinsicNode()->Id().Is(intrinsicName);
    });
    return storeIntrin == nullptr ? nullptr : storeIntrin->AsETSIntrinsicNode();
}

void CheckRelaxedAnyStoreLowering(es2panda_Impl const *impl, es2panda_Config *cfg, char const *text,
                                  char const *intrinsicName)
{
    auto *ctx = impl->CreateContextFromString(cfg, text, "relaxed_any_lowering.ets");
    ASSERT_NE(ctx, nullptr);

    impl->ProceedToState(ctx, ES2PANDA_STATE_LOWERED);
    ASSERT_EQ(impl->ContextState(ctx), ES2PANDA_STATE_LOWERED) << impl->ContextErrorMessage(ctx);

    auto *storeIntrin = FindStoreIntrinsic(impl, ctx, intrinsicName);
    ASSERT_NE(storeIntrin, nullptr);
    EXPECT_FALSE(storeIntrin->Parent()->IsTSAsExpression());

    impl->DestroyContext(ctx);
}

TEST_F(RelaxedAnyLoweringTest, StoreByIndexDoesNotCreateNumericAsExpression)
{
    char const *text = R"(
        function fn(v: any, i: double): void {
            v[i - 1] = i
        }
    )";

    CheckRelaxedAnyStoreLowering(impl_, cfg_, text, "anystbyidx");
}

TEST_F(RelaxedAnyLoweringTest, StoreByNameDoesNotCreateNumericAsExpression)
{
    char const *text = R"(
        function fn(v: any, value: double): void {
            v.result = value
        }
    )";

    CheckRelaxedAnyStoreLowering(impl_, cfg_, text, "anystbyname");
}

TEST_F(RelaxedAnyLoweringTest, StoreByValueDoesNotCreateNumericAsExpression)
{
    char const *text = R"(
        function fn(v: any, key: string, value: double): void {
            v[key] = value
        }
    )";

    CheckRelaxedAnyStoreLowering(impl_, cfg_, text, "anystbyval");
}

}  // namespace ark::es2panda
