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
#include "test/utils/ast_verifier_test.h"
#include "public/public.h"
#include "ir/base/classDefinition.h"
#include "ir/expressions/identifier.h"
#include "util/eheap.h"

namespace ark::es2panda::public_lib::test {
using ApiResultsTest = ::test::utils::AstVerifierTest;

TEST_F(ApiResultsTest, RepeatedNamesDoNotGrowEHeapAndEditsKeepOldResults)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class RepeatedName {}", "names.ets")
    {
        ir::Identifier *id = nullptr;
        GetAst()->IterateRecursively([&id](ir::AstNode *node) {
            if (node->IsIdentifier() && node->AsIdentifier()->Name().Is("RepeatedName")) {
                id = node->AsIdentifier();
            }
        });
        ASSERT_NE(id, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(id);
        auto *oldName = GetImpl()->IdentifierNameConst(GetContext(), peer);
        ASSERT_STREQ(oldName, "RepeatedName");
        const auto before = EHeap::AllocatedSize();
        const size_t nameReads = 10000;
        for (size_t i = 0; i < nameReads; ++i) {
            ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "RepeatedName");
        }
        EXPECT_EQ(EHeap::AllocatedSize(), before);
        id->SetName(util::StringView("ChangedName"));
        EXPECT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "ChangedName");
        EXPECT_STREQ(oldName, "RepeatedName");
    }
}

TEST_F(ApiResultsTest, LargeClassBodyReadsReuseSnapshotsAndDetectInPlaceEdits)
{
    std::string source = "class Component {";
    const size_t propertyCount = 502;
    for (size_t i = 0; i < propertyCount; ++i) {
        source += "state" + std::to_string(i) + ": string = 'default';";
    }
    source += "}";
    CONTEXT(ES2PANDA_STATE_PARSED, source.c_str(), "body.ets")
    {
        ir::ClassDefinition *definition = nullptr;
        GetAst()->IterateRecursively([&definition](ir::AstNode *node) {
            if (node->IsClassDefinition()) {
                definition = node->AsClassDefinition();
            }
        });
        ASSERT_NE(definition, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(definition);
        size_t count = 0;
        auto *oldBody = GetImpl()->ClassDefinitionBodyConst(GetContext(), peer, &count);
        const size_t minPropertyCount = 502;
        ASSERT_GE(count, minPropertyCount);
        const std::vector<es2panda_AstNode *> saved(oldBody, oldBody + count);
        const auto before = EHeap::AllocatedSize();
        const size_t bodyReads = 1000;
        for (size_t i = 0; i < bodyReads; ++i) {
            size_t length = 0;
            auto *body = GetImpl()->ClassDefinitionBodyConst(GetContext(), peer, &length);
            ASSERT_EQ(length, count);
            EXPECT_EQ(body, oldBody);
        }
        EXPECT_EQ(EHeap::AllocatedSize(), before);
        auto &body = definition->BodyForUpdate();
        std::swap(body[0], body[1]);
        size_t changedCount = 0;
        auto *changedBody = GetImpl()->ClassDefinitionBodyConst(GetContext(), peer, &changedCount);
        EXPECT_EQ(changedCount, count);
        EXPECT_EQ(changedBody[0], saved[1]);
        EXPECT_EQ(changedBody[1], saved[0]);
        EXPECT_TRUE(std::equal(saved.begin(), saved.end(), oldBody));
        std::swap(body[0], body[1]);
        body.push_back(body[0]);
        size_t extendedCount = 0;
        auto *extended = GetImpl()->ClassDefinitionBodyConst(GetContext(), peer, &extendedCount);
        EXPECT_EQ(extendedCount, count + 1U);
        EXPECT_EQ(extended[count], saved[0]);
        EXPECT_TRUE(std::equal(saved.begin(), saved.end(), oldBody));
        body.pop_back();
        EXPECT_EQ(GetImpl()->ClassDefinitionBodyConst(GetContext(), peer, &count), oldBody);
    }
}

TEST(ApiResultStorageTest, StringsAndArraysStayStableAcrossTableGrowth)
{
    ApiResultStorage results;
    auto *text = results.String("persistent");
    char first = 1;
    char second = 2;
    char *input[] = {&first, nullptr, &second};
    auto *array = CopyApiPointerResult<char *>(results, 3, [&input](size_t index) { return input[index]; });
    const size_t growthEntries = 10000;
    for (size_t i = 0; i < growthEntries; ++i) {
        results.String(std::to_string(i));
    }
    EXPECT_STREQ(text, "persistent");
    EXPECT_EQ(array[0], &first);
    EXPECT_EQ(array[1], nullptr);
    EXPECT_EQ(array[2], &second);
    EXPECT_EQ(CopyApiPointerResult<char *>(results, 3, [&input](size_t index) { return input[index]; }), array);
    auto *empty = CopyApiPointerResult<char *>(results, 0, [](size_t) -> char * { return nullptr; });
    EXPECT_NE(empty, nullptr);
    EXPECT_NE(results.String(""), nullptr);
    const std::string embedded("a\0b", 3);
    auto *binary = results.String(embedded);
    EXPECT_EQ(std::string_view(binary, 3), embedded);
    EXPECT_EQ(binary[3], '\0');
}

TEST(ApiResultStorageTest, SeparateContextsDoNotShareResultStorage)
{
    ApiResultStorage first;
    ApiResultStorage second;
    EXPECT_NE(first.String("name"), second.String("name"));
    char node = 0;
    char *values[] = {&node};
    EXPECT_NE(CopyApiPointerResult<char *>(first, 1, [&values](size_t index) { return values[index]; }),
              CopyApiPointerResult<char *>(second, 1, [&values](size_t index) { return values[index]; }));
    first.Clear();
    EXPECT_STREQ(second.String("name"), "name");
}

TEST_F(ApiResultsTest, DumpReadsReuseContentsAndReflectAstEdits)
{
    CONTEXT(ES2PANDA_STATE_PARSED, "class Before {}", "dump.ets")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const auto before = EHeap::AllocatedSize();
        const size_t dumpReads = 100;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
        // Dump still runs and may allocate small internal helpers; the final
        // source-sized CString must not be allocated on every read.
        EXPECT_LE(EHeap::AllocatedSize() - before, dumpReads * 16U);
        GetAst()->IterateRecursively([](ir::AstNode *node) {
            if (node->IsIdentifier() && node->AsIdentifier()->Name().Is("Before")) {
                node->AsIdentifier()->SetName(util::StringView("After"));
            }
        });
        const std::string changed(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root));
        EXPECT_NE(changed, original);
        EXPECT_NE(changed.find("After"), std::string::npos);
        EXPECT_STREQ(oldDump, original.c_str());
    }
}

TEST_F(ApiResultsTest, RecheckKeepsPreviouslyReturnedCopiesReadable)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, "class C { value: int = 1; }", "recheck.ets")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        GetImpl()->AstNodeRecheck(GetContext(), root);
        ASSERT_NE(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        EXPECT_STREQ(oldDump, original.c_str());
        auto *currentDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root), currentDump);
    }
}
}  // namespace ark::es2panda::public_lib::test
