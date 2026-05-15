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
    ClassSignature,
    CONSTRUCTOR_NAME,
    FileSignature,
    MethodSignature,
    Type,
    Value,
} from 'arkanalyzer/lib';
import { SdkUtils as ArkAnalyzerSdkUtils } from 'arkanalyzer/lib/core/common/SdkUtils';
import { BuiltinApiRule, BuiltinFieldRule, NumberCategory } from '../../../core/NumericSemanticTypes';
import { BuiltinApiRuleMatcher } from './BuiltinApiRuleMatcher';

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

    public checkReturnType(invokeExpr: AbstractInvokeExpr): NumberCategory | null {
        const matcher = this.getRuleMatcher();
        if (matcher.hasAmbiguousIntLongReturnFromInvokeExpr(invokeExpr)) {
            return null;
        }
        const rule = matcher.getRuleFromInvokeExpr(invokeExpr);
        return this.parseNumberCategory(rule?.returnType);
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
        return fileSignature.getProjectName() === ArkAnalyzerSdkUtils.BUILT_IN_NAME;
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
}
