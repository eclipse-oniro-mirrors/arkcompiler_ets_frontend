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
import path from 'path';
import { RECORD_TYPE } from '../types'

export const BS_PERF_FILE_NAME = 'bs_record_perf.csv'

export enum RECORDE_RUN_NODE {
  GEN_MODULE    = 'run generateModuleInfos',
  COMPILE_FILES = 'run compileMultiFiles',
  END           = 'run end',
}

export enum RECORDE_COMPILE_NODE {
  PROCEED_PARSE = 'compileMultiFiles proceedToState parsed',
  PLUGIN_PARSE  = 'compileMultiFiles plugin parsed',
  PROCEED_CHECK = 'compileMultiFiles proceedToState checked',
  PLUGIN_CHECK  = 'compileMultiFiles plugin checked',
  BIN_GENERATE  = 'compileMultiFiles bin generated',
  CFG_DESTROY   = 'compileMultiFiles config destroyed',
  END           = 'compileMultiFiles end',
}

export enum RECORDE_MODULE_NODE {
  COLLECT_INFO = 'generateModuleInfos collectModuleInfos',
  GEN_CONFIG   = 'generateModuleInfos generateArkTSConfigForModules',
  CLT_FILES    = 'generateModuleInfos collectCompileFiles',
  SAVE_CACHE   = 'generateModuleInfos saveHashCache',
  END          = 'generateModuleInfos end',
}

export class SingleData {
  public time: number = 0;
  public mem: number = 0;
}

export class CompileSingleData {
  private timeMemMap: Map<string, SingleData>;
  private startTime: number = 0;
  private startMem: number = 0;
  private file: string = '';
  private recordType: RECORD_TYPE;

  constructor(file: string, recordType?: RECORD_TYPE) {
    this.file = file;
    this.timeMemMap = new Map<string, SingleData>();
    // close by default
    this.recordType = recordType ?? RECORD_TYPE.DEFAULT_TYPE;
  }

  public record(startKey: string, lastEndKey: string = ''): void {
    if (this.recordType === RECORD_TYPE.DEFAULT_TYPE) {
      return;
    }
    let currentTime = new Date().getTime();
    let currentMem = process.memoryUsage.rss();
    let tmp: SingleData | undefined = this.timeMemMap.get(lastEndKey);
    if (tmp) {
      tmp.time = currentTime - this.startTime;
      tmp.mem = (currentMem > this.startMem) ? (currentMem - this.startMem) : 0;
      this.timeMemMap.set(lastEndKey, tmp);
    }

    if (startKey === '') {
      return;
    }

    if (this.timeMemMap.get(startKey) !== undefined) {
      return;
    }
    this.startTime = currentTime;
    this.startMem = currentMem;
    let data: SingleData = new SingleData();
    data.time = 0;
    data.mem = 0;
    this.timeMemMap.set(startKey, data);
  }

  writeSumSingle(cachePath: string, deputyName: string = ''): void {
    if (this.recordType === RECORD_TYPE.DEFAULT_TYPE) {
      return;
    }
    const csvData: string[] = [
      'timeKey, time(ms), mem(M)'
    ];
    this.timeMemMap.forEach((v: SingleData, k: string) => {
      let element = `${k}` +', ' + `${v.time}` + 'ms' + ', ' + `${Math.round(v.mem / 1024 / 1024)}` + 'M' ;
      csvData.push(element);
    });
    let name = path.basename(this.file)
    let currentExt = path.extname(name)
    let fileWithoutExt = name.substring(0, name.lastIndexOf(currentExt));
    let fileName = `${fileWithoutExt}`+ deputyName +'.csv';
    let filePath = path.join(cachePath, fileName);
    csvData.forEach(row => {
      fs.appendFileSync(filePath, `${row}\n`);
    });
  }
}