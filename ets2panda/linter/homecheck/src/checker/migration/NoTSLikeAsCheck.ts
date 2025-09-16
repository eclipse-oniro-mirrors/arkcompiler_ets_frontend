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

import path from 'path';
import {
    AbstractFieldRef,
    ArkAssignStmt,
    ArkCastExpr,
    ArkField,
    ArkIfStmt,
    ArkInstanceFieldRef,
    ArkInstanceOfExpr,
    ArkMethod,
    ArkNamespace,
    ArkNormalBinopExpr,
    ArkParameterRef,
    ArkUnopExpr,
    BasicBlock,
    CallGraph,
    Cfg,
    ClassSignature,
    classSignatureCompare,
    ClassType,
    DVFGBuilder,
    FieldSignature,
    fileSignatureCompare,
    LineColPosition,
    Local,
    NormalBinaryOperator,
    RelationalBinaryOperator,
    Scene,
    Stmt,
    UnaryOperator,
    Value,
} from 'arkanalyzer/lib';
import Logger, { LOG_MODULE_TYPE } from 'arkanalyzer/lib/utils/logger';
import { BaseChecker, BaseMetaData } from '../BaseChecker';
import { Defects, MatcherCallback, Rule } from '../../Index';
import { IssueReport } from '../../model/Defects';
import { DVFG, DVFGNode } from 'arkanalyzer/lib/VFG/DVFG';
import { CALL_DEPTH_LIMIT, getGlobalsDefineInDefaultMethod, getLineAndColumn, GlobalCallGraphHelper } from './Utils';
import { ClassCategory } from 'arkanalyzer/lib/core/model/ArkClass';
import { Language } from 'arkanalyzer/lib/core/model/ArkFile';
import { BooleanConstant, NumberConstant } from 'arkanalyzer/lib/core/base/Constant';
import { ArkClass, NumberType } from 'arkanalyzer';

const logger = Logger.getLogger(LOG_MODULE_TYPE.HOMECHECK, 'NoTSLikeAsCheck');
const gMetaData: BaseMetaData = {
    severity: 1,
    ruleDocPath: '',
    description: '',
};

enum TypeAssuranceCondition {
    Positive,
    Negative,
    NotExist,
}

export class NoTSLikeAsCheck implements BaseChecker {
    readonly metaData: BaseMetaData = gMetaData;
    readonly checkedBinaryOperator: string[] = ['+', '-', '*', '/', '%', '**'];
    public rule: Rule;
    public defects: Defects[] = [];
    public issues: IssueReport[] = [];
    private scene: Scene;
    private cg: CallGraph;
    private dvfg: DVFG;
    private dvfgBuilder: DVFGBuilder;
    private visited: Set<ArkMethod> = new Set();

    public registerMatchers(): MatcherCallback[] {
        const matchBuildCb: MatcherCallback = {
            matcher: undefined,
            callback: this.check,
        };
        return [matchBuildCb];
    }

    public check = (scene: Scene): void => {
        this.scene = scene;
        this.cg = GlobalCallGraphHelper.getCGInstance(scene);

        this.dvfg = new DVFG(this.cg);
        this.dvfgBuilder = new DVFGBuilder(this.dvfg, scene);

        for (let arkFile of scene.getFiles()) {
            // 此规则仅对arkts1.1和arkts1.2进行检查，typescript在编译阶段会报错，javascript没有类型断言语法
            if (!(arkFile.getLanguage() === Language.ARKTS1_1 || arkFile.getLanguage() === Language.ARKTS1_2)) {
                continue;
            }
            const defaultMethod = arkFile.getDefaultClass().getDefaultArkMethod();
            let globalVarMap: Map<string, Stmt[]> = new Map();
            if (defaultMethod) {
                this.dvfgBuilder.buildForSingleMethod(defaultMethod);
                globalVarMap = getGlobalsDefineInDefaultMethod(defaultMethod);
            }
            for (let clazz of arkFile.getClasses()) {
                this.processClass(clazz, globalVarMap);
            }
            for (let namespace of arkFile.getAllNamespacesUnderThisFile()) {
                this.processNameSpace(namespace, globalVarMap);
            }
        }
    };

    public processClass(arkClass: ArkClass, globalVarMap: Map<string, Stmt[]>): void {
        for (let field of arkClass.getFields()) {
            this.processClassField(field, globalVarMap);
        }
        for (let mtd of arkClass.getMethods()) {
            this.processArkMethod(mtd, globalVarMap);
        }
    }

    public processNameSpace(namespace: ArkNamespace, globalVarMap: Map<string, Stmt[]>): void {
        for (let ns of namespace.getNamespaces()) {
            this.processNameSpace(ns, globalVarMap);
        }
        for (let clazz of namespace.getClasses()) {
            this.processClass(clazz, globalVarMap);
        }
    }

    public processClassField(field: ArkField, globalVarMap: Map<string, Stmt[]>): void {
        const instInit = field.getDeclaringArkClass().getInstanceInitMethod();
        this.processArkMethod(instInit, globalVarMap);
    }

    public processArkMethod(target: ArkMethod, globalVarMap: Map<string, Stmt[]>): void {
        const stmts = target.getBody()?.getCfg().getStmts() ?? [];
        for (const stmt of stmts) {
            // cast表达式所在语句为sink点，从该点开始进行逆向数据流分析
            const castExpr = this.getCastExpr(stmt);
            if (castExpr === null) {
                continue;
            }

            // 判断是否为cast表达式的算数运算，属于告警场景之一
            if (this.isCastExprWithNumericOperation(stmt)) {
                this.addIssueReport(stmt, castExpr, undefined, true);
                continue;
            }

            // 判断cast类型断言的类型是否是class，非class的场景不在本规则检查范围内
            if (!(castExpr.getType() instanceof ClassType)) {
                continue;
            }
            if (this.hasCheckedWithInstanceof(stmt.getCfg(), stmt)) {
                continue;
            }
            if (!this.visited.has(target)) {
                this.dvfgBuilder.buildForSingleMethod(target);
                this.visited.add(target);
            }

            let checkAll = { value: true };
            let visited: Set<Stmt> = new Set();
            const result = this.checkFromStmt(stmt, globalVarMap, checkAll, visited);
            if (result !== null) {
                this.addIssueReport(stmt, castExpr, result);
            } else {
                if (!checkAll.value) {
                    this.addIssueReport(stmt, castExpr);
                }
            }
        }
    }

    private isCastExprWithNumericOperation(stmt: Stmt): boolean {
        if (this.isCastExprWithIncrementDecrement(stmt)) {
            return true;
        }
        if (!(stmt instanceof ArkAssignStmt)) {
            return false;
        }
        const leftOp = stmt.getLeftOp();
        if (!(leftOp instanceof ArkCastExpr)) {
            return false;
        }
        const rightOp = stmt.getRightOp();
        if (!(rightOp instanceof ArkNormalBinopExpr)) {
            return false;
        }
        const op1 = rightOp.getOp1();
        if (leftOp !== op1) {
            return false;
        }
        const operator = rightOp.getOperator();
        return this.checkedBinaryOperator.includes(operator);
    }

    private isCastExprWithIncrementDecrement(stmt: Stmt): boolean {
        if (!(stmt instanceof ArkAssignStmt) || !(stmt.getRightOp() instanceof ArkCastExpr)) {
            return false;
        }
        const castLocal = stmt.getLeftOp();
        if (!(castLocal instanceof Local)) {
            return false;
        }
        // 判断是否为自增或自减语句，需要判断used stmt是否至少包含%0 = %0 + 1 和 castExpr = %0两条语句，不新增临时变量
        // 非自增或自减语句，used stmt中仅包含%1 = %0 + 1
        const usedStmts = castLocal.getUsedStmts();
        if (usedStmts.length !== 2) {
            return false;
        }
        let selfAssignFlag = false;
        let assignBackFlag = false;
        for (const usedStmt of usedStmts) {
            if (!(usedStmt instanceof ArkAssignStmt)) {
                return false;
            }
            const leftOp = usedStmt.getLeftOp();
            const rightOp = usedStmt.getRightOp();
            if (leftOp instanceof Local) {
                if (leftOp !== castLocal) {
                    return false;
                }
                if (!(rightOp instanceof ArkNormalBinopExpr)) {
                    return false;
                }
                const op1 = rightOp.getOp1();
                const op2 = rightOp.getOp2();
                const operator = rightOp.getOperator();
                if (op1 !== castLocal) {
                    return false;
                }
                if (operator !== NormalBinaryOperator.Addition && operator !== NormalBinaryOperator.Subtraction) {
                    return false;
                }
                if (!(op2 instanceof NumberConstant) || !(op2.getType() instanceof NumberType) || op2.getValue() !== '1') {
                    return false;
                }
                selfAssignFlag = true;
            }
            if (leftOp instanceof ArkCastExpr) {
                if (leftOp !== stmt.getRightOp()) {
                    return false;
                }
                if (rightOp !== castLocal) {
                    return false;
                }
                assignBackFlag = true;
            }
        }
        return selfAssignFlag && assignBackFlag;
    }

    private hasCheckedWithInstanceof(cfg: Cfg, stmt: Stmt): boolean {
        const castExpr = this.getCastExpr(stmt);
        if (castExpr === null) {
            return false;
        }
        for (const block of cfg.getBlocks()) {
            // 这里仅判断了cast op是否进行了instanceof判断，如果op是由op1赋值，op1进行了instanceof判断，此处不认为是做了有效检查，因为此赋值链可能很长且中途发生类型变化，极易判断错误
            const checkRes = this.checkTypeAssuranceInBasicBlock(block, castExpr);
            if (checkRes === TypeAssuranceCondition.NotExist) {
                continue;
            }
            let checkedBB: Set<number> = new Set<number>();
            let needCheckBB: number[] = [];
            checkedBB.add(block.getId());
            const allSuccessors = block.getSuccessors();
            if (allSuccessors.length > 0 && checkRes === TypeAssuranceCondition.Positive) {
                needCheckBB.push(allSuccessors[0].getId());
            }
            if (allSuccessors.length > 1 && checkRes === TypeAssuranceCondition.Negative) {
                needCheckBB.push(allSuccessors[1].getId());
            }
            while (needCheckBB.length > 0) {
                const bbId = needCheckBB.shift();
                if (bbId === undefined) {
                    break;
                }
                if (checkedBB.has(bbId)) {
                    continue;
                }
                checkedBB.add(bbId);
                const bb = this.getBlockWithId(bbId, cfg);
                if (bb === null) {
                    continue;
                }
                if (this.isStmtInBlock(stmt, bb)) {
                    return true;
                } else {
                    bb.getSuccessors().forEach(b => needCheckBB.push(b.getId()));
                }
            }
        }
        return false;
    }

    private checkTypeAssuranceInBasicBlock(bb: BasicBlock, castExpr: ArkCastExpr): TypeAssuranceCondition {
        for (const stmt of bb.getStmts()) {
            if (!(stmt instanceof ArkIfStmt)) {
                continue;
            }
            const conditionExpr = stmt.getConditionExpr();
            const op1 = conditionExpr.getOp1();
            const op2 = conditionExpr.getOp2();
            const operator = conditionExpr.getOperator();
            // 对于if (i instanceof A)这种条件语句，op1总是临时变量，op2总是false，操作符总是！=
            if (!(op1 instanceof Local && op2 instanceof BooleanConstant && op2.getValue() === 'false' && operator === RelationalBinaryOperator.InEquality)) {
                break;
            }
            return this.checkTypeAssuranceWithLocal(op1, castExpr, stmt.getOriginPositionInfo(), true);
        }
        return TypeAssuranceCondition.NotExist;
    }

    private checkTypeAssuranceWithLocal(operand: Local, castExpr: ArkCastExpr, ifStmtPos: LineColPosition, shouldBe: boolean): TypeAssuranceCondition {
        const declaringStmt = operand.getDeclaringStmt();
        if (declaringStmt === null) {
            return TypeAssuranceCondition.NotExist;
        }
        // if语句中的所有条件遵从三地址码原则拆分成多个语句时，所有语句的位置信息是一致的，不一致时表示是条件语句之前的赋值或声明情况，不在本判断范围内
        const stmtPos = declaringStmt.getOriginPositionInfo();
        if (stmtPos.getLineNo() !== ifStmtPos.getLineNo() || stmtPos.getColNo() !== ifStmtPos.getColNo()) {
            return TypeAssuranceCondition.NotExist;
        }
        if (!(declaringStmt instanceof ArkAssignStmt)) {
            return TypeAssuranceCondition.NotExist;
        }
        const rightOp = declaringStmt.getRightOp();
        if (rightOp instanceof ArkInstanceOfExpr) {
            if (this.isTypeAssuranceMatchCast(rightOp, castExpr)) {
                if (shouldBe) {
                    return TypeAssuranceCondition.Positive;
                } else {
                    return TypeAssuranceCondition.Negative;
                }
            }
            return TypeAssuranceCondition.NotExist;
        }
        if (rightOp instanceof ArkUnopExpr && rightOp.getOperator() === UnaryOperator.LogicalNot) {
            const unaryOp = rightOp.getOp();
            if (unaryOp instanceof Local) {
                return this.checkTypeAssuranceWithLocal(unaryOp, castExpr, ifStmtPos, !shouldBe);
            }
            return TypeAssuranceCondition.NotExist;
        }
        if (rightOp instanceof ArkNormalBinopExpr) {
            const op1 = rightOp.getOp1();
            const op2 = rightOp.getOp2();
            const operator = rightOp.getOperator();
            // 这里仅判断&&和||两种逻辑运算符的场景，其他场景在包含类型守卫判断的条件语句中不常见，暂不考虑
            let res: TypeAssuranceCondition;
            if (operator === NormalBinaryOperator.LogicalAnd) {
                if (op1 instanceof Local) {
                    res = this.checkTypeAssuranceWithLocal(op1, castExpr, ifStmtPos, shouldBe);
                    if (res !== TypeAssuranceCondition.NotExist) {
                        return res;
                    }
                }
                if (op2 instanceof Local) {
                    res = this.checkTypeAssuranceWithLocal(op2, castExpr, ifStmtPos, shouldBe);
                    if (res !== TypeAssuranceCondition.NotExist) {
                        return res;
                    }
                }
                return TypeAssuranceCondition.NotExist;
            }
            if (operator === NormalBinaryOperator.LogicalOr) {
                // a or b，不论a或b里是类型守卫判断，均无法保证分支中的类型明确
                if (shouldBe) {
                    return TypeAssuranceCondition.NotExist;
                }
            }
        }
        return TypeAssuranceCondition.NotExist;
    }

    private isTypeAssuranceMatchCast(instanceOfExpr: ArkInstanceOfExpr, castExpr: ArkCastExpr): boolean {
        const castOp = castExpr.getOp();
        const castType = castExpr.getType();
        const instanceofType = instanceOfExpr.getCheckType();
        if (castType.getTypeString() !== instanceofType.getTypeString()) {
            return false;
        }
        const instanceofOp = instanceOfExpr.getOp();
        if (!(castOp instanceof Local && instanceofOp instanceof Local)) {
            return false;
        }
        return castOp.getName() === instanceofOp.getName();
    }

    private isStmtInBlock(stmt: Stmt, block: BasicBlock): boolean {
        for (const s of block.getStmts()) {
            if (s === stmt) {
                return true;
            }
        }
        return false;
    }

    private getBlockWithId(id: number, cfg: Cfg): BasicBlock | null {
        const blocks = cfg.getBlocks();
        for (const bb of blocks) {
            if (bb.getId() === id) {
                return bb;
            }
        }
        return null;
    }

    private checkFromStmt(stmt: Stmt, globalVarMap: Map<string, Stmt[]>, checkAll: { value: boolean }, visited: Set<Stmt>, depth: number = 0): Stmt | null {
        if (depth > CALL_DEPTH_LIMIT) {
            checkAll.value = false;
            return null;
        }
        const node = this.dvfg.getOrNewDVFGNode(stmt);
        let worklist: DVFGNode[] = [node];
        while (worklist.length > 0) {
            const current = worklist.shift()!;
            const currentStmt = current.getStmt();
            if (visited.has(currentStmt)) {
                continue;
            }
            visited.add(currentStmt);
            if (this.isWithInterfaceAnnotation(currentStmt)) {
                return currentStmt;
            }

            const fieldDeclareStmt = this.isCastOpFieldWithInterfaceType(currentStmt);
            if (fieldDeclareStmt) {
                return fieldDeclareStmt;
            }
            const gv = this.checkIfCastOpIsGlobalVar(currentStmt);
            if (gv) {
                const globalDefs = globalVarMap.get(gv.getName());
                if (globalDefs === undefined) {
                    const importValue = this.checkIfCastOpIsFromImport(currentStmt);
                    if (importValue && importValue.getDeclaringStmt() !== null) {
                        worklist.push(this.dvfg.getOrNewDVFGNode(importValue.getDeclaringStmt()!));
                    }
                } else {
                    globalDefs.forEach(d => worklist.push(this.dvfg.getOrNewDVFGNode(d)));
                }
                continue;
            }

            const callsite = this.cg.getCallSiteByStmt(currentStmt);
            for (const cs of callsite) {
                const declaringMtd = this.cg.getArkMethodByFuncID(cs.calleeFuncID);
                if (!declaringMtd || !declaringMtd.getCfg()) {
                    continue;
                }
                if (!this.visited.has(declaringMtd)) {
                    this.dvfgBuilder.buildForSingleMethod(declaringMtd);
                    this.visited.add(declaringMtd);
                }
                const returnStmts = declaringMtd.getReturnStmt();
                for (const stmt of returnStmts) {
                    const res = this.checkFromStmt(stmt, globalVarMap, checkAll, visited, depth + 1);
                    if (res !== null) {
                        return res;
                    }
                }
            }
            const paramRef = this.isFromParameter(currentStmt);
            if (paramRef) {
                const paramIdx = paramRef.getIndex();
                const callsites = this.cg.getInvokeStmtByMethod(currentStmt.getCfg().getDeclaringMethod().getSignature());
                this.processCallsites(callsites);
                const argDefs = this.collectArgDefs(paramIdx, callsites);
                for (const stmt of argDefs) {
                    const res = this.checkFromStmt(stmt, globalVarMap, checkAll, visited, depth + 1);
                    if (res !== null) {
                        return res;
                    }
                }
            }
            current.getIncomingEdge().forEach(e => worklist.push(e.getSrcNode() as DVFGNode));
        }
        return null;
    }

    private isCastOpFieldWithInterfaceType(stmt: Stmt): Stmt | undefined {
        const obj = this.getCastOp(stmt);
        if (obj === null || !(obj instanceof Local)) {
            return undefined;
        }
        const declaringStmt = obj.getDeclaringStmt();
        if (declaringStmt === null || !(declaringStmt instanceof ArkAssignStmt)) {
            return undefined;
        }
        const rightOp = declaringStmt.getRightOp();
        if (!(rightOp instanceof AbstractFieldRef)) {
            return undefined;
        }
        const fieldDeclaring = rightOp.getFieldSignature().getDeclaringSignature();
        if (fieldDeclaring instanceof ClassSignature) {
            const field = this.scene.getClass(fieldDeclaring)?.getField(rightOp.getFieldSignature());
            if (!field) {
                return undefined;
            }
            const fieldInitializer = field.getInitializer();
            const lastStmt = fieldInitializer[fieldInitializer.length - 1];
            if (this.isWithInterfaceAnnotation(lastStmt)) {
                return lastStmt;
            }
        }
        return undefined;
    }

    private checkIfCastOpIsGlobalVar(stmt: Stmt): Local | undefined {
        const obj = this.getCastOp(stmt);
        if (obj instanceof Local && !obj.getDeclaringStmt()) {
            return obj;
        }
        return undefined;
    }

    private checkIfCastOpIsFromImport(stmt: Stmt): Local | undefined {
        const obj = this.getCastOp(stmt);
        if (obj === null || !(obj instanceof Local)) {
            return undefined;
        }
        const importInfos = stmt.getCfg().getDeclaringMethod().getDeclaringArkFile().getImportInfos();
        for (const importInfo of importInfos) {
            if (importInfo.getImportClauseName() === obj.getName()) {
                const exportInfo = importInfo.getLazyExportInfo();
                if (exportInfo === null) {
                    return undefined;
                }
                const arkExport = exportInfo.getArkExport();
                if (arkExport === null || arkExport === undefined) {
                    return undefined;
                }
                if (!(arkExport instanceof Local)) {
                    return undefined;
                }
                return arkExport;
            }
        }
        return undefined;
    }

    private processCallsites(callsites: Stmt[]): void {
        callsites.forEach(cs => {
            const declaringMtd = cs.getCfg().getDeclaringMethod();
            if (!this.visited.has(declaringMtd)) {
                this.dvfgBuilder.buildForSingleMethod(declaringMtd);
                this.visited.add(declaringMtd);
            }
        });
    }

    // 判断语句是否为赋值语句，且左值的类型注解为Interface，右值的类型与左值不一样
    private isWithInterfaceAnnotation(stmt: Stmt): boolean {
        if (!(stmt instanceof ArkAssignStmt)) {
            return false;
        }
        const leftOpType = stmt.getLeftOp().getType();
        if (!(leftOpType instanceof ClassType)) {
            return false;
        }
        const leftOpTypeclass = this.scene.getClass(leftOpType.getClassSignature());
        if (leftOpTypeclass === null) {
            return false;
        }
        if (leftOpTypeclass.getCategory() !== ClassCategory.INTERFACE) {
            return false;
        }
        const rightOpType = stmt.getRightOp().getType();
        if (!(rightOpType instanceof ClassType)) {
            return true;
        }
        return !classSignatureCompare(leftOpType.getClassSignature(), rightOpType.getClassSignature());
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

    private getCastOp(stmt: Stmt): Value | null {
        if (!(stmt instanceof ArkAssignStmt)) {
            return null;
        }
        const rightOp = stmt.getRightOp();
        if (!(rightOp instanceof ArkCastExpr)) {
            return null;
        }
        return rightOp.getOp();
    }

    private getCastExpr(stmt: Stmt): ArkCastExpr | null {
        // method中使用as断言的地方可能是body体中，函数调用的实参，返回值，均会表示成ArkAssignStmt
        if (!(stmt instanceof ArkAssignStmt)) {
            return null;
        }
        const rightOp = stmt.getRightOp();
        if (rightOp instanceof ArkCastExpr) {
            return rightOp;
        }
        if (rightOp instanceof ArkNormalBinopExpr) {
            const op1 = rightOp.getOp1();
            const op2 = rightOp.getOp2();
            if (op1 instanceof ArkCastExpr) {
                return op1;
            }
            if (op2 instanceof ArkCastExpr) {
                return op2;
            }
        }
        return null;
    }

    private collectArgDefs(argIdx: number, callsites: Stmt[]): Stmt[] {
        const getKey = (v: Value): Value | FieldSignature => {
            return v instanceof ArkInstanceFieldRef ? v.getFieldSignature() : v;
        };
        return callsites.flatMap(callsite => {
            const target: Value | FieldSignature = getKey(callsite.getInvokeExpr()!.getArg(argIdx));
            return Array.from(this.dvfg.getOrNewDVFGNode(callsite).getIncomingEdge())
                .map(e => (e.getSrcNode() as DVFGNode).getStmt())
                .filter(s => {
                    return s instanceof ArkAssignStmt && target === getKey(s.getLeftOp());
                });
        });
    }

    private addIssueReport(stmt: Stmt, operand: ArkCastExpr, relatedStmt?: Stmt, incrementCase: boolean = false): void {
        const severity = this.rule.alert ?? this.metaData.severity;
        const warnInfo = getLineAndColumn(stmt, operand);
        const problem = 'As';
        const descPrefix = 'The value in type assertion is assigned by value with interface annotation';
        let desc = `(${this.rule.ruleId.replace('@migration/', '')})`;
        if (incrementCase) {
            desc = 'Can not use neither increment nor decrement with cast expression ' + desc;
        } else if (relatedStmt === undefined) {
            desc = `Can not check when function call chain depth exceeds ${CALL_DEPTH_LIMIT}, please check it manually ` + desc;
        } else {
            const sinkFile = stmt.getCfg().getDeclaringMethod().getDeclaringArkFile();
            const relatedFile = relatedStmt.getCfg().getDeclaringMethod().getDeclaringArkFile();
            if (fileSignatureCompare(sinkFile.getFileSignature(), relatedFile.getFileSignature())) {
                desc = `${descPrefix} in Line ${relatedStmt.getOriginPositionInfo().getLineNo()} ` + desc;
            } else {
                desc = `${descPrefix} in file ${path.normalize(relatedFile.getName())}: ${relatedStmt.getOriginPositionInfo().getLineNo()} ` + desc;
            }
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
}
