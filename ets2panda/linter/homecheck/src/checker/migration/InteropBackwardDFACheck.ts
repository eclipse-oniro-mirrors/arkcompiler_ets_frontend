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
    ArkInstanceInvokeExpr,
    AnyType,
    ClassType,
    ArkStaticInvokeExpr,
    AbstractInvokeExpr,
    FunctionType,
    UnknownType,
    Local,
    ArkClass,
    ArkInvokeStmt,
    ArkReturnStmt,
    ArkThrowStmt,
    PrimitiveType,
} from 'arkanalyzer/lib';
import Logger, { LOG_MODULE_TYPE } from 'arkanalyzer/lib/utils/logger';
import { BaseChecker, BaseMetaData } from '../BaseChecker';
import { Rule, Defects, MatcherCallback } from '../../Index';
import { IssueReport } from '../../model/Defects';
import { DVFGNode } from 'arkanalyzer/lib/VFG/DVFG';
import { CALL_DEPTH_LIMIT, GlobalCallGraphHelper, DVFGHelper } from './Utils';
import { findInteropRule } from './InteropRuleInfo';
import { ArkFile, Language } from 'arkanalyzer/lib/core/model/ArkFile';

const logger = Logger.getLogger(LOG_MODULE_TYPE.HOMECHECK, 'InteropBackwardDFACheck');
const gMetaData: BaseMetaData = {
    severity: 1,
    ruleDocPath: '',
    description: '',
};

const REFLECT_API: Map<string, number> = new Map([
    ['apply', 0],
    ['construct', 0],
    ['defineProperty', 0],
    ['deleteProperty', 0],
    ['get', 0],
    ['getOwnPropertyDescriptor', 0],
    ['getPrototypeOf', 0],
    ['has', 0],
    ['isExtensible', 0],
    ['ownKeys', 0],
    ['preventExtensions', 0],
    ['set', 0],
    ['setPrototypeOf', 0],
]);

const OBJECT_API: Map<string, number> = new Map([
    ['getOwnPropertyDescriptor', 0],
    ['getOwnPropertyDescriptors', 0],
    ['getOwnPropertyNames', 0],
    ['hasOwn', 0],
    ['isExtensible', 0],
    ['isFrozen', 0],
    ['isSealed', 0],
    ['keys', 0],
    ['setPrototypeOf', 0],
    ['values', 0],
    ['assign', 1],
    ['entries', 0],
]);

class ObjDefInfo {
    problemStmt: Stmt;
    objLanguage: Language;
}

export class InteropBackwardDFACheck implements BaseChecker {
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
            const importVarMap: Map<string, Language> = new Map();
            this.collectImportedVar(importVarMap, arkFile, scene);
            const topLevelVarMap: Map<string, Stmt[]> = new Map();
            this.collectTopLevelVar(topLevelVarMap, arkFile, scene);

            const handleClass = (cls: ArkClass): void => {
                cls.getMethods().forEach(m => this.processArkMethod(m, scene, importVarMap, topLevelVarMap));
            };

            arkFile.getClasses().forEach(cls => handleClass(cls));
            arkFile.getAllNamespacesUnderThisFile().forEach(n => n.getClasses().forEach(cls => handleClass(cls)));
        }
    };

    private collectImportedVar(importVarMap: Map<string, Language>, file: ArkFile, scene: Scene): void {
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
            const definedLang = this.getTypeDefinedLang(arkExport.getType(), scene) ?? file.getLanguage();
            importVarMap.set(arkExport.getName(), definedLang);
        });
    }

    private collectTopLevelVar(topLevelVarMap: Map<string, Stmt[]>, file: ArkFile, scene: Scene): void {
        const defaultMethod = file.getDefaultClass().getDefaultArkMethod();
        if (defaultMethod) {
            DVFGHelper.buildSingleDVFG(defaultMethod, scene);
            const stmts = defaultMethod.getBody()?.getCfg().getStmts() ?? [];
            for (const stmt of stmts) {
                if (!(stmt instanceof ArkAssignStmt)) {
                    continue;
                }
                const leftOp = stmt.getLeftOp();
                if (!(leftOp instanceof Local)) {
                    continue;
                }
                const name = leftOp.getName();
                if (name.startsWith('%') || name === 'this') {
                    continue;
                }
                topLevelVarMap.set(name, [...(topLevelVarMap.get(name) ?? []), stmt]);
            }
        }
    }

    private processArkMethod(
        target: ArkMethod,
        scene: Scene,
        importVarMap: Map<string, Language>,
        topLevelVarMap: Map<string, Stmt[]>
    ): void {
        const currentLang = target.getLanguage();
        if (currentLang === Language.UNKNOWN) {
            logger.warn(`cannot find the language for method: ${target.getSignature()}`);
            return;
        }
        const stmts = target.getBody()?.getCfg().getStmts() ?? [];
        for (const stmt of stmts) {
            const invoke = stmt.getInvokeExpr();
            let isReflect = false;
            let paramIdx = -1;
            if (invoke && invoke instanceof ArkStaticInvokeExpr) {
                const classSig = invoke.getMethodSignature().getDeclaringClassSignature();
                if (
                    classSig.getDeclaringFileSignature().getProjectName() === 'built-in' &&
                    classSig.getDeclaringNamespaceSignature()?.getNamespaceName() === 'Reflect'
                ) {
                    isReflect = true;
                    paramIdx = REFLECT_API.get(invoke.getMethodSignature().getMethodSubSignature().getMethodName()) ?? -1;
                }
            }
            if (invoke && invoke instanceof ArkStaticInvokeExpr) {
                const methodSig = invoke.getMethodSignature();
                const classSig = methodSig.getDeclaringClassSignature();
                if (classSig.getClassName() === 'ObjectConstructor' || classSig.getClassName() === 'Object') {
                    paramIdx =
                        OBJECT_API.get(invoke.getMethodSignature().getMethodSubSignature().getMethodName()) ?? -1;
                }
            }
            if (paramIdx === -1) {
                continue;
            }
            DVFGHelper.buildSingleDVFG(target, scene);

            const argDefs = this.findArgumentDef(stmt, paramIdx, currentLang, importVarMap, topLevelVarMap, scene);
            if (this.isLanguage(argDefs)) {
                this.reportIssue({ problemStmt: stmt, objLanguage: argDefs as Language }, currentLang, isReflect);
            } else {
                argDefs.forEach(def => {
                    let result: ObjDefInfo[] = [];
                    let visited: Set<Stmt> = new Set();
                    this.checkFromStmt(def, currentLang, result, visited, importVarMap, topLevelVarMap, scene);
                    result.forEach(objDefInfo => {
                        this.reportIssue(objDefInfo, currentLang, isReflect);
                    });
                });
            }
        }
    }

    private reportIssue(objDefInfo: ObjDefInfo, apiLang: Language, isReflect: boolean): void {
        const problemStmt = objDefInfo.problemStmt;
        const problemStmtMtd = problemStmt.getCfg().getDeclaringMethod();
        const problemStmtLang = problemStmtMtd?.getLanguage();
        const objLanguage = objDefInfo.objLanguage;
        if (objLanguage === Language.UNKNOWN || problemStmtLang === Language.UNKNOWN) {
            logger.warn(`cannot find the language for def: ${problemStmt.toString()}`);
            return;
        }
        const interopRule = findInteropRule(apiLang, objLanguage, problemStmtLang, isReflect);
        if (!interopRule) {
            return;
        }
        const line = problemStmt.getOriginPositionInfo().getLineNo();
        const column = problemStmt.getOriginPositionInfo().getColNo();
        const problem = 'Interop';
        const desc = `${interopRule.description} (${interopRule.ruleId})`;
        const severity = interopRule.severity;
        const ruleId = this.rule.ruleId;
        const filePath = problemStmtMtd?.getDeclaringArkFile()?.getFilePath() ?? '';
        const defeats = new Defects(
            line,
            column,
            column,
            problem,
            desc,
            severity,
            ruleId,
            filePath,
            '',
            true,
            false,
            false
        );
        this.issues.push(new IssueReport(defeats, undefined));
    }

    private checkFromStmt(
        stmt: Stmt,
        apiLanguage: Language,
        res: ObjDefInfo[],
        visited: Set<Stmt>,
        importVarMap: Map<string, Language>,
        topLevelVarMap: Map<string, Stmt[]>,
        scene: Scene,
        depth: number = 0
    ): void {
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
                const rightOp = currentStmt.getRightOp();
                if (rightOp instanceof ArkInstanceFieldRef) {
                    // 处理 Reflect.apply(obj.getName, {a : 12})
                    const base = rightOp.getBase();
                    if (base instanceof Local && base.getDeclaringStmt()) {
                        worklist.push(DVFGHelper.getOrNewDVFGNode(base.getDeclaringStmt()!, scene));
                        continue;
                    }
                }
                if (rightOp instanceof Local && !rightOp.getDeclaringStmt()) {
                    const name = rightOp.getName();
                    if (importVarMap.has(name)) {
                        res.push({ problemStmt: currentStmt, objLanguage: importVarMap.get(name)! });
                        continue;
                    }
                }
                const rightOpTy = rightOp.getType();
                if (!this.isIrrelevantType(rightOpTy) && !(rightOpTy instanceof PrimitiveType)) {
                    const rightOpTyLang = this.getTypeDefinedLang(rightOpTy, scene) ?? currentStmt.getCfg().getDeclaringMethod().getLanguage();
                    if (rightOpTyLang && rightOpTyLang !== apiLanguage) {
                        res.push({ problemStmt: currentStmt, objLanguage: rightOpTyLang });
                        continue;
                    }
                }
            }
            const callsite = this.cg.getCallSiteByStmt(currentStmt);
            if (callsite.length > 0) {
                callsite.forEach(cs => {
                    const declaringMtd = this.cg.getArkMethodByFuncID(cs.calleeFuncID);
                    if (!declaringMtd || !declaringMtd.getCfg()) {
                        return;
                    }
                    DVFGHelper.buildSingleDVFG(declaringMtd, scene);
                    declaringMtd
                        .getReturnStmt()
                        .forEach(r =>
                            this.checkFromStmt(
                                r,
                                apiLanguage,
                                res,
                                visited,
                                importVarMap,
                                topLevelVarMap,
                                scene,
                                depth + 1
                            )
                        );
                });
                continue;
            }
            const paramRef = this.isFromParameter(currentStmt);
            if (paramRef) {
                const paramIdx = paramRef.getIndex();
                this.cg.getInvokeStmtByMethod(currentStmt.getCfg().getDeclaringMethod().getSignature()).forEach(cs => {
                    const declaringMtd = cs.getCfg().getDeclaringMethod();
                    DVFGHelper.buildSingleDVFG(declaringMtd, scene);
                    const argDefs = this.findArgumentDef(cs, paramIdx, apiLanguage, importVarMap, topLevelVarMap, scene);
                    if (this.isLanguage(argDefs)) {
                        // imported var
                        res.push({ problemStmt: cs, objLanguage: argDefs as Language });
                    } else {
                        argDefs.forEach(d => {
                            this.checkFromStmt(d, apiLanguage, res, visited, importVarMap, topLevelVarMap, scene, depth + 1);
                        });
                    }
                });
                continue;
            }
            if (stmt instanceof ArkAssignStmt) {
                const rightOp = stmt.getRightOp();
                if (rightOp instanceof Local && !rightOp.getDeclaringStmt()) {
                    (topLevelVarMap.get(rightOp.getName()) ?? []).forEach(def => {
                        worklist.push(DVFGHelper.getOrNewDVFGNode(def, scene));
                    });
                } else {
                    current.getIncomingEdge().forEach(e => {
                        const srcNode = e.getSrcNode() as DVFGNode;
                        const srcStmt = srcNode.getStmt();
                        if (srcStmt.getDef() === stmt.getRightOp()) {
                            worklist.push(srcNode);
                        }
                    })
                }
            } else if ((stmt instanceof ArkReturnStmt) || (stmt instanceof ArkThrowStmt)) {
                 current.getIncomingEdge().forEach(e => worklist.push(e.getSrcNode() as DVFGNode));
            }
        }
    }

    private isIrrelevantType(ty: Type): boolean {
        const isObjectTy = (ty: Type): boolean => {
            return ty instanceof ClassType && ty.getClassSignature().getClassName() === 'Object';
        };
        const isESObjectTy = (ty: Type): boolean => {
            return ty.toString() === 'ESObject';
        };
        const isAnyTy = (ty: Type): ty is AnyType => {
            return ty instanceof AnyType;
        };
        const isUnkwonTy = (ty: Type): ty is UnknownType => {
            return ty instanceof UnknownType;
        };
        return isObjectTy(ty) || isESObjectTy(ty) || isAnyTy(ty) || isUnkwonTy(ty);
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

    private isLanguage(value: Stmt[] | Language): value is Language {
        return Object.values(Language).includes(value as Language);
    }

    private findArgumentDef(
        stmt: Stmt,
        argIdx: number,
        apiLanguage: Language,
        importVarMap: Map<string, Language>,
        topLevelVarMap: Map<string, Stmt[]>,
        scene: Scene
    ): Stmt[] | Language {
        const invoke = stmt.getInvokeExpr();
        const getKey = (v: Value): Value | FieldSignature => {
            return v instanceof ArkInstanceFieldRef ? v.getFieldSignature() : v;
        };
        const arg: Value | FieldSignature = getKey((invoke as AbstractInvokeExpr).getArg(argIdx));
        if (!arg) {
            logger.error(`arg${argIdx} of invoke ${stmt.toString()} is undefined`);
            return [];
        }
        if (arg instanceof Local && arg.getDeclaringStmt() instanceof ArkAssignStmt) {
            // 特殊处理，obj.getName 的类型有 bug
            const rightOp = (arg.getDeclaringStmt() as ArkAssignStmt).getRightOp();
            if (rightOp instanceof ArkInstanceFieldRef) {
                const base = rightOp.getBase();
                if (base instanceof Local && base.getDeclaringStmt()) {
                    return [base.getDeclaringStmt()!];
                }
            }
        }
        const argTy = arg.getType();
        if (!this.isIrrelevantType(argTy) && !(argTy instanceof PrimitiveType)) {
            const argTyLang = this.getTypeDefinedLang(argTy, scene) ?? stmt.getCfg().getDeclaringMethod().getLanguage();
            if (argTyLang && argTyLang !== apiLanguage) {
                return argTyLang;
            }
        }
        if (arg instanceof Local && !arg.getDeclaringStmt()) {
            const name = arg.getName();
            return topLevelVarMap.get(name) ?? importVarMap.get(name) ?? [];
        }
        return Array.from(DVFGHelper.getOrNewDVFGNode(stmt, scene).getIncomingEdge())
            .map(e => (e.getSrcNode() as DVFGNode).getStmt())
            .filter(s => {
                return s instanceof ArkAssignStmt && arg === getKey(s.getLeftOp());
            });
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
