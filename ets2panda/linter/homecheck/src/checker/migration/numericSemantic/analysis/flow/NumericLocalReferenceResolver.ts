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
    ArkAssignStmt,
    ArkMethod,
    ArkNormalBinopExpr,
    GlobalRef,
    Local,
    NAME_DELIMITER,
    NormalBinaryOperator,
    Stmt,
    Value,
} from 'arkanalyzer/lib';
import Logger, { LOG_MODULE_TYPE } from 'arkanalyzer/lib/utils/logger';
import {
    ArkClass,
} from 'arkanalyzer';
import { ClassCategory } from 'arkanalyzer/lib/core/model/ArkClass';
import {
    IssueInfo,
    IssueReason,
    NumberCategory,
} from '../../core/NumericSemanticTypes';

const logger = Logger.getLogger(LOG_MODULE_TYPE.HOMECHECK, 'NumericLocalReferenceResolver');

export class NumericLocalReferenceResolver {
    public getLocalFromGlobal(local: Local, method: ArkMethod): Local | null {
        const defaultMethod = method.getDeclaringArkFile().getDefaultClass().getDefaultArkMethod();
        if (!defaultMethod) {
            return null;
        }
        const global = defaultMethod.getBody()?.getLocals().get(local.getName());
        if (global) {
            return global;
        }
        return null;
    }

    public getLocalFromImportInfo(local: Local, method: ArkMethod): Local | null {
        const importInfo = method.getDeclaringArkFile().getImportInfoBy(local.getName());
        if (importInfo === undefined) {
            return null;
        }
        const exportInfo = importInfo.getLazyExportInfo();
        if (exportInfo === null) {
            return null;
        }
        const exportLocal = importInfo.getLazyExportInfo()?.getArkExport();
        if (exportLocal === null || exportLocal === undefined) {
            return null;
        }
        if (exportLocal instanceof Local) {
            return exportLocal;
        }
        return null;
    }

    public getLocalFromOuterMethod(local: Local, method: ArkMethod): Local | null {
        const outerMethod = method.getOuterMethod();
        if (outerMethod) {
            const newLocal = outerMethod.getBody()?.getLocals().get(local.getName());
            if (newLocal) {
                if (newLocal.getDeclaringStmt()) {
                    return newLocal;
                }
                return this.getLocalFromOuterMethod(newLocal, outerMethod);
            }
        }

        const clazz = method.getDeclaringArkClass();
        return this.findLocalFromOuterClass(local, clazz);
    }

    public checkLocalUsedInAnonymousClassFieldInitializers(local: Local, containingMethod: ArkMethod): IssueReason {
        const declaringFile = containingMethod.getDeclaringArkFile();
        const localName = local.getName();
        const isValueUsingLocal = (value: Value, name: string): boolean => value instanceof Local && value.getName() === name;

        for (const clazz of declaringFile.getClasses()) {
            if (!clazz.isAnonymousClass()) {
                continue;
            }

            const initMethod = clazz.getInstanceInitMethod() ?? clazz.getStaticInitMethod();
            if (!initMethod) {
                continue;
            }

            const stmts = initMethod.getBody()?.getCfg()?.getStmts() ?? [];
            for (const stmt of stmts) {
                if (!(stmt instanceof ArkAssignStmt)) {
                    continue;
                }

                const rightOp = stmt.getRightOp();
                if (!(rightOp instanceof ArkNormalBinopExpr && rightOp.getOperator() === NormalBinaryOperator.Division)) {
                    continue;
                }

                if (isValueUsingLocal(rightOp.getOp1(), localName) ||
                    isValueUsingLocal(rightOp.getOp2(), localName)) {
                    return IssueReason.UsedWithOtherType;
                }
            }
        }

        return IssueReason.OnlyUsedAsIntLong;
    }

    public handleGlobalLocal(stmt: Stmt, local: Local, hasChecked: Map<Local, IssueInfo>): void {
        if (local.getDeclaringStmt() !== null) {
            return;
        }

        const globals = stmt.getCfg().getDeclaringMethod().getBody()?.getUsedGlobals();
        const global = globals?.get(local.getName());

        if (!(global instanceof GlobalRef)) {
            return;
        }
        const newLocal = global.getRef();
        if (newLocal instanceof Local) {
            hasChecked.set(newLocal, {
                issueReason: IssueReason.UsedWithOtherType,
                numberCategory: NumberCategory.number,
            });
        }
    }

    public isClassDerivedFromArray(arkClass: ArkClass, visited: Set<ArkClass> = new Set()): boolean {
        if (visited.has(arkClass)) {
            return false;
        }
        visited.add(arkClass);

        if (arkClass.getSignature().getDeclaringFileSignature().getProjectName() === 'internalSdk') {
            return true;
        }

        const heritageClasses = arkClass.getAllHeritageClasses();
        if (heritageClasses === null || heritageClasses.length === 0) {
            return false;
        }

        for (const parentClass of heritageClasses) {
            if (this.isClassDerivedFromArray(parentClass, visited)) {
                return true;
            }
        }

        return false;
    }

    private findLocalFromOuterClass(local: Local, objectClass: ArkClass): Local | null {
        if (objectClass.getCategory() !== ClassCategory.INTERFACE && objectClass.getCategory() !== ClassCategory.OBJECT) {
            return null;
        }
        const firstDelimiterIndex = objectClass.getName().indexOf(NAME_DELIMITER);
        const classAndMethodName = objectClass.getName().substring(firstDelimiterIndex + 1);
        const lastDotIndex = classAndMethodName.lastIndexOf('.');
        const className = classAndMethodName.substring(0, lastDotIndex);
        const methodName = classAndMethodName.substring(lastDotIndex + 1);
        const outerClass = objectClass.getDeclaringArkFile().getClassWithName(className);
        if (outerClass === null) {
            logger.error(`Failed to find outer class of anonymous class: ${objectClass.getName()}, outerClass name: ${className}`);
            return null;
        }
        const outerMethod = outerClass.getMethodWithName(methodName) ?? outerClass.getStaticMethodWithName(methodName);
        if (outerMethod === null) {
            logger.error(
                `Failed to find outer method of anonymous class: ${objectClass.getName()}, outerClass name: ${className}, outerMethod name:${methodName}`
            );
            return null;
        }
        const newLocal = outerMethod.getBody()?.getLocals().get(local.getName());
        if (newLocal) {
            const declaringStmt = newLocal.getDeclaringStmt();
            if (declaringStmt) {
                return newLocal;
            }
            return this.getLocalFromOuterMethod(newLocal, outerMethod);
        }
        const globalRef = outerMethod.getBody()?.getUsedGlobals()?.get(local.getName());
        if (globalRef && globalRef instanceof GlobalRef) {
            const ref = globalRef.getRef();
            if (ref && ref instanceof Local) {
                return ref;
            }
            return null;
        }
        if (outerClass.isAnonymousClass()) {
            return this.findLocalFromOuterClass(local, outerClass);
        }
        return null;
    }
}
