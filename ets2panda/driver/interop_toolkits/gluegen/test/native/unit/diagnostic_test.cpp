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

#include <sstream>
#include <thread>
#include <vector>
#include <gtest/gtest.h>

#include "diagnostic.h"
#include "nlohmann/json.hpp"

using ark::es2panda::gluegen::Diagnostic;
using ark::es2panda::gluegen::DiagnosticConsumer;
using ark::es2panda::gluegen::DiagnosticEngine;
using ark::es2panda::gluegen::DiagnosticSeverity;
using ark::es2panda::gluegen::MultiplexDiagnosticConsumer;
using ark::es2panda::gluegen::StreamDiagnosticConsumer;

namespace {

// Records every diagnostic it's handed, in order, without any rendering -- used by tests that
// need to assert on exactly what a DiagnosticEngine forwarded to its consumer.
class RecordingConsumer final : public DiagnosticConsumer {
public:
    void HandleDiagnostic(DiagnosticSeverity severity, const Diagnostic &diagnostic) override
    {
        seen.emplace_back(severity, diagnostic.code);
    }

    std::vector<std::pair<DiagnosticSeverity, uint32_t>> seen;
};

TEST(GluegenDiagnosticTest, StartsWithNoDiagnostics)
{
    DiagnosticEngine engine;
    EXPECT_FALSE(engine.HasErrors());
    EXPECT_EQ(engine.ErrorCount(), 0U);
    EXPECT_EQ(engine.WarningCount(), 0U);
    EXPECT_TRUE(engine.Records().empty());
}

TEST(GluegenDiagnosticTest, ErrorIncrementsErrorCountAndHasErrors)
{
    DiagnosticEngine engine;
    engine.Error(1001, "failed to resolve import");
    EXPECT_TRUE(engine.HasErrors());
    EXPECT_EQ(engine.ErrorCount(), 1U);
    EXPECT_EQ(engine.WarningCount(), 0U);
}

TEST(GluegenDiagnosticTest, WarningIncrementsWarningCountOnly)
{
    DiagnosticEngine engine;
    engine.Warning(2001, "unused export");
    EXPECT_FALSE(engine.HasErrors());
    EXPECT_EQ(engine.ErrorCount(), 0U);
    EXPECT_EQ(engine.WarningCount(), 1U);
}

TEST(GluegenDiagnosticTest, NoteAffectsNeitherCount)
{
    DiagnosticEngine engine;
    engine.Note(3001, "for context");
    EXPECT_FALSE(engine.HasErrors());
    EXPECT_EQ(engine.ErrorCount(), 0U);
    EXPECT_EQ(engine.WarningCount(), 0U);
    ASSERT_EQ(engine.Records().size(), 1U);
}

TEST(GluegenDiagnosticTest, RecordsPreserveReportOrderAndFields)
{
    DiagnosticEngine engine;
    engine.Error(1001, "failed to resolve import", "module not found", "foo.ets:3:5", {"check the import path"});
    engine.Warning(2001, "unused export");

    auto records = engine.Records();
    ASSERT_EQ(records.size(), 2U);

    EXPECT_EQ(records[0].severity, DiagnosticSeverity::ERROR);
    EXPECT_EQ(records[0].diagnostic.code, 1001U);
    EXPECT_EQ(records[0].diagnostic.description, "failed to resolve import");
    EXPECT_EQ(records[0].diagnostic.cause, "module not found");
    EXPECT_EQ(records[0].diagnostic.position, "foo.ets:3:5");
    ASSERT_EQ(records[0].diagnostic.solutions.size(), 1U);
    EXPECT_EQ(records[0].diagnostic.solutions[0], "check the import path");

    EXPECT_EQ(records[1].severity, DiagnosticSeverity::WARNING);
    EXPECT_EQ(records[1].diagnostic.code, 2001U);
}

TEST(GluegenDiagnosticTest, ClearResetsCountsAndRecords)
{
    DiagnosticEngine engine;
    engine.Error(1001, "failed to resolve import");
    engine.Warning(2001, "unused export");
    engine.Clear();

    EXPECT_FALSE(engine.HasErrors());
    EXPECT_EQ(engine.ErrorCount(), 0U);
    EXPECT_EQ(engine.WarningCount(), 0U);
    EXPECT_TRUE(engine.Records().empty());
}

TEST(GluegenDiagnosticTest, ConsumerIsForwardedEveryDiagnosticInOrder)
{
    RecordingConsumer consumer;
    DiagnosticEngine engine(&consumer);

    engine.Error(1001, "e1");
    engine.Warning(2001, "w1");
    engine.Note(3001, "n1");

    ASSERT_EQ(consumer.seen.size(), 3U);
    EXPECT_EQ(consumer.seen[0], std::make_pair(DiagnosticSeverity::ERROR, 1001U));
    EXPECT_EQ(consumer.seen[1], std::make_pair(DiagnosticSeverity::WARNING, 2001U));
    EXPECT_EQ(consumer.seen[2], std::make_pair(DiagnosticSeverity::NOTE, 3001U));
}

TEST(GluegenDiagnosticTest, SetConsumerSwapsConsumerForSubsequentReports)
{
    RecordingConsumer first;
    RecordingConsumer second;
    DiagnosticEngine engine(&first);

    engine.Error(1001, "e1");
    engine.SetConsumer(&second);
    engine.Error(1002, "e2");

    EXPECT_EQ(first.seen.size(), 1U);
    EXPECT_EQ(second.seen.size(), 1U);
}

TEST(GluegenDiagnosticTest, SetConsumerToNullptrStopsForwarding)
{
    RecordingConsumer consumer;
    DiagnosticEngine engine(&consumer);
    engine.Error(1001, "e1");
    engine.SetConsumer(nullptr);
    engine.Error(1002, "e2");

    // Both diagnostics are still recorded, only forwarding to the (now-removed) consumer stops.
    EXPECT_EQ(consumer.seen.size(), 1U);
    EXPECT_EQ(engine.Records().size(), 2U);
}

TEST(GluegenDiagnosticTest, SerializeProducesValidJsonArrayInReportOrder)
{
    DiagnosticEngine engine;
    engine.Error(1001, "failed to resolve import", "module not found", "foo.ets:3:5", {"check the import path"});
    engine.Warning(2001, "unused export");

    auto json = nlohmann::json::parse(engine.Serialize());
    ASSERT_TRUE(json.is_array());
    ASSERT_EQ(json.size(), 2U);
    EXPECT_EQ(json[0]["severity"], "error");
    EXPECT_EQ(json[0]["diagnostic"]["code"], "1001");
    EXPECT_EQ(json[0]["diagnostic"]["solutions"][0], "check the import path");
    EXPECT_EQ(json[1]["severity"], "warning");
    EXPECT_EQ(json[1]["diagnostic"]["code"], "2001");
}

TEST(GluegenDiagnosticTest, StreamDiagnosticConsumerRendersHeadlinePositionCauseAndSolutions)
{
    std::ostringstream out;
    StreamDiagnosticConsumer consumer(out);
    DiagnosticEngine engine(&consumer);

    engine.Error(1001, "failed to resolve import", "module not found", "foo.ets:3:5", {"check the import path"});

    auto rendered = out.str();
    EXPECT_NE(rendered.find("error[1001]: failed to resolve import"), std::string::npos);
    EXPECT_NE(rendered.find("--> foo.ets:3:5"), std::string::npos);
    EXPECT_NE(rendered.find("cause: module not found"), std::string::npos);
    EXPECT_NE(rendered.find("- check the import path"), std::string::npos);
}

TEST(GluegenDiagnosticTest, StreamDiagnosticConsumerOmitsEmptyOptionalFields)
{
    std::ostringstream out;
    StreamDiagnosticConsumer consumer(out);
    DiagnosticEngine engine(&consumer);

    engine.Warning(2001, "unused export");

    auto rendered = out.str();
    EXPECT_NE(rendered.find("warning[2001]: unused export"), std::string::npos);
    EXPECT_EQ(rendered.find("-->"), std::string::npos);
    EXPECT_EQ(rendered.find("cause:"), std::string::npos);
    EXPECT_EQ(rendered.find("solutions:"), std::string::npos);
}

TEST(GluegenDiagnosticTest, MultiplexDiagnosticConsumerForwardsToEveryConsumerInOrderAdded)
{
    RecordingConsumer first;
    RecordingConsumer second;
    MultiplexDiagnosticConsumer multiplex;
    multiplex.AddConsumer(&first);
    multiplex.AddConsumer(&second);

    DiagnosticEngine engine(&multiplex);
    engine.Error(1001, "e1");

    ASSERT_EQ(first.seen.size(), 1U);
    ASSERT_EQ(second.seen.size(), 1U);
    EXPECT_EQ(first.seen[0], std::make_pair(DiagnosticSeverity::ERROR, 1001U));
    EXPECT_EQ(second.seen[0], std::make_pair(DiagnosticSeverity::ERROR, 1001U));
}

TEST(GluegenDiagnosticTest, MultiplexDiagnosticConsumerCanCombineStreamAndRecordingConsumers)
{
    std::ostringstream out;
    StreamDiagnosticConsumer streamConsumer(out);
    RecordingConsumer recordingConsumer;
    MultiplexDiagnosticConsumer multiplex;
    multiplex.AddConsumer(&streamConsumer);
    multiplex.AddConsumer(&recordingConsumer);

    DiagnosticEngine engine(&multiplex);
    engine.Warning(2001, "unused export");

    EXPECT_NE(out.str().find("warning[2001]: unused export"), std::string::npos);
    ASSERT_EQ(recordingConsumer.seen.size(), 1U);
    EXPECT_EQ(recordingConsumer.seen[0], std::make_pair(DiagnosticSeverity::WARNING, 2001U));
}

TEST(GluegenDiagnosticTest, ReportIsThreadSafeUnderConcurrentReporting)
{
    DiagnosticEngine engine;
    constexpr int kThreadCount = 8;
    constexpr int kReportsPerThread = 200;

    std::vector<std::thread> threads;
    threads.reserve(kThreadCount);
    for (int t = 0; t < kThreadCount; ++t) {
        threads.emplace_back([&engine]() {
            for (int i = 0; i < kReportsPerThread; ++i) {
                engine.Error(1000 + i, "concurrent error");
            }
        });
    }
    for (auto &thread : threads) {
        thread.join();
    }

    EXPECT_EQ(engine.ErrorCount(), static_cast<std::size_t>(kThreadCount * kReportsPerThread));
    EXPECT_EQ(engine.Records().size(), static_cast<std::size_t>(kThreadCount * kReportsPerThread));
}

}  // namespace
