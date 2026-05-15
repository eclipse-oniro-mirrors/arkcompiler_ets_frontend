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
    AbstractFieldRef,
    AbstractInvokeExpr,
    ArkAssignStmt,
    Stmt,
} from 'arkanalyzer/lib';
import { BuiltinApiChangeDetector } from './runtime/BuiltinApiChangeDetector';
import {
    ApiNumberChangeProvider,
    ChangedArgCategories,
    ChangedResultCategory,
    IssueReason,
    NumberCategory,
    RuleCategory,
} from '../../core/NumericSemanticTypes';
import { NumericIssueReporter } from '../../diagnostics/report/NumericIssueReporter';

export class BuiltinNumberChangeProvider implements ApiNumberChangeProvider {
    constructor(private detector: BuiltinApiChangeDetector, private reporter: NumericIssueReporter) {}

    public beforeArgCheck(stmt: Stmt, invokeExpr: AbstractInvokeExpr): void {
        const ambiguousArgs = this.detector.getAmbiguousIntLongArgsFromInvokeExpr(invokeExpr);
        ambiguousArgs.forEach(arg => {
            this.reporter.addApiArgIssue(
                RuleCategory.BuiltinIntType,
                NumberCategory.number,
                IssueReason.AmbiguousIntLong,
                false,
                stmt,
                arg
            );
        });
    }

    public getChangedArgCategories(invokeExpr: AbstractInvokeExpr): ChangedArgCategories {
        return {
            ruleCategory: RuleCategory.BuiltinIntType,
            args: this.detector.getIntLongArgsFromInvokeExpr(invokeExpr),
        };
    }

    public getChangedReturnCategory(_stmt: ArkAssignStmt, rightInvokeExpr: AbstractInvokeExpr | null): ChangedResultCategory {
        return {
            ruleCategory: RuleCategory.BuiltinIntType,
            category: rightInvokeExpr ? this.detector.checkReturnType(rightInvokeExpr) : null,
        };
    }

    public getChangedFieldCategory(fieldRef: AbstractFieldRef): ChangedResultCategory {
        return {
            ruleCategory: RuleCategory.BuiltinIntType,
            category: this.detector.checkFieldType(fieldRef),
            requireNumberLikeLeft: true,
        };
    }
}
