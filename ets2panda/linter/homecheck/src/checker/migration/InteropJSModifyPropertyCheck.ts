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
    Scene,
    ArkInstanceFieldRef,
    FunctionType,
    ClassType,
    MethodSignature,
    ArkParameterRef,
    Value,
    Stmt,
    ArkDeleteExpr,
    FieldSignature,
    ArkNamespace,
} from 'arkanalyzer/lib';
import Logger, { LOG_MODULE_TYPE } from 'arkanalyzer/lib/utils/logger';
import { BaseChecker, BaseMetaData } from '../BaseChecker';
import { Rule, Defects, MatcherCallback } from '../../Index';
import { IssueReport } from '../../model/Defects';
import { ArkFile, Language } from 'arkanalyzer/lib/core/model/ArkFile';
import { CALL_DEPTH_LIMIT, DVFGHelper } from './Utils';
import { DVFGNode } from 'arkanalyzer/lib/VFG/DVFG';

const logger = Logger.getLogger(LOG_MODULE_TYPE.HOMECHECK, 'InteropJSModifyPropertyCheck');
const gMetaData: BaseMetaData = {
    severity: 1,
    ruleDocPath: '',
    description: 'The layout of staic objects should not be modified',
};

const RULE_ID = 'arkts-interop-js2s-js-add-detele-static-prop';

class ParamInfo {
    flag: boolean;
    paramAssign: ArkAssignStmt;
    callsites: Set<Stmt>;
}

export class InteropJSModifyPropertyCheck implements BaseChecker {
    readonly metaData: BaseMetaData = gMetaData;
    public rule: Rule;
    public defects: Defects[] = [];
    public issues: IssueReport[] = [];

    public registerMatchers(): MatcherCallback[] {
        const matchBuildCb: MatcherCallback = {
            matcher: undefined,
            callback: this.check,
        };
        return [matchBuildCb];
    }

    public check = (scene: Scene): void => {
        const targetMethods: Map<MethodSignature, ParamInfo[]> = new Map();
        this.collectTargetMethods(targetMethods, scene);
        this.checkTargetMethods(targetMethods, scene);
    };

    private collectTargetMethods(targetMethods: Map<MethodSignature, ParamInfo[]>, scene: Scene): void {
        scene.getFiles().forEach(file => {
            // 只处理 ArkTS1.2 import JS 函数的情况
            if (file.getLanguage() !== Language.ARKTS1_2) {
                return;
            }
            file.getImportInfos().forEach(importInfo => {
                const exportInfo = importInfo.getLazyExportInfo();
                if (exportInfo === null) {
                    return;
                }
                const arkExport = exportInfo.getArkExport();
                if (arkExport === null || arkExport === undefined) {
                    return;
                }
                if (!(arkExport instanceof ArkMethod && arkExport.getLanguage() === Language.JAVASCRIPT)) {
                    return;
                }
                // 创建初始化的参数标志位信息（标志位代表该参数是否被传入了 1.2 对象，默认为 false）
                const paramInfo = this.createParamInfo(arkExport);
                if (paramInfo.length > 0) {
                    targetMethods.set(arkExport.getSignature(), paramInfo);
                }
            });

            // 寻找 JS 函数的被调用点，检查是否传入 ArkTS1.2 类型的对象并维护参数标志位信息
            for (let clazz of file.getClasses()) {
                for (let mtd of clazz.getMethods()) {
                    this.findCallsite(mtd, targetMethods, scene);
                }
            }
            for (let namespace of file.getAllNamespacesUnderThisFile()) {
                this.processNameSpace(namespace, targetMethods, scene);
            }
        });

        // 跨函数检查 ArkTS1.2 类型对象是否被跨函数传到其他 JS 函数
        for (let i = 0; i < CALL_DEPTH_LIMIT; ++i) {
            this.collectInterprocedualCallSites(targetMethods, scene);
        }
    }

    public processNameSpace(
        namespace: ArkNamespace,
        targetMethods: Map<MethodSignature, ParamInfo[]>,
        scene: Scene
    ): void {
        for (let clazz of namespace.getClasses()) {
            for (let mtd of clazz.getMethods()) {
                this.findCallsite(mtd, targetMethods, scene);
            }
        }
    }

    private createParamInfo(method: ArkMethod): ParamInfo[] {
        // 初始化参数标志数组
        const idxFlag = new Array(method.getParameters().length).fill(false);
        // 寻找参数对应的 xxx = parameter 语句
        const paramAssigns = (method.getCfg()?.getStmts() ?? [])
            .filter(s => s instanceof ArkAssignStmt && s.getRightOp() instanceof ArkParameterRef)
            .map(s => s as ArkAssignStmt);
        if (idxFlag.length !== paramAssigns.length) {
            logger.error('param index num != param assign num');
            return [];
        }
        const result: ParamInfo[] = [];
        for (let i = 0; i < idxFlag.length; i++) {
            result.push({ flag: idxFlag[i], paramAssign: paramAssigns[i], callsites: new Set() });
        }
        return result;
    }

    private findCallsite(method: ArkMethod, targets: Map<MethodSignature, ParamInfo[]>, scene: Scene): void {
        const stmts = method.getBody()?.getCfg().getStmts() ?? [];
        for (const stmt of stmts) {
            const invoke = stmt.getInvokeExpr();
            if (!invoke) {
                continue;
            }
            const methodSig = invoke.getMethodSignature();
            if (!targets.has(methodSig)) {
                continue;
            }
            invoke.getArgs().forEach((arg, idx): void => {
                if (this.getTypeDefinedLang(arg.getType(), method.getDeclaringArkFile(), scene) !== Language.ARKTS1_2) {
                    return;
                }
                targets.get(methodSig)![idx].flag = true;
                targets.get(methodSig)![idx].callsites.add(stmt);
            });
        }
    }

    private collectInterprocedualCallSites(targets: Map<MethodSignature, ParamInfo[]>, scene: Scene): void {
        new Map(targets).forEach((paramInfo, sig) => {
            const method = scene.getMethod(sig)!;
            DVFGHelper.buildSingleDVFG(method, scene);
            paramInfo.forEach((paramInfo): void => {
                if (paramInfo.flag) {
                    this.checkIfParamPassedToOtherMethod(paramInfo, targets, scene);
                }
            });
        });
    }

    private checkIfParamPassedToOtherMethod(
        callerParamInfo: ParamInfo,
        targets: Map<MethodSignature, ParamInfo[]>,
        scene: Scene
    ): void {
        const worklist: DVFGNode[] = [DVFGHelper.getOrNewDVFGNode(callerParamInfo.paramAssign, scene)];
        const visited: Set<Stmt> = new Set();
        while (worklist.length > 0) {
            const current = worklist.shift()!;
            const cunrrentStmt = current.getStmt();
            if (visited.has(cunrrentStmt)) {
                continue;
            }
            visited.add(cunrrentStmt);
            if (!(cunrrentStmt instanceof ArkAssignStmt)) {
                continue;
            }
            current.getOutgoingEdges().forEach(edge => {
                const dst = edge.getDstNode() as DVFGNode;
                const dstStmt = dst.getStmt();
                // 假设有 JS 函数声明： function foo(obj)，其中 obj 为受关注的参数
                // 只有类似 let obj2 = obj 或者 goo(obj) 这样的语句受到关注
                if (dstStmt instanceof ArkAssignStmt && dstStmt.getRightOp() === cunrrentStmt.getLeftOp()) {
                    worklist.push(dst);
                    return;
                }
                const invoke = dstStmt.getInvokeExpr();
                if (!invoke) {
                    return;
                }
                const calleeSig = invoke.getMethodSignature();
                const callee = scene.getMethod(calleeSig);
                if (!callee || !callee.getCfg() || callee.getLanguage() !== Language.JAVASCRIPT) {
                    return;
                }
                if (!targets.has(calleeSig)) {
                    targets.set(calleeSig, this.createParamInfo(callee));
                }
                const getKey = (v: Value): Value | FieldSignature => {
                    return v instanceof ArkInstanceFieldRef ? v.getFieldSignature() : v;
                };
                invoke.getArgs().forEach((arg, idx) => {
                    if (getKey(arg) === getKey(cunrrentStmt.getLeftOp())) {
                        targets.get(calleeSig)![idx].flag = true;
                        callerParamInfo.callsites.forEach(cs => {
                            targets.get(calleeSig)![idx].callsites.add(cs);
                        });
                    }
                });
            });
        }
    }

    private checkTargetMethods(targetMethods: Map<MethodSignature, ParamInfo[]>, scene: Scene): void {
        targetMethods.forEach((paramInfos, methodSig): void => {
            const method = scene.getMethod(methodSig);
            if (!method) {
                logger.error(`cannot find ark method by method sig: ${methodSig.toString()}`);
                return;
            }
            const targetParams = paramInfos
                .filter(paramInfo => paramInfo.flag)
                .map(paramInfo => paramInfo.paramAssign.getLeftOp());
            const stmts = method.getBody()?.getCfg().getStmts() ?? [];
            for (const stmt of stmts) {
                if (!(stmt instanceof ArkAssignStmt)) {
                    continue;
                }
                let paramIdx = -1;
                const rightOp = stmt.getRightOp();
                if (rightOp instanceof ArkDeleteExpr) {
                    const fieldRef = rightOp.getField();
                    if (fieldRef instanceof ArkInstanceFieldRef) {
                        paramIdx = targetParams.findIndex(p => p === fieldRef.getBase());
                    }
                }
                const leftOp = stmt.getLeftOp();
                if (leftOp instanceof ArkInstanceFieldRef) {
                    paramIdx = targetParams.findIndex(p => p === leftOp.getBase());
                }
                if (paramIdx !== -1) {
                    const callers = paramInfos[paramIdx]!;
                    callers.callsites.forEach(cs => this.reportIssue(cs));
                }
            }
        });
    }

    private getTypeDefinedLang(type: Type, defaultFile: ArkFile, scene: Scene): Language {
        let file;
        if (type instanceof ClassType) {
            file = scene.getFile(type.getClassSignature().getDeclaringFileSignature());
        } else if (type instanceof FunctionType) {
            file = scene.getFile(type.getMethodSignature().getDeclaringClassSignature().getDeclaringFileSignature());
        } else {
            file = defaultFile;
        }
        if (file) {
            return file.getLanguage();
        }
        return Language.UNKNOWN;
    }

    private reportIssue(problemStmt: Stmt): void {
        const line = problemStmt.getOriginPositionInfo().getLineNo();
        const column = problemStmt.getOriginPositionInfo().getColNo();
        const problem = 'Interop';
        const desc = `${this.metaData.description} (${RULE_ID})`;
        const severity = this.metaData.severity;
        const ruleId = this.rule.ruleId;
        const filePath = problemStmt.getCfg().getDeclaringMethod().getDeclaringArkFile()?.getFilePath() ?? '';
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
}
