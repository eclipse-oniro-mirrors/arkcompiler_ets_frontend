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
    AbstractInvokeExpr,
    ArkAssignStmt,
    ArkIfStmt,
    ArkInstanceInvokeExpr,
    ArkInvokeStmt,
    ArkNormalBinopExpr,
    ArkReturnStmt,
    BooleanType,
    Local,
    NormalBinaryOperator,
    Scene,
    Stmt,
    Value,
    UnknownType,
} from 'arkanalyzer/lib';
import Logger, { LOG_MODULE_TYPE } from 'arkanalyzer/lib/utils/logger';
import {
    ArkArrayRef,
    EnumValueType,
    UnclearReferenceType,
} from 'arkanalyzer';
import { ModifierType } from 'arkanalyzer/lib/core/model/ArkBaseModel';
import { Language } from 'arkanalyzer/lib/core/model/ArkFile';
import { RuleFix, Utils } from '../../../../../Index';
import { SdkUtils } from '../../../../../utils/common/SDKUtils';
import { BuiltinApiChangeDetector } from '../../providers/builtin/runtime/BuiltinApiChangeDetector';
import { NumericTypeClassifier } from '../../core/NumericTypeClassifier';
import {
    IssueInfo,
    IssueReason,
    NumberCategory,
} from '../../core/NumericSemanticTypes';
import { NumericIssueReporter } from '../../diagnostics/report/NumericIssueReporter';
import { SdkApiChangeDetector } from '../../providers/sdk/SdkApiChangeDetector';
import { NumericLocalReferenceResolver } from './NumericLocalReferenceResolver';

const logger = Logger.getLogger(LOG_MODULE_TYPE.HOMECHECK, 'NumericLocalUsageAnalyzer');

interface NumericLocalUsageAnalyzerOptions {
    scene: Scene;
    getIssueReporter(): NumericIssueReporter;
    getBuiltinApiChangeDetector(): BuiltinApiChangeDetector;
    getSdkApiChangeDetector(): SdkApiChangeDetector;
    getNumericTypeClassifier(): NumericTypeClassifier;
    getLocalReferenceResolver(): NumericLocalReferenceResolver;
    checkValueOnlyUsedAsIntLong(stmt: Stmt, value: Value, hasChecked: Map<Local, IssueInfo>, numberCategory: NumberCategory): IssueReason;
}

export class NumericLocalUsageAnalyzer {
    constructor(private options: NumericLocalUsageAnalyzerOptions) {}

    public isLocalOnlyUsedAsIntLong(stmt: Stmt, local: Local, hasChecked: Map<Local, IssueInfo>, numberCategory: NumberCategory): IssueReason {
        const checkedReason = this.resolveCheckedLocalReason(local, hasChecked, numberCategory);
        if (checkedReason !== null) {
            return checkedReason;
        }

        const existingIssueReason = this.resolveExistingLocalIssueReason(stmt, local, hasChecked, numberCategory);
        if (existingIssueReason !== null) {
            return existingIssueReason;
        }

        const languageReason = this.checkLocalLanguage(stmt, local, hasChecked);
        if (languageReason !== null) {
            return languageReason;
        }

        hasChecked.set(local, { issueReason: IssueReason.Other, numberCategory: NumberCategory.number });

        const resWithLocalType = this.checkResWithLocalType(local, stmt);
        if (resWithLocalType) {
            this.updateLocalTypeCheckResult(local, hasChecked, resWithLocalType, numberCategory);
            return resWithLocalType;
        }

        const declaringStmt = local.getDeclaringStmt();
        if (declaringStmt === null) {
            return this.checkLocalWithoutDeclaringStmt(stmt, local, hasChecked, numberCategory);
        }
        if (this.isExportedDefaultMethodLocal(local, declaringStmt)) {
            hasChecked.set(local, { issueReason: IssueReason.UsedWithOtherType, numberCategory: NumberCategory.number });
            return IssueReason.UsedWithOtherType;
        }

        const checkStmts = this.collectRelatedStmts(stmt, local, declaringStmt);
        const declaringMethod = declaringStmt.getCfg().getDeclaringMethod();
        const anonymousClassCheckRes = this.options.getLocalReferenceResolver().checkLocalUsedInAnonymousClassFieldInitializers(local, declaringMethod);
        if (anonymousClassCheckRes !== IssueReason.OnlyUsedAsIntLong) {
            hasChecked.set(local, { issueReason: anonymousClassCheckRes, numberCategory: NumberCategory.number });
            return anonymousClassCheckRes;
        }

        for (const s of checkStmts) {
            const res = this.checkRelatedStmtForLocal(s, local, hasChecked, numberCategory);
            if (res.issueReason !== IssueReason.OnlyUsedAsIntLong) {
                hasChecked.set(local, res);
                return res.issueReason;
            }
        }
        hasChecked.set(local, { issueReason: IssueReason.OnlyUsedAsIntLong, numberCategory: numberCategory });
        return IssueReason.OnlyUsedAsIntLong;
    }

    private resolveCheckedLocalReason(local: Local, hasChecked: Map<Local, IssueInfo>, numberCategory: NumberCategory): IssueReason | null {
        const currentInfo = hasChecked.get(local);
        if (!currentInfo) {
            return null;
        }
        if (currentInfo.numberCategory === NumberCategory.int && numberCategory === NumberCategory.long) {
            hasChecked.set(local, { issueReason: IssueReason.OnlyUsedAsIntLong, numberCategory: NumberCategory.long });
            return null;
        }
        if (currentInfo.issueReason === IssueReason.OnlyUsedAsIntLong && numberCategory === NumberCategory.number) {
            return null;
        }
        return IssueReason.OnlyUsedAsIntLong;
    }

    private resolveExistingLocalIssueReason(
        stmt: Stmt,
        local: Local,
        hasChecked: Map<Local, IssueInfo>,
        numberCategory: NumberCategory
    ): IssueReason | null {
        const currentIssue = this.options.getIssueReporter().getLocalIssue(local, stmt);
        if (!(currentIssue?.fix instanceof RuleFix)) {
            return null;
        }
        const issueReason = this.options.getIssueReporter().getIssueReasonFromDefectInfo(currentIssue.defect);
        const issueCategory = this.options.getIssueReporter().getNumberCategoryFromFixInfo(currentIssue.fix);
        if (issueReason === null || issueCategory === null) {
            return null;
        }
        if (issueReason !== IssueReason.OnlyUsedAsIntLong) {
            hasChecked.set(local, { issueReason: issueReason, numberCategory: numberCategory });
            return issueReason;
        }
        if (numberCategory === NumberCategory.long || numberCategory === NumberCategory.int) {
            hasChecked.set(local, { issueReason: issueReason, numberCategory: numberCategory });
            return issueReason;
        }
        return null;
    }

    private checkLocalLanguage(stmt: Stmt, local: Local, hasChecked: Map<Local, IssueInfo>): IssueReason | null {
        if (stmt.getCfg().getDeclaringMethod().getLanguage() === Language.ARKTS1_2) {
            return null;
        }
        hasChecked.set(local, { issueReason: IssueReason.RelatedWithNonETS2, numberCategory: NumberCategory.number });
        return IssueReason.RelatedWithNonETS2;
    }

    private updateLocalTypeCheckResult(
        local: Local,
        hasChecked: Map<Local, IssueInfo>,
        issueReason: IssueReason,
        numberCategory: NumberCategory
    ): void {
        const resultCategory = issueReason === IssueReason.OnlyUsedAsIntLong ? numberCategory : NumberCategory.number;
        hasChecked.set(local, { issueReason, numberCategory: resultCategory });
    }

    private checkResWithLocalType(local: Local, stmt: Stmt): IssueReason | null {
        const localType = local.getType();
        if (Utils.isNearlyNumberType(localType) || localType instanceof BooleanType) {
            return null;
        }
        if (localType instanceof UnknownType || localType instanceof UnclearReferenceType) {
            return this.checkUnknownOrUnclearLocalType(local, stmt);
        }
        if (localType instanceof EnumValueType) {
            return IssueReason.UsedWithOtherType;
        }
        logger.trace(`Local type is not number, local: ${local.getName()}, local type: ${local.getType().getTypeString()}`);
        return IssueReason.UsedWithOtherType;
    }

    private checkUnknownOrUnclearLocalType(local: Local, stmt: Stmt): IssueReason | null {
        if (this.isArrayIndexLocal(stmt, local)) {
            return null;
        }
        return IssueReason.CannotFindAll;
    }

    private isArrayIndexLocal(stmt: Stmt, local: Local): boolean {
        if (!(stmt instanceof ArkAssignStmt)) {
            return false;
        }
        const rightOp = stmt.getRightOp();
        return rightOp instanceof ArkArrayRef && rightOp.getIndex() === local;
    }

    private checkLocalWithoutDeclaringStmt(
        stmt: Stmt,
        local: Local,
        hasChecked: Map<Local, IssueInfo>,
        numberCategory: NumberCategory
    ): IssueReason {
        const declaringMethod = stmt.getCfg().getDeclaringMethod();
        const resolver = this.options.getLocalReferenceResolver();
        const newLocal =
            resolver.getLocalFromOuterMethod(local, declaringMethod) ??
            resolver.getLocalFromGlobal(local, declaringMethod) ??
            resolver.getLocalFromImportInfo(local, declaringMethod);
        if (newLocal === null) {
            logger.error(`Missing declaring stmt, local: ${local.getName()}`);
            return hasChecked.get(local)!.issueReason;
        }
        const declaringStmt = newLocal.getDeclaringStmt();
        if (declaringStmt === null) {
            logger.error(`Missing declaring stmt, local: ${local.getName()}`);
            hasChecked.set(local, { issueReason: IssueReason.CannotFindAll, numberCategory: NumberCategory.number });
            return IssueReason.CannotFindAll;
        }
        hasChecked.delete(local);
        return this.isLocalOnlyUsedAsIntLong(declaringStmt, newLocal, hasChecked, numberCategory);
    }

    private isExportedDefaultMethodLocal(local: Local, declaringStmt: Stmt): boolean {
        const declaringMethod = declaringStmt.getCfg().getDeclaringMethod();
        if (!declaringMethod.isDefaultArkMethod()) {
            return false;
        }
        const exportInfo = declaringMethod.getDeclaringArkFile().getExportInfoBy(local.getName());
        if (exportInfo === undefined) {
            return false;
        }
        return exportInfo.getArkExport() instanceof Local;
    }

    private collectRelatedStmts(stmt: Stmt, local: Local, declaringStmt: Stmt): Stmt[] {
        const checkStmts: Stmt[] = [declaringStmt];
        local.getUsedStmts().forEach(s => {
            if (s !== stmt) {
                checkStmts.push(s);
            }
        });
        declaringStmt
            .getCfg()
            .getStmts()
            .forEach(s => {
                if (s === declaringStmt || !(s instanceof ArkAssignStmt) || s.getLeftOp() !== local) {
                    return;
                }
                checkStmts.push(s);
            });
        return checkStmts;
    }

    private checkRelatedStmtForLocal(stmt: Stmt, local: Local, hasChecked: Map<Local, IssueInfo>, numberCategory: NumberCategory): IssueInfo {
        if (stmt instanceof ArkAssignStmt) {
            return this.checkRelatedAssignStmtForLocal(stmt, local, hasChecked, numberCategory);
        }
        if (stmt instanceof ArkInvokeStmt) {
            return this.checkRelatedInvokeStmtForLocal(stmt, local, hasChecked, numberCategory);
        }
        if (stmt instanceof ArkReturnStmt) {
            return this.checkRelatedReturnStmtForLocal(stmt, numberCategory);
        }
        if (stmt instanceof ArkIfStmt) {
            return this.createOnlyUsedIssueInfo(numberCategory);
        }
        logger.error(`Need to check new type of stmt: ${stmt.toString()}, method: ${stmt.getCfg().getDeclaringMethod().getSignature().toString()}`);
        return this.createNumberUsedIssueInfo(IssueReason.Other);
    }

    private checkRelatedAssignStmtForLocal(stmt: ArkAssignStmt, local: Local, hasChecked: Map<Local, IssueInfo>, numberCategory: NumberCategory): IssueInfo {
        if (stmt.getLeftOp() === local) {
            return this.checkLocalReassignmentStmt(stmt, hasChecked, numberCategory);
        }
        return this.checkLocalUsedInAssignRight(stmt, local, hasChecked, numberCategory);
    }

    private checkLocalReassignmentStmt(stmt: ArkAssignStmt, hasChecked: Map<Local, IssueInfo>, numberCategory: NumberCategory): IssueInfo {
        const builtinReturnCategory = this.options.getBuiltinApiChangeDetector().checkNestedReturnType(stmt.getRightOp());
        if (builtinReturnCategory === numberCategory) {
            return this.createOnlyUsedIssueInfo(numberCategory);
        }

        const issueReason = this.options.checkValueOnlyUsedAsIntLong(stmt, stmt.getRightOp(), hasChecked, numberCategory);
        if (issueReason !== IssueReason.OnlyUsedAsIntLong) {
            return this.createNumberUsedIssueInfo(issueReason);
        }
        if (numberCategory === NumberCategory.number) {
            return { issueReason: IssueReason.UsedWithOtherType, numberCategory };
        }
        return this.createOnlyUsedIssueInfo(numberCategory);
    }

    private checkLocalUsedInAssignRight(stmt: ArkAssignStmt, local: Local, hasChecked: Map<Local, IssueInfo>, numberCategory: NumberCategory): IssueInfo {
        const rightOp = stmt.getRightOp();
        if (rightOp instanceof ArkNormalBinopExpr && rightOp.getOperator() === NormalBinaryOperator.Division) {
            return this.createNumberUsedIssueInfo(IssueReason.UsedWithOtherType);
        }
        if (rightOp instanceof AbstractInvokeExpr) {
            return this.checkLocalUsedAsApiArg(rightOp, local, hasChecked) ??
                this.checkLocalUsedAsUnchangedInvokeArg(rightOp, local, numberCategory);
        }
        return this.createOnlyUsedIssueInfo(numberCategory);
    }

    private checkRelatedInvokeStmtForLocal(stmt: ArkInvokeStmt, local: Local, hasChecked: Map<Local, IssueInfo>, numberCategory: NumberCategory): IssueInfo {
        const invokeExpr = stmt.getInvokeExpr();
        return this.checkLocalUsedAsApiArg(invokeExpr, local, hasChecked) ??
            this.checkLocalUsedAsUnchangedInvokeArg(invokeExpr, local, numberCategory);
    }

    private checkRelatedReturnStmtForLocal(stmt: ArkReturnStmt, numberCategory: NumberCategory): IssueInfo {
        const method = stmt.getCfg().getDeclaringMethod();
        if (method.containsModifier(ModifierType.ASYNC)) {
            return this.createNumberUsedIssueInfo(IssueReason.UsedWithOtherType);
        }
        if (method.getOuterMethod()?.containsModifier(ModifierType.ASYNC)) {
            return this.createNumberUsedIssueInfo(IssueReason.UsedWithOtherType);
        }
        return this.createOnlyUsedIssueInfo(numberCategory);
    }

    private checkLocalUsedAsApiArg(expr: AbstractInvokeExpr, local: Local, hasChecked: Map<Local, IssueInfo>): IssueInfo | null {
        return this.checkLocalUsedAsBuiltinArg(expr, local, hasChecked) ?? this.checkLocalUsedAsSDKArg(expr, local, hasChecked);
    }

    private checkLocalUsedAsUnchangedInvokeArg(expr: AbstractInvokeExpr, local: Local, numberCategory: NumberCategory): IssueInfo {
        if (expr.getArgs().includes(local)) {
            return this.createNumberUsedIssueInfo(IssueReason.UsedWithOtherType);
        }
        return this.createOnlyUsedIssueInfo(numberCategory);
    }

    private createOnlyUsedIssueInfo(numberCategory: NumberCategory): IssueInfo {
        return { issueReason: IssueReason.OnlyUsedAsIntLong, numberCategory };
    }

    private createNumberUsedIssueInfo(issueReason: IssueReason): IssueInfo {
        return { issueReason, numberCategory: NumberCategory.number };
    }

    private checkLocalUsedAsBuiltinArg(expr: AbstractInvokeExpr, local: Local, hasChecked: Map<Local, IssueInfo>): IssueInfo | null {
        const intArgs = this.options.getBuiltinApiChangeDetector().getIntLongArgsFromInvokeExpr(expr);
        if (intArgs === null) {
            return null;
        }
        const category = intArgs.get(local);
        if (!category) {
            return null;
        }
        const currLocal = hasChecked.get(local);
        if (category === NumberCategory.int && currLocal?.numberCategory === NumberCategory.long) {
            return { issueReason: IssueReason.OnlyUsedAsIntLong, numberCategory: NumberCategory.long };
        }
        return { issueReason: IssueReason.OnlyUsedAsIntLong, numberCategory: category };
    }

    private checkLocalUsedAsSDKArg(expr: AbstractInvokeExpr, local: Local, hasChecked: Map<Local, IssueInfo>): IssueInfo | null {
        const method = this.options.scene.getMethod(expr.getMethodSignature());
        if (method === null) {
            if (expr instanceof ArkInstanceInvokeExpr && Utils.isNearlyPrimitiveType(expr.getBase().getType())) {
                return null;
            }
            logger.trace(`Failed to find method: ${expr.getMethodSignature().toString()}`);
            return null;
        }
        const args = expr.getArgs();
        if (SdkUtils.isMethodFromSdk(method)) {
            const ets2SDKSig = this.options.getSdkApiChangeDetector().getEts2SdkSignatureWithEts1Method(method, args, true);
            if (ets2SDKSig === null) {
                return null;
            }
            const argIndex = expr.getArgs().indexOf(local);
            if (argIndex < 0 || argIndex >= expr.getArgs().length) {
                return null;
            }
            const params = ets2SDKSig.getMethodSubSignature().getParameters();
            const currLocal = hasChecked.get(local);
            const typeClassifier = this.options.getNumericTypeClassifier();
            if (typeClassifier.isIntType(params[argIndex].getType())) {
                if (currLocal === undefined) {
                    return { issueReason: IssueReason.OnlyUsedAsIntLong, numberCategory: NumberCategory.int };
                }
                if (currLocal.numberCategory === NumberCategory.long) {
                    return { issueReason: IssueReason.OnlyUsedAsIntLong, numberCategory: NumberCategory.long };
                }
                return { issueReason: IssueReason.OnlyUsedAsIntLong, numberCategory: NumberCategory.int };
            }
            if (typeClassifier.isLongType(params[argIndex].getType())) {
                return { issueReason: IssueReason.OnlyUsedAsIntLong, numberCategory: NumberCategory.long };
            }
        }
        return null;
    }
}
