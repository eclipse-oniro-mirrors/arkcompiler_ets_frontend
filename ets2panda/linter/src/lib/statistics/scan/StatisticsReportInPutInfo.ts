/*
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

import type { CommandLineOptions } from '../../CommandLineOptions';
import type { TimeRecorder } from './TimeRecorder';

export class StatisticsReportInPutInfo {
  totalProblemNumbers: number = 0;
  arkOnePointOneProblemNumbers: number = 0;
  ruleToNumbersMap: Map<string, number> = {} as Map<string, number>;
  wholeRules: string[] = [] as string[];
  ruleToAutoFixedNumbersMap: Map<string, number> = {} as Map<string, number>;
  cmdOptions: CommandLineOptions = {} as CommandLineOptions;
  timeRecorder: TimeRecorder = {} as TimeRecorder;
  statisticsReportName: string = '';
}
