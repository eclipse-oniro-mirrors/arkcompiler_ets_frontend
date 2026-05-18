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

import { AbstractInvokeExpr, ArkAssignStmt, Local, MethodSignature, Type, Value } from 'arkanalyzer/lib';
import { NumberConstant } from 'arkanalyzer/lib/core/base/Constant';
import {
    BuiltinApiRule,
    INTERNAL_BUILTIN_DECLARATION_PREFIX,
    INTERNAL_SDK_PROJECT_NAME,
    NumberCategory,
} from '../../../core/NumericSemanticTypes';
import { NumericLiteralUtils } from '../../../core/NumericLiteralUtils';

interface BuiltinApiRuleMatcherOptions {
    rules: BuiltinApiRule[];
    isBuiltinApiSignature(methodSignature: MethodSignature): boolean;
    isSignatureMatched(dynSignature: MethodSignature, staSignature: MethodSignature): boolean;
    normalizeClassName(className: string): string;
    isIntType(type: Type): boolean;
    isLongType(type: Type): boolean;
}

export class BuiltinApiRuleMatcher {
    constructor(private options: BuiltinApiRuleMatcherOptions) {}

    public getRuleFromInvokeExpr(invokeExpr: AbstractInvokeExpr): BuiltinApiRule | null {
        return this.getRule(invokeExpr.getMethodSignature(), invokeExpr.getArgs().length, invokeExpr.getArgs());
    }

    public getAmbiguousIntLongArgsFromInvokeExpr(invokeExpr: AbstractInvokeExpr): Set<Value> {
        const ambiguousArgs = new Set<Value>();
        const candidates = this.getTopMatchedCandidates(invokeExpr);
        if (candidates.length <= 1) {
            return ambiguousArgs;
        }
        const args = invokeExpr.getArgs();
        args.forEach((arg, index) => {
            if (this.options.isIntType(arg.getType()) || this.options.isLongType(arg.getType()) || this.getClearIntLongCategoryFromValue(arg)) {
                return;
            }
            const categories = new Set<NumberCategory>();
            candidates.forEach(rule => {
                const category = this.parseNumberCategory(rule.args?.[index]);
                if (this.isIntLongCategory(category)) {
                    categories.add(category);
                }
            });
            if (categories.has(NumberCategory.int) && categories.has(NumberCategory.long)) {
                ambiguousArgs.add(arg);
            }
        });
        return ambiguousArgs;
    }

    public hasAmbiguousIntLongReturnFromInvokeExpr(invokeExpr: AbstractInvokeExpr): boolean {
        const candidates = this.getTopMatchedCandidates(invokeExpr);
        if (candidates.length <= 1) {
            return false;
        }
        const categories = new Set<NumberCategory>();
        candidates.forEach(rule => {
            const category = this.parseNumberCategory(rule.returnType);
            if (this.isIntLongCategory(category)) {
                categories.add(category);
            }
        });
        return categories.has(NumberCategory.int) && categories.has(NumberCategory.long);
    }

    private getRule(methodSignature: MethodSignature, argCount: number, args?: Value[]): BuiltinApiRule | null {
        if (!this.options.isBuiltinApiSignature(methodSignature)) {
            return null;
        }
        const candidates = this.getCandidates(methodSignature, argCount);
        if (candidates.length === 0) {
            return null;
        }
        return this.getTopArgFitCandidates(candidates, args)[0];
    }

    private getCandidates(methodSignature: MethodSignature, argCount: number): BuiltinApiRule[] {
        const className = methodSignature.getDeclaringClassSignature().getClassName();
        const methodName = methodSignature.getMethodSubSignature().getMethodName();
        const paramCount = methodSignature.getMethodSubSignature().getParameters().length;
        const firstFilterCandidates = this.options.rules.filter(rule =>
            this.isClassNameMatched(rule.className, className) &&
            rule.methodName === methodName &&
            this.isRuleMatchedByArgCount(rule, argCount)
        );

        if (paramCount === 0 && this.isInternalBuiltinDeclarationSignature(methodSignature)) {
            return firstFilterCandidates;
        }

        return firstFilterCandidates.filter(rule => this.options.isSignatureMatched(methodSignature, rule.signature));
    }

    private isInternalBuiltinDeclarationSignature(methodSignature: MethodSignature): boolean {
        const fileSignature = methodSignature.getDeclaringClassSignature().getDeclaringFileSignature();
        const projectName = fileSignature.getProjectName();
        const normalizedProjectName = projectName.startsWith('@') ? projectName.substring(1) : projectName;
        return normalizedProjectName === INTERNAL_SDK_PROJECT_NAME &&
            fileSignature.getFileName().startsWith(INTERNAL_BUILTIN_DECLARATION_PREFIX);
    }

    private isRuleMatchedByArgCount(rule: BuiltinApiRule, argCount: number): boolean {
        if (rule.paramCount === undefined) {
            return true;
        }
        if (rule.hasRest) {
            return argCount >= Math.max(0, rule.paramCount - 1);
        }
        return argCount <= rule.paramCount;
    }

    private getTopMatchedCandidates(invokeExpr: AbstractInvokeExpr): BuiltinApiRule[] {
        const methodSignature = invokeExpr.getMethodSignature();
        if (!this.options.isBuiltinApiSignature(methodSignature)) {
            return [];
        }
        const candidates = this.getCandidates(methodSignature, invokeExpr.getArgs().length);
        if (candidates.length <= 1) {
            return candidates;
        }
        return this.getTopArgFitCandidates(candidates, invokeExpr.getArgs());
    }

    private getTopArgFitCandidates(candidates: BuiltinApiRule[], args?: Value[]): BuiltinApiRule[] {
        if (candidates.length <= 1) {
            return candidates;
        }
        const ranks = candidates.map(rule => this.getArgFitRank(rule, args));
        const topRank = Math.max(...ranks);
        return candidates.filter((_, index) => ranks[index] === topRank);
    }

    private getArgFitRank(rule: BuiltinApiRule, args?: Value[]): number {
        if (!args || !rule.args) {
            return 0;
        }
        let rank = 0;
        for (const [indexText, categoryText] of Object.entries(rule.args)) {
            const index = Number(indexText);
            const expectedCategory = this.parseNumberCategory(categoryText);
            const actualCategory = this.getClearIntLongCategoryFromValue(args[index]);
            if (!expectedCategory || !actualCategory) {
                continue;
            }
            rank += expectedCategory === actualCategory ? 2 : -2;
        }
        return rank;
    }

    private getClearIntLongCategoryFromValue(value: Value | undefined): NumberCategory.int | NumberCategory.long | null {
        if (!value) {
            return null;
        }
        if (value instanceof NumberConstant) {
            return this.getClearIntLongCategoryFromNumberConstant(value);
        }
        if (value instanceof Local) {
            const declaringStmt = value.getDeclaringStmt();
            if (declaringStmt instanceof ArkAssignStmt) {
                const rightOp = declaringStmt.getRightOp();
                if (rightOp instanceof NumberConstant) {
                    return this.getClearIntLongCategoryFromNumberConstant(rightOp);
                }
            }
        }
        return null;
    }

    private getClearIntLongCategoryFromNumberConstant(value: NumberConstant): NumberCategory.int | NumberCategory.long | null {
        if (NumericLiteralUtils.isNumberConstantActuallyFloat(value)) {
            const num = Number(value.getValue());
            if (!Number.isNaN(num) && Number.isInteger(num)) {
                return NumberCategory.long;
            }
            return null;
        }
        return NumberCategory.int;
    }

    private isClassNameMatched(configuredClassName: string | string[], actualClassName: string): boolean {
        const normalizedActualName = this.options.normalizeClassName(actualClassName);
        if (Array.isArray(configuredClassName)) {
            return configuredClassName.some(item => this.options.normalizeClassName(item) === normalizedActualName);
        }
        return this.options.normalizeClassName(configuredClassName) === normalizedActualName;
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

    private isIntLongCategory(category: NumberCategory | null | undefined): category is NumberCategory.int | NumberCategory.long {
        return category === NumberCategory.int || category === NumberCategory.long;
    }
}
