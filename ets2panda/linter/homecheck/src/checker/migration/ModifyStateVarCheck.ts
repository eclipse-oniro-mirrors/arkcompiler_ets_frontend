/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
    ArkInstanceInvokeExpr,
    CallGraph,
    CallGraphBuilder,
    Local,
    Stmt,
    Value,
    FieldSignature,
    ArkMethod,
} from 'arkanalyzer';
import Logger, { LOG_MODULE_TYPE } from 'arkanalyzer/lib/utils/logger';
import { BaseChecker, BaseMetaData } from '../BaseChecker';
import { Rule, Defects, MatcherTypes, MatcherCallback, ClassMatcher, MethodMatcher } from '../../Index';
import { IssueReport } from '../../model/Defects';
import { CALL_DEPTH_LIMIT, CALLBACK_METHOD_NAME, CallGraphHelper } from './Utils';
import { WarnInfo } from '../../utils/common/Utils';

const logger = Logger.getLogger(LOG_MODULE_TYPE.HOMECHECK, 'ModifyStateVarCheck');
const gMetaData: BaseMetaData = {
    severity: 1,
    ruleDocPath: '',
    description: 'It is not allowed to update state when the build function is running',
};

export class ModifyStateVarCheck implements BaseChecker {
    readonly metaData: BaseMetaData = gMetaData;
    public rule: Rule;
    public defects: Defects[] = [];
    public issues: IssueReport[] = [];

    private classMatcher: ClassMatcher = {
        matcherType: MatcherTypes.CLASS,
        hasViewTree: true,
    };

    private buildMatcher: MethodMatcher = {
        matcherType: MatcherTypes.METHOD,
        class: [this.classMatcher],
        name: ['build'],
    };

    public registerMatchers(): MatcherCallback[] {
        const matchBuildCb: MatcherCallback = {
            matcher: this.buildMatcher,
            callback: this.check,
        };
        return [matchBuildCb];
    }

    public check = (target: ArkMethod): void => {
        const scene = target.getDeclaringArkFile().getScene();
        let callGraph = CallGraphHelper.getCGInstance(scene);
        let callGraphBuilder = new CallGraphBuilder(callGraph, scene);
        callGraphBuilder.buildClassHierarchyCallGraph([target.getSignature()]);

        const arkClass = target.getDeclaringArkClass();
        const stateVars = new Set(
            arkClass
                .getFields()
                .filter(f => f.hasDecorator('State'))
                .map(f => f.getSignature())
        );
        if (stateVars.size > 0) {
            this.checkMethod(target, callGraph, stateVars);
        }
    };

    private checkMethod(target: ArkMethod, cg: CallGraph, stateVars: Set<FieldSignature>, depth: number = 0): void {
        if (depth > CALL_DEPTH_LIMIT) {
            return;
        }
        let aliases = new Set<Local>();
        const stmts = target.getBody()?.getCfg().getStmts() ?? [];
        for (const stmt of stmts) {
            const invokeExpr = stmt.getInvokeExpr();
            if (invokeExpr && invokeExpr instanceof ArkInstanceInvokeExpr) {
                if (
                    CALLBACK_METHOD_NAME.includes(
                        invokeExpr.getMethodSignature().getMethodSubSignature().getMethodName()
                    )
                ) {
                    continue;
                }
            }
            cg.getCallSiteByStmt(stmt).forEach(cs => {
                const callee = cg.getArkMethodByFuncID(cs.calleeFuncID);
                if (callee) {
                    this.checkMethod(callee, cg, stateVars, depth + 1);
                }
            });
            if (!(stmt instanceof ArkAssignStmt)) {
                continue;
            }
            if (this.isAssignToStateVar(stmt, stateVars, aliases)) {
                this.addIssueReport(stmt, stmt.getLeftOp());
            } else {
                const alias = this.isAliasOfStateVar(stmt, stateVars, aliases);
                if (alias) {
                    aliases.add(alias);
                }
            }
        }
    }

    private isAssignToStateVar(stmt: ArkAssignStmt, stateVars: Set<FieldSignature>, aliases: Set<Local>): boolean {
        const leftOp = stmt.getLeftOp();
        if (leftOp instanceof ArkInstanceFieldRef) {
            // this.n = 1 or this.obj.n = 1
            return stateVars.has(leftOp.getFieldSignature()) || aliases.has(leftOp.getBase());
        }
        return false;
    }

    private isAliasOfStateVar(
        stmt: ArkAssignStmt,
        stateVars: Set<FieldSignature>,
        aliases: Set<Local>
    ): Local | undefined {
        const leftOp = stmt.getLeftOp();
        const rightOp = stmt.getRightOp();
        if (leftOp instanceof Local && rightOp instanceof ArkInstanceFieldRef) {
            // %1 = this.n
            // or
            // %1 = this.obj; %2 = %1.n
            if (stateVars.has(rightOp.getFieldSignature()) || aliases.has(rightOp.getBase())) {
                return leftOp;
            }
        }
        return undefined;
    }

    private addIssueReport(stmt: Stmt, operand: Value): void {
        const severity = this.rule.alert ?? this.metaData.severity;
        const warnInfo = this.getLineAndColumn(stmt, operand);
        const problem = 'NoStateUpdateDuringRender';
        const desc = `${this.metaData.description} (${this.rule.ruleId.replace('@migration/', '')})`;
        let defects = new Defects(
            warnInfo.line,
            warnInfo.startCol,
            warnInfo.endCol,
            problem,
            desc,
            severity,
            this.rule.ruleId,
            warnInfo.filePath,
            this.metaData.ruleDocPath,
            true,
            false,
            false
        );
        this.issues.push(new IssueReport(defects, undefined));
    }

    private getLineAndColumn(stmt: Stmt, operand: Value): WarnInfo {
        const arkFile = stmt.getCfg().getDeclaringMethod().getDeclaringArkFile();
        const originPosition = stmt.getOperandOriginalPosition(operand);
        if (arkFile && originPosition) {
            const originPath = arkFile.getFilePath();
            const line = originPosition.getFirstLine();
            const startCol = originPosition.getFirstCol();
            const endCol = startCol;
            return { line, startCol, endCol, filePath: originPath };
        } else {
            logger.debug('ArkFile is null.');
        }
        return { line: -1, startCol: -1, endCol: -1, filePath: '' };
    }
}
