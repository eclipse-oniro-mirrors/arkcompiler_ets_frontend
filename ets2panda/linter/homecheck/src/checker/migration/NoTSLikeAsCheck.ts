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
    ArkArrayRef,
    ArkAssignStmt,
    ArkCastExpr,
    ArkField,
    ArkIfStmt,
    ArkInstanceInvokeExpr,
    ArkInstanceFieldRef,
    ArkStaticFieldRef,
    ArkInstanceOfExpr,
    ArkMethod,
    ArkNamespace,
    ArkNewExpr,
    ArkNormalBinopExpr,
    ArkParameterRef,
    ArkReturnStmt,
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
    Type,
    TypeInference,
    UnaryOperator,
    UnionType,
    UnknownType,
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
    readonly checkedBinaryOperator: string[] = [
        NormalBinaryOperator.Addition,
        NormalBinaryOperator.Subtraction,
        NormalBinaryOperator.Multiplication,
        NormalBinaryOperator.Division,
        NormalBinaryOperator.Remainder,
        NormalBinaryOperator.Exponentiation,
        NormalBinaryOperator.NullishCoalescing,
        NormalBinaryOperator.LeftShift,
        NormalBinaryOperator.RightShift,
        NormalBinaryOperator.UnsignedRightShift,
        NormalBinaryOperator.BitwiseAnd,
        NormalBinaryOperator.BitwiseOr,
        NormalBinaryOperator.BitwiseXor,
    ];
    readonly checkedUnaryOperator: string[] = [
        UnaryOperator.LogicalNot,
        UnaryOperator.BitwiseNot,
    ];
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
            const castType = this.unwrapAliasType(castExpr.getType());
            const opType = this.unwrapAliasType(castExpr.getOp().getType());

            // 判断是否为cast表达式的算数运算，属于告警场景之一
            if (this.isCastExprWithNumericOperation(stmt)) {
                this.addIssueReport(stmt, castExpr, undefined, true);
                continue;
            }

            // 判断cast类型断言的类型是否是class，非class的场景不在本规则检查范围内
            if (!(castType instanceof ClassType)) {
                continue;
            }
            if (this.hasCheckedWithInstanceof(stmt.getCfg(), stmt)) {
                continue;
            }
            if (!this.visited.has(target)) {
                this.dvfgBuilder.buildForSingleMethod(target);
                this.visited.add(target);
            }

            // Check if castExpr.op.getType() is a parent type of castExpr.getType()
            if (!this.isOpTypeSuperTypeOfCastType(opType, castType)) {
                continue;
            }

            let checkAll = { value: true };
            let visited: Set<Stmt> = new Set();
            const result = this.checkFromStmt(stmt, globalVarMap, checkAll, visited, 0, castType);
            if (result !== null) {
                this.addIssueReport(stmt, castExpr, result);
            } else {
                if (!checkAll.value && !this.checkTypesMatch(opType, castType)) {
                    this.addIssueReport(stmt, castExpr);
                }
            }
        }
    }

    private checkTypesMatch(opType: Type, castType: Type): boolean {
        opType = this.unwrapAliasType(opType);
        castType = this.unwrapAliasType(castType);
        if (opType instanceof UnionType) {
            return opType.getTypes().some(type => this.unwrapAliasType(type).toString() === castType.toString())
        }

        if (opType instanceof ClassType) {
            // we know that the cast type is a class type already
            return opType.toString() === castType.toString()
        }

        return false;
    }

    private isCastExprWithNumericOperation(stmt: Stmt): boolean {
        if (this.isCastExprWithIncrementDecrement(stmt)) {
            return true;
        }
        const castExpr = this.getCastExpr(stmt);
        if (castExpr === null || !(this.unwrapAliasType(castExpr.getType()) instanceof NumberType)) {
            return false;
        }
        if (!(stmt instanceof ArkAssignStmt)) {
            return false;
        }
        if (this.isCastExprUsedDirectlyInNumericOperation(stmt, castExpr)) {
            return true;
        }
        return this.isCastLocalUsedByNumericOperation(stmt, castExpr);
    }

    private isCastExprUsedDirectlyInNumericOperation(stmt: ArkAssignStmt, castExpr: ArkCastExpr): boolean {
        const leftOp = stmt.getLeftOp();
        const rightOp = stmt.getRightOp();
        if (rightOp instanceof ArkNormalBinopExpr) {
            return this.isNumericBinopWithValue(rightOp, castExpr);
        }
        if (rightOp instanceof ArkUnopExpr) {
            return this.isNumericUnopWithValue(rightOp, castExpr);
        }
        return leftOp instanceof ArkCastExpr && leftOp === castExpr;
    }

    private isCastLocalUsedByNumericOperation(stmt: ArkAssignStmt, castExpr: ArkCastExpr): boolean {
        if (!(stmt.getRightOp() instanceof ArkCastExpr) || stmt.getRightOp() !== castExpr) {
            return false;
        }
        const castLocal = stmt.getLeftOp();
        if (!(castLocal instanceof Local)) {
            return false;
        }
        return castLocal.getUsedStmts().some(usedStmt => {
            if (!(usedStmt instanceof ArkAssignStmt) || !this.isSameSourceLine(stmt, usedStmt)) {
                return false;
            }
            const rightOp = usedStmt.getRightOp();
            if (rightOp instanceof ArkNormalBinopExpr) {
                return this.isNumericBinopWithValue(rightOp, castLocal);
            }
            if (rightOp instanceof ArkUnopExpr) {
                return this.isNumericUnopWithValue(rightOp, castLocal);
            }
            return false;
        });
    }

    private isNumericBinopWithValue(expr: ArkNormalBinopExpr, value: Value): boolean {
        if (!this.checkedBinaryOperator.includes(expr.getOperator())) {
            return false;
        }
        return expr.getOp1() === value || expr.getOp2() === value;
    }

    private isNumericUnopWithValue(expr: ArkUnopExpr, value: Value): boolean {
        if (!this.checkedUnaryOperator.includes(expr.getOperator())) {
            return false;
        }
        return expr.getOp() === value;
    }

    private isSameSourceLine(left: Stmt, right: Stmt): boolean {
        const leftPos = left.getOriginPositionInfo();
        const rightPos = right.getOriginPositionInfo();
        return leftPos.getLineNo() > 0 && leftPos.getLineNo() === rightPos.getLineNo();
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
        const castType = this.unwrapAliasType(castExpr.getType());
        const instanceofType = this.unwrapAliasType(instanceOfExpr.getCheckType());
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

    private checkFromStmt(
        stmt: Stmt,
        globalVarMap: Map<string, Stmt[]>,
        checkAll: { value: boolean },
        visited: Set<Stmt>,
        depth: number = 0,
        castType: Type
    ): Stmt | null {
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
            if (this.hasConcreteClassOriginForCast(currentStmt, castType)) {
                return currentStmt;
            }

            const directOriginStmt = this.getDirectOriginForCast(currentStmt, castType);
            if (directOriginStmt) {
                return directOriginStmt;
            }

            const globalOrigin = this.checkGlobalOriginFromStmt(
                currentStmt,
                globalVarMap,
                checkAll,
                visited,
                depth,
                castType,
                worklist
            );
            if (globalOrigin.result !== null) {
                return globalOrigin.result;
            }
            if (globalOrigin.handled) {
                continue;
            }

            const returnOriginStmt = this.checkReturnOriginFromStmt(
                currentStmt,
                globalVarMap,
                checkAll,
                visited,
                depth,
                castType
            );
            if (returnOriginStmt !== null) {
                return returnOriginStmt;
            }

            const parameterOriginStmt = this.checkParameterOriginFromStmt(
                currentStmt,
                globalVarMap,
                checkAll,
                visited,
                depth,
                castType
            );
            if (parameterOriginStmt !== null) {
                return parameterOriginStmt;
            }
            current.getIncomingEdge().forEach(e => worklist.push(e.getSrcNode() as DVFGNode));
        }
        return null;
    }

    private getDirectOriginForCast(stmt: Stmt, castType: Type): Stmt | undefined {
        const fieldDeclareStmt = this.isCastOpFieldWithInterfaceType(stmt, castType);
        if (fieldDeclareStmt) {
            return fieldDeclareStmt;
        }
        return this.getContainerOriginForCast(stmt, castType);
    }

    private checkGlobalOriginFromStmt(
        stmt: Stmt,
        globalVarMap: Map<string, Stmt[]>,
        checkAll: { value: boolean },
        visited: Set<Stmt>,
        depth: number,
        castType: Type,
        worklist: DVFGNode[]
    ): { handled: boolean; result: Stmt | null } {
        const globalVar = this.checkIfCastOpIsGlobalVar(stmt);
        if (!globalVar) {
            return { handled: false, result: null };
        }

        const globalDefs = globalVarMap.get(globalVar.getName());
        if (globalDefs !== undefined) {
            globalDefs.forEach(d => worklist.push(this.dvfg.getOrNewDVFGNode(d)));
            return { handled: true, result: null };
        }

        const importValue = this.checkIfCastOpIsFromImport(stmt);
        if (!importValue || !importValue.getDeclaringStmt()) {
            return { handled: true, result: null };
        }

        const originStmt = importValue.getDeclaringStmt()!;
        const originMethod = originStmt.getCfg().getDeclaringMethod();
        if (!originMethod) {
            return { handled: true, result: null };
        }

        this.ensureDvfgBuiltForMethod(originMethod);
        return {
            handled: true,
            result: this.checkFromStmt(originStmt, globalVarMap, checkAll, visited, depth + 1, castType),
        };
    }

    private checkReturnOriginFromStmt(
        stmt: Stmt,
        globalVarMap: Map<string, Stmt[]>,
        checkAll: { value: boolean },
        visited: Set<Stmt>,
        depth: number,
        castType: Type
    ): Stmt | null {
        const callsite = this.cg.getCallSiteByStmt(stmt);
        for (const cs of callsite) {
            const declaringMtd = this.cg.getArkMethodByFuncID(cs.calleeFuncID);
            if (!declaringMtd) {
                continue;
            }
            if (!declaringMtd.getCfg()) {
                continue;
            }
            this.ensureDvfgBuiltForMethod(declaringMtd);

            for (const stmt of declaringMtd.getReturnStmt()) {
                const res = this.checkFromStmt(stmt, globalVarMap, checkAll, visited, depth + 1, castType);
                if (res !== null) {
                    return res;
                }
            }
        }
        return null;
    }

    private checkParameterOriginFromStmt(
        stmt: Stmt,
        globalVarMap: Map<string, Stmt[]>,
        checkAll: { value: boolean },
        visited: Set<Stmt>,
        depth: number,
        castType: Type
    ): Stmt | null {
        const paramRef = this.isFromParameter(stmt);
        if (!paramRef) {
            return null;
        }

        const paramIdx = paramRef.getIndex();
        const callsites = this.cg.getInvokeStmtByMethod(stmt.getCfg().getDeclaringMethod().getSignature());
        this.processCallsites(callsites);
        const argDefs = this.collectArgDefs(paramIdx, callsites);
        if (argDefs.length === 0 && this.isOpTypeSuperTypeOfCastType(paramRef.getType(), castType)) {
            return stmt;
        }

        for (const stmt of argDefs) {
            const res = this.checkFromStmt(stmt, globalVarMap, checkAll, visited, depth + 1, castType);
            if (res !== null) {
                return res;
            }
        }
        return null;
    }

    private isCastOpFieldWithInterfaceType(stmt: Stmt, castType: Type): Stmt | undefined {
        const fieldRef = this.getFieldRefFromCastOp(stmt);
        if (!fieldRef) {
            return undefined;
        }
        const fieldDeclaring = fieldRef.getFieldSignature().getDeclaringSignature();
        if (fieldDeclaring instanceof ClassSignature) {
            const field = this.scene.getClass(fieldDeclaring)?.getField(fieldRef.getFieldSignature());
            if (!field) {
                return undefined;
            }
            // find the origin define stmt of the field
            const originStmt = this.traceFieldInitializerToOrigin(field, new Set<ArkField>());
            if (originStmt && this.hasConcreteClassOriginForCast(originStmt, castType)) {
                return originStmt;
            }
        }
        return undefined;
    }

    private getFieldRefFromCastOp(stmt: Stmt): AbstractFieldRef | undefined {
        const obj = this.getCastOp(stmt);
        if (obj instanceof AbstractFieldRef) {
            return obj;
        }

        const rightOp = this.getLocalDeclaringRightOp(obj);
        if (rightOp instanceof AbstractFieldRef) {
            return rightOp;
        }
        return undefined;
    }

    private traceFieldInitializerToOrigin(field: ArkField, visited: Set<ArkField>): Stmt | undefined {
        // avoid circular reference
        if (visited.has(field)) {
            return undefined;
        }
        visited.add(field);

        const fieldInitializer = field.getInitializer();
        if (fieldInitializer.length === 0) {
            return undefined;
        }

        // get the first initialize stmt
        const firstStmt = fieldInitializer[0];
        if (!(firstStmt instanceof ArkAssignStmt)) {
            // if the first stmt is not an assign stmt, return the last stmt
            return fieldInitializer[fieldInitializer.length - 1];
        }

        const rightOp = this.getFieldInitializerRightOp(firstStmt);

        // if rightOp is an ArkInstanceFieldRef or ArkStaticFieldRef, continue to trace the origin define stmt
        const targetField = this.resolveFieldFromInitializerOp(field, rightOp);
        if (!targetField) {
            return fieldInitializer[fieldInitializer.length - 1];
        }

        const originStmt = this.traceFieldInitializerToOrigin(targetField, visited);
        if (originStmt) {
            return originStmt;
        }

        // if cannot trace anymore, return the last stmt
        return fieldInitializer[fieldInitializer.length - 1];
    }

    private getFieldInitializerRightOp(firstStmt: ArkAssignStmt): Value {
        const rightOp = firstStmt.getRightOp();
        return this.getLocalDeclaringRightOp(rightOp) ?? rightOp;
    }

    private getLocalDeclaringRightOp(op: Value | null): Value | undefined {
        if (!(op instanceof Local)) {
            return undefined;
        }

        const declaringStmt = op.getDeclaringStmt();
        if (declaringStmt instanceof ArkAssignStmt) {
            return declaringStmt.getRightOp();
        }
        return undefined;
    }

    private resolveFieldFromInitializerOp(currentField: ArkField, op: Value): ArkField | undefined {
        const declaringRightOp = this.getLocalDeclaringRightOp(op);
        if (declaringRightOp) {
            return this.resolveFieldFromInitializerOp(currentField, declaringRightOp);
        }

        if (op instanceof Local) {
            return currentField.getDeclaringArkClass().getFields().find(field => field.getName() === op.getName());
        }

        if (!(op instanceof ArkInstanceFieldRef || op instanceof ArkStaticFieldRef)) {
            return undefined;
        }

        const fieldSig = op.getFieldSignature();
        const declaringSig = fieldSig.getDeclaringSignature();
        if (!(declaringSig instanceof ClassSignature)) {
            return undefined;
        }

        return this.scene.getClass(declaringSig)?.getField(fieldSig) ?? undefined;
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

    private ensureDvfgBuiltForMethod(method: ArkMethod): void {
        if (!this.visited.has(method)) {
            this.dvfgBuilder.buildForSingleMethod(method);
            this.visited.add(method);
        }
    }

    private processCallsites(callsites: Stmt[]): void {
        callsites.forEach(cs => {
            const declaringMtd = cs.getCfg().getDeclaringMethod();
            this.ensureDvfgBuiltForMethod(declaringMtd);
        });
    }

    private hasConcreteClassOriginForCast(stmt: Stmt, castType: Type): boolean {
        if (!(stmt instanceof ArkAssignStmt) && !(stmt instanceof ArkReturnStmt)) {
            return false;
        }
    
        const newExprType = this.extractNewExprClassType(stmt);
        if (!newExprType) {
            return false;
        }

        const originType = this.getOriginTypeFromNewExpr(stmt, newExprType);
        const normalizedCastType = this.unwrapAliasType(castType);
        if (!(originType instanceof ClassType) || !(normalizedCastType instanceof ClassType)) {
            return false;
        }
        return !this.isRuntimeSafeConcreteCast(originType, normalizedCastType);
    }

    private isRuntimeSafeConcreteCast(originType: ClassType, castType: ClassType): boolean {
        if (classSignatureCompare(originType.getClassSignature(), castType.getClassSignature())) {
            return true;
        }
        const originClass = this.scene.getClass(originType.getClassSignature());
        if (originClass === null) {
            return false;
        }
        return this.isClassDerivedFrom(originClass, castType.getClassSignature());
    }
    
    private extractNewExprClassType(stmt: Stmt): ClassType | null {
        const targetLocal = stmt instanceof ArkAssignStmt 
            ? stmt.getRightOp() 
            : (stmt as ArkReturnStmt).getOp();
        if (targetLocal instanceof ArkNewExpr) {
            return targetLocal.getClassType();
        }

        if (!(targetLocal instanceof Local)) {
            return null;
        }
    
        const declaringStmt = targetLocal.getDeclaringStmt();
        if (!(declaringStmt instanceof ArkAssignStmt)) {
            return null;
        }
    
        const declaringRightOp = declaringStmt.getRightOp();
        if (!(declaringRightOp instanceof ArkNewExpr)) {
            return null;
        }
    
        return declaringRightOp.getClassType();
    }

    private getOriginTypeFromNewExpr(stmt: Stmt, newExprClassType: ClassType): Type {
        let originType: Type;
        if (stmt instanceof ArkAssignStmt) {
            const classSig = newExprClassType.getClassSignature();
            const arkClass = this.scene.getClass(classSig);
            const isAutoGenerated = arkClass?.isAnonymousClass();
            originType = isAutoGenerated ? stmt.getLeftOp().getType() : newExprClassType;
        } else {
            originType = newExprClassType;
        }

        return this.unwrapAliasType(originType);
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

    private getContainerOriginForCast(stmt: Stmt, castType: Type): Stmt | undefined {
        const castOp = this.getCastOp(stmt);
        if (!(castOp instanceof Local)) {
            return undefined;
        }
        const declaringStmt = castOp.getDeclaringStmt();
        if (!(declaringStmt instanceof ArkAssignStmt)) {
            return undefined;
        }
        const invokeExpr = declaringStmt.getRightOp();
        if (!(invokeExpr instanceof ArkInstanceInvokeExpr)) {
            return undefined;
        }
        const methodName = invokeExpr.getMethodSignature().getMethodSubSignature().getMethodName();
        if (methodName === 'pop') {
            return this.getArrayElementOriginForCast(stmt, declaringStmt, invokeExpr.getBase(), castType);
        }
        if (methodName === 'get') {
            return this.getMapElementOriginForCast(stmt, declaringStmt, invokeExpr.getBase(), castType);
        }
        return undefined;
    }

    private getArrayElementOriginForCast(stmt: Stmt, callStmt: ArkAssignStmt, arrayLocal: Local, castType: Type): Stmt | undefined {
        const arrayRoot = this.resolveLocalAlias(arrayLocal);
        for (const candidate of this.getPreviousStmts(stmt, callStmt)) {
            if (!(candidate instanceof ArkAssignStmt)) {
                continue;
            }
            const leftOp = candidate.getLeftOp();
            if (!(leftOp instanceof ArkArrayRef)) {
                continue;
            }
            if (this.resolveLocalAlias(leftOp.getBase()) !== arrayRoot) {
                continue;
            }
            if (this.hasConcreteClassOriginForCast(candidate, castType)) {
                return candidate;
            }
        }
        return undefined;
    }

    private getMapElementOriginForCast(stmt: Stmt, callStmt: ArkAssignStmt, mapLocal: Local, castType: Type): Stmt | undefined {
        const mapRoot = this.resolveLocalAlias(mapLocal);
        for (const candidate of this.getPreviousStmts(stmt, callStmt)) {
            const invokeExpr = candidate.getInvokeExpr();
            if (!(invokeExpr instanceof ArkInstanceInvokeExpr)) {
                continue;
            }
            if (invokeExpr.getMethodSignature().getMethodSubSignature().getMethodName() !== 'set') {
                continue;
            }
            if (this.resolveLocalAlias(invokeExpr.getBase()) !== mapRoot) {
                continue;
            }
            const valueOriginStmt = this.getOriginStmtForValue(invokeExpr.getArg(1));
            if (valueOriginStmt && this.hasConcreteClassOriginForCast(valueOriginStmt, castType)) {
                return valueOriginStmt;
            }
        }
        return undefined;
    }

    private getPreviousStmts(stmt: Stmt, stopStmt: Stmt): Stmt[] {
        const stmts = stmt.getCfg().getStmts();
        const stopIndex = stmts.indexOf(stopStmt);
        if (stopIndex <= 0) {
            return [];
        }
        return stmts.slice(0, stopIndex);
    }

    private resolveLocalAlias(local: Local): Local {
        let current = local;
        const visited = new Set<Local>();
        while (!visited.has(current)) {
            visited.add(current);
            const declaringStmt = current.getDeclaringStmt();
            if (!(declaringStmt instanceof ArkAssignStmt)) {
                break;
            }
            const rightOp = declaringStmt.getRightOp();
            if (!(rightOp instanceof Local)) {
                break;
            }
            current = rightOp;
        }
        return current;
    }

    private getOriginStmtForValue(value: Value): Stmt | undefined {
        if (!(value instanceof Local)) {
            return undefined;
        }
        const declaringStmt = value.getDeclaringStmt();
        return declaringStmt ?? undefined;
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

    private addIssueReport(stmt: Stmt, operand: ArkCastExpr, relatedStmt?: Stmt, numericOperationCase: boolean = false): void {
        const severity = this.rule.alert ?? this.metaData.severity;
        const warnInfo = getLineAndColumn(stmt, operand);
        const problem = 'As';
        const descPrefix = 'The value in type assertion is assigned by value with interface annotation';
        let desc = `(${this.rule.ruleId.replace('@migration/', '')})`;
        if (numericOperationCase) {
            desc = 'Can not use cast expression in numeric operation ' + desc;
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

    private isOpTypeSuperTypeOfCastType(opType: Type, castType: Type): boolean {
        opType = this.unwrapAliasType(opType);
        castType = this.unwrapAliasType(castType);
        if (opType instanceof UnknownType) {
            return true;
        }
        if (opType instanceof UnionType) {
            const unionTypes = opType.getTypes();
            return unionTypes.some(memberType => this.isOpTypeSuperTypeOfCastType(memberType, castType));
        }

        if (!(opType instanceof ClassType) || !(castType instanceof ClassType)) {
            return false;
        }

        const opClass = this.scene.getClass(opType.getClassSignature());
        const castClass = this.scene.getClass(castType.getClassSignature());

        if (opClass === null || castClass === null) {
            return false;
        }

        // if the types are the same, return false
        if (classSignatureCompare(opType.getClassSignature(), castType.getClassSignature())) {
            return false;
        }

        // recursively check all parent classes of castType
        return this.isClassDerivedFrom(castClass, opType.getClassSignature());
    }

    private unwrapAliasType(type: Type): Type {
        return TypeInference.replaceAliasType(type);
    }

    private isClassDerivedFrom(arkClass: ArkClass, targetClassSignature: ClassSignature, visited: Set<ArkClass> = new Set()): boolean {
        if (visited.has(arkClass)) {
            return false;
        }
        visited.add(arkClass);

        const superClasses = arkClass.getAllHeritageClasses();
        if (superClasses === null) {
            return false;
        }
        for (const superClass of superClasses) {
            if (classSignatureCompare(superClass.getSignature(), targetClassSignature)) {
                return true;
            }
            // recursively check the parent class of the parent class
            if (this.isClassDerivedFrom(superClass, targetClassSignature, visited)) {
                return true;
            }
        }
        return false;
    }
}
