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
    ArkParameterRef,
    Local,
    Stmt,
    Value,
} from 'arkanalyzer/lib';
import { Utils } from '../../../../../Index';
import {
    IssueInfo,
    IssueReason,
    NumberCategory,
    RuleCategory,
} from '../../core/NumericSemanticTypes';
import { NumericIssueReporter } from '../report/NumericIssueReporter';

interface NumericSourceIssueEmitterOptions {
    getIssueReporter(): NumericIssueReporter;
    getClassFieldRes(): Map<ArkField, IssueInfo>;
    shouldIgnoreLocal(local: Local): boolean;
}

export class NumericSourceIssueEmitter {
    constructor(private options: NumericSourceIssueEmitterOptions) {}

    public emitFieldInitializerIssues(): void {
        this.options.getClassFieldRes().forEach((fieldInfo, field) => {
            this.options.getIssueReporter().addIssue(
                RuleCategory.NumericLiteral,
                fieldInfo.numberCategory,
                fieldInfo.issueReason,
                true,
                undefined,
                undefined,
                field
            );
        });
    }

    public emitNumericLiteralLocalIssues(res: Map<Local, IssueInfo>): void {
        res.forEach((issueInfo, local) => {
            if (this.options.shouldIgnoreLocal(local)) {
                return;
            }
            const declaringStmt = local.getDeclaringStmt();
            if (declaringStmt === null) {
                return;
            }
            // 无论local的判定结果是什么，均需要进行自动修复类型注解为int或者number
            this.options.getIssueReporter().addIssue(RuleCategory.NumericLiteral, issueInfo.numberCategory, issueInfo.issueReason, true, declaringStmt, local);
        });
    }

    public emitNumericLiteralFieldIssues(): void {
        this.options.getClassFieldRes().forEach((fieldInfo, field) => {
            if (fieldInfo.issueReason === IssueReason.OnlyUsedAsIntLong || fieldInfo.issueReason === IssueReason.UsedWithOtherType) {
                // 如果能明确判断出field是int或非int，则添加类型注解int或number，其他找不全的场景不变
                this.options.getIssueReporter().addIssue(
                    RuleCategory.NumericLiteral,
                    fieldInfo.numberCategory,
                    fieldInfo.issueReason,
                    true,
                    undefined,
                    undefined,
                    field
                );
            }
        });
    }

    public emitActuallyIntArrayIndexIssue(stmt: Stmt, index: Value): void {
        this.options.getIssueReporter().addIssue(
            RuleCategory.ArrayIndex,
            NumberCategory.number,
            IssueReason.ActuallyIntConstant,
            true,
            stmt,
            index
        );
    }

    public emitUnsafeArrayIndexIssue(stmt: Stmt, index: Value, issueReason: IssueReason): void {
        this.options.getIssueReporter().addIssue(RuleCategory.ArrayIndex, NumberCategory.number, issueReason, false, stmt, index);
    }

    public emitArrayIndexLocalIssues(res: Map<Local, IssueInfo>): void {
        res.forEach((issueInfo, local) => {
            if (this.options.shouldIgnoreLocal(local)) {
                return;
            }
            const declaringStmt = local.getDeclaringStmt();
            if (declaringStmt instanceof ArkAssignStmt && declaringStmt.getRightOp() instanceof ArkParameterRef && !Utils.isNearlyNumberType(local.getType())) {
                this.options.getIssueReporter().addIssue(RuleCategory.ArrayIndex, issueInfo.numberCategory, issueInfo.issueReason, false, declaringStmt, local);
            } else if (declaringStmt !== null) {
                this.options.getIssueReporter().addIssue(RuleCategory.ArrayIndex, issueInfo.numberCategory, issueInfo.issueReason, true, declaringStmt, local);
            }
        });
    }

    public emitArrayIndexFieldIssues(): void {
        this.options.getClassFieldRes().forEach((fieldInfo, field) => {
            if (fieldInfo.issueReason === IssueReason.OnlyUsedAsIntLong) {
                // 如果能明确判断出field是int，则添加类型注解int，其他找不全的场景不变
                this.options.getIssueReporter().addIssue(
                    RuleCategory.ArrayIndex,
                    NumberCategory.int,
                    fieldInfo.issueReason,
                    true,
                    undefined,
                    undefined,
                    field
                );
            }
        });
    }

    public emitAsyncReturnIntConstantIssue(stmt: Stmt, value: Value): void {
        this.options.getIssueReporter().addIssue(
            RuleCategory.NumericLiteral,
            NumberCategory.number,
            IssueReason.UsedWithOtherType,
            true,
            stmt,
            value
        );
    }
}
