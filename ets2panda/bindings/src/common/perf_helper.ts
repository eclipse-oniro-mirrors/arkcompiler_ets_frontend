/*
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

import * as fs from 'fs';
import * as path from 'path';

type PerfMetricStats = {
    lastTimeMs: number;
    lastMemBytes: number;
    count: number;
};

const DEFAULT_LOG_FILE = path.join(__dirname, 'perf_metric.txt');

class PerfMetricRecord {
    public name: string;
    public stats: PerfMetricStats = { lastTimeMs: 0, lastMemBytes: 0, count: 0 };
    private startTime: bigint = BigInt(0);
    private startMem: number = 0;
    private logFile: string;

    constructor(name: string, logFile: string) {
        this.name = name;
        this.logFile = (logFile === undefined || logFile === "") ? DEFAULT_LOG_FILE : logFile;
    }

    beginTrace() {
        this.startTime = process.hrtime.bigint();
        this.startMem = process.memoryUsage().heapUsed;
    }

    endTrace() {
        const endTime = process.hrtime.bigint();
        const endMem = process.memoryUsage().heapUsed;
        this.stats.lastTimeMs = Number(endTime - this.startTime) / 1e6;
        this.stats.lastMemBytes = endMem - this.startMem;
        this.stats.count += 1;
    }

    printLast() {
        const msg =
            `\n========== PERF METRIC ==========\n` +
            `#${this.name}\n` +
            `  Time: ${this.stats.lastTimeMs.toFixed(3)} ms\n` +
            `  Memory: ${(this.stats.lastMemBytes / 1024).toFixed(3)} KB\n` +
            `=================================\n`;
        try {
            fs.appendFileSync(this.logFile, msg);
        } catch (e) {
            console.log("error writing to perf metric log file:", e);
        }
    }
}

export class PerfMetricScope {
    private static records: Map<string, PerfMetricRecord> = new Map();
    private record: PerfMetricRecord;
    private ended = false;

    constructor(name: string, logFile: string) {
        if (!PerfMetricScope.records.has(name)) {
            PerfMetricScope.records.set(name, new PerfMetricRecord(name, logFile));
        }
        this.record = PerfMetricScope.records.get(name)!;
        this.record.beginTrace();
    }

    end() {
        if (!this.ended) {
            this.record.endTrace();
            this.record.printLast();
            this.ended = true;
        }
    }

}