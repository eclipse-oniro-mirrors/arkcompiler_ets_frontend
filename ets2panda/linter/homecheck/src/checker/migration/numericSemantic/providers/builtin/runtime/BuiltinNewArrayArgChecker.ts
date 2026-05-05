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
    ClosureType,
    FunctionType,
    Local,
    Stmt,
    Value,
} from 'arkanalyzer/lib';
import { ArkArrayRef } from 'arkanalyzer';
import { NumberConstant } from 'arkanalyzer/lib/core/base/Constant';
import { ArkNewArrayExpr } from 'arkanalyzer/lib/core/base/Expr';
import { BuiltinApiChangeDetector } from './BuiltinApiChangeDetector';
import {
    BuiltinNewArrayArgInfo,
    NumberCategory,
    RuleCategory,
} from '../../../core/NumericSemanticTypes';
import { NumericUsageIssueEmitter } from '../../../diagnostics/report/NumericUsageIssueEmitter';

interface BuiltinNewArrayArgCheckerOptions {
    getBuiltinApiChangeDetector(): BuiltinApiChangeDetector;
    getUsageIssueEmitter(): NumericUsageIssueEmitter;
}

export class BuiltinNewArrayArgChecker {
    constructor(private options: BuiltinNewArrayArgCheckerOptions) {}

    public checkInMethod(stmts: Stmt[]): void {
        const pendingCtorArgs = new Map<Local, Map<number, NumberCategory>>();
        const pendingArgValues = new Map<Local, Map<number, BuiltinNewArrayArgInfo>>();
        for (const stmt of stmts) {
            this.collectArgInfo(stmt, pendingCtorArgs, pendingArgValues);
        }

        this.checkPendingArgs(pendingCtorArgs, pendingArgValues);
    }

    private collectArgInfo(
        stmt: Stmt,
        pendingCtorArgs: Map<Local, Map<number, NumberCategory>>,
        pendingArgValues: Map<Local, Map<number, BuiltinNewArrayArgInfo>>
    ): void {
        if (!(stmt instanceof ArkAssignStmt)) {
            return;
        }
        const leftOp = stmt.getLeftOp();
        const rightOp = stmt.getRightOp();
        if (leftOp instanceof Local && rightOp instanceof ArkNewArrayExpr) {
            this.collectCtorInfo(stmt, leftOp, rightOp, pendingCtorArgs, pendingArgValues);
            return;
        }
        if (leftOp instanceof ArkArrayRef) {
            this.collectArgValue(stmt, leftOp, rightOp, pendingCtorArgs, pendingArgValues);
        }
    }

    private collectCtorInfo(
        stmt: ArkAssignStmt,
        arrayLocal: Local,
        arrayExpr: ArkNewArrayExpr,
        pendingCtorArgs: Map<Local, Map<number, NumberCategory>>,
        pendingArgValues: Map<Local, Map<number, BuiltinNewArrayArgInfo>>
    ): void {
        if (arrayExpr.isFromLiteral()) {
            return;
        }
        const detector = this.options.getBuiltinApiChangeDetector();
        const ctorArgs = detector.getConstructorIntLongArgs('Array');
        if (ctorArgs && ctorArgs.size > 0) {
            pendingCtorArgs.set(arrayLocal, ctorArgs);
            pendingArgValues.set(arrayLocal, new Map<number, BuiltinNewArrayArgInfo>());
        }
        const sizeCategory = detector.getNewArraySizeCategory();
        const size = arrayExpr.getSize();
        if (sizeCategory && !(size instanceof NumberConstant) && !detector.isNumberCategoryType(size.getType(), sizeCategory)) {
            this.checkArrayConstructorArg(stmt, size, sizeCategory);
        }
    }

    private collectArgValue(
        stmt: ArkAssignStmt,
        arrayRef: ArkArrayRef,
        value: Value,
        pendingCtorArgs: Map<Local, Map<number, NumberCategory>>,
        pendingArgValues: Map<Local, Map<number, BuiltinNewArrayArgInfo>>
    ): void {
        const argRules = pendingCtorArgs.get(arrayRef.getBase());
        if (!argRules) {
            return;
        }
        const index = arrayRef.getIndex();
        if (!(index instanceof NumberConstant)) {
            return;
        }
        const argIndex = Number(index.getValue());
        if (!Number.isInteger(argIndex)) {
            return;
        }
        pendingArgValues.get(arrayRef.getBase())?.set(argIndex, { stmt, value });
    }

    private checkPendingArgs(
        pendingCtorArgs: Map<Local, Map<number, NumberCategory>>,
        pendingArgValues: Map<Local, Map<number, BuiltinNewArrayArgInfo>>
    ): void {
        pendingArgValues.forEach((argValues, arrayLocal) => {
            if (!this.isArrayLengthInitializerCtorArgs(argValues)) {
                return;
            }
            const argRules = pendingCtorArgs.get(arrayLocal);
            if (!argRules) {
                return;
            }
            argRules.forEach((category, argIndex) => {
                const argInfo = argValues.get(argIndex);
                if (!argInfo || this.options.getBuiltinApiChangeDetector().isNumberCategoryType(argInfo.value.getType(), category)) {
                    return;
                }
                this.checkArrayConstructorArg(argInfo.stmt, argInfo.value, category);
            });
        });
    }

    private isArrayLengthInitializerCtorArgs(argValues: Map<number, BuiltinNewArrayArgInfo>): boolean {
        const initializer = argValues.get(1)?.value;
        if (!initializer) {
            return false;
        }
        const initializerType = initializer.getType();
        return initializerType instanceof FunctionType || initializerType instanceof ClosureType;
    }

    private checkArrayConstructorArg(stmt: Stmt, arg: Value, numberCategory: NumberCategory): void {
        this.options.getUsageIssueEmitter().emitSingleApiArgIssue(RuleCategory.BuiltinIntType, stmt, arg, numberCategory);
    }
}
