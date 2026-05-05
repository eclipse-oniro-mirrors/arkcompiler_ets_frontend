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
    AbstractExpr,
    AbstractFieldRef,
    AbstractInvokeExpr,
    ArkAssignStmt,
    ArkCastExpr,
    ArkConditionExpr,
    ArkIfStmt,
    ArkInstanceFieldRef,
    ArkInvokeStmt,
    ArkMethod,
    ArkNormalBinopExpr,
    ArkTypeOfExpr,
    ArkUnopExpr,
    ClassSignature,
    ClosureFieldRef,
    ClosureType,
    FunctionType,
    INSTANCE_INIT_METHOD_NAME,
    Local,
    NormalBinaryOperator,
    STATIC_INIT_METHOD_NAME,
    Stmt,
    TEMP_LOCAL_PREFIX,
    UnaryOperator,
    Value,
} from 'arkanalyzer/lib';
import { NumberConstant } from 'arkanalyzer/lib/core/base/Constant';
import Logger, { LOG_MODULE_TYPE } from 'arkanalyzer/lib/utils/logger';
import {
    ArkArrayRef,
    ArkReturnStmt,
    NumberType,
    TupleType,
} from 'arkanalyzer';
import { ArkAwaitExpr } from 'arkanalyzer/lib/core/base/Expr';
import { Utils } from '../../../../../Index';
import {
    INT32_BOUNDARY,
    IssueInfo,
    IssueReason,
    NumberCategory,
} from '../../core/NumericSemanticTypes';
import { NumericArrayIndexUsageChecker } from './NumericArrayIndexUsageChecker';
import { NumericSourceIssueEmitter } from '../../diagnostics/emitters/NumericSourceIssueEmitter';

const logger = Logger.getLogger(LOG_MODULE_TYPE.HOMECHECK, 'NumericSourceUsageChecker');

interface NumericSourceUsageCheckerOptions {
    getArrayIndexUsageChecker(): NumericArrayIndexUsageChecker;
    getSourceIssueEmitter(): NumericSourceIssueEmitter;
    resetCallDepth(): void;
    checkValueOnlyUsedAsIntLong(stmt: Stmt, value: Value, hasChecked: Map<Local, IssueInfo>, numberCategory: NumberCategory): IssueReason;
    isLocalOnlyUsedAsIntLong(stmt: Stmt, local: Local, hasChecked: Map<Local, IssueInfo>, numberCategory: NumberCategory): IssueReason;
    isAbstractExprOnlyUsedAsIntLong(stmt: Stmt, expr: AbstractExpr, hasChecked: Map<Local, IssueInfo>, numberCategory: NumberCategory): IssueReason;
    checkFieldRef(fieldRef: AbstractFieldRef, currentClassSig: ClassSignature, numberCategory: NumberCategory, hasChecked: Map<Local, IssueInfo>): IssueReason;
    isNumberConstantActuallyFloat(constant: NumberConstant): boolean;
}

export class NumericSourceUsageChecker {
    constructor(private options: NumericSourceUsageCheckerOptions) {}

    public checkFieldInitializerWithIntLiteral(method: ArkMethod): void {
        // 仅对类属性的初始化语句进行检查，判断其中是否有涉及整型字面量的赋值或涉及除法运算
        if (method.getName() !== STATIC_INIT_METHOD_NAME && method.getName() !== INSTANCE_INIT_METHOD_NAME) {
            return;
        }
        const stmts = method.getCfg()?.getStmts();
        if (stmts === undefined) {
            return;
        }
        for (const stmt of stmts) {
            if (!(stmt instanceof ArkAssignStmt)) {
                continue;
            }
            const leftOp = stmt.getLeftOp();
            if (!(leftOp instanceof AbstractFieldRef) || (!Utils.isNearlyNumberType(leftOp.getType()) && !(stmt.getRightOp().getType() instanceof NumberType))) {
                continue;
            }
            const rightOp = stmt.getRightOp();
            if (rightOp instanceof ArkNormalBinopExpr || (rightOp instanceof Local && rightOp.getName().startsWith(TEMP_LOCAL_PREFIX))) {
                // 类属性的初始化语句使用Local赋值，且Local为临时变量，则可能涉及除法运算
                // 整型字面量参与除法运算的告警和自动修复信息在检查过程中就已生成，无需在此处额外生成
                this.options.checkValueOnlyUsedAsIntLong(stmt, rightOp, new Map<Local, IssueInfo>(), NumberCategory.int);
                this.options.checkFieldRef(
                    leftOp,
                    stmt.getCfg().getDeclaringMethod().getDeclaringArkClass().getSignature(),
                    NumberCategory.int,
                    new Map<Local, IssueInfo>()
                );
            }
            if (rightOp instanceof NumberConstant && !this.options.isNumberConstantActuallyFloat(rightOp)) {
                this.options.checkFieldRef(
                    leftOp,
                    stmt.getCfg().getDeclaringMethod().getDeclaringArkClass().getSignature(),
                    NumberCategory.int,
                    new Map<Local, IssueInfo>()
                );
            }
        }
        this.options.getSourceIssueEmitter().emitFieldInitializerIssues();
    }

    public checkStmtContainsNumericLiteral(stmt: Stmt): void {
        const res = new Map<Local, IssueInfo>();
        this.options.resetCallDepth();

        // 场景1：先判断是否涉及除法运算
        if (stmt instanceof ArkAssignStmt) {
            const leftOp = stmt.getLeftOp();
            const rightOp = stmt.getRightOp();
            if (leftOp instanceof Local && rightOp instanceof ArkNormalBinopExpr && rightOp.getOperator() === NormalBinaryOperator.Division) {
                this.checkDivisionStmtWithNumericLiteral(stmt, leftOp, rightOp, res);
                return;
            }
        }

        // 场景2：非除法运算场景，处理其余涉及整型字面量的场景
        if (!this.isStmtContainsIntLiteral(stmt)) {
            return;
        }
        // 这些类型的语句中的整型字面量无需进一步进行分析，直接返回
        if (stmt instanceof ArkInvokeStmt || stmt instanceof ArkReturnStmt || stmt instanceof ArkIfStmt) {
            return;
        }
        // 除赋值语句外的其余语句类型理论上不应该出现，如果出现日志报错，需要分析日志进行场景补充
        if (!(stmt instanceof ArkAssignStmt)) {
            logger.error(`Need to handle new type of stmt: ${stmt.toString()}, method: ${stmt.getCfg().getDeclaringMethod().getSignature().toString()}`);
            return;
        }

        this.checkAssignStmtWithNumericLiteral(stmt, res);
    }

    public checkArrayIndexInStmt(stmt: Stmt): void {
        this.options.getArrayIndexUsageChecker().checkArrayIndexInStmt(stmt);
    }

    public checkAsyncReturnStmts(stmts: Stmt[]): void {
        const res = new Map<Local, IssueInfo>();
        this.options.resetCallDepth();
        for (const stmt of stmts) {
            if (!(stmt instanceof ArkReturnStmt)) {
                continue;
            }
            const returnOp = stmt.getOp();
            if (!Utils.isNearlyNumberType(returnOp.getType())) {
                continue;
            }

            if (returnOp instanceof NumberConstant && !this.options.isNumberConstantActuallyFloat(returnOp)) {
                // 场景1：直接return整型字面量，需要将整型字面量改为浮点型字面量
                this.options.getSourceIssueEmitter().emitAsyncReturnIntConstantIssue(stmt, returnOp);
            } else if (returnOp instanceof Local) {
                // 场景2：检查return变量以及其生命周期内有关联的其他变量，全部需要定义为number
                // 检查入口stmt为local的声明语句，便于查找当前是否已有该变量的issue生成
                const declaringStmt = returnOp.getDeclaringStmt();
                if (declaringStmt === null) {
                    continue;
                }
                this.options.isLocalOnlyUsedAsIntLong(declaringStmt, returnOp, res, NumberCategory.number);
            } else {
                logger.error(
                    `Need to handle new return op type, stmt: ${stmt.toString()}, method: ${stmt.getCfg().getDeclaringMethod().getSignature().toString()}`
                );
            }
        }
        this.options.getSourceIssueEmitter().emitNumericLiteralLocalIssues(res);
        this.options.getSourceIssueEmitter().emitNumericLiteralFieldIssues();
    }

    private checkDivisionStmtWithNumericLiteral(
        stmt: ArkAssignStmt,
        leftOp: Local,
        rightOp: ArkNormalBinopExpr,
        res: Map<Local, IssueInfo>
    ): void {
        if (this.isLocalAssigned2Array(leftOp)) {
            // local为临时变量，用于给数组元素赋值的场景，不在本规则的实现范围内，归另一处的规则开发实现
            return;
        }
        if (!Utils.isNearlyNumberType(leftOp.getType())) {
            // 对左值进行检查决定是否对其添加类型注解int或number，如果不是number相关类型则无需继续进行检查
            return;
        }
        this.options.checkValueOnlyUsedAsIntLong(stmt, stmt.getLeftOp(), res, NumberCategory.number);
        // 因为如果let a10 = a1/2; a10 = a2/3;第1句能判断a10为number，则不会继续后面的检查，所以需要额外对除法表达式的op1和op2进行number类型注解的补充
        this.options.isAbstractExprOnlyUsedAsIntLong(stmt, rightOp, res, NumberCategory.number);
        this.options.getSourceIssueEmitter().emitNumericLiteralLocalIssues(res);
        this.options.getSourceIssueEmitter().emitNumericLiteralFieldIssues();
    }

    private checkAssignStmtWithNumericLiteral(stmt: ArkAssignStmt, res: Map<Local, IssueInfo>): void {
        const leftOp = stmt.getLeftOp();
        const rightOp = stmt.getRightOp();
        if (!(leftOp instanceof Local)) {
            if (leftOp instanceof ArkArrayRef) {
                // 对数组元素的赋值中的整型字面量的检查，不在本规则的实现范围内，归另一处的规则开发实现
                return;
            }
            if (leftOp instanceof AbstractFieldRef) {
                // 对类属性直接使用整型字面量进行赋值，int可以赋值给number，不修改属性的类型，保持number
                return;
            }
            logger.error(`Need to handle leftOp type in assign stmt with non Local type, stmt: ${stmt.toString()}`);
            return;
        }
        if (this.isLocalAssigned2Array(leftOp)) {
            // local为临时变量，用于给数组元素赋值的场景，不在此规则中检查
            return;
        }
        if (!Utils.isNearlyNumberType(leftOp.getType())) {
            // 对左值进行检查决定是否对其添加类型注解int或number，如果不是number相关类型则无需继续进行检查
            return;
        }

        if (rightOp instanceof NumberConstant && !this.options.isNumberConstantActuallyFloat(rightOp)) {
            // 整型字面量直接赋值给左值，判断左值在生命周期内是否仅作为int使用，并且判断左值是否继续赋值给其他变量，其他变量是否也可以定义为int
            if (Number(rightOp.getValue()) >= INT32_BOUNDARY) {
                // 不考虑int32范围外的情况，此处为int32边界值
                return;
            }
            this.checkAllLocalsAroundLocal(stmt, leftOp, res, NumberCategory.int);
        } else if (rightOp instanceof AbstractExpr) {
            // 整型字面量作为表达式的一部分，在赋值语句右边出现
            this.checkAbstractExprWithIntLiteral(stmt, leftOp, rightOp, res, NumberCategory.int);
        } else if (rightOp instanceof ArkArrayRef) {
            // 整型字面量作为数组访问的index，无需做任何处理，直接返回
            return;
        } else {
            logger.error(`Need to handle new rightOp type, stmt: ${stmt.toString()}, method: ${stmt.getCfg().getDeclaringMethod().getSignature().toString()}`);
            return;
        }
        this.options.getSourceIssueEmitter().emitNumericLiteralLocalIssues(res);
        this.options.getSourceIssueEmitter().emitNumericLiteralFieldIssues();
    }

    private isLocalAssigned2Array(local: Local): boolean {
        if (!local.getName().startsWith(TEMP_LOCAL_PREFIX)) {
            return false;
        }
        const usedStmts = local.getUsedStmts();
        for (const stmt of usedStmts) {
            if (!(stmt instanceof ArkAssignStmt)) {
                continue;
            }
            const leftOp = stmt.getLeftOp();
            if (leftOp instanceof ArkArrayRef) {
                // 临时变量赋值给数组元素，不在此规则中检查，例如a[0] = 2/3
                return true;
            }
            if (leftOp instanceof ArkInstanceFieldRef) {
                const base = leftOp.getBase();
                if (base.getType() instanceof TupleType) {
                    // 临时变量赋值给元组元素，不在此规则中检查，例如a[0] = 2/3
                    return true;
                }
            }
        }
        return false;
    }

    private isStmtContainsIntLiteral(stmt: Stmt): boolean {
        const uses = stmt.getUses();
        for (const use of uses) {
            if (use instanceof NumberConstant && !this.options.isNumberConstantActuallyFloat(use)) {
                return true;
            }
        }
        return false;
    }

    private checkAllLocalsAroundLocal(stmt: Stmt, local: Local, hasChecked: Map<Local, IssueInfo>, numberCategory: NumberCategory): void {
        const issueReason = this.options.isLocalOnlyUsedAsIntLong(stmt, local, hasChecked, NumberCategory.int);
        if (issueReason !== IssueReason.OnlyUsedAsIntLong) {
            return;
        }
        // res中是所有赋值给local的值传递链上的所有Local的结果，还需要查找所有由local进行赋值的链上的所有Local的结果
        local.getUsedStmts().forEach(s => {
            if (s instanceof ArkAssignStmt && s.getRightOp() instanceof Local && s.getRightOp() === local) {
                const leftOp = s.getLeftOp();
                if (leftOp instanceof Local) {
                    if (hasChecked.has(leftOp)) {
                        // 对于a = a语句，此处必须判断，否则会死循环
                        return;
                    }
                    this.checkAllLocalsAroundLocal(s, leftOp, hasChecked, numberCategory);
                }
            }
        });
    }

    private checkAbstractExprWithIntLiteral(
        stmt: Stmt,
        leftOp: Local,
        rightOp: AbstractExpr,
        hasChecked: Map<Local, IssueInfo>,
        numberCategory: NumberCategory
    ): void {
        if (rightOp instanceof AbstractInvokeExpr || rightOp instanceof ArkAwaitExpr) {
            // 整型字面量作为函数调用的入参，不继续分析，后续如果有需要可以进一步对调用的函数进行检查，是否能将入参改为int
            return;
        }
        if (rightOp instanceof ArkConditionExpr || rightOp instanceof ArkCastExpr || rightOp instanceof ArkTypeOfExpr) {
            // 整型字面量参与这些表达式的运算，不是直接给number变量赋值，无需继续分析
            return;
        }

        const declaringStmt = leftOp.getDeclaringStmt();
        if (declaringStmt === null) {
            return;
        }

        if (rightOp instanceof ArkUnopExpr) {
            // 整型字面量参与取反一元操作符的运算，得到左值是int还是number，与右边有关
            const operator = rightOp.getOperator();
            if (operator === UnaryOperator.Neg) {
                this.checkAllLocalsAroundLocal(declaringStmt, leftOp, hasChecked, numberCategory);
            }
            return;
        }
        if (rightOp instanceof ArkNormalBinopExpr) {
            this.checkBinaryExprWithIntLiteral(stmt, leftOp, rightOp, hasChecked, numberCategory);
            return;
        }
        logger.error(`Need to handle new type of expr: ${rightOp.toString()}`);
    }

    private checkBinaryExprWithIntLiteral(
        stmt: Stmt,
        leftOp: Local,
        rightOp: ArkNormalBinopExpr,
        hasChecked: Map<Local, IssueInfo>,
        numberCategory: NumberCategory
    ): void {
        const operator = rightOp.getOperator();

        if (operator === NormalBinaryOperator.LogicalAnd || operator === NormalBinaryOperator.LogicalOr) {
            // 整型字面量参与||、&&运算，不会影响左值的类型，不处理，直接退出
            return;
        }

        if (operator === NormalBinaryOperator.Division) {
            const declaringStmt = leftOp.getDeclaringStmt();
            if (declaringStmt !== null) {
                this.checkAllLocalsAroundLocal(declaringStmt, leftOp, hasChecked, numberCategory);
            }
            return;
        }
        if (
            operator === NormalBinaryOperator.Addition ||
            operator === NormalBinaryOperator.Subtraction ||
            operator === NormalBinaryOperator.Multiplication ||
            operator === NormalBinaryOperator.Exponentiation ||
            operator === NormalBinaryOperator.NullishCoalescing
        ) {
            this.checkValueSensitiveBinaryExpr(stmt, leftOp, rightOp, hasChecked, numberCategory);
            return;
        }
        if (
            operator === NormalBinaryOperator.BitwiseAnd ||
            operator === NormalBinaryOperator.BitwiseOr ||
            operator === NormalBinaryOperator.BitwiseXor ||
            operator === NormalBinaryOperator.LeftShift ||
            operator === NormalBinaryOperator.RightShift ||
            operator === NormalBinaryOperator.UnsignedRightShift ||
            operator === NormalBinaryOperator.Remainder
        ) {
            // 位运算与取余运算，左边一定是整型，与右边是否为整型字面量无关，与1.1,1.2也无关，无需处理
            return;
        }
        logger.error(`Need to handle new type of binary operator: ${operator}`);
    }

    private checkValueSensitiveBinaryExpr(
        stmt: Stmt,
        leftOp: Local,
        rightOp: ArkNormalBinopExpr,
        hasChecked: Map<Local, IssueInfo>,
        numberCategory: NumberCategory
    ): void {
        // 整型字面量参与+、-、*、**、??二元运算，左值的类型取决于另外一个操作数的类型，若其为int则左值可以为int，若其为number则左值为number
        const op1Res = this.options.checkValueOnlyUsedAsIntLong(stmt, rightOp.getOp1(), hasChecked, numberCategory);
        const op2Res = this.options.checkValueOnlyUsedAsIntLong(stmt, rightOp.getOp2(), hasChecked, numberCategory);
        if (op1Res === IssueReason.OnlyUsedAsIntLong && op2Res === IssueReason.OnlyUsedAsIntLong) {
            const declaringStmt = leftOp.getDeclaringStmt();
            if (declaringStmt !== null) {
                this.checkAllLocalsAroundLocal(declaringStmt, leftOp, hasChecked, numberCategory);
            }
            return;
        }
        // If the left-hand side value is assigned from a closure, it indicates that the closure is marked as a number,
        // and the outer variables captured by the closure need to be rechecked
        const leftOpDeclaringStmt = leftOp.getDeclaringStmt();
        if (leftOpDeclaringStmt instanceof ArkAssignStmt) {
            const valueFromClosure = leftOpDeclaringStmt.getRightOp();
            if (valueFromClosure instanceof ClosureFieldRef) {
                this.options.isLocalOnlyUsedAsIntLong(stmt, leftOp, hasChecked, NumberCategory.number);
            }
        }
        hasChecked.set(leftOp, { issueReason: IssueReason.UsedWithOtherType, numberCategory: NumberCategory.number });
    }
}
