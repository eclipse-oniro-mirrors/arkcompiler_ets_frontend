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
    ArkMethod,
    ArkParameterRef,
    CallGraph,
    ClosureFieldRef,
    ClosureType,
    DVFGBuilder,
    FunctionType,
    LexicalEnvType,
    Local,
    Stmt,
    Value,
} from 'arkanalyzer/lib';
import Logger, { LOG_MODULE_TYPE } from 'arkanalyzer/lib/utils/logger';
import { DVFG, DVFGNode } from 'arkanalyzer/lib/VFG/DVFG';
import { CALL_DEPTH_LIMIT } from '../../../Utils';
import {
    IssueInfo,
    IssueReason,
    NumberCategory,
    PROMISE_CLASS_NAME,
    THEN_METHOD_NAME,
} from '../../core/NumericSemanticTypes';

const logger = Logger.getLogger(LOG_MODULE_TYPE.HOMECHECK, 'NumericParameterFlowAnalyzer');

interface NumericParameterFlowAnalyzerOptions {
    cg: CallGraph;
    dvfg: DVFG;
    dvfgBuilder: DVFGBuilder;
    visited: Set<ArkMethod>;
    incrementCallDepth(): number;
    checkValueOnlyUsedAsIntLong(stmt: Stmt, value: Value, hasChecked: Map<Local, IssueInfo>, numberCategory: NumberCategory): IssueReason;
    isLocalOnlyUsedAsIntLong(stmt: Stmt, local: Local, hasChecked: Map<Local, IssueInfo>, numberCategory: NumberCategory): IssueReason;
}

export class NumericParameterFlowAnalyzer {
    constructor(private options: NumericParameterFlowAnalyzerOptions) {}

    public checkAllArgsOfParameter(stmt: Stmt, hasChecked: Map<Local, IssueInfo>, numberCategory: NumberCategory): IssueReason {
        const checkAll = { value: true };
        const visited: Set<Stmt> = new Set();
        const result = this.checkFromStmt(stmt, hasChecked, numberCategory, checkAll, visited);
        if (!checkAll.value) {
            return IssueReason.CannotFindAll;
        }
        return result;
    }

    public checkClosureFieldRef(closureRef: ClosureFieldRef, hasChecked: Map<Local, IssueInfo>, numberCategory: NumberCategory): IssueReason {
        const closureBase = closureRef.getBase();
        const baseType = closureBase.getType();
        if (!(baseType instanceof LexicalEnvType)) {
            logger.error(`ClosureRef base must be LexicalEnvType, but here is ${baseType.getTypeString()}`);
            return IssueReason.Other;
        }
        const outerLocal = baseType.getClosures().filter(local => local.getName() === closureRef.getFieldName());
        if (outerLocal.length !== 1) {
            logger.error('Failed to find the local from outer method of the closure local.');
            return IssueReason.Other;
        }
        const declaringStmt = outerLocal[0].getDeclaringStmt();
        if (declaringStmt === null) {
            logger.error('Failed to find the declaring stmt of the local from outer method of the closure local.');
            return IssueReason.Other;
        }
        return this.options.isLocalOnlyUsedAsIntLong(declaringStmt, outerLocal[0], hasChecked, numberCategory);
    }

    public isFromParameter(stmt: Stmt): ArkParameterRef | undefined {
        if (!(stmt instanceof ArkAssignStmt)) {
            return undefined;
        }
        const rightOp = stmt.getRightOp();
        if (rightOp instanceof ArkParameterRef) {
            return rightOp;
        }
        return undefined;
    }

    private checkFromStmt(
        stmt: Stmt,
        hasChecked: Map<Local, IssueInfo>,
        numberCategory: NumberCategory,
        checkAll: { value: boolean },
        visited: Set<Stmt>
    ): IssueReason {
        const method = stmt.getCfg().getDeclaringMethod();
        if (!this.options.visited.has(method)) {
            this.options.dvfgBuilder.buildForSingleMethod(method);
            this.options.visited.add(method);
        }

        const node = this.options.dvfg.getOrNewDVFGNode(stmt);
        const workList: DVFGNode[] = [node];
        while (workList.length > 0) {
            const current = workList.shift()!;
            const currentStmt = current.getStmt();
            if (visited.has(currentStmt)) {
                continue;
            }
            visited.add(currentStmt);

            const paramRef = this.isFromParameter(currentStmt);
            if (paramRef) {
                return this.checkParameterCallsites(currentStmt, paramRef, hasChecked, numberCategory, checkAll);
            }
        }
        return IssueReason.Other;
    }

    private checkParameterCallsites(
        currentStmt: Stmt,
        paramRef: ArkParameterRef,
        hasChecked: Map<Local, IssueInfo>,
        numberCategory: NumberCategory,
        checkAll: { value: boolean }
    ): IssueReason {
        const paramIdx = paramRef.getIndex();
        const arrowMethod = currentStmt.getCfg().getDeclaringMethod();
        const callsites = this.options.cg.getInvokeStmtByMethod(arrowMethod.getSignature());

        if (this.isArrowFunctionUsedAsPromiseThenArg(arrowMethod, callsites, paramIdx)) {
            const paramLocal = arrowMethod.getParameterInstances()[paramIdx];
            if (paramLocal instanceof Local) {
                return IssueReason.UsedWithOtherType;
            }
        }

        this.processCallsites(callsites);
        const argMap = this.collectCallSiteArgs(paramIdx, callsites);
        if (this.options.incrementCallDepth() > CALL_DEPTH_LIMIT) {
            checkAll.value = false;
            return IssueReason.CannotFindAll;
        }
        for (const [callSite, arg] of argMap) {
            const res = this.options.checkValueOnlyUsedAsIntLong(callSite, arg, hasChecked, numberCategory);
            if (res !== IssueReason.OnlyUsedAsIntLong) {
                return res;
            }
        }
        return IssueReason.OnlyUsedAsIntLong;
    }

    private processCallsites(callsites: Stmt[]): void {
        callsites.forEach(cs => {
            const declaringMtd = cs.getCfg().getDeclaringMethod();
            if (!this.options.visited.has(declaringMtd)) {
                this.options.dvfgBuilder.buildForSingleMethod(declaringMtd);
                this.options.visited.add(declaringMtd);
            }
        });
    }

    private collectCallSiteArgs(argIdx: number, callsites: Stmt[]): Map<Stmt, Value> {
        const argMap = new Map<Stmt, Value>();
        callsites.forEach(callsite => {
            const arg = callsite.getInvokeExpr()!.getArg(argIdx);
            if (arg !== undefined) {
                argMap.set(callsite, arg);
            }
        });
        return argMap;
    }

    private isArrowFunctionUsedAsPromiseThenArg(arrowMethod: ArkMethod, callsites: Stmt[], paramIdx: number): boolean {
        const arrowMethodSignature = arrowMethod.getSignature();
        for (const callsite of callsites) {
            const invokeExpr = callsite.getInvokeExpr();
            if (!invokeExpr) {
                continue;
            }
            if (invokeExpr.getMethodSignature().getDeclaringClassSignature().getClassName() === PROMISE_CLASS_NAME &&
                invokeExpr.getMethodSignature().getMethodSubSignature().getMethodName() === THEN_METHOD_NAME) {
                const args = invokeExpr.getArgs();
                if (!args || args.length === 0) {
                    continue;
                }
                const arg = args[0];
                if (!arg) {
                    continue;
                }
                const argType = arg.getType();
                if (!(argType instanceof ClosureType || argType instanceof FunctionType)) {
                    continue;
                }
                const methodSignature = argType.getMethodSignature();
                if (methodSignature && methodSignature.toString() === arrowMethodSignature.toString()) {
                    return true;
                }
            }
        }
        return false;
    }
}
