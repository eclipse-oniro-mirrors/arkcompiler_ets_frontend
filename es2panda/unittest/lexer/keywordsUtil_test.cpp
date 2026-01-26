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

#include <lexer/keywordsUtil.h>
#include <lexer/lexer.h>
#include <memory>
#include <parser/context/parserContext.h>
#include <parser/program/program.h>
#include <es2panda.h>
#include <gtest/gtest.h>
#include <mem/pool_manager.h>

namespace panda::es2panda::lexer {

using mem::MemConfig;

class MemManager {
public:
    explicit MemManager()
    {
        constexpr auto COMPILER_SIZE = 8192_MB;

        MemConfig::Initialize(0, 0, COMPILER_SIZE, 0);
        PoolManager::Initialize(PoolType::MMAP);
    }

    NO_COPY_SEMANTIC(MemManager);
    NO_MOVE_SEMANTIC(MemManager);

    ~MemManager()
    {
        PoolManager::Finalize();
        MemConfig::Finalize();
    }
};

class KeywordsUtilTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        mm_ = std::make_unique<MemManager>();
        program_ = std::make_unique<parser::Program>(es2panda::ScriptExtension::JS);
        parserContext_ = std::make_unique<parser::ParserContext>(program_.get());
    }

    void TearDown() override
    {
        parserContext_.reset();
        program_.reset();
        mm_.reset();
    }

    std::unique_ptr<MemManager> mm_;
    std::unique_ptr<parser::Program> program_;
    std::unique_ptr<parser::ParserContext> parserContext_;
};

// Test IsIdentifierStart with ASCII characters
TEST_F(KeywordsUtilTest, TestIsIdentifierStartASCII)
{
    // Letters (should be identifier start)
    EXPECT_TRUE(KeywordsUtil::IsIdentifierStart('a'));
    EXPECT_TRUE(KeywordsUtil::IsIdentifierStart('z'));
    EXPECT_TRUE(KeywordsUtil::IsIdentifierStart('A'));
    EXPECT_TRUE(KeywordsUtil::IsIdentifierStart('Z'));
    EXPECT_TRUE(KeywordsUtil::IsIdentifierStart('_'));
    EXPECT_TRUE(KeywordsUtil::IsIdentifierStart('$'));

    // Numbers (should NOT be identifier start)
    EXPECT_FALSE(KeywordsUtil::IsIdentifierStart('0'));
    EXPECT_FALSE(KeywordsUtil::IsIdentifierStart('9'));

    // Punctuators (should NOT be identifier start)
    EXPECT_FALSE(KeywordsUtil::IsIdentifierStart('+'));
    EXPECT_FALSE(KeywordsUtil::IsIdentifierStart('-'));
    EXPECT_FALSE(KeywordsUtil::IsIdentifierStart('('));
    EXPECT_FALSE(KeywordsUtil::IsIdentifierStart(')'));
    EXPECT_FALSE(KeywordsUtil::IsIdentifierStart(' '));
    EXPECT_FALSE(KeywordsUtil::IsIdentifierStart('\n'));
}

// Test IsIdentifierStart with Unicode characters (covers Unicode branch)
TEST_F(KeywordsUtilTest, TestIsIdentifierStartUnicode)
{
    // Chinese characters (should be identifier start)
    EXPECT_TRUE(KeywordsUtil::IsIdentifierStart(U'中'));
    EXPECT_TRUE(KeywordsUtil::IsIdentifierStart(U'文'));
    EXPECT_TRUE(KeywordsUtil::IsIdentifierStart(U'变'));
    EXPECT_TRUE(KeywordsUtil::IsIdentifierStart(U'量'));

    // Japanese Hiragana (should be identifier start)
    EXPECT_TRUE(KeywordsUtil::IsIdentifierStart(U'あ'));
    EXPECT_TRUE(KeywordsUtil::IsIdentifierStart(U'い'));

    // Japanese Katakana (should be identifier start)
    EXPECT_TRUE(KeywordsUtil::IsIdentifierStart(U'ア'));
    EXPECT_TRUE(KeywordsUtil::IsIdentifierStart(U'イ'));

    // Greek letters (should be identifier start)
    EXPECT_TRUE(KeywordsUtil::IsIdentifierStart(U'α'));
    EXPECT_TRUE(KeywordsUtil::IsIdentifierStart(U'β'));
    EXPECT_TRUE(KeywordsUtil::IsIdentifierStart(U'Α'));
    EXPECT_TRUE(KeywordsUtil::IsIdentifierStart(U'Β'));

    // Cyrillic letters (should be identifier start)
    EXPECT_TRUE(KeywordsUtil::IsIdentifierStart(U'а'));
    EXPECT_TRUE(KeywordsUtil::IsIdentifierStart(U'б'));
    EXPECT_TRUE(KeywordsUtil::IsIdentifierStart(U'А'));
    EXPECT_TRUE(KeywordsUtil::IsIdentifierStart(U'Б'));

    // Unicode symbols that are NOT identifier start
    EXPECT_FALSE(KeywordsUtil::IsIdentifierStart(U'©'));
    EXPECT_FALSE(KeywordsUtil::IsIdentifierStart(U'®'));
    EXPECT_FALSE(KeywordsUtil::IsIdentifierStart(U'€'));
    EXPECT_FALSE(KeywordsUtil::IsIdentifierStart(U'£'));
}

// Test IsIdentifierPart with ASCII characters
TEST_F(KeywordsUtilTest, TestIsIdentifierPartASCII)
{
    // Letters (should be identifier part)
    EXPECT_TRUE(KeywordsUtil::IsIdentifierPart('a'));
    EXPECT_TRUE(KeywordsUtil::IsIdentifierPart('z'));
    EXPECT_TRUE(KeywordsUtil::IsIdentifierPart('A'));
    EXPECT_TRUE(KeywordsUtil::IsIdentifierPart('Z'));
    EXPECT_TRUE(KeywordsUtil::IsIdentifierPart('_'));
    EXPECT_TRUE(KeywordsUtil::IsIdentifierPart('$'));

    // Numbers (should be identifier part, but not start)
    EXPECT_TRUE(KeywordsUtil::IsIdentifierPart('0'));
    EXPECT_TRUE(KeywordsUtil::IsIdentifierPart('9'));

    // Punctuators (should NOT be identifier part)
    EXPECT_FALSE(KeywordsUtil::IsIdentifierPart('+'));
    EXPECT_FALSE(KeywordsUtil::IsIdentifierPart('-'));
    EXPECT_FALSE(KeywordsUtil::IsIdentifierPart('('));
    EXPECT_FALSE(KeywordsUtil::IsIdentifierPart(')'));
    EXPECT_FALSE(KeywordsUtil::IsIdentifierPart(' '));
    EXPECT_FALSE(KeywordsUtil::IsIdentifierPart('\n'));
}

// Test IsIdentifierPart with Unicode characters (covers Unicode branch)
TEST_F(KeywordsUtilTest, TestIsIdentifierPartUnicode)
{
    // Chinese characters (should be identifier part)
    EXPECT_TRUE(KeywordsUtil::IsIdentifierPart(U'中'));
    EXPECT_TRUE(KeywordsUtil::IsIdentifierPart(U'文'));
    EXPECT_TRUE(KeywordsUtil::IsIdentifierPart(U'变'));
    EXPECT_TRUE(KeywordsUtil::IsIdentifierPart(U'量'));

    // Japanese Hiragana (should be identifier part)
    EXPECT_TRUE(KeywordsUtil::IsIdentifierPart(U'あ'));
    EXPECT_TRUE(KeywordsUtil::IsIdentifierPart(U'い'));

    // Japanese Katakana (should be identifier part)
    EXPECT_TRUE(KeywordsUtil::IsIdentifierPart(U'ア'));
    EXPECT_TRUE(KeywordsUtil::IsIdentifierPart(U'イ'));

    // Greek letters (should be identifier part)
    EXPECT_TRUE(KeywordsUtil::IsIdentifierPart(U'α'));
    EXPECT_TRUE(KeywordsUtil::IsIdentifierPart(U'β'));
    EXPECT_TRUE(KeywordsUtil::IsIdentifierPart(U'Α'));
    EXPECT_TRUE(KeywordsUtil::IsIdentifierPart(U'Β'));

    // Cyrillic letters (should be identifier part)
    EXPECT_TRUE(KeywordsUtil::IsIdentifierPart(U'а'));
    EXPECT_TRUE(KeywordsUtil::IsIdentifierPart(U'б'));
    EXPECT_TRUE(KeywordsUtil::IsIdentifierPart(U'А'));
    EXPECT_TRUE(KeywordsUtil::IsIdentifierPart(U'Б'));

    // Unicode combining marks (should be identifier part)
    EXPECT_TRUE(KeywordsUtil::IsIdentifierPart(U'\u0300'));
    EXPECT_TRUE(KeywordsUtil::IsIdentifierPart(U'\u0301'));

    // Unicode symbols that are NOT identifier part
    EXPECT_FALSE(KeywordsUtil::IsIdentifierPart(U'©'));
    EXPECT_FALSE(KeywordsUtil::IsIdentifierPart(U'®'));
    EXPECT_FALSE(KeywordsUtil::IsIdentifierPart(U'€'));
    EXPECT_FALSE(KeywordsUtil::IsIdentifierPart(U'£'));
}

}  // namespace panda::es2panda::lexer

