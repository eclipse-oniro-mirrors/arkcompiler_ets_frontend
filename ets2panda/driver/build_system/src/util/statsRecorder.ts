/*
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import * as fs from 'fs';
import * as path from 'path';
import { formatTimestamp, getPid, getBatchId, formatMsCompact } from './utils';
import { ENABLE_STATS_RECORDER } from '../pre_define';

export const BS_PERF_DIR = 'perf';
export const BS_PERF_FILE_NAME = 'bs_record_perf.csv'
const BS_CLUSTER_DIR = 'cluster';
const ONE_MB = (1024 * 1024);

export enum RecordEvent {
    START = 'Start recording',
    END = 'End recording'
}

export class EventData {
    public startTime: number = 0;
    public endTime: number = 0;
    public startRss: number = 0;
    public endRss: number = 0;
}

export class StatisticsRecorder {
    private eventMap = new Map<string, EventData>;

    private currentEvent: string = RecordEvent.START;
    private readonly enable: boolean = false;
    private totalStartTime: number = 0;
    private totalEndTime: number = 0;
    private totalStartRss: number = 0;
    private totalEndRss: number = 0;
    private clusterDir: string = '';
    private pid: number = 0
;
    constructor(private readonly perfDir: string, private readonly title?: string) {
        this.enable = ENABLE_STATS_RECORDER;
        if (!this.enable) {
            return;
        }

        this.totalStartTime = new Date().getTime();
        this.totalStartRss = process.memoryUsage().rss;
        this.pid = getPid();

        if (!fs.existsSync(this.perfDir)) {
           fs.mkdirSync(this.perfDir, { recursive: true });
        }

        this.clusterDir = path.resolve(this.perfDir , BS_CLUSTER_DIR);
        if (!fs.existsSync(this.clusterDir)) {
            fs.mkdirSync(this.clusterDir , {recursive: true});
        }
    }

    public cluster(clusterContent: string): void {
        if (!this.enable) {
            return;
        }

        const clusterFile = path.join(this.clusterDir, `${getPid()}-${getBatchId()}.txt`);
 	    fs.appendFileSync(clusterFile, clusterContent, 'utf8');
    }

    private printLog(event: string , eventData: EventData): void {
        let timeDiff = eventData.endTime - eventData.startTime;
        let startTimeStr = formatTimestamp(eventData.startTime);
        let endTimeStr = formatTimestamp(eventData.endTime);
        console.log(`${event};timeDiff=${timeDiff}ms;pid=${getPid()};batchId=${getBatchId()};startTime=${startTimeStr};endTime=${endTimeStr}`);
    }

    // Event is of type string (not enum) to let users to define own events
    public record(event: string, printLog: boolean = false): void {
        if (!this.enable) {
            return;
        }

        if (event === RecordEvent.START) {
            return;
        }

        let time = new Date().getTime();
        let mem = process.memoryUsage().rss;

        let currEvent: EventData | undefined = this.eventMap.get(this.currentEvent);
        if (currEvent) {
            currEvent.endTime = time;
            currEvent.endRss = mem;
            if (printLog) {
                this.printLog(this.currentEvent , currEvent);
            }
        }

        if (event === RecordEvent.END) {
            this.totalEndTime = time;
            this.totalEndRss = mem;
            let totalEvent: EventData = new EventData();
            totalEvent.startTime = this.totalStartTime;
            totalEvent.endTime = this.totalEndTime;
            totalEvent.startRss = this.totalStartRss;
            totalEvent.endRss = this.totalEndRss;
            this.eventMap.set(event , totalEvent);
            this.currentEvent = event;

            if(printLog){
                this.printLog(this.currentEvent , totalEvent);
            }
            return;
        }

        let newEvent: EventData = new EventData();

        newEvent.startTime = time;
        newEvent.startRss = mem;

        this.currentEvent = event;
        this.eventMap.set(event, newEvent);
    }

    writeSumSingle(): void {
        if (!this.enable) {
            return;
        }

        if (this.currentEvent !== RecordEvent.END) {
            this.record(RecordEvent.END , true);
        }

        const csvData: string[] = []
        if (this.title) {
            csvData.push(`title, ${this.title};pid=${getPid()};batchId=${getBatchId()}`)
        }
        csvData.push('Stage, time(ms), startTime, endTime, increMem, startMem, endMem');

        this.eventMap.forEach((data: EventData, event: string) => {
            const cost = formatMsCompact(data.endTime - data.startTime);
            const startTime = formatTimestamp(data.startTime);
            const endTime = formatTimestamp(data.endTime);

            const increMem: number = Math.round((data.endRss - data.startRss) / ONE_MB);
            const startMem = Math.round(data.startRss / ONE_MB);
            const endMem = Math.round(data.endRss / ONE_MB);
            let element = `${event}` + ', ' + `${cost}` +', '+`${startTime}`+ ', ' +`${endTime}`+', '+ `${increMem}` + 'M'+', '+`${startMem}`+'M, '+`${endMem}`+'M';
            csvData.push(element);
        });
        csvData.push('\n');
        const perfFile = path.join(this.perfDir, `${this.pid}_${BS_PERF_FILE_NAME}`);
        fs.appendFileSync(perfFile, csvData.join('\n'));
    }
}
