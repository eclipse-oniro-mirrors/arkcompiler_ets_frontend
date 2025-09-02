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
    Type,
    ArkMethod,
    ArkAssignStmt,
    FieldSignature,
    Stmt,
    Scene,
    Value,
    CallGraph,
    ArkParameterRef,
    ArkInstanceFieldRef,
    FunctionType,
    ClassType,
    ArkNamespace,
    PrimitiveType,
    UnclearReferenceType,
    AnyType,
    UnionType,
    NullType,
    UndefinedType,
} from 'arkanalyzer/lib';
import Logger, { LOG_MODULE_TYPE } from 'arkanalyzer/lib/utils/logger';
import { BaseChecker, BaseMetaData } from '../BaseChecker';
import { Rule, Defects, MatcherCallback } from '../../Index';
import { IssueReport } from '../../model/Defects';
import { DVFGNode } from 'arkanalyzer/lib/VFG/DVFG';
import { CALL_DEPTH_LIMIT, DVFGHelper, GlobalCallGraphHelper } from './Utils';
import { Language } from 'arkanalyzer/lib/core/model/ArkFile';

const logger = Logger.getLogger(LOG_MODULE_TYPE.HOMECHECK, 'InteropAssignCheck');
const gMetaData: BaseMetaData = {
    severity: 1,
    ruleDocPath: '',
    description: 'Should not pass or assign a dynamic object to a variable of static type',
};

const RULE_ID = 'arkts-interop-s2d-dynamic-args-to-static';
const BOXED_SET: Set<string> = new Set<string>(['String', 'Boolean', 'BigInt', 'Number']);
export class InteropAssignCheck implements BaseChecker {
    readonly metaData: BaseMetaData = gMetaData;
    public rule: Rule;
    public defects: Defects[] = [];
    public issues: IssueReport[] = [];
    private cg: CallGraph;

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
            for (let clazz of arkFile.getClasses()) {
                for (let mtd of clazz.getMethods()) {
                    this.processArkMethod(mtd, scene);
                }
            }
            for (let namespace of arkFile.getAllNamespacesUnderThisFile()) {
                this.processNameSpace(namespace, scene);
            }
        }
    };

    public processNameSpace(namespace: ArkNamespace, scene: Scene): void {
        for (let clazz of namespace.getClasses()) {
            for (let mtd of clazz.getMethods()) {
                this.processArkMethod(mtd, scene);
            }
        }
    }

    public processArkMethod(target: ArkMethod, scene: Scene): void {
        if (target.getLanguage() === Language.ARKTS1_2) {
            this.checkPassToFunction(target, scene);
        } else if (target.getLanguage() === Language.ARKTS1_1) {
            this.checkAssignToField(target, scene);
        }
    }

    private checkPassToFunction(target: ArkMethod, scene: Scene): void {
        const callsites = this.cg.getInvokeStmtByMethod(target.getSignature());
        callsites
            .filter(cs => cs.getCfg().getDeclaringMethod().getLanguage() === Language.ARKTS1_1)
            .forEach(cs => {
                let hasTargetArg = false;
                const invoke = cs.getInvokeExpr()!;
                const csMethod = cs.getCfg().getDeclaringMethod();
                invoke.getArgs().forEach((arg, argIdx) => {
                    const argTy = arg.getType();
                    if (argTy instanceof PrimitiveType || this.isBoxedType(argTy)) {
                        return;
                    }
                    const paramTy = invoke.getMethodSignature().getMethodSubSignature().getParameterTypes()[argIdx];
                    if (this.isAnyType(paramTy) || this.isESObjectType(paramTy)) {
                        return;
                    }
                    const argTyLang = this.getTypeDefinedLang(argTy, scene) ?? csMethod?.getLanguage() ?? Language.UNKNOWN;
                    if (argTyLang === Language.ARKTS1_1) {
                        hasTargetArg = true;
                    }
                });
                if (!hasTargetArg) {
                    return;
                }
                let line = cs.getOriginPositionInfo().getLineNo();
                let column = cs.getOriginPositionInfo().getColNo();
                const problem = 'Interop';
                const desc = `${this.metaData.description} (${RULE_ID})`;
                const severity = this.metaData.severity;
                const ruleId = this.rule.ruleId;
                const filePath = csMethod?.getDeclaringArkFile()?.getFilePath() ?? '';
                const defeats = new Defects(line, column, column, problem, desc, severity, ruleId, filePath, '', true, false, false);
                this.issues.push(new IssueReport(defeats, undefined));
            });
    }

    private isAnyType(ty: Type): boolean {
        return ty instanceof AnyType || (ty instanceof UnclearReferenceType && ty.getName() === 'Any');
    }

    private isESObjectType(ty: Type): boolean {
        if (!(ty instanceof UnionType)) {
            return false;
        }
        const types = ty.getTypes();
        if (types.length !== 3) {
            return false;
        }
        const isObjectTy = (type: Type): boolean => {
            return type instanceof ClassType && type.getClassSignature().getClassName() === 'Object';
        };
        return (
            types.find(ty => ty instanceof NullType) !== undefined &&
            types.find(ty => ty instanceof UndefinedType) !== undefined &&
            types.find(ty => isObjectTy(ty)) !== undefined
        );
    }

    private isBoxedType(checkType: Type): boolean {
        const unclear = checkType instanceof UnclearReferenceType && BOXED_SET.has(checkType.getName());
        const cls = checkType instanceof ClassType && BOXED_SET.has(checkType.getClassSignature().getClassName());
        return unclear || cls;
    }

    private checkAssignToField(target: ArkMethod, scene: Scene): void {
        const assigns: Stmt[] = this.collectAssignToObjectField(target, scene);
        if (assigns.length > 0) {
            DVFGHelper.buildSingleDVFG(target, scene);
        }
        assigns.forEach(assign => {
            let result: Stmt[] = [];
            let visited: Set<Stmt> = new Set();
            this.checkFromStmt(assign, scene, result, visited);
            if (result.length === 0) {
                // 这句 a.data = y 右侧的值没有从 1.1 传入的可能
                return;
            }
            let line = assign.getOriginPositionInfo().getLineNo();
            let column = assign.getOriginPositionInfo().getColNo();
            const problem = 'Interop';
            const desc = `${this.metaData.description} (${RULE_ID})`;
            const severity = this.metaData.severity;
            const ruleId = this.rule.ruleId;
            const filePath = assign.getCfg().getDeclaringMethod().getDeclaringArkFile()?.getFilePath() ?? '';
            const defeats = new Defects(line, column, column, problem, desc, severity, ruleId, filePath, '', true, false, false);
            this.issues.push(new IssueReport(defeats, undefined));
        });
    }

    private collectAssignToObjectField(method: ArkMethod, scene: Scene): Stmt[] {
        const res: Stmt[] = [];
        const stmts = method.getBody()?.getCfg().getStmts() ?? [];
        for (const stmt of stmts) {
            if (!(stmt instanceof ArkAssignStmt)) {
                continue;
            }
            const leftOp = stmt.getLeftOp();
            if (!(leftOp instanceof ArkInstanceFieldRef)) {
                continue;
            }
            if (!this.isObjectTy(leftOp.getType())) {
                continue;
            }
            const baseTy = leftOp.getBase().getType();
            if (baseTy instanceof ClassType) {
                const klass = scene.getClass(baseTy.getClassSignature());
                if (!klass) {
                    logger.warn(`check field of type 'Object' failed: cannot find arkclass by sig ${baseTy.getClassSignature().toString()}`);
                } else if (klass.getLanguage() === Language.ARKTS1_2) {
                    res.push(stmt);
                }
            } else {
                logger.warn(`check field of type 'Object' failed: unexpected base type ${baseTy.toString()}`);
            }
        }
        return res;
    }

    private checkFromStmt(stmt: Stmt, scene: Scene, res: Stmt[], visited: Set<Stmt>, depth: number = 0): void {
        if (depth > CALL_DEPTH_LIMIT) {
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
            if (currentStmt instanceof ArkAssignStmt) {
                const rightOpTy = currentStmt.getRightOp().getType();
                if (!this.isObjectTy(rightOpTy) && this.getTypeDefinedLang(rightOpTy, scene) === Language.ARKTS1_1) {
                    res.push(currentStmt);
                    continue;
                }
            }
            const callsite = this.cg.getCallSiteByStmt(currentStmt);
            callsite.forEach(cs => {
                const declaringMtd = this.cg.getArkMethodByFuncID(cs.calleeFuncID);
                if (!declaringMtd || !declaringMtd.getCfg()) {
                    return;
                }
                DVFGHelper.buildSingleDVFG(declaringMtd, scene);
                declaringMtd.getReturnStmt().forEach(r => this.checkFromStmt(r, scene, res, visited, depth + 1));
            });
            const paramRef = this.isFromParameter(currentStmt);
            if (paramRef) {
                const paramIdx = paramRef.getIndex();
                const callsites = this.cg.getInvokeStmtByMethod(currentStmt.getCfg().getDeclaringMethod().getSignature());
                callsites.forEach(cs => {
                    const declaringMtd = cs.getCfg().getDeclaringMethod();
                    DVFGHelper.buildSingleDVFG(declaringMtd, scene);
                });
                this.collectArgDefs(paramIdx, callsites, scene).forEach(d => this.checkFromStmt(d, scene, res, visited, depth + 1));
            }
            current.getIncomingEdge().forEach(e => worklist.push(e.getSrcNode() as DVFGNode));
        }
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

    private isObjectTy(ty: Type): boolean {
        return ty instanceof ClassType && ty.getClassSignature().getClassName() === 'Object';
    }

    private getTypeDefinedLang(type: Type, scene: Scene): Language | undefined {
        let file = undefined;
        if (type instanceof ClassType) {
            file = scene.getFile(type.getClassSignature().getDeclaringFileSignature());
        } else if (type instanceof FunctionType) {
            file = scene.getFile(type.getMethodSignature().getDeclaringClassSignature().getDeclaringFileSignature());
        }
        if (file) {
            return file.getLanguage();
        }
        return undefined;
    }
}
