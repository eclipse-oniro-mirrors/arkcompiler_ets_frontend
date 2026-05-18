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
    AbstractInvokeExpr,
    ArkInstanceFieldRef,
    ArkPtrInvokeExpr,
    ArkInstanceInvokeExpr,
    ArkAssignStmt,
    ClassSignature,
    CONSTRUCTOR_NAME,
    FileSignature,
    Local,
    MethodSignature,
    Type,
    Value,
} from 'arkanalyzer/lib';
import { ArkArrayRef } from 'arkanalyzer';
import { NumberConstant } from 'arkanalyzer/lib/core/base/Constant';
import { SdkUtils as ArkAnalyzerSdkUtils } from 'arkanalyzer/lib/core/common/SdkUtils';
import {
    BuiltinApiRule,
    BuiltinFieldRule,
    ChangedFunctionParamCategory,
    BuiltinNumberChange,
    BuiltinNumberChangePathStep,
    INTERNAL_BUILTIN_DECLARATION_PREFIX,
    INTERNAL_SDK_PROJECT_NAME,
    NumberCategory,
} from '../../../core/NumericSemanticTypes';
import { BuiltinApiRuleMatcher } from './BuiltinApiRuleMatcher';

interface BuiltinReturnAccess {
    invokeExpr: AbstractInvokeExpr;
    steps: BuiltinNumberChangePathStep[];
}

interface BuiltinApiChangeDetectorOptions {
    apiRules: BuiltinApiRule[];
    fieldRules: BuiltinFieldRule[];
    isSignatureMatched(dynSignature: MethodSignature, staSignature: MethodSignature): boolean;
    isIntType(type: Type): boolean;
    isLongType(type: Type): boolean;
    isNumberLikeType(type: Type): boolean;
}

export class BuiltinApiChangeDetector {
    private ruleMatcher?: BuiltinApiRuleMatcher;

    constructor(private options: BuiltinApiChangeDetectorOptions) {}

    public static normalizeClassName(className: string): string {
        if (className.endsWith('Constructor') && className.length > 'Constructor'.length) {
            return className.substring(0, className.length - 'Constructor'.length);
        }
        return className;
    }

    public static getClassNameKey(className: string | string[]): string {
        if (Array.isArray(className)) {
            return className.map(item => BuiltinApiChangeDetector.normalizeClassName(item)).join('|');
        }
        return BuiltinApiChangeDetector.normalizeClassName(className);
    }

    public static isClassNameMatched(configuredClassName: string | string[], actualClassName: string): boolean {
        const normalizedActualName = BuiltinApiChangeDetector.normalizeClassName(actualClassName);
        if (Array.isArray(configuredClassName)) {
            return configuredClassName.some(item => BuiltinApiChangeDetector.normalizeClassName(item) === normalizedActualName);
        }
        return BuiltinApiChangeDetector.normalizeClassName(configuredClassName) === normalizedActualName;
    }

    public getConstructorIntLongArgs(className: string): Map<number, NumberCategory> | null {
        const constructorRules = this.options.apiRules.filter(rule =>
            BuiltinApiChangeDetector.isClassNameMatched(rule.className, className) &&
            rule.methodName === CONSTRUCTOR_NAME
        );
        return this.mergeRuleArgCategories(constructorRules);
    }

    public getNewArraySizeCategory(): NumberCategory | null {
        const rules = this.options.apiRules.filter(rule =>
            BuiltinApiChangeDetector.isClassNameMatched(rule.className, 'Array') &&
            (rule.methodName === CONSTRUCTOR_NAME || rule.methodName === '$_invoke')
        );
        return this.mergeRuleArgCategories(rules)?.get(0) ?? null;
    }

    public getIntLongArgsFromInvokeExpr(invokeExpr: AbstractInvokeExpr): Map<Value, NumberCategory> | null {
        const matcher = this.getRuleMatcher();
        const rule = matcher.getRuleFromInvokeExpr(invokeExpr);
        if (!rule?.args) {
            return null;
        }
        const args = invokeExpr.getArgs();
        const ambiguousArgs = matcher.getAmbiguousIntLongArgsFromInvokeExpr(invokeExpr);
        const res: Map<Value, NumberCategory> = new Map<Value, NumberCategory>();
        args.forEach((arg, index) => {
            if (ambiguousArgs.has(arg)) {
                return;
            }
            const category = this.parseNumberCategory(rule.args?.[index]);
            if (!category || this.isNumberCategoryType(arg.getType(), category)) {
                return;
            }
            res.set(arg, category);
        });
        return res.size === 0 ? null : res;
    }

    public getFunctionReturnCategoriesFromInvokeExpr(invokeExpr: AbstractInvokeExpr): Map<Value, NumberCategory> | null {
        if (invokeExpr instanceof ArkPtrInvokeExpr) {
            return null;
        }
        const rule = this.getRuleMatcher().getRuleFromInvokeExpr(invokeExpr);
        const callbackReturnCategories = this.getCallbackReturnCategories(rule);
        if (!callbackReturnCategories) {
            return null;
        }

        const args = invokeExpr.getArgs();
        const res: Map<Value, NumberCategory> = new Map<Value, NumberCategory>();
        callbackReturnCategories.forEach((category, index) => {
            const arg = args[index];
            if (!arg) {
                return;
            }
            res.set(arg, category);
        });
        return res.size === 0 ? null : res;
    }

    public getFunctionParamCategoriesFromInvokeExpr(invokeExpr: AbstractInvokeExpr): ChangedFunctionParamCategory[] | null {
        if (invokeExpr instanceof ArkPtrInvokeExpr) {
            return null;
        }
        const rule = this.getRuleMatcher().getRuleFromInvokeExpr(invokeExpr);
        const callbackParamCategories = this.getCallbackParamCategories(rule);
        if (!callbackParamCategories) {
            return null;
        }

        const args = invokeExpr.getArgs();
        const res: ChangedFunctionParamCategory[] = [];
        callbackParamCategories.forEach((paramCategories, argIndex) => {
            const callback = args[argIndex];
            if (!callback) {
                return;
            }
            paramCategories.forEach((category, paramIndex) => {
                res.push({ callback, paramIndex, category });
            });
        });
        return res.length === 0 ? null : res;
    }

    public checkReturnType(invokeExpr: AbstractInvokeExpr): NumberCategory | null {
        const matcher = this.getRuleMatcher();
        if (matcher.hasAmbiguousIntLongReturnFromInvokeExpr(invokeExpr)) {
            return null;
        }
        const rule = matcher.getRuleFromInvokeExpr(invokeExpr);
        return this.parseNumberCategory(rule?.returnType);
    }

    public checkNestedReturnType(value: Value): NumberCategory | null {
        const accesses = this.collectReturnAccesses(value, new Set<Local>());
        if (accesses.length === 0) {
            return null;
        }

        const categories = new Set<NumberCategory>();
        for (const access of accesses) {
            const rule = this.getRuleMatcher().getRuleFromInvokeExpr(access.invokeExpr);
            rule?.changes
                ?.filter(change => change.path.root === 'return' && this.arePathStepsEqual(change.path.steps, access.steps))
                .forEach(change => {
                    const category = this.parseNumberCategory(change.category);
                    if (category) {
                        categories.add(category);
                    }
                });
        }
        return categories.size === 1 ? [...categories][0] : null;
    }

    public checkFieldType(fieldRef: AbstractFieldRef): NumberCategory | null {
        const rule = this.getFieldRule(fieldRef);
        const category = this.parseNumberCategory(rule?.type);
        if (!category || !this.options.isNumberLikeType(fieldRef.getType())) {
            return null;
        }
        return category;
    }

    public getAmbiguousIntLongArgsFromInvokeExpr(invokeExpr: AbstractInvokeExpr): Set<Value> {
        return this.getRuleMatcher().getAmbiguousIntLongArgsFromInvokeExpr(invokeExpr);
    }

    public isNumberCategoryType(type: Type, category: NumberCategory): boolean {
        if (category === NumberCategory.int) {
            return this.options.isIntType(type);
        }
        if (category === NumberCategory.long) {
            return this.options.isLongType(type);
        }
        return this.options.isNumberLikeType(type);
    }

    private getRuleMatcher(): BuiltinApiRuleMatcher {
        if (this.ruleMatcher) {
            return this.ruleMatcher;
        }
        this.ruleMatcher = new BuiltinApiRuleMatcher({
            rules: this.options.apiRules,
            isBuiltinApiSignature: methodSignature => this.isBuiltinApiSignature(methodSignature),
            isSignatureMatched: (dynSignature, staSignature) => this.options.isSignatureMatched(dynSignature, staSignature),
            normalizeClassName: className => BuiltinApiChangeDetector.normalizeClassName(className),
            isIntType: type => this.options.isIntType(type),
            isLongType: type => this.options.isLongType(type),
        });
        return this.ruleMatcher;
    }

    private isBuiltinApiSignature(methodSignature: MethodSignature): boolean {
        return this.isBuiltinFileSignature(methodSignature.getDeclaringClassSignature().getDeclaringFileSignature());
    }

    private getFieldRule(fieldRef: AbstractFieldRef): BuiltinFieldRule | null {
        const fieldName = fieldRef.getFieldName();
        const declaringSignature = fieldRef.getFieldSignature().getDeclaringSignature();
        if (!(declaringSignature instanceof ClassSignature) || !this.isBuiltinClassSignature(declaringSignature)) {
            return null;
        }
        const className = declaringSignature.getClassName();
        return this.options.fieldRules.find(item =>
            BuiltinApiChangeDetector.isClassNameMatched(item.className, className) &&
            item.fieldName === fieldName
        ) ?? null;
    }

    private isBuiltinClassSignature(classSignature: ClassSignature): boolean {
        return this.isBuiltinFileSignature(classSignature.getDeclaringFileSignature());
    }

    private isBuiltinFileSignature(fileSignature: FileSignature): boolean {
        const projectName = fileSignature.getProjectName();
        if (projectName === ArkAnalyzerSdkUtils.BUILT_IN_NAME) {
            return true;
        }
        const normalizedProjectName = projectName.startsWith('@') ? projectName.substring(1) : projectName;
        return normalizedProjectName === INTERNAL_SDK_PROJECT_NAME &&
            fileSignature.getFileName().startsWith(INTERNAL_BUILTIN_DECLARATION_PREFIX);
    }

    private collectReturnAccesses(value: Value, visitedLocals: Set<Local>): BuiltinReturnAccess[] {
        if (value instanceof Local) {
            return this.collectReturnAccessesFromLocal(value, visitedLocals);
        }
        if (value instanceof AbstractFieldRef) {
            return this.collectReturnAccessesFromFieldRef(value, visitedLocals);
        }
        if (value instanceof ArkArrayRef) {
            return this.collectReturnAccessesFromArrayRef(value, visitedLocals);
        }
        if (value instanceof AbstractInvokeExpr && this.getRuleMatcher().getRuleFromInvokeExpr(value)) {
            return [{ invokeExpr: value, steps: [] }];
        }
        return [];
    }

    private collectReturnAccessesFromLocal(local: Local, visitedLocals: Set<Local>): BuiltinReturnAccess[] {
        if (visitedLocals.has(local)) {
            return [];
        }
        visitedLocals.add(local);
        const declaringStmt = local.getDeclaringStmt();
        if (!(declaringStmt instanceof ArkAssignStmt)) {
            return [];
        }
        return this.collectReturnAccesses(declaringStmt.getRightOp(), visitedLocals);
    }

    private collectReturnAccessesFromFieldRef(fieldRef: AbstractFieldRef, visitedLocals: Set<Local>): BuiltinReturnAccess[] {
        if (!(fieldRef instanceof ArkInstanceFieldRef)) {
            return [];
        }

        const fieldName = fieldRef.getFieldName();
        if (fieldName === 'value') {
            const iteratorValueAccesses = this.collectIteratorValueAccesses(fieldRef.getBase(), visitedLocals);
            if (iteratorValueAccesses.length > 0) {
                return iteratorValueAccesses;
            }
        }

        const tupleIndex = this.getTupleIndexFromFieldName(fieldName);
        if (tupleIndex === null) {
            return [];
        }
        return this.collectReturnAccesses(fieldRef.getBase(), visitedLocals)
            .map(access => this.appendAccessStep(access, { kind: 'tuple', index: tupleIndex }));
    }

    private collectReturnAccessesFromArrayRef(arrayRef: ArkArrayRef, visitedLocals: Set<Local>): BuiltinReturnAccess[] {
        const baseAccesses = this.collectReturnAccesses(arrayRef.getBase(), visitedLocals);
        const tupleIndex = this.getTupleIndexFromArrayIndex(arrayRef.getIndex());
        if (tupleIndex === null) {
            return baseAccesses.map(access => this.appendAccessStep(access, { kind: 'arrayElement' }));
        }

        return [
            ...baseAccesses.map(access => this.appendAccessStep(access, { kind: 'tuple', index: tupleIndex })),
            ...baseAccesses.map(access => this.appendAccessStep(access, { kind: 'arrayElement' })),
        ];
    }

    private collectIteratorValueAccesses(value: Value, visitedLocals: Set<Local>): BuiltinReturnAccess[] {
        if (!(value instanceof Local)) {
            return [];
        }
        const declaringStmt = value.getDeclaringStmt();
        if (!(declaringStmt instanceof ArkAssignStmt)) {
            return [];
        }
        const rightOp = declaringStmt.getRightOp();
        if (!(rightOp instanceof ArkInstanceInvokeExpr) || rightOp.getMethodSignature().getMethodSubSignature().getMethodName() !== 'next') {
            return [];
        }
        return this.collectReturnAccesses(rightOp.getBase(), visitedLocals)
            .map(access => this.appendAccessStep(access, { kind: 'generic', index: 0 }));
    }

    private appendAccessStep(access: BuiltinReturnAccess, step: BuiltinNumberChangePathStep): BuiltinReturnAccess {
        return {
            invokeExpr: access.invokeExpr,
            steps: [...access.steps, step],
        };
    }

    private getTupleIndexFromFieldName(fieldName: string): number | null {
        const index = Number(fieldName);
        return Number.isInteger(index) && index >= 0 ? index : null;
    }

    private getTupleIndexFromArrayIndex(index: Value): number | null {
        if (!(index instanceof NumberConstant)) {
            return null;
        }
        const value = Number(index.getValue());
        return Number.isInteger(value) && value >= 0 ? value : null;
    }

    private arePathStepsEqual(left: BuiltinNumberChangePathStep[], right: BuiltinNumberChangePathStep[]): boolean {
        if (left.length !== right.length) {
            return false;
        }
        return left.every((step, index) => this.isPathStepEqual(step, right[index]));
    }

    private isPathStepEqual(left: BuiltinNumberChangePathStep, right: BuiltinNumberChangePathStep): boolean {
        if (left.kind !== right.kind) {
            return false;
        }
        if ('index' in left || 'index' in right) {
            return 'index' in left && 'index' in right && left.index === right.index;
        }
        return true;
    }

    private parseNumberCategory(category?: NumberCategory | string): NumberCategory | null {
        if (category === NumberCategory.int) {
            return NumberCategory.int;
        }
        if (category === NumberCategory.long) {
            return NumberCategory.long;
        }
        return null;
    }

    private mergeRuleArgCategories(rules: BuiltinApiRule[]): Map<number, NumberCategory> | null {
        const res = new Map<number, NumberCategory>();
        const ambiguousIndexes = new Set<number>();
        for (const rule of rules) {
            Object.entries(rule.args ?? {}).forEach(([indexText, categoryText]) => {
                const category = this.parseNumberCategory(categoryText);
                if (!category) {
                    return;
                }
                const index = Number(indexText);
                if (!Number.isInteger(index)) {
                    return;
                }
                const currentCategory = res.get(index);
                if (currentCategory && currentCategory !== category) {
                    ambiguousIndexes.add(index);
                    return;
                }
                if (!ambiguousIndexes.has(index)) {
                    res.set(index, category);
                }
            });
        }
        ambiguousIndexes.forEach(index => res.delete(index));
        return res.size > 0 ? res : null;
    }

    private getCallbackParamCategories(rule: BuiltinApiRule | null): Map<number, Map<number, NumberCategory>> | null {
        const changes = rule?.changes?.filter(change => this.isDirectCallbackParamChange(change)) ?? [];
        if (changes.length === 0) {
            return null;
        }
        return this.mergeCallbackParamCategories(changes);
    }

    private isDirectCallbackParamChange(change: BuiltinNumberChange): boolean {
        return change.path.root === 'arg' &&
            change.path.argIndex !== undefined &&
            change.path.steps.length === 1 &&
            change.path.steps[0].kind === 'functionParam' &&
            this.parseNumberCategory(change.category) !== null;
    }

    private mergeCallbackParamCategories(changes: BuiltinNumberChange[]): Map<number, Map<number, NumberCategory>> | null {
        const res = new Map<number, Map<number, NumberCategory>>();
        const ambiguousKeys = new Set<string>();
        changes.forEach(change => {
            const argIndex = change.path.argIndex;
            const paramStep = change.path.steps[0];
            const category = this.parseNumberCategory(change.category);
            if (argIndex === undefined || paramStep.kind !== 'functionParam' || !category) {
                return;
            }
            const key = `${argIndex}#${paramStep.index}`;
            const paramCategories = res.get(argIndex) ?? new Map<number, NumberCategory>();
            const currentCategory = paramCategories.get(paramStep.index);
            if (currentCategory && currentCategory !== category) {
                ambiguousKeys.add(key);
                paramCategories.delete(paramStep.index);
                res.set(argIndex, paramCategories);
                return;
            }
            if (!ambiguousKeys.has(key)) {
                paramCategories.set(paramStep.index, category);
                res.set(argIndex, paramCategories);
            }
        });
        for (const [argIndex, paramCategories] of res.entries()) {
            if (paramCategories.size === 0) {
                res.delete(argIndex);
            }
        }
        return res.size > 0 ? res : null;
    }

    private getCallbackReturnCategories(rule: BuiltinApiRule | null): Map<number, NumberCategory> | null {
        const changes = rule?.changes?.filter(change => this.isDirectCallbackReturnChange(change)) ?? [];
        if (changes.length === 0) {
            return null;
        }
        return this.mergeCallbackReturnCategories(changes);
    }

    private isDirectCallbackReturnChange(change: BuiltinNumberChange): boolean {
        return change.path.root === 'arg' &&
            change.path.argIndex !== undefined &&
            change.path.steps.length === 1 &&
            change.path.steps[0].kind === 'functionReturn' &&
            this.parseNumberCategory(change.category) !== null;
    }

    private mergeCallbackReturnCategories(changes: BuiltinNumberChange[]): Map<number, NumberCategory> | null {
        const res = new Map<number, NumberCategory>();
        const ambiguousIndexes = new Set<number>();
        changes.forEach(change => {
            const index = change.path.argIndex;
            const category = this.parseNumberCategory(change.category);
            if (index === undefined || !category) {
                return;
            }
            const currentCategory = res.get(index);
            if (currentCategory && currentCategory !== category) {
                ambiguousIndexes.add(index);
                return;
            }
            if (!ambiguousIndexes.has(index)) {
                res.set(index, category);
            }
        });
        ambiguousIndexes.forEach(index => res.delete(index));
        return res.size > 0 ? res : null;
    }
}
