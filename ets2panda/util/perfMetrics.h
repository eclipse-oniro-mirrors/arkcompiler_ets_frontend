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

#ifndef ES2PANDA_UTIL_PERF_METRICS_H
#define ES2PANDA_UTIL_PERF_METRICS_H

#include "libpandabase/macros.h"

namespace ark::es2panda::util {

// NOLINTBEGIN(cppcoreguidelines-macro-usage)
#define ES2PANDA_PERF_CONCAT2_(a, b) a##b
// CC-OFFNXT(G.PRE.02-CPP) macro definition
#define ES2PANDA_PERF_CONCAT2(a, b) ES2PANDA_PERF_CONCAT2_(a, b)

// Opens a RAII scope of performance tracking. RAII scope is associated with a single
// unique record, which accumulates metrics on each subsequent invokation.
// CC-OFFNXT(G.PRE.02-CPP) macro definition
#define ES2PANDA_PERF_SCOPE(name)                                                       \
    thread_local auto ES2PANDA_PERF_CONCAT2(e2pPerfRecord, __LINE__) =                  \
        ark::es2panda::util::RegisterPerfMetricRecord(name);                            \
    ark::es2panda::util::PerfMetricScope ES2PANDA_PERF_CONCAT2(e2pPerfScope, __LINE__)( \
        ES2PANDA_PERF_CONCAT2(e2pPerfRecord, __LINE__))

// Similar to ES2PANDA_PERF_SCOPE, the current functions name is assigned to the performance record.
// CC-OFFNXT(G.PRE.02-CPP) macro definition
#define ES2PANDA_PERF_FN_SCOPE() ES2PANDA_PERF_SCOPE(__PRETTY_FUNCTION__)

// Opens a RAII scope of performance tracking. Each subsequent invokation is associated with
// a new performance record. Do not use it if scope is created frequently.
// CC-OFFNXT(G.PRE.02-CPP) macro definition
#define ES2PANDA_PERF_EVENT_SCOPE(name) \
    ark::es2panda::util::PerfMetricScope e2pPerfScope(ark::es2panda::util::RegisterPerfMetricRecord(name))
// NOLINTEND(cppcoreguidelines-macro-usage)

// Dump all the collected performance records. Should not be called if there are active threads using the profile.
void DumpPerfMetrics();

class PerfMetricRecord;
PerfMetricRecord *RegisterPerfMetricRecord(std::string_view name);

class PerfMetricScope {
public:
    explicit PerfMetricScope(PerfMetricRecord *record);
    ~PerfMetricScope();

    NO_COPY_SEMANTIC(PerfMetricScope);
    NO_MOVE_SEMANTIC(PerfMetricScope);

private:
    PerfMetricRecord *record_;
};

}  // namespace ark::es2panda::util

#endif
