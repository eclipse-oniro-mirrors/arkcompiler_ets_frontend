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
    ArkMethod,
    ArkAssignStmt,
    FieldSignature,
    Stmt,
    Scene,
    Value,
    ArkClass,
    ArkFile,
    ArkInstanceOfExpr,
    ArkNewExpr,
    CallGraph,
    ArkParameterRef,
    ArkInstanceFieldRef,
    AbstractFieldRef,
    Local,
    ArkArrayRef,
    ClassSignature,
} from 'arkanalyzer/lib';
import Logger, { LOG_MODULE_TYPE } from 'arkanalyzer/lib/utils/logger';
import { BaseChecker, BaseMetaData } from '../BaseChecker';
import { Rule, Defects, MatcherCallback } from '../../Index';
import { IssueReport } from '../../model/Defects';
import { DVFGNode } from 'arkanalyzer/lib/VFG/DVFG';
import { DVFGHelper, CALL_DEPTH_LIMIT, GlobalCallGraphHelper } from './Utils';
import { WarnInfo } from '../../utils/common/Utils';

const logger = Logger.getLogger(LOG_MODULE_TYPE.HOMECHECK, 'ObjectLiteralCheck');
const gMetaData: BaseMetaData = {
    severity: 1,
    ruleDocPath: '',
    description: 'Object literal shall generate instance of a specific class',
};

export class ObjectLiteralCheck implements BaseChecker {
    readonly metaData: BaseMetaData = gMetaData;
    public rule: Rule;
    public defects: Defects[] = [];
    public issues: IssueReport[] = [];
    private cg: CallGraph;
    private visited: Set<ArkMethod> = new Set();

    public registerMatchers(): MatcherCallback[] {
        const matchBuildCb: MatcherCallback = {
            matcher: undefined,
            callback: this.check,
        };
        return [matchBuildCb];
    }

    public check = (scene: Scene): void => {
        this.cg = GlobalCallGraphHelper.getCGInstance(scene);

        for (let arkFile of scene.getFiles()) {
            const topLevelVarMap: Map<string, Stmt[]> = new Map();
            this.collectImportedVar(topLevelVarMap, arkFile, scene);
            this.collectTopLevelVar(topLevelVarMap, arkFile.getDefaultClass(), scene);

            const handleClass = (cls: ArkClass, localTopLevelVar: Map<string, Stmt[]> | undefined = undefined): void => {
                if (!localTopLevelVar) {
                    cls.getMethods().forEach(m => this.processArkMethod(m, topLevelVarMap, scene));
                } else {
                    topLevelVarMap.forEach((v, k) => localTopLevelVar.set(k, v));
                    cls.getMethods().forEach(m => this.processArkMethod(m, localTopLevelVar, scene));
                }
            };

            arkFile.getClasses().forEach(cls => handleClass(cls));
            arkFile.getAllNamespacesUnderThisFile().forEach(n => {
                const localTopLevelVar: Map<string, Stmt[]> = new Map();
                this.collectTopLevelVar(localTopLevelVar, n.getDefaultClass(), scene);
                n.getClasses().forEach(cls => handleClass(cls, localTopLevelVar));
            });
        }
    };

    public processArkMethod(target: ArkMethod, topLevelVarMap: Map<string, Stmt[]>, scene: Scene): void {
        const stmts = target.getBody()?.getCfg().getStmts() ?? [];
        for (const stmt of stmts) {
            if (!(stmt instanceof ArkAssignStmt)) {
                continue;
            }
            const rightOp = stmt.getRightOp();
            if (!(rightOp instanceof ArkInstanceOfExpr)) {
                continue;
            }
            if (!this.visited.has(target)) {
                DVFGHelper.buildSingleDVFG(target, scene);
                this.visited.add(target);
            }

            let result: Stmt[] = [];
            let checkAll = { value: true };
            let visited: Set<Stmt> = new Set();
            this.checkFromStmt(stmt, scene, result, topLevelVarMap, checkAll, visited);
            result.forEach(s => this.addIssueReport(s, (s as ArkAssignStmt).getRightOp()));
            if (!checkAll.value) {
                this.addIssueReport(stmt, rightOp, checkAll.value);
            }
        }
    }

    private collectImportedVar(importVarMap: Map<string, Stmt[]>, file: ArkFile, scene: Scene): void {
        file.getImportInfos().forEach(importInfo => {
            const exportInfo = importInfo.getLazyExportInfo();
            if (exportInfo === null) {
                return;
            }
            const arkExport = exportInfo.getArkExport();
            if (!arkExport || !(arkExport instanceof Local)) {
                return;
            }
            const declaringStmt = arkExport.getDeclaringStmt();
            if (!declaringStmt) {
                return;
            }
            DVFGHelper.buildSingleDVFG(declaringStmt.getCfg().getDeclaringMethod(), scene);
            importVarMap.set(arkExport.getName(), [declaringStmt]);
        });
    }

    private collectTopLevelVar(topLevelVarMap: Map<string, Stmt[]>, defaultClass: ArkClass, scene: Scene): void {
        const defaultMethod = defaultClass.getDefaultArkMethod();
        if (!defaultMethod) {
            return;
        }
        DVFGHelper.buildSingleDVFG(defaultMethod, scene);
        const stmts = defaultMethod.getBody()?.getCfg().getStmts() ?? [];
        for (const stmt of stmts) {
            if (!(stmt instanceof ArkAssignStmt)) {
                continue;
            }
            let name = undefined;
            const leftOp = stmt.getLeftOp();
            if (leftOp instanceof Local) {
                name = leftOp.getName();
            } else if (leftOp instanceof ArkInstanceFieldRef && leftOp.getBase() instanceof Local) {
                name = `${leftOp.getBase().getName()}.${leftOp.getFieldSignature().getFieldName()}`;
            }
            if (!name) {
                continue;
            }
            if (name.startsWith('%') || name === 'this') {
                continue;
            }
            topLevelVarMap.set(name, [...(topLevelVarMap.get(name) ?? []), stmt]);
        }
    }

    private checkFromStmt(
        stmt: Stmt,
        scene: Scene,
        res: Stmt[],
        topLevelVarMap: Map<string, Stmt[]>,
        checkAll: { value: boolean },
        visited: Set<Stmt>,
        depth: number = 0
    ): void {
        if (depth > CALL_DEPTH_LIMIT) {
            checkAll.value = false;
            return;
        }
        const node = DVFGHelper.getOrNewDVFGNode(stmt, scene);
        let worklist: DVFGNode[] = [node];
        while (worklist.length > 0) {
            const current = worklist.shift()!;
            const currentStmt = current.getStmt();
            if (visited.has(currentStmt)) {
                continue;
            }
            visited.add(currentStmt);
            if (this.isObjectLiteral(currentStmt, scene)) {
                res.push(currentStmt);
                continue;
            }
            const gvName = this.checkIfIsTopLevelVar(currentStmt);
            if (gvName) {
                const globalDefs = topLevelVarMap.get(gvName);
                globalDefs?.forEach(d => {
                    worklist.push(DVFGHelper.getOrNewDVFGNode(d, scene));
                });
            }
            const isClsField = this.isClassField(currentStmt, scene);
            if (isClsField) {
                isClsField.forEach(d => worklist.push(DVFGHelper.getOrNewDVFGNode(d, scene)));
            }
            const isArrayField = this.isArrayField(currentStmt, topLevelVarMap);
            if (isArrayField) {
                isArrayField.forEach(d => worklist.push(DVFGHelper.getOrNewDVFGNode(d, scene)));
            }
            const callsite = this.cg.getCallSiteByStmt(currentStmt);
            callsite.forEach(cs => {
                const declaringMtd = this.cg.getArkMethodByFuncID(cs.calleeFuncID);
                if (!declaringMtd || !declaringMtd.getCfg()) {
                    return;
                }
                if (!this.visited.has(declaringMtd)) {
                    DVFGHelper.buildSingleDVFG(declaringMtd, scene);
                    this.visited.add(declaringMtd);
                }
                declaringMtd.getReturnStmt().forEach(r => this.checkFromStmt(r, scene, res, topLevelVarMap, checkAll, visited, depth + 1));
            });
            const paramRef = this.isFromParameter(currentStmt);
            if (paramRef) {
                const paramIdx = paramRef.getIndex();
                const callsites = this.cg.getInvokeStmtByMethod(currentStmt.getCfg().getDeclaringMethod().getSignature());
                this.processCallsites(callsites, scene);
                this.collectArgDefs(paramIdx, callsites, scene).forEach(d => this.checkFromStmt(d, scene, res, topLevelVarMap, checkAll, visited, depth + 1));
            }
            current.getIncomingEdge().forEach(e => worklist.push(e.getSrcNode() as DVFGNode));
        }
    }

    private checkIfIsTopLevelVar(stmt: Stmt): string | undefined {
        if (!(stmt instanceof ArkAssignStmt)) {
            return undefined;
        }
        const rightOp = stmt.getRightOp();
        if (rightOp instanceof Local && !rightOp.getDeclaringStmt()) {
            return rightOp.getName();
        }
        if (rightOp instanceof ArkInstanceFieldRef && rightOp.getBase() instanceof Local) {
            return `${rightOp.getBase().getName()}.${rightOp.getFieldSignature().getFieldName()}`;
        }
        if (!(rightOp instanceof ArkInstanceOfExpr)) {
            return undefined;
        }
        const obj = rightOp.getOp();
        if (obj instanceof Local && !obj.getDeclaringStmt()) {
            return obj.getName();
        }
        return undefined;
    }

    private processCallsites(callsites: Stmt[], scene: Scene): void {
        callsites.forEach(cs => {
            const declaringMtd = cs.getCfg().getDeclaringMethod();
            if (!this.visited.has(declaringMtd)) {
                DVFGHelper.buildSingleDVFG(declaringMtd, scene);
                this.visited.add(declaringMtd);
            }
        });
    }

    private isObjectLiteral(stmt: Stmt, scene: Scene): boolean {
        if (!(stmt instanceof ArkAssignStmt)) {
            return false;
        }
        const rightOp = stmt.getRightOp();
        if (!(rightOp instanceof ArkNewExpr)) {
            return false;
        }
        const classSig = rightOp.getClassType().getClassSignature();
        if (scene.getClass(classSig)?.isAnonymousClass()) {
            return true;
        }
        return false;
    }

    private isClassField(stmt: Stmt, scene: Scene): Stmt[] | undefined {
        if (!(stmt instanceof ArkAssignStmt)) {
            return undefined;
        }
        const clsField = stmt.getRightOp();
        if (!(clsField instanceof AbstractFieldRef)) {
            return undefined;
        }
        const fieldSig = clsField.getFieldSignature();
        const clsSig = fieldSig.getDeclaringSignature();
        if (!(clsSig instanceof ClassSignature)) {
            return undefined;
        }
        const cls = scene.getClass(clsSig);
        if (!cls) {
            logger.error(`cannot find class based on class sig: ${clsSig.toString()}`);
            return undefined;
        }
        const field = cls.getField(fieldSig);
        if (!field) {
            logger.error(`cannot find field based on field sig: ${fieldSig.toString()}`);
            return undefined;
        }
        if (!field.isStatic()) {
            DVFGHelper.buildSingleDVFG(cls.getInstanceInitMethod(), scene);
        } else {
            DVFGHelper.buildSingleDVFG(cls.getStaticInitMethod(), scene);
        }
        return field.getInitializer().slice(-1);
    }

    private isArrayField(stmt: Stmt, topLevelVarMap: Map<string, Stmt[]>): Stmt[] | undefined {
        if (!(stmt instanceof ArkAssignStmt)) {
            return undefined;
        }
        const arrField = stmt.getRightOp();
        if (!(arrField instanceof ArkArrayRef)) {
            return undefined;
        }
        const arr = arrField.getBase();
        const index = arrField.getIndex();
        let arrDeclarations: Stmt[] = [];
        if (arr.getDeclaringStmt()) {
            arrDeclarations.push(arr.getDeclaringStmt()!);
        } else if (topLevelVarMap.has(arr.getName())) {
            arrDeclarations = topLevelVarMap.get(arr.getName())!;
        }
        const res: Stmt[] = arrDeclarations.flatMap(d => {
            // arr = %0
            // %0[0] = ...
            if (!(d instanceof ArkAssignStmt)) {
                return [];
            }
            const arrVal = d.getRightOp();
            if (!(arrVal instanceof Local)) {
                return [];
            }
            return arrVal.getUsedStmts().filter(u => {
                if (!(u instanceof ArkAssignStmt)) {
                    return false;
                }
                const left = u.getLeftOp();
                return left instanceof ArkArrayRef && left.getBase() === arrVal && left.getIndex() === index;
            });
        });
        return res;
    }

    private isFromParameter(stmt: Stmt): ArkParameterRef | undefined {
        if (!(stmt instanceof ArkAssignStmt)) {
            return undefined;
        }
        const rightOp = stmt.getRightOp();
        if (rightOp instanceof ArkParameterRef) {
            return rightOp;
        }
        return undefined;
    }

    private collectArgDefs(argIdx: number, callsites: Stmt[], scene: Scene): Stmt[] {
        const getKey = (v: Value): Value | FieldSignature => {
            return v instanceof ArkInstanceFieldRef ? v.getFieldSignature() : v;
        };
        return callsites.flatMap(callsite => {
            const target: Value | FieldSignature = getKey(callsite.getInvokeExpr()!.getArg(argIdx));
            return Array.from(DVFGHelper.getOrNewDVFGNode(callsite, scene).getIncomingEdge())
                .map(e => (e.getSrcNode() as DVFGNode).getStmt())
                .filter(s => {
                    return s instanceof ArkAssignStmt && target === getKey(s.getLeftOp());
                });
        });
    }

    private addIssueReport(stmt: Stmt, operand: Value, checkAll: boolean = true): void {
        const severity = this.rule.alert ?? this.metaData.severity;
        const warnInfo = this.getLineAndColumn(stmt, operand);
        const problem = 'ObjectLiteral';
        let desc = `${this.metaData.description} (${this.rule.ruleId.replace('@migration/', '')})`;
        if (!checkAll) {
            desc = `Can not check when function call chain depth exceeds ${CALL_DEPTH_LIMIT}, please check it manually (${this.rule.ruleId.replace(
                '@migration/',
                ''
            )})`;
        }
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
