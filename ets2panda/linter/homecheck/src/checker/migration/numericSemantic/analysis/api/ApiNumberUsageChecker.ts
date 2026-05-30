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
    ArkReturnStmt,
    Stmt,
} from 'arkanalyzer/lib';
import { ApiNumberChangeProvider } from '../../core/NumericSemanticTypes';
import { NumericUsageIssueEmitter } from '../../diagnostics/report/NumericUsageIssueEmitter';

interface ApiNumberUsageCheckerOptions {
    getProviders(): ApiNumberChangeProvider[];
    getUsageIssueEmitter(): NumericUsageIssueEmitter;
}

export class ApiNumberUsageChecker {
    constructor(private options: ApiNumberUsageCheckerOptions) {}

    public checkInStmt(stmt: Stmt): void {
        const providers = this.options.getProviders();
        for (const invokeExpr of this.collectInvokeExprs(stmt)) {
            this.checkInvokeExprArgs(stmt, invokeExpr, providers);
        }
        if (stmt instanceof ArkReturnStmt) {
            for (const provider of providers) {
                const changedReturnedValue = provider.getChangedReturnedValueCategory?.(stmt.getOp());
                if (changedReturnedValue) {
                    this.options.getUsageIssueEmitter().emitChangedMethodReturnIssues(stmt, changedReturnedValue);
                }
            }
            return;
        }
        if (!(stmt instanceof ArkAssignStmt)) {
            return;
        }

        const rightOp = stmt.getRightOp();
        const rightInvokeExpr = rightOp instanceof AbstractInvokeExpr ? rightOp : null;
        for (const provider of providers) {
            this.options.getUsageIssueEmitter().emitChangedResultIssues(stmt, provider.getChangedReturnCategory(stmt, rightInvokeExpr));
        }
        if (rightOp instanceof AbstractFieldRef) {
            for (const provider of providers) {
                this.options.getUsageIssueEmitter().emitChangedResultIssues(stmt, provider.getChangedFieldCategory(rightOp));
            }
        }
    }

    private collectInvokeExprs(stmt: Stmt): AbstractInvokeExpr[] {
        const invokeExprs: AbstractInvokeExpr[] = [];
        const invokeExpr = stmt.getInvokeExpr();
        if (invokeExpr) {
            invokeExprs.push(invokeExpr);
        }
        const directInvokeExpr = this.getDirectInvokeExpr(stmt);
        if (directInvokeExpr && !invokeExprs.includes(directInvokeExpr)) {
            invokeExprs.push(directInvokeExpr);
        }
        return invokeExprs;
    }

    private getDirectInvokeExpr(stmt: Stmt): AbstractInvokeExpr | null {
        if (stmt instanceof ArkAssignStmt) {
            const rightOp = stmt.getRightOp();
            if (rightOp instanceof AbstractInvokeExpr) {
                return rightOp;
            }
        }
        if (stmt instanceof ArkReturnStmt) {
            const returnOp = stmt.getOp();
            if (returnOp instanceof AbstractInvokeExpr) {
                return returnOp;
            }
        }
        return null;
    }

    private checkInvokeExprArgs(stmt: Stmt, invokeExpr: AbstractInvokeExpr, providers: ApiNumberChangeProvider[]): void {
        for (const provider of providers) {
            provider.beforeArgCheck?.(stmt, invokeExpr);
            this.options.getUsageIssueEmitter().emitChangedArgIssues(stmt, provider.getChangedArgCategories(invokeExpr));
            const changedFunctionParams = provider.getChangedFunctionParamCategories?.(invokeExpr);
            if (changedFunctionParams) {
                this.options.getUsageIssueEmitter().emitChangedFunctionParamIssues(stmt, changedFunctionParams);
            }
            const changedFunctionReturns = provider.getChangedFunctionReturnCategories?.(invokeExpr);
            if (changedFunctionReturns) {
                this.options.getUsageIssueEmitter().emitChangedFunctionReturnIssues(stmt, changedFunctionReturns);
            }
        }
    }
}
