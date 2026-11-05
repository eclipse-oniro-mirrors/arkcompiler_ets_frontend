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

#include "util/diagnosticEngine.h"
#include <gtest/gtest.h>
#include <memory>
#include <ostream>
#include <string>

using ark::es2panda::util::DiagnosticBase;
using ark::es2panda::util::DiagnosticEngine;
using ark::es2panda::util::DiagnosticPrinter;

namespace {

class SilentPrinter : public DiagnosticPrinter {
public:
    void Print(const DiagnosticBase &, const std::string &) const override {}
    void Print(const DiagnosticBase &, std::ostream &, const std::string &) const override {}
};

TEST(DiagnosticEngineErrorLimitTest, GetErrorCount_CountsSyntaxAndSemanticErrors)
{
    DiagnosticEngine de;
    EXPECT_EQ(de.GetErrorCount(), 0U);

    de.LogSemanticError("semantic err");
    de.LogSyntaxError("syntax err");
    EXPECT_EQ(de.GetErrorCount(), 2U);
}

TEST(DiagnosticEngineErrorLimitTest, ZeroLimit_DisablesLimit)
{
    DiagnosticEngine de;
    de.SetMaxErrorCount(0U);

    for (size_t i = 0; i < 100U; i++) {
        de.LogSemanticError("err");
    }

    EXPECT_FALSE(de.HasErrorLimitExceeded());
    EXPECT_EQ(de.GetErrorCount(), 100U);
}

TEST(DiagnosticEngineErrorLimitTest, ClearDiagnostics_ClearsCount)
{
    DiagnosticEngine de;
    de.SetMaxErrorCount(100U);

    de.LogSemanticError("e0");
    de.LogSemanticError("e1");
    EXPECT_EQ(de.GetErrorCount(), 2U);

    de.ClearDiagnostics();
    EXPECT_EQ(de.GetErrorCount(), 0U);
}

TEST(DiagnosticEngineErrorLimitTest, FlushDiagnostic_ClearsCount)
{
    DiagnosticEngine de;
    de.SetPrinter(std::make_unique<SilentPrinter>());
    de.SetMaxErrorCount(100U);

    de.LogSemanticError("e0");
    de.LogSemanticError("e1");
    EXPECT_EQ(de.GetErrorCount(), 2U);

    de.FlushDiagnostic();
    EXPECT_EQ(de.GetErrorCount(), 0U);
}

}  // namespace
