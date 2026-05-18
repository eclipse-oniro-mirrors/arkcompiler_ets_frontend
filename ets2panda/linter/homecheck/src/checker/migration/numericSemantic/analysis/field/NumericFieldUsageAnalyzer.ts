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
    AbstractFieldRef,
    ArkAssignStmt,
    ArkField,
    ArkInstanceFieldRef,
    ArkMethod,
    ClassSignature,
    CONSTRUCTOR_NAME,
    Local,
    NamespaceSignature,
    Scene,
    Stmt,
    TEMP_LOCAL_PREFIX,
    Type,
    UnknownType,
    Value,
} from 'arkanalyzer/lib';
import Logger, { LOG_MODULE_TYPE } from 'arkanalyzer/lib/utils/logger';
import {
    ArkClass,
    ArkReturnStmt,
} from 'arkanalyzer';
import { ModifierType } from 'arkanalyzer/lib/core/model/ArkBaseModel';
import { ClassCategory } from 'arkanalyzer/lib/core/model/ArkClass';
import { Language } from 'arkanalyzer/lib/core/model/ArkFile';
import { Utils } from '../../../../../Index';
import { SdkUtils } from '../../../../../utils/common/SDKUtils';
import { BuiltinApiChangeDetector } from '../../providers/builtin/runtime/BuiltinApiChangeDetector';
import { SdkApiChangeDetector } from '../../providers/sdk/SdkApiChangeDetector';
import {
    IssueInfo,
    IssueReason,
    LENGTH_FIELD_NAME,
    NumberCategory,
} from '../../core/NumericSemanticTypes';

const logger = Logger.getLogger(LOG_MODULE_TYPE.HOMECHECK, 'NumericFieldUsageAnalyzer');

interface NumericFieldUsageAnalyzerOptions {
    scene: Scene;
    getClassFieldRes(): Map<ArkField, IssueInfo>;
    getBuiltinApiChangeDetector(): BuiltinApiChangeDetector;
    getSdkApiChangeDetector(): SdkApiChangeDetector;
    checkValueOnlyUsedAsIntLong(stmt: Stmt, value: Value, hasChecked: Map<Local, IssueInfo>, numberCategory: NumberCategory): IssueReason;
    isLocalOnlyUsedAsIntLong(stmt: Stmt, local: Local, hasChecked: Map<Local, IssueInfo>, numberCategory: NumberCategory): IssueReason;
    isClassDerivedFromArray(arkClass: ArkClass): boolean;
}

export class NumericFieldUsageAnalyzer {
    constructor(private options: NumericFieldUsageAnalyzerOptions) {}

    public checkFieldRef(
        fieldRef: AbstractFieldRef,
        currentClassSig: ClassSignature,
        numberCategory: NumberCategory,
        hasChecked: Map<Local, IssueInfo>
    ): IssueReason {
        const builtinFieldType = this.options.getBuiltinApiChangeDetector().checkFieldType(fieldRef);
        if (builtinFieldType && (builtinFieldType === NumberCategory.int || builtinFieldType === NumberCategory.long)) {
            return IssueReason.OnlyUsedAsIntLong;
        }
        if (SdkUtils.isFieldFromSdk(fieldRef)) {
            const ets2FieldType = this.options.getSdkApiChangeDetector().checkFieldType(fieldRef);
            if (ets2FieldType && (ets2FieldType === NumberCategory.int || ets2FieldType === NumberCategory.long)) {
                return IssueReason.OnlyUsedAsIntLong;
            }
            return IssueReason.UsedWithOtherType;
        }

        const refType = fieldRef.getType();
        const fieldBase = fieldRef.getFieldSignature().getDeclaringSignature();
        if (fieldBase instanceof NamespaceSignature) {
            return IssueReason.CannotFindAll;
        }
        const baseClass = this.options.scene.getClass(fieldBase);
        if (baseClass === null) {
            return IssueReason.CannotFindAll;
        }
        const fieldName = fieldRef.getFieldName();
        if (fieldName === LENGTH_FIELD_NAME) {
            if (this.options.isClassDerivedFromArray(baseClass)) {
                return IssueReason.OnlyUsedAsIntLong;
            }
        }
        if (baseClass.getLanguage() !== Language.ARKTS1_2) {
            return IssueReason.RelatedWithNonETS2;
        }
        if (
            baseClass.getCategory() === ClassCategory.ENUM ||
            baseClass.getCategory() === ClassCategory.OBJECT ||
            baseClass.getCategory() === ClassCategory.INTERFACE
        ) {
            return IssueReason.UsedWithOtherType;
        }
        if (baseClass.getSignature().toString() !== currentClassSig.toString()) {
            return IssueReason.CannotFindAll;
        }
        const field = baseClass.getField(fieldRef.getFieldSignature());
        if (field === null) {
            return IssueReason.CannotFindAll;
        }
        return this.checkClassField(field, baseClass, refType, numberCategory, hasChecked);
    }

    private checkClassField(
        field: ArkField,
        baseClass: ArkClass,
        refType: Type,
        numberCategory: NumberCategory,
        hasChecked: Map<Local, IssueInfo>
    ): IssueReason {
        const classFieldRes = this.options.getClassFieldRes();
        const existRes = classFieldRes.get(field);
        if (existRes !== undefined) {
            if (
                existRes.issueReason === IssueReason.OnlyUsedAsIntLong &&
                existRes.numberCategory === NumberCategory.int &&
                numberCategory === NumberCategory.long
            ) {
                classFieldRes.set(field, { issueReason: existRes.issueReason, numberCategory });
            }
            return existRes.issueReason;
        }
        if (!Utils.isNearlyNumberType(refType)) {
            if (refType instanceof UnknownType) {
                const res = IssueReason.CannotFindAll;
                classFieldRes.set(field, { issueReason: res, numberCategory: NumberCategory.number });
                return res;
            }
            const res = IssueReason.UsedWithOtherType;
            classFieldRes.set(field, { issueReason: res, numberCategory: NumberCategory.number });
            return res;
        }
        if (field.containsModifier(ModifierType.PRIVATE)) {
            return this.checkPrivateClassField(field, baseClass, numberCategory, hasChecked);
        }
        const res = IssueReason.CannotFindAll;
        classFieldRes.set(field, { issueReason: res, numberCategory: NumberCategory.number });
        return res;
    }

    private checkPrivateClassField(
        field: ArkField,
        baseClass: ArkClass,
        numberCategory: NumberCategory,
        hasChecked: Map<Local, IssueInfo>
    ): IssueReason {
        const classFieldRes = this.options.getClassFieldRes();
        if (this.fieldWithSetter(field, baseClass) || this.fieldWithGetter(field, baseClass)) {
            const res = IssueReason.CannotFindAll;
            classFieldRes.set(field, { issueReason: res, numberCategory: NumberCategory.number });
            return res;
        }
        if (field.containsModifier(ModifierType.READONLY)) {
            classFieldRes.set(field, { issueReason: IssueReason.OnlyUsedAsIntLong, numberCategory: numberCategory });
            const res = this.checkReadonlyFieldInitializer(field, baseClass, numberCategory, hasChecked);
            this.setFieldCheckResult(field, res, numberCategory);
            return res;
        }
        classFieldRes.set(field, { issueReason: IssueReason.OnlyUsedAsIntLong, numberCategory: numberCategory });
        const res = this.checkPrivateField(field, baseClass, numberCategory, hasChecked);
        this.setFieldCheckResult(field, res, numberCategory);
        return res;
    }

    private setFieldCheckResult(field: ArkField, issueReason: IssueReason, numberCategory: NumberCategory): void {
        this.options.getClassFieldRes().set(field, {
            issueReason,
            numberCategory: issueReason === IssueReason.OnlyUsedAsIntLong ? numberCategory : NumberCategory.number,
        });
    }

    private checkReadonlyFieldInitializer(
        field: ArkField,
        baseClass: ArkClass,
        numberCategory: NumberCategory,
        hasChecked: Map<Local, IssueInfo>
    ): IssueReason {
        const constructorMethod = baseClass.getMethodWithName(CONSTRUCTOR_NAME);
        if (constructorMethod === null) {
            return IssueReason.CannotFindAll;
        }
        const res =
            this.checkReadonlyFieldInitInMethod(field, constructorMethod, numberCategory, hasChecked) ??
            this.checkReadonlyFieldInitInMethod(field, baseClass.getStaticInitMethod(), numberCategory, hasChecked) ??
            this.checkReadonlyFieldInitInMethod(field, baseClass.getInstanceInitMethod(), numberCategory, hasChecked);

        if (res === null) {
            return IssueReason.CannotFindAll;
        }
        return res;
    }

    private checkReadonlyFieldInitInMethod(
        field: ArkField,
        method: ArkMethod,
        numberCategory: NumberCategory,
        hasChecked: Map<Local, IssueInfo>
    ): IssueReason | null {
        const stmts = method.getCfg()?.getStmts();
        if (stmts === undefined) {
            return null;
        }
        for (const stmt of stmts) {
            if (!(stmt instanceof ArkAssignStmt)) {
                continue;
            }
            const leftOp = stmt.getLeftOp();
            if (!(leftOp instanceof AbstractFieldRef)) {
                continue;
            }
            if (leftOp.getFieldName() === field.getName()) {
                const builtinReturnCategory = this.options.getBuiltinApiChangeDetector().checkNestedReturnType(stmt.getRightOp());
                if (builtinReturnCategory === numberCategory) {
                    return IssueReason.OnlyUsedAsIntLong;
                }
                return this.options.checkValueOnlyUsedAsIntLong(stmt, stmt.getRightOp(), hasChecked, numberCategory);
            }
        }
        return null;
    }

    private checkPrivateField(field: ArkField, baseClass: ArkClass, numberCategory: NumberCategory, hasChecked: Map<Local, IssueInfo>): IssueReason {
        const methods = baseClass.getMethods(true);
        for (const method of methods) {
            if (method.getName().startsWith('Set-') || method.getName().startsWith('Get-')) {
                continue;
            }
            const stmts = method.getCfg()?.getStmts();
            if (stmts === undefined) {
                continue;
            }
            for (const stmt of stmts) {
                const res = this.checkFieldUsedInStmt(field, stmt, numberCategory, hasChecked);
                if (res === null) {
                    continue;
                }
                if (res !== IssueReason.OnlyUsedAsIntLong) {
                    return res;
                }
            }
        }
        return IssueReason.OnlyUsedAsIntLong;
    }

    private checkFieldUsedInStmt(field: ArkField, stmt: Stmt, numberCategory: NumberCategory, hasChecked: Map<Local, IssueInfo>): IssueReason | null {
        if (stmt instanceof ArkAssignStmt) {
            return this.checkFieldUsedInAssignStmt(field, stmt, numberCategory, hasChecked);
        }
        const usedFieldRef = stmt.getFieldRef();
        if (usedFieldRef === undefined) {
            return null;
        }
        if (this.isFieldRefMatchArkField(usedFieldRef, field)) {
            return IssueReason.CannotFindAll;
        }
        return null;
    }

    private checkFieldUsedInAssignStmt(
        field: ArkField,
        stmt: ArkAssignStmt,
        numberCategory: NumberCategory,
        hasChecked: Map<Local, IssueInfo>
    ): IssueReason | null {
        const leftOp = stmt.getLeftOp();
        const rightOp = stmt.getRightOp();
        if (leftOp instanceof AbstractFieldRef) {
            return this.checkFieldWritten(field, stmt, leftOp, numberCategory, hasChecked);
        }
        if (rightOp instanceof AbstractFieldRef) {
            return this.checkFieldReadInAssign(field, stmt, rightOp, numberCategory, hasChecked);
        }
        return null;
    }

    private checkFieldWritten(
        field: ArkField,
        stmt: ArkAssignStmt,
        fieldRef: AbstractFieldRef,
        numberCategory: NumberCategory,
        hasChecked: Map<Local, IssueInfo>
    ): IssueReason | null {
        if (!this.isFieldRefMatchArkField(fieldRef, field)) {
            return null;
        }
        const builtinReturnCategory = this.options.getBuiltinApiChangeDetector().checkNestedReturnType(stmt.getRightOp());
        if (builtinReturnCategory === numberCategory) {
            return IssueReason.OnlyUsedAsIntLong;
        }
        return this.options.checkValueOnlyUsedAsIntLong(stmt, stmt.getRightOp(), hasChecked, numberCategory);
    }

    private checkFieldReadInAssign(
        field: ArkField,
        stmt: ArkAssignStmt,
        fieldRef: AbstractFieldRef,
        numberCategory: NumberCategory,
        hasChecked: Map<Local, IssueInfo>
    ): IssueReason | null {
        if (!this.isFieldRefMatchArkField(fieldRef, field)) {
            return null;
        }
        const leftOp = stmt.getLeftOp();
        if (leftOp instanceof Local && leftOp.getName().startsWith(TEMP_LOCAL_PREFIX)) {
            return this.options.isLocalOnlyUsedAsIntLong(stmt, leftOp, hasChecked, numberCategory);
        }
        return IssueReason.OnlyUsedAsIntLong;
    }

    private isFieldRefMatchArkField(fieldRef: AbstractFieldRef, field: ArkField): boolean {
        const refDeclaringSig = fieldRef.getFieldSignature().getDeclaringSignature();
        if (refDeclaringSig instanceof NamespaceSignature) {
            return false;
        }
        if (refDeclaringSig.toString() !== field.getDeclaringArkClass().getSignature().toString()) {
            return false;
        }
        return fieldRef.getFieldName() === field.getName();
    }

    private fieldWithSetter(field: ArkField, baseClass: ArkClass): boolean {
        const methods = baseClass.getMethods();
        for (const method of methods) {
            if (!method.getName().startsWith('Set-')) {
                continue;
            }
            const stmts = method.getCfg()?.getStmts();
            if (stmts === undefined) {
                continue;
            }
            for (const stmt of stmts) {
                if (!(stmt instanceof ArkAssignStmt)) {
                    continue;
                }
                const leftOp = stmt.getLeftOp();
                if (!(leftOp instanceof AbstractFieldRef)) {
                    continue;
                }
                if (field.getName() === leftOp.getFieldName()) {
                    return true;
                }
            }
        }
        return false;
    }

    private fieldWithGetter(field: ArkField, baseClass: ArkClass): boolean {
        const methods = baseClass.getMethods();
        for (const method of methods) {
            if (!method.getName().startsWith('Get-')) {
                continue;
            }
            const stmts = method.getCfg()?.getStmts();
            if (stmts === undefined) {
                continue;
            }
            for (const stmt of stmts) {
                if (this.isFieldReturnedByGetter(field, stmt)) {
                    return true;
                }
            }
        }
        return false;
    }

    private isFieldReturnedByGetter(field: ArkField, stmt: Stmt): boolean {
        if (!(stmt instanceof ArkReturnStmt)) {
            return false;
        }
        const op = stmt.getOp();
        if (!(op instanceof Local)) {
            return false;
        }
        const opDeclaringStmt = op.getDeclaringStmt();
        if (!(opDeclaringStmt instanceof ArkAssignStmt)) {
            return false;
        }
        const rightOp = opDeclaringStmt.getRightOp();
        if (!(rightOp instanceof ArkInstanceFieldRef)) {
            return false;
        }
        return field.getName() === rightOp.getFieldName();
    }
}
