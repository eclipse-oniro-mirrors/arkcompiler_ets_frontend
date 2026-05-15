/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

import {
    ArkField,
    FullPosition,
    Local,
    Stmt,
    Value,
} from 'arkanalyzer/lib';
import { NumberConstant } from 'arkanalyzer/lib/core/base/Constant';
import Logger, { LOG_MODULE_TYPE } from 'arkanalyzer/lib/utils/logger';
import path from 'path';
import { Defects, Rule, RuleFix } from '../../../../../Index';
import { IssueReport } from '../../../../../model/Defects';
import { WarnInfo } from '../../../../../utils/common/Utils';
import { getLineAndColumn } from '../../../Utils';
import { NumericAutofixBuilder } from '../autofix/NumericAutofixBuilder';
import { NumericSemanticIssueText } from '../../core/NumericSemanticIssueText';
import { IssueReason, NumberCategory, RuleCategory } from '../../core/NumericSemanticTypes';
import { NumericTypeAnnotationText } from '../../core/NumericTypeAnnotationText';

const logger = Logger.getLogger(LOG_MODULE_TYPE.HOMECHECK, 'NumericIssueReporter');

interface NumericIssueReporterOptions {
    rule: Rule;
    defaultSeverity: number;
    ruleDocPath: string;
    issuesMap: Map<string, IssueReport>;
    getAutofixBuilder(): NumericAutofixBuilder;
    getActualIndexPosInStmt(stmt: Stmt): FullPosition;
}

export class NumericIssueReporter {
    constructor(private options: NumericIssueReporterOptions) {}

    public getIssueReasonFromDefectInfo(defect: Defects): IssueReason | null {
        const issueProblem = defect.problem;
        // Ensure that IssueReason.UsedWithOtherType is evaluated before IssueReason.OnlyUsedAsIntLong,
        // as they have an inclusion relationship, and switching their order would lead to errors
        if (issueProblem.includes(IssueReason.UsedWithOtherType)) {
            return IssueReason.UsedWithOtherType;
        }
        if (issueProblem.includes(IssueReason.OnlyUsedAsIntLong)) {
            return IssueReason.OnlyUsedAsIntLong;
        }
        if (issueProblem.includes(IssueReason.RelatedWithNonETS2)) {
            return IssueReason.RelatedWithNonETS2;
        }
        if (issueProblem.includes(IssueReason.CannotFindAll)) {
            return IssueReason.CannotFindAll;
        }
        if (issueProblem.includes(IssueReason.Other)) {
            return IssueReason.Other;
        }
        return null;
    }

    public getNumberCategoryFromFixInfo(fix: RuleFix): NumberCategory | null {
        const match = fix.text.match(/^([^=;]+:[^=;]+)([\s\S]*)$/);
        if (match === null || match.length < 2) {
            return null;
        }
        const typeText = match[1].substring(match[1].indexOf(':') + 1);
        return NumericTypeAnnotationText.getNumberCategory(typeText);
    }

    public getFieldIssue(field: ArkField): IssueReport | null {
        const filePath = field.getDeclaringArkClass().getDeclaringArkFile().getFilePath();
        const position: WarnInfo = {
            line: field.getOriginPosition().getLineNo(),
            startCol: field.getOriginPosition().getColNo(),
            endCol: field.getOriginPosition().getColNo(),
            filePath,
        };
        const mapKey = `${filePath}%${position.line}%${position.startCol}%${position.endCol}%${this.options.rule.ruleId}`;
        return this.options.issuesMap.get(mapKey) ?? null;
    }

    // stmt should be the declaring stmt of this local
    public getLocalIssue(local: Local, stmt: Stmt): IssueReport | null {
        const filePath = stmt.getCfg().getDeclaringMethod().getDeclaringArkFile().getFilePath();
        const position = getLineAndColumn(stmt, local, true);
        const mapKey = `${filePath}%${position.line}%${position.startCol}%${position.endCol}%${this.options.rule.ruleId}`;
        return this.options.issuesMap.get(mapKey) ?? null;
    }

    public addApiArgIssue(
        ruleCategory: RuleCategory,
        numberCategory: NumberCategory,
        reason: IssueReason,
        couldAutofix: boolean,
        issueStmt?: Stmt,
        value?: Value,
        field?: ArkField,
        usedStmt?: Stmt
    ): void {
        const warnInfo = this.getWarnInfo(field, issueStmt, value);
        const problem = NumericSemanticIssueText.getProblem(ruleCategory, reason);
        if (!warnInfo || !problem) {
            return;
        }
        const desc = this.getApiArgDescription(ruleCategory, numberCategory, reason, issueStmt, usedStmt);
        if (!desc || this.shouldSkipDuplicatedIssue(ruleCategory, numberCategory, field, value, issueStmt)) {
            return;
        }

        const defects = this.createDefects(warnInfo, problem, desc, couldAutofix);
        if (!couldAutofix) {
            this.setIssue(defects, undefined);
            return;
        }
        const autofix = this.options.getAutofixBuilder().generateApiArgRuleFix(warnInfo, reason, numberCategory, issueStmt, value, field);
        if (autofix === null) {
            // 此规则必须修复，若autofix为null，则表示无需修复，不添加issue
            return;
        }
        this.setIssue(defects, autofix);
    }

    public addApiReturnOrFieldIssue(
        ruleCategory: RuleCategory,
        numberCategory: NumberCategory,
        reason: IssueReason,
        issueStmt?: Stmt,
        value?: Value,
        field?: ArkField,
        usedStmt?: Stmt
    ): void {
        const warnInfo = this.getWarnInfo(field, issueStmt, value);
        const problem = NumericSemanticIssueText.getProblem(ruleCategory, reason);
        if (!warnInfo || !problem || this.shouldSkipDuplicatedIssue(ruleCategory, numberCategory, field, value, issueStmt)) {
            return;
        }
        const desc = this.getApiReturnOrFieldDescription(ruleCategory, numberCategory, reason, issueStmt, usedStmt);
        if (!desc) {
            return;
        }

        const defects = this.createDefects(warnInfo, problem, desc, true);
        const autofix = this.options.getAutofixBuilder().generateApiReturnOrFieldRuleFix(warnInfo, numberCategory, issueStmt, field);
        if (autofix === null) {
            // 此规则必须修复，若autofix为null，则表示无需修复，不添加issue
            return;
        }
        this.setIssue(defects, autofix);
    }

    public addIssue(
        ruleCategory: RuleCategory,
        numberCategory: NumberCategory,
        reason: IssueReason,
        couldAutofix: boolean,
        issueStmt?: Stmt,
        value?: Value,
        field?: ArkField
    ): void {
        const warnInfo = this.getWarnInfo(field, issueStmt, value);
        const problem = NumericSemanticIssueText.getProblem(ruleCategory, reason);
        const desc = NumericSemanticIssueText.getDesc(ruleCategory, reason, couldAutofix);
        if (!warnInfo || !problem || !desc) {
            return;
        }
        if (field && field.getDecorators().length > 0) {
            return;
        }
        if (this.shouldSkipDuplicatedIssue(ruleCategory, numberCategory, field, value, issueStmt)) {
            return;
        }

        const defects = this.createDefects(warnInfo, problem, desc, couldAutofix);
        if (!couldAutofix) {
            this.setIssue(defects, undefined);
            return;
        }
        this.addFixableNumericIssue(defects, ruleCategory, numberCategory, reason, warnInfo, issueStmt, value, field);
    }

    private addFixableNumericIssue(
        defects: Defects,
        ruleCategory: RuleCategory,
        _numberCategory: NumberCategory,
        reason: IssueReason,
        warnInfo: WarnInfo,
        issueStmt?: Stmt,
        value?: Value,
        field?: ArkField
    ): void {
        if (ruleCategory === RuleCategory.NumericLiteral) {
            const autofix = this.options.getAutofixBuilder().generateNumericLiteralRuleFix(warnInfo, reason, issueStmt, value, field);
            if (autofix === null) {
                // 此规则必须修复，若autofix为null，则表示无需修复，不添加issue
                return;
            }
            this.setIssue(defects, autofix);
            return;
        }
        if (ruleCategory !== RuleCategory.ArrayIndex) {
            return;
        }
        if (reason === IssueReason.ActuallyIntConstant && issueStmt && value instanceof NumberConstant) {
            const autofix = this.options.getAutofixBuilder().generateIntConstantIndexRuleFix(warnInfo, issueStmt, value);
            if (autofix === null) {
                defects.fixable = false;
                this.setIssue(defects, undefined);
                return;
            }
            this.setIssue(defects, autofix);
            return;
        }
        const autofix = this.options.getAutofixBuilder().generateNumericLiteralRuleFix(warnInfo, reason, issueStmt, value, field);
        if (autofix === null) {
            // 此规则必须修复，若autofix为null，则表示无需修复，不添加issue
            return;
        }
        this.setIssue(defects, autofix);
    }

    private getWarnInfo(field?: ArkField, issueStmt?: Stmt, value?: Value): WarnInfo | null {
        let warnInfo: WarnInfo | null = null;

        if (field === undefined) {
            if (issueStmt && value) {
                warnInfo = getLineAndColumn(issueStmt, value, true);
                if (warnInfo.line === -1) {
                    // 可能是因为获取array index时，array是联合类型导致index未推导成功，checker里面额外去body里找local替换index
                    // 但是获取index的position信息时，需要使用原始的index去stmt中查找位置
                    const actualPosition = this.options.getActualIndexPosInStmt(issueStmt);
                    const originPath = issueStmt.getCfg().getDeclaringMethod().getDeclaringArkFile().getFilePath();
                    warnInfo = {
                        line: actualPosition.getFirstLine(),
                        startCol: actualPosition.getFirstCol(),
                        endLine: actualPosition.getLastLine(),
                        endCol: actualPosition.getLastCol(),
                        filePath: originPath,
                    };
                }
            } else {
                logger.error('Missing stmt or value when adding issue.');
                return warnInfo;
            }
        } else {
            warnInfo = {
                line: field.getOriginPosition().getLineNo(),
                startCol: field.getOriginPosition().getColNo(),
                endCol: field.getOriginPosition().getColNo(),
                filePath: field.getDeclaringArkClass().getDeclaringArkFile().getFilePath(),
            };
        }
        if (warnInfo.line === -1) {
            this.logInvalidWarnInfo(field, issueStmt);
            return null;
        }
        return warnInfo;
    }

    private logInvalidWarnInfo(field?: ArkField, issueStmt?: Stmt): void {
        if (issueStmt) {
            logger.error(`failed to get position info of value in issue stmt: ${issueStmt.toString()}`);
            return;
        }
        if (field) {
            logger.error(`failed to get position info of field: ${field.getSignature().toString()}`);
            return;
        }
        logger.error(`failed to get position info`);
    }

    private getApiArgDescription(
        ruleCategory: RuleCategory,
        numberCategory: NumberCategory,
        reason: IssueReason,
        issueStmt?: Stmt,
        usedStmt?: Stmt
    ): string | null {
        const apiSourceDesc = NumericSemanticIssueText.getApiSourceDesc(ruleCategory);
        if (reason === IssueReason.AmbiguousIntLong) {
            return `The arg of ${apiSourceDesc} should be int or long here, please check it manually (${ruleCategory})`;
        }
        if (reason === IssueReason.OnlyUsedAsIntLong) {
            if (usedStmt) {
                return `It has relationship with the arg of ${apiSourceDesc} in ${this.getUsedStmtDesc(usedStmt, issueStmt)} and only used as ${numberCategory}, should be defined as ${numberCategory} (${ruleCategory})`;
            }
            logger.error('Missing used stmt when getting issue description');
            return null;
        }
        return `The arg of ${apiSourceDesc} should be ${numberCategory} here (${ruleCategory})`;
    }

    private getApiReturnOrFieldDescription(
        ruleCategory: RuleCategory,
        numberCategory: NumberCategory,
        reason: IssueReason,
        issueStmt?: Stmt,
        usedStmt?: Stmt
    ): string | null {
        const apiSourceDesc = NumericSemanticIssueText.getApiSourceDesc(ruleCategory);
        if (reason === IssueReason.OnlyUsedAsIntLong) {
            if (usedStmt) {
                return `It has relationship with the ${apiSourceDesc} in ${this.getUsedStmtDesc(usedStmt, issueStmt)} and only used as ${numberCategory}, should be defined as ${numberCategory} (${ruleCategory})`;
            }
            logger.error('Missing used stmt when getting issue description');
            return null;
        }
        return `It is used as number (${ruleCategory})`;
    }

    private getUsedStmtDesc(usedStmt: Stmt, issueStmt?: Stmt): string {
        const issueFile = issueStmt?.getCfg().getDeclaringMethod().getDeclaringArkFile();
        const usedFile = usedStmt.getCfg().getDeclaringMethod().getDeclaringArkFile();
        const line = usedStmt.getOriginPositionInfo().getLineNo();
        if (issueFile && issueFile !== usedFile) {
            return `${path.normalize(usedFile.getName())}: ${line}`;
        }
        return `line ${line}`;
    }

    private createDefects(warnInfo: WarnInfo, problem: string, desc: string, couldAutofix: boolean): Defects {
        return new Defects(
            warnInfo.line,
            warnInfo.startCol,
            warnInfo.endCol,
            problem,
            desc,
            this.options.rule.alert ?? this.options.defaultSeverity,
            this.options.rule.ruleId,
            warnInfo.filePath,
            this.options.ruleDocPath,
            true,
            false,
            couldAutofix
        );
    }

    private shouldSkipDuplicatedIssue(
        ruleCategory: RuleCategory,
        numberCategory: NumberCategory,
        field?: ArkField,
        value?: Value,
        issueStmt?: Stmt
    ): boolean {
        // 添加新的issue之前需要检查一下已有issue，避免重复issue，或2个issue之间冲突，一个issue要改为int，一个issue要改为long
        const duplicatedIssue = this.getDuplicatedIssueInfo(field, value, issueStmt);
        if (!duplicatedIssue) {
            return false;
        }
        const { currentIssue, issueCategory } = duplicatedIssue;
        const priorityDecision = this.getDuplicatedIssuePriorityDecision(ruleCategory, currentIssue);
        if (priorityDecision !== null) {
            return priorityDecision;
        }
        const issueReason = this.getIssueReasonFromDefectInfo(currentIssue.defect);
        if (issueReason === null) {
            return false;
        }
        if (issueReason !== IssueReason.OnlyUsedAsIntLong) {
            return true;
        }
        return this.shouldSkipSamePriorityNumberIssue(numberCategory, issueCategory, currentIssue);
    }

    private getDuplicatedIssueInfo(
        field?: ArkField,
        value?: Value,
        issueStmt?: Stmt
    ): { currentIssue: IssueReport; issueCategory: NumberCategory } | null {
        const currentIssue = this.getDuplicatedIssue(field, value, issueStmt);
        if (!currentIssue || !(currentIssue.fix instanceof RuleFix)) {
            return null;
        }
        const issueCategory = this.getNumberCategoryFromFixInfo(currentIssue.fix);
        if (issueCategory === null) {
            return null;
        }
        return { currentIssue, issueCategory };
    }

    private getDuplicatedIssue(field?: ArkField, value?: Value, issueStmt?: Stmt): IssueReport | null {
        if (field !== undefined) {
            return this.getFieldIssue(field);
        }
        if (value instanceof Local && issueStmt) {
            return this.getLocalIssue(value, issueStmt);
        }
        return null;
    }

    private getDuplicatedIssuePriorityDecision(ruleCategory: RuleCategory, currentIssue: IssueReport): boolean | null {
        const currentRuleCategory = NumericSemanticIssueText.getRuleCategoryFromProblem(currentIssue.defect.problem);
        const currentPriority = NumericSemanticIssueText.getRuleCategoryPriority(currentRuleCategory);
        const nextPriority = NumericSemanticIssueText.getRuleCategoryPriority(ruleCategory);
        if (nextPriority > currentPriority) {
            this.deleteIssueFromMap(currentIssue);
            return false;
        }
        if (nextPriority < currentPriority) {
            return true;
        }
        return null;
    }

    private shouldSkipSamePriorityNumberIssue(
        numberCategory: NumberCategory,
        issueCategory: NumberCategory,
        currentIssue: IssueReport
    ): boolean {
        if (numberCategory === NumberCategory.long) {
            return this.shouldSkipLongDuplicatedIssue(issueCategory, currentIssue);
        }
        if (numberCategory === NumberCategory.number) {
            return this.shouldSkipNumberDuplicatedIssue(issueCategory, currentIssue);
        }
        if (numberCategory === NumberCategory.int) {
            return true;
        }
        return false;
    }

    private shouldSkipLongDuplicatedIssue(issueCategory: NumberCategory, currentIssue: IssueReport): boolean {
        if (issueCategory === NumberCategory.int) {
            // 删除掉之前的修复为int的，用本次即将add的新的issue替代
            this.deleteIssueFromMap(currentIssue);
            return false;
        }
        if (issueCategory === NumberCategory.number || issueCategory === NumberCategory.long) {
            return true;
        }
        // 其他情况理论上不存在，按照不冲突处理，正常写入新的告警
        return false;
    }

    private shouldSkipNumberDuplicatedIssue(issueCategory: NumberCategory, currentIssue: IssueReport): boolean {
        if (issueCategory === NumberCategory.int || issueCategory === NumberCategory.long) {
            // 删除掉之前的修复为int或long的，用本次即将add的新的issue替代
            this.deleteIssueFromMap(currentIssue);
            return false;
        }
        if (issueCategory === NumberCategory.number) {
            return true;
        }
        // 其他情况理论上不存在，按照不冲突处理，正常写入新的告警
        return false;
    }

    private setIssue(defects: Defects, fix?: RuleFix): void {
        this.options.issuesMap.set(this.getIssuesMapKey(defects.mergeKey), new IssueReport(defects, fix));
    }

    private deleteIssueFromMap(issue: IssueReport): void {
        this.options.issuesMap.delete(this.getIssuesMapKey(issue.defect.mergeKey));
    }

    private getIssuesMapKey(mergeKey: string): string {
        const lastIndex = mergeKey.lastIndexOf('%');
        return mergeKey.substring(0, lastIndex);
    }
}
