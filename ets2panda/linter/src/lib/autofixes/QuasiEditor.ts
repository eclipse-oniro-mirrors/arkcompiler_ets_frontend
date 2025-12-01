/*
 * Copyright (c) 2023-2026 Huawei Device Co., Ltd.
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

import * as fs from 'node:fs';
import * as path from 'node:path';
import { Logger } from '../Logger';
import type { ProblemInfo } from '../ProblemInfo';
import type { Autofix } from './Autofixer';
import type { LinterOptions } from '../LinterOptions';
import { USE_STATIC } from '../utils/consts/InteropAPI';
import { AUTOFIX_HTML_TEMPLATE_TEXT, AutofixHtmlTemplate } from './AutofixReportHtmlHelper';
import { ARKUI_MODULE } from '../utils/consts/ArkuiConstants';
import { getRulePriority } from '../utils/consts/RulePriority';

const BACKUP_AFFIX = '~';
export const DEFAULT_MAX_AUTOFIX_PASSES = 10;

export class QuasiEditor {
  constructor(
    readonly srcFileName: string,
    readonly sourceText: string,
    readonly linterOpts: LinterOptions,
    readonly reportPath?: string
  ) {}

  private static getBackupFileName(filePath: string): string {
    return filePath + BACKUP_AFFIX;
  }

  static backupSrcFile(filePath: string): void {
    fs.copyFileSync(filePath, QuasiEditor.getBackupFileName(filePath));
  }

  private generateReport(acceptedPatches: Autofix[]): void {
    const report = {
      filePath: this.srcFileName,
      fixCount: acceptedPatches.length,
      fixes: acceptedPatches.map((fix) => {
        return {
          line: fix.line,
          colum: fix.column,
          endLine: fix.endLine,
          endColum: fix.endColumn,
          start: fix.start,
          end: fix.end,
          replacement: fix.replacementText,
          original: this.sourceText.slice(fix.start, fix.end)
        };
      })
    };

    let reportFilePath = './autofix-report.html';
    if (this.reportPath !== undefined) {
      reportFilePath = path.join(path.normalize(this.reportPath), 'autofix-report.html');
    }
    const getOldJsonArray = (reportFilePath: string): Set<any> => {
      try {
        const RegexCaptureBraketFirst = 1;
        const rawData = fs.readFileSync(reportFilePath, 'utf-8');
        const rawContent = rawData.match(/`([\s\S]*?)`/)?.[RegexCaptureBraketFirst] ?? '';
        return new Set(JSON.parse(rawContent) || []);
      } catch {
        return new Set();
      }
    };

    try {
      const existingReports = getOldJsonArray(reportFilePath);
      existingReports.add(report);
      const str = JSON.stringify([...existingReports], null, 2);
      const HtmlContent = AutofixHtmlTemplate.replace(AUTOFIX_HTML_TEMPLATE_TEXT, str);
      if (!fs.existsSync(path.dirname(reportFilePath))) {
        fs.mkdirSync(path.dirname(reportFilePath), { recursive: true });
      }
      fs.writeFileSync(reportFilePath, HtmlContent, { encoding: 'utf-8' });
    } catch (error) {
      Logger.error(`Failed to update autofix report: ${(error as Error).message}`);
    }
  }

  fix(problemInfos: ProblemInfo[], needAddUseStatic: boolean | undefined): string {
    const acceptedPatches = QuasiEditor.sortAndRemoveIntersections(problemInfos);
    let result = this.applyFixes(acceptedPatches);

    if (this.linterOpts.migrationReport) {
      this.generateReport(acceptedPatches);
    }
    if (needAddUseStatic) {
      result = QuasiEditor.addUseStaticDirective(result);
    }
    return result;
  }

  private applyFixes(autofixes: Autofix[]): string {
    let output: string = '';
    let lastFixEnd = Number.NEGATIVE_INFINITY;

    const doFix = (fix: Autofix): void => {
      const { replacementText, start, end } = fix;

      if (lastFixEnd > start || start > end) {
        Logger.error(`Failed to apply autofix in range [${start}, ${end}] at ${this.srcFileName}`);
        return;
      }

      output += this.sourceText.slice(Math.max(0, lastFixEnd), Math.max(0, start));
      output += replacementText;
      lastFixEnd = end;
    };

    autofixes.forEach(doFix);
    output += this.sourceText.slice(Math.max(0, lastFixEnd));

    return output;
  }

  private static sortAndRemoveIntersections(problemInfos: ProblemInfo[]): Autofix[] {
    // Track accepted patches and their source problemInfo
    let acceptedPatches: Autofix[] = [];
    const patchToProblemInfoMap = new Map<Autofix, ProblemInfo>();

    problemInfos.forEach((problemInfo): void => {
      if (!problemInfo.autofix) {
        return;
      }

      const consideredAutofix = QuasiEditor.sortAutofixes(problemInfo.autofix);

      // Check for intersections
      if (QuasiEditor.intersect(consideredAutofix, acceptedPatches)) {
        // If completely same, no conflict
        if (QuasiEditor.isContainSame(consideredAutofix, acceptedPatches)) {
          return;
        }
        // If autofix of UI Import, no conflict
        if (consideredAutofix.length === 1 && consideredAutofix[0].replacementText.includes(ARKUI_MODULE)) {
          return;
        }
        // check if we can replace based on rule priority
        if (
          QuasiEditor.handleSingleAutofixByPriority(
            consideredAutofix,
            acceptedPatches,
            patchToProblemInfoMap,
            problemInfo
          )
        ) {
          return;
        }
        // Default behavior: mark as not autofixable
        problemInfo.autofixable = false;
        return;
      }

      // No conflicts, add the patches
      acceptedPatches.push(...consideredAutofix);
      acceptedPatches = QuasiEditor.sortAutofixes(acceptedPatches);
      consideredAutofix.forEach((fix) => {
        return patchToProblemInfoMap.set(fix, problemInfo);
      });
    });
    return acceptedPatches;
  }

  private static sortAutofixes(autofixes: Autofix[]): Autofix[] {
    return autofixes.sort((a, b): number => {
      return a.start - b.start;
    });
  }

  /**
   * Determine if considered autofix can be accepted.
   *
   * @param consideredAutofix sorted patches of the considered autofix
   * @param acceptedFixes sorted list of accepted autofixes
   * @returns
   */
  private static intersect(consideredAutofix: readonly Autofix[], acceptedFixes: readonly Autofix[]): boolean {
    const start = consideredAutofix[0].start;
    const end = consideredAutofix[consideredAutofix.length - 1].end;

    for (const acceptedFix of acceptedFixes) {
      if (acceptedFix.start > end) {
        break;
      }

      if (acceptedFix.end < start) {
        continue;
      }

      for (const consideredFix of consideredAutofix) {
        if (QuasiEditor.autofixesIntersect(acceptedFix, consideredFix)) {
          return true;
        }
      }
    }
    return false;
  }

  private static autofixesIntersect(lhs: Autofix, rhs: Autofix): boolean {

    /*
     * Ranges don't intersect if either
     * [--]         (lhs)
     *      [--]    (rhs)
     * or
     *      [--]    (lhs)
     * [--]         (rhs)
     */
    return !(lhs.end < rhs.start || rhs.end < lhs.start);
  }

  /**
   * Handle conflict resolution between single autofixes using rule priority.
   */
  private static handleSingleAutofixByPriority(
    consideredAutofix: Autofix[],
    acceptedPatches: Autofix[],
    patchToProblemInfoMap: Map<Autofix, ProblemInfo>,
    problemInfo: ProblemInfo
  ): boolean {
    if (consideredAutofix.length !== 1) {
      return false;
    }
    const currentFix = consideredAutofix[0];
    const currentPriority = getRulePriority(problemInfo.rule);
    const conflictingPatches = QuasiEditor.findConflictingPatches(currentFix, acceptedPatches, patchToProblemInfoMap);

    if (!QuasiEditor.canReplaceByPriority(currentPriority, conflictingPatches)) {
      return false;
    }
    QuasiEditor.replaceConflictingPatches(conflictingPatches, acceptedPatches, patchToProblemInfoMap);
    acceptedPatches.push(...consideredAutofix);
    consideredAutofix.forEach((fix) => {
      return patchToProblemInfoMap.set(fix, problemInfo);
    });
    return true;
  }

  private static findConflictingPatches(
    currentFix: Autofix,
    acceptedPatches: Autofix[],
    patchToProblemInfoMap: Map<Autofix, ProblemInfo>
  ): { patch: Autofix; sourceProblemInfo: ProblemInfo }[] {
    const result: { patch: Autofix; sourceProblemInfo: ProblemInfo }[] = [];
    acceptedPatches.forEach((patch) => {
      if (QuasiEditor.autofixesIntersect(currentFix, patch)) {
        const sourceProblemInfo = patchToProblemInfoMap.get(patch);
        if (sourceProblemInfo?.autofix && sourceProblemInfo.autofix.length === 1) {
          result.push({ patch, sourceProblemInfo });
        }
      }
    });
    return result;
  }

  private static canReplaceByPriority(
    currentPriority: number,
    conflictingPatches: { sourceProblemInfo: ProblemInfo }[]
  ): boolean {
    if (conflictingPatches.length === 0) {
      return false;
    }
    return conflictingPatches.every((c) => {
      return currentPriority > getRulePriority(c.sourceProblemInfo.rule);
    });
  }

  private static replaceConflictingPatches(
    conflictingPatches: { patch: Autofix; sourceProblemInfo: ProblemInfo }[],
    acceptedPatches: Autofix[],
    patchToProblemInfoMap: Map<Autofix, ProblemInfo>
  ): void {
    conflictingPatches.forEach((conflicting) => {
      conflicting.sourceProblemInfo.autofixable = false;
      const index = acceptedPatches.indexOf(conflicting.patch);
      if (index !== -1) {
        acceptedPatches.splice(index, 1);
        patchToProblemInfoMap.delete(conflicting.patch);
      }
    });
  }

  private static addUseStaticDirective(content: string): string {
    const lines = content.split('\n');
    if (lines.length > 0 && lines[0].trim() === USE_STATIC) {
      return content;
    }
    return USE_STATIC + '\n' + content;
  }

  static hasAnyAutofixes(problemInfos: ProblemInfo[]): boolean {
    return problemInfos.some((problemInfo) => {
      return problemInfo.autofix !== undefined;
    });
  }

  /**
   * Determine if accepted autofixes contains all the same autofixes of considered autofixes.
   *
   * @param consideredAutofix sorted patches of the considered autofix
   * @param acceptedFixes sorted list of accepted autofixes
   * @returns
   */
  private static isContainSame(consideredAutofix: readonly Autofix[], acceptedFixes: readonly Autofix[]): boolean {
    for (const consideredFix of consideredAutofix) {
      let found = false;
      for (const acceptedFix of acceptedFixes) {
        if (
          consideredFix.start === acceptedFix.start &&
          consideredFix.end === acceptedFix.end &&
          consideredFix.replacementText === acceptedFix.replacementText
        ) {
          found = true;
          break;
        }
      }
      if (!found) {
        return false;
      }
    }
    return true;
  }
}
