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
    ArkAssignStmt,
    ArkField,
    Local,
    Stmt,
    Value,
} from 'arkanalyzer/lib';
import {
    ChangedArgCategories,
    ChangedResultCategory,
    IssueInfo,
    IssueReason,
    NumberCategory,
    RuleCategory,
} from '../../core/NumericSemanticTypes';
import { NumericIssueReporter } from './NumericIssueReporter';

interface NumericUsageIssueEmitterOptions {
    getIssueReporter(): NumericIssueReporter;
    getClassFieldRes(): Map<ArkField, IssueInfo>;
    resetCallDepth(): void;
    checkValueOnlyUsedAsIntLong(stmt: Stmt, value: Value, hasChecked: Map<Local, IssueInfo>, numberCategory: NumberCategory): IssueReason;
    shouldIgnoreLocal(local: Local): boolean;
    isNumberLikeValue(value: Value): boolean;
}

export class NumericUsageIssueEmitter {
    constructor(private options: NumericUsageIssueEmitterOptions) {}

    public emitChangedArgIssues(stmt: Stmt, changedArgs: ChangedArgCategories): void {
        const args = changedArgs.args;
        if (args === null || args.size === 0) {
            return;
        }

        const res = new Map<Local, IssueInfo>();
        this.options.resetCallDepth();
        for (const [arg, category] of args) {
            const issueReason = this.options.checkValueOnlyUsedAsIntLong(stmt, arg, res, category);
            if (issueReason !== IssueReason.OnlyUsedAsIntLong) {
                this.options.getIssueReporter().addApiArgIssue(changedArgs.ruleCategory, category, issueReason, true, stmt, arg);
            }
        }
        this.emitRelatedArgIssues(changedArgs.ruleCategory, res, stmt);
    }

    public emitSingleApiArgIssue(ruleCategory: RuleCategory, stmt: Stmt, arg: Value, numberCategory: NumberCategory): void {
        this.emitChangedArgIssues(stmt, {
            ruleCategory,
            args: new Map<Value, NumberCategory>([[arg, numberCategory]]),
        });
    }

    public emitChangedResultIssues(stmt: ArkAssignStmt, changedResult: ChangedResultCategory): void {
        if (!changedResult.category) {
            return;
        }
        if (changedResult.requireNumberLikeLeft && !this.options.isNumberLikeValue(stmt.getLeftOp())) {
            return;
        }

        const res = new Map<Local, IssueInfo>();
        this.options.resetCallDepth();
        const leftOp = stmt.getLeftOp();
        this.options.checkValueOnlyUsedAsIntLong(stmt, leftOp, res, changedResult.category);
        this.emitRelatedResultIssues(changedResult.ruleCategory, res, stmt);
    }

    private emitRelatedArgIssues(ruleCategory: RuleCategory, res: Map<Local, IssueInfo>, usedStmt: Stmt): void {
        res.forEach((issueInfo, local) => {
            if (this.options.shouldIgnoreLocal(local)) {
                return;
            }
            const declaringStmt = local.getDeclaringStmt();
            if (declaringStmt !== null && issueInfo.issueReason === IssueReason.OnlyUsedAsIntLong) {
                this.options.getIssueReporter().addApiArgIssue(
                    ruleCategory,
                    issueInfo.numberCategory,
                    issueInfo.issueReason,
                    true,
                    declaringStmt,
                    local,
                    undefined,
                    usedStmt
                );
            }
        });
        this.options.getClassFieldRes().forEach((fieldInfo, field) => {
            if (fieldInfo.issueReason === IssueReason.OnlyUsedAsIntLong || fieldInfo.issueReason === IssueReason.UsedWithOtherType) {
                this.options.getIssueReporter().addApiArgIssue(
                    ruleCategory,
                    fieldInfo.numberCategory,
                    fieldInfo.issueReason,
                    true,
                    undefined,
                    undefined,
                    field,
                    usedStmt
                );
            }
        });
    }

    private emitRelatedResultIssues(ruleCategory: RuleCategory, res: Map<Local, IssueInfo>, usedStmt: Stmt): void {
        res.forEach((issueInfo, local) => {
            if (this.options.shouldIgnoreLocal(local)) {
                return;
            }
            const declaringStmt = local.getDeclaringStmt();
            if (declaringStmt !== null) {
                this.options.getIssueReporter().addApiReturnOrFieldIssue(
                    ruleCategory,
                    issueInfo.numberCategory,
                    issueInfo.issueReason,
                    declaringStmt,
                    local,
                    undefined,
                    usedStmt
                );
            }
        });
        this.options.getClassFieldRes().forEach((fieldInfo, field) => {
            if (fieldInfo.issueReason === IssueReason.OnlyUsedAsIntLong || fieldInfo.issueReason === IssueReason.UsedWithOtherType) {
                this.options.getIssueReporter().addApiReturnOrFieldIssue(
                    ruleCategory,
                    fieldInfo.numberCategory,
                    fieldInfo.issueReason,
                    undefined,
                    undefined,
                    field,
                    usedStmt
                );
            }
        });
    }
}
