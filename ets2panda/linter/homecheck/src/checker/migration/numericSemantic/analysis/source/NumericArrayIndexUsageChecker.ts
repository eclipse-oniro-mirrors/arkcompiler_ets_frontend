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
    ArkInstanceFieldRef,
    ArkParameterRef,
    FullPosition,
    GlobalRef,
    Local,
    Stmt,
    Value,
} from 'arkanalyzer/lib';
import { NumberConstant } from 'arkanalyzer/lib/core/base/Constant';
import { ArkArrayRef } from 'arkanalyzer';
import {
    IssueInfo,
    IssueReason,
    LENGTH_FIELD_NAME,
    NumberCategory,
} from '../../core/NumericSemanticTypes';
import { NumericSourceIssueEmitter } from '../../diagnostics/emitters/NumericSourceIssueEmitter';

interface NumericArrayIndexUsageCheckerOptions {
    getSourceIssueEmitter(): NumericSourceIssueEmitter;
    resetCallDepth(): void;
    checkValueOnlyUsedAsIntLong(stmt: Stmt, value: Value, hasChecked: Map<Local, IssueInfo>, numberCategory: NumberCategory): IssueReason;
    isFloatActuallyInt(constant: NumberConstant): boolean;
    isFromParameter(stmt: Stmt): ArkParameterRef | undefined;
}

export class NumericArrayIndexUsageChecker {
    constructor(private options: NumericArrayIndexUsageCheckerOptions) {}

    public checkArrayIndexInStmt(stmt: Stmt): void {
        const res = new Map<Local, IssueInfo>();
        this.options.resetCallDepth();
        const index = this.getIndexValue(stmt);
        if (index === null) {
            return;
        }
        // 对于index为1.0、2.0这种number constant，需要告警并自动修复成1、2
        if (index instanceof NumberConstant && this.options.isFloatActuallyInt(index)) {
            this.options.getSourceIssueEmitter().emitActuallyIntArrayIndexIssue(stmt, index);
            return;
        }
        if (index instanceof Local) {
            const isParameter = index.getDeclaringStmt();
            if (isParameter instanceof ArkAssignStmt && this.options.isFromParameter(isParameter)) {
                return;
            }
        }
        const issueReason = this.options.checkValueOnlyUsedAsIntLong(stmt, index, res, NumberCategory.int);
        if (issueReason !== IssueReason.OnlyUsedAsIntLong) {
            // 若index原先非int，则获取的数组元素应该是undefined，不可以对其进行强转int，否则对原始代码的语义有修改
            this.options.getSourceIssueEmitter().emitUnsafeArrayIndexIssue(stmt, index, issueReason);
        }
        this.options.getSourceIssueEmitter().emitArrayIndexLocalIssues(res);
        this.options.getSourceIssueEmitter().emitArrayIndexFieldIssues();
    }

    public getActualIndexPosInStmt(stmt: Stmt): FullPosition {
        // 处理array定义时为unionType，例如string[] | undefined，array的index访问语句被错误表示成ArkInstanceFieldRef的场景
        // 由于IR表示在此处的能力限制，此处判断unionType中包含arrayType，且fieldRef的名字非length，取第4个操作数的位置为实际index的位置
        const fieldRef = this.getFieldRefActualArrayRef(stmt);
        if (!fieldRef) {
            return FullPosition.DEFAULT;
        }
        const positions = stmt.getOperandOriginalPositions();
        if (!positions || positions.length !== 4) {
            return FullPosition.DEFAULT;
        }
        return positions[3];
    }

    private getFieldRefActualArrayRef(stmt: Stmt): ArkInstanceFieldRef | null {
        const fieldRef = stmt.getFieldRef();
        if (fieldRef === undefined || !(fieldRef instanceof ArkInstanceFieldRef)) {
            return null;
        }
        if (!fieldRef.isDynamic()) {
            return null;
        }
        const fieldName = fieldRef.getFieldName();
        if (fieldName === LENGTH_FIELD_NAME) {
            return null;
        }
        return fieldRef;
    }

    private getIndexValue(stmt: Stmt): Value | null {
        // 处理stmt的uses中的array的index访问语句
        const arrayRef = stmt.getArrayRef();
        if (arrayRef !== undefined) {
            return arrayRef.getIndex();
        }

        // 处理array的index访问出现在赋值语句左边的情况
        if (stmt.getDef() instanceof ArkArrayRef) {
            return (stmt.getDef() as ArkArrayRef).getIndex();
        }

        // 处理array定义时为unionType，例如string[] | undefined，array的index访问语句被错误表示成ArkInstanceFieldRef的场景
        // 由于IR表示在此处的能力限制，此处判断unionType中包含arrayType，且fieldRef的名字非length，且在local或global中能找到同名的情况
        const methodBody = stmt.getCfg().getDeclaringMethod().getBody();
        if (methodBody === undefined) {
            return null;
        }

        const fieldRef = this.getFieldRefActualArrayRef(stmt);
        if (!fieldRef) {
            return null;
        }
        const index = methodBody.getLocals().get(fieldRef.getFieldName());
        if (index !== undefined) {
            return index;
        }
        const global = methodBody.getUsedGlobals()?.get(fieldRef.getFieldName());
        if (global === undefined || !(global instanceof GlobalRef)) {
            return null;
        }
        return global.getRef();
    }
}
