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
    AbstractRef,
    ArkAssignStmt,
    ArkCastExpr,
    ArkMethod,
    ArkNormalBinopExpr,
    ArkParameterRef,
    ArkUnopExpr,
    ClassSignature,
    ClosureFieldRef,
    Local,
    NormalBinaryOperator,
    Scene,
    Stmt,
    TEMP_LOCAL_PREFIX,
    UnaryOperator,
    Value,
} from 'arkanalyzer/lib';
import Logger, { LOG_MODULE_TYPE } from 'arkanalyzer/lib/utils/logger';
import {
    ArkArrayRef,
    ArkReturnStmt,
    EnumValueType,
} from 'arkanalyzer';
import { ArkAwaitExpr } from 'arkanalyzer/lib/core/base/Expr';
import { NumberConstant } from 'arkanalyzer/lib/core/base/Constant';
import { Language } from 'arkanalyzer/lib/core/model/ArkFile';
import { Utils } from '../../../../../Index';
import { SdkUtils } from '../../../../../utils/common/SDKUtils';
import { BuiltinApiChangeDetector } from '../../providers/builtin/runtime/BuiltinApiChangeDetector';
import { NumericTypeClassifier } from '../../core/NumericTypeClassifier';
import {
    IssueInfo,
    IssueReason,
    NumberCategory,
    RuleCategory,
} from '../../core/NumericSemanticTypes';
import { NumericIssueReporter } from '../../diagnostics/report/NumericIssueReporter';
import { SdkApiChangeDetector } from '../../providers/sdk/SdkApiChangeDetector';
import { NumericLocalReferenceResolver } from './NumericLocalReferenceResolver';

const logger = Logger.getLogger(LOG_MODULE_TYPE.HOMECHECK, 'NumericExpressionUsageAnalyzer');

interface NumericExpressionUsageAnalyzerOptions {
    scene: Scene;
    getIssueReporter(): NumericIssueReporter;
    getBuiltinApiChangeDetector(): BuiltinApiChangeDetector;
    getSdkApiChangeDetector(): SdkApiChangeDetector;
    getNumericTypeClassifier(): NumericTypeClassifier;
    getLocalReferenceResolver(): NumericLocalReferenceResolver;
    checkValueOnlyUsedAsIntLong(stmt: Stmt, value: Value, hasChecked: Map<Local, IssueInfo>, numberCategory: NumberCategory): IssueReason;
    isNumberConstantActuallyFloat(constant: NumberConstant): boolean;
    checkFieldRef(fieldRef: AbstractFieldRef, currentClassSig: ClassSignature, numberCategory: NumberCategory, hasChecked: Map<Local, IssueInfo>): IssueReason;
    checkAllArgsOfParameter(stmt: Stmt, hasChecked: Map<Local, IssueInfo>, numberCategory: NumberCategory): IssueReason;
    checkClosureFieldRef(closureRef: ClosureFieldRef, hasChecked: Map<Local, IssueInfo>, numberCategory: NumberCategory): IssueReason;
}

export class NumericExpressionUsageAnalyzer {
    constructor(private options: NumericExpressionUsageAnalyzerOptions) {}

    public isAbstractExprOnlyUsedAsIntLong(
        stmt: Stmt,
        expr: AbstractExpr,
        hasChecked: Map<Local, IssueInfo>,
        numberCategory: NumberCategory
    ): IssueReason {
        if (expr instanceof ArkNormalBinopExpr) {
            return this.checkBinaryExpr(stmt, expr, hasChecked, numberCategory);
        }
        if (expr instanceof AbstractInvokeExpr) {
            return this.checkInvokeExpr(expr, hasChecked, numberCategory);
        }
        if (expr instanceof ArkAwaitExpr) {
            return IssueReason.UsedWithOtherType;
        }
        if (expr instanceof ArkCastExpr) {
            return this.options.checkValueOnlyUsedAsIntLong(stmt, expr.getOp(), hasChecked, numberCategory);
        }
        if (expr instanceof ArkUnopExpr) {
            return this.checkUnaryExpr(stmt, expr, hasChecked, numberCategory);
        }
        logger.error(`Need to handle new type of expr: ${expr.toString()}`);
        return IssueReason.Other;
    }

    public isAbstractRefOnlyUsedAsIntLong(
        stmt: Stmt,
        ref: AbstractRef,
        hasChecked: Map<Local, IssueInfo>,
        numberCategory: NumberCategory
    ): IssueReason {
        if (ref instanceof ArkArrayRef) {
            return IssueReason.CannotFindAll;
        }
        if (ref instanceof AbstractFieldRef) {
            return this.options.checkFieldRef(ref, stmt.getCfg().getDeclaringMethod().getDeclaringArkClass().getSignature(), numberCategory, hasChecked);
        }
        if (ref instanceof ArkParameterRef) {
            return this.options.checkAllArgsOfParameter(stmt, hasChecked, numberCategory);
        }
        if (ref instanceof ClosureFieldRef) {
            return this.options.checkClosureFieldRef(ref, hasChecked, numberCategory);
        }
        logger.error(`Need to check new type of ref in stmt: ${stmt.toString()}`);
        return IssueReason.Other;
    }

    private checkBinaryExpr(
        stmt: Stmt,
        expr: ArkNormalBinopExpr,
        hasChecked: Map<Local, IssueInfo>,
        numberCategory: NumberCategory
    ): IssueReason {
        if (expr.getOperator() === NormalBinaryOperator.Division) {
            return this.checkDivisionExpr(stmt, expr, hasChecked);
        }
        const isOp1Int = this.options.checkValueOnlyUsedAsIntLong(stmt, expr.getOp1(), hasChecked, numberCategory);
        const isOp2Int = this.options.checkValueOnlyUsedAsIntLong(stmt, expr.getOp2(), hasChecked, numberCategory);
        if (isOp1Int === IssueReason.OnlyUsedAsIntLong && isOp2Int === IssueReason.OnlyUsedAsIntLong) {
            return IssueReason.OnlyUsedAsIntLong;
        }
        if (isOp1Int === IssueReason.UsedWithOtherType || isOp2Int === IssueReason.UsedWithOtherType) {
            return IssueReason.UsedWithOtherType;
        }
        if (isOp1Int === IssueReason.RelatedWithNonETS2 || isOp2Int === IssueReason.RelatedWithNonETS2) {
            return IssueReason.RelatedWithNonETS2;
        }
        if (isOp1Int === IssueReason.CannotFindAll || isOp2Int === IssueReason.CannotFindAll) {
            return IssueReason.CannotFindAll;
        }
        return IssueReason.Other;
    }

    private checkDivisionExpr(stmt: Stmt, expr: ArkNormalBinopExpr, hasChecked: Map<Local, IssueInfo>): IssueReason {
        const op1 = expr.getOp1();
        const op2 = expr.getOp2();
        let fixedEnumOp2Number = false;
        if (op1 instanceof NumberConstant && !this.options.isNumberConstantActuallyFloat(op1)) {
            this.options.getIssueReporter().addIssue(RuleCategory.NumericLiteral, NumberCategory.number, IssueReason.UsedWithOtherType, true, stmt, op1);
        } else if (op1 instanceof Local) {
            this.options.getLocalReferenceResolver().handleGlobalLocal(stmt, op1, hasChecked);
            hasChecked.set(op1, { issueReason: IssueReason.UsedWithOtherType, numberCategory: NumberCategory.number });
            if (op1.getName().startsWith(TEMP_LOCAL_PREFIX) && op1.getType() instanceof EnumValueType) {
                this.options.getIssueReporter().addIssue(RuleCategory.NumericLiteral, NumberCategory.number, IssueReason.UsedWithOtherType, true, stmt, op1);
                fixedEnumOp2Number = true;
            }
            this.checkDivisionWithLocal(op1);
        }
        if (op2 instanceof NumberConstant && !this.options.isNumberConstantActuallyFloat(op2)) {
            this.options.getIssueReporter().addIssue(RuleCategory.NumericLiteral, NumberCategory.number, IssueReason.UsedWithOtherType, true, stmt, op2);
        } else if (op2 instanceof Local) {
            this.options.getLocalReferenceResolver().handleGlobalLocal(stmt, op2, hasChecked);
            hasChecked.set(op2, { issueReason: IssueReason.UsedWithOtherType, numberCategory: NumberCategory.number });
            if (!fixedEnumOp2Number && op2.getName().startsWith(TEMP_LOCAL_PREFIX) && op2.getType() instanceof EnumValueType) {
                this.options.getIssueReporter().addIssue(RuleCategory.NumericLiteral, NumberCategory.number, IssueReason.UsedWithOtherType, true, stmt, op2);
            }
        }
        return IssueReason.UsedWithOtherType;
    }

    private checkInvokeExpr(
        expr: AbstractInvokeExpr,
        hasChecked: Map<Local, IssueInfo>,
        numberCategory: NumberCategory
    ): IssueReason {
        const builtinReturnType = this.options.getBuiltinApiChangeDetector().checkReturnType(expr);
        if (builtinReturnType !== null) {
            return IssueReason.OnlyUsedAsIntLong;
        }
        const method = this.options.scene.getMethod(expr.getMethodSignature());
        if (method === null) {
            logger.trace(`Failed to find method: ${expr.getMethodSignature().toString()}`);
            return IssueReason.Other;
        }
        if (SdkUtils.isMethodFromSdk(method)) {
            return this.checkSdkInvokeExpr(method, expr);
        }
        if (method.getLanguage() !== Language.ARKTS1_2) {
            return IssueReason.RelatedWithNonETS2;
        }
        const returnStmt = method.getReturnStmt();
        for (const s of returnStmt) {
            if (!(s instanceof ArkReturnStmt)) {
                continue;
            }
            const res = this.options.checkValueOnlyUsedAsIntLong(s, s.getOp(), hasChecked, numberCategory);
            if (res !== IssueReason.OnlyUsedAsIntLong) {
                return res;
            }
        }
        return IssueReason.OnlyUsedAsIntLong;
    }

    private checkSdkInvokeExpr(method: ArkMethod, expr: AbstractInvokeExpr): IssueReason {
        const ets2SDKSig = this.options.getSdkApiChangeDetector().getEts2SdkSignatureWithEts1Method(method, expr.getArgs(), false);
        if (ets2SDKSig === null) {
            return IssueReason.UsedWithOtherType;
        }
        const returnType = ets2SDKSig.getType();
        const typeClassifier = this.options.getNumericTypeClassifier();
        if (typeClassifier.isIntType(returnType) || typeClassifier.isLongType(returnType)) {
            return IssueReason.OnlyUsedAsIntLong;
        }
        return IssueReason.UsedWithOtherType;
    }

    private checkUnaryExpr(
        stmt: Stmt,
        expr: ArkUnopExpr,
        hasChecked: Map<Local, IssueInfo>,
        numberCategory: NumberCategory
    ): IssueReason {
        if (expr.getOperator() === UnaryOperator.Neg || expr.getOperator() === UnaryOperator.BitwiseNot) {
            return this.options.checkValueOnlyUsedAsIntLong(stmt, expr.getOp(), hasChecked, numberCategory);
        }
        if (expr.getOperator() === UnaryOperator.LogicalNot) {
            return IssueReason.OnlyUsedAsIntLong;
        }
        logger.error(`Need to handle new type of unary operator: ${expr.getOperator().toString()}`);
        return IssueReason.Other;
    }

    private checkDivisionWithLocal(local: Local): void {
        if (!local.getName().startsWith(TEMP_LOCAL_PREFIX)) {
            return;
        }

        const decl = local.getDeclaringStmt();
        if (!(decl instanceof ArkAssignStmt)) {
            return;
        }

        const rightSide = decl.getRightOp();
        if (!(rightSide instanceof ArkNormalBinopExpr)) {
            return;
        }

        const operator = rightSide.getOperator();
        switch (operator) {
            case NormalBinaryOperator.Division:
            case NormalBinaryOperator.Addition:
            case NormalBinaryOperator.Multiplication:
            case NormalBinaryOperator.Exponentiation:
            case NormalBinaryOperator.Subtraction: {
                const lhs = rightSide.getOp1();
                if (lhs instanceof Local) {
                    this.checkDivisionWithLocal(lhs);
                }
                if (lhs instanceof NumberConstant) {
                    this.options.getIssueReporter().addIssue(RuleCategory.NumericLiteral, NumberCategory.number, IssueReason.UsedWithOtherType, true, decl, lhs);
                    return;
                }
                break;
            }
            default: {
                break;
            }
        }
    }
}
