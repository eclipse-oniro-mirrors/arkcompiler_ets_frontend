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

import {
  AVERAGE_LINE_FOR_REPAIRE_RULE_COEFFICIENT,
  NPAI_REPAIRE_WORKLOADA_COEFFICIEN,
  TEST_DEBUG_WORKLOAD_COEFFICIENT
} from '../../utils/consts/WorkloadRelatedConst';
import type { FloderScanResultInfo } from './FloderScanResultInfo';
import type { ProblemNumbersInfo } from './ProblemNumbersInfo';

export class WorkLoadInfo {
  scanFilePathList: string[] = [];
  totalArkTSCodeLines = 0;
  totalCAndCPPCodeLines = 0;
  totalNapiCodeLines = 0;
  totalJsCodeLines = 0;
  totalTsCodeLines = 0;
  totalJsonCodeLines = 0;
  totalXmlCodeLines = 0;
  manualFixRate: string | undefined;

  addFloderResult(result: FloderScanResultInfo): void {
    this.scanFilePathList.push(result.normalizedPath);
    this.totalArkTSCodeLines += result.arkTSCodeLines;
    this.totalCAndCPPCodeLines += result.cAndCPPCodeLines;
    this.totalNapiCodeLines += result.napiCodeLines;
    this.totalJsCodeLines += result.jsCodeLines;
    this.totalTsCodeLines += result.tsCodeLines;
    this.totalJsonCodeLines += result.jsonCodeLines;
    this.totalXmlCodeLines += result.xmlCodeLines;
  }

  calculateFixRate(problemNumbers: ProblemNumbersInfo): void {
    const totalLines = this.totalArkTSCodeLines + this.totalCAndCPPCodeLines;
    if (totalLines <= 0) {
      this.manualFixRate = '0.00%';
    }

    const problemCount = problemNumbers.needToManualFixproblemNumbers;
    const ratio =
      (problemCount * AVERAGE_LINE_FOR_REPAIRE_RULE_COEFFICIENT * TEST_DEBUG_WORKLOAD_COEFFICIENT +
        this.totalNapiCodeLines * NPAI_REPAIRE_WORKLOADA_COEFFICIEN) /
      totalLines;

    this.manualFixRate = `${(ratio * 100).toFixed(2)}%`;
  }
}
