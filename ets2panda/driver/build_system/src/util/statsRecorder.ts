/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

export const BS_PERF_FILE_NAME = 'bs_record_perf.csv'


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

    constructor(private readonly output: string, enable?: 'OFF' | 'ON', private readonly title?: string) {
        this.enable = enable == 'ON';
    }

    // Event is of type string (not enum) to let users to define own events
    public record(event: string): void {
        if (!this.enable) {
            return;
        }

        if (event == RecordEvent.START) {
            return;
        }

        let time = new Date().getTime();
        let mem = process.memoryUsage.rss();

        let currEvent: EventData | undefined = this.eventMap.get(this.currentEvent);
        if (currEvent) {
            currEvent.endTime = time;
            currEvent.endRss = mem;
        }

        if (event == RecordEvent.END) {
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

        if (this.currentEvent != RecordEvent.END) {
            this.record(RecordEvent.END)
        }

        const csvData: string[] = []
        if (this.title) {
            csvData.push(`title, ${this.title},`)
        }
        csvData.push('timeKey, time(ms), mem(M)');

        this.eventMap.forEach((data: EventData, event: string) => {
            const totalRss: number = (data.endRss < data.startRss) ? 0 :
                Math.round((data.endRss - data.startRss) / 1024 / 1024)
            let element = `${event}` + ', ' + `${data.endTime - data.startTime}` + 'ms' + ', ' + `${totalRss}` + 'M';
            csvData.push(element);
        });
        csvData.push('\n');
        fs.appendFileSync(this.output, csvData.join('\n'));
    }
}
