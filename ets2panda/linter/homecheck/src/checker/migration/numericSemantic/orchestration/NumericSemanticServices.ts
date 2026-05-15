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
    AbstractRef,
    ArkField,
    ArkMethod,
    ArkParameterRef,
    CallGraph,
    ClassSignature,
    ClosureFieldRef,
    DVFGBuilder,
    Local,
    Scene,
    Stmt,
    Value,
} from 'arkanalyzer/lib';
import { DVFG } from 'arkanalyzer/lib/VFG/DVFG';
import { Sdk } from 'arkanalyzer/lib/Config';
import { Rule, Utils } from '../../../../Index';
import { IssueReport } from '../../../../model/Defects';
import { ApiNumberUsageChecker } from '../analysis/api/ApiNumberUsageChecker';
import { BuiltinNumberChangeProvider } from '../providers/builtin/BuiltinNumberChangeProvider';
import { SdkNumberChangeProvider } from '../providers/sdk/SdkNumberChangeProvider';
import { NumericAutofixBuilder } from '../diagnostics/autofix/NumericAutofixBuilder';
import { BuiltinApiChangeDetector } from '../providers/builtin/runtime/BuiltinApiChangeDetector';
import { BuiltinNewArrayArgChecker } from '../providers/builtin/runtime/BuiltinNewArrayArgChecker';
import { NumericFieldUsageAnalyzer } from '../analysis/field/NumericFieldUsageAnalyzer';
import { NumericExpressionUsageAnalyzer } from '../analysis/flow/NumericExpressionUsageAnalyzer';
import { NumericLocalIgnorePolicy } from '../analysis/flow/NumericLocalIgnorePolicy';
import { NumericLocalReferenceResolver } from '../analysis/flow/NumericLocalReferenceResolver';
import { NumericLocalUsageAnalyzer } from '../analysis/flow/NumericLocalUsageAnalyzer';
import { NumericParameterFlowAnalyzer } from '../analysis/flow/NumericParameterFlowAnalyzer';
import { NumericValueUsageAnalyzer } from '../analysis/flow/NumericValueUsageAnalyzer';
import { NumericSourceIssueEmitter } from '../diagnostics/emitters/NumericSourceIssueEmitter';
import { NumericIssueReporter } from '../diagnostics/report/NumericIssueReporter';
import { NumericUsageIssueEmitter } from '../diagnostics/report/NumericUsageIssueEmitter';
import { SdkApiChangeDetector } from '../providers/sdk/SdkApiChangeDetector';
import { NumericArrayIndexUsageChecker } from '../analysis/source/NumericArrayIndexUsageChecker';
import { NumericSourceUsageChecker } from '../analysis/source/NumericSourceUsageChecker';
import { NumericLiteralUtils } from '../core/NumericLiteralUtils';
import { NumericSourceFileProvider } from '../core/NumericSourceFileProvider';
import {
    ApiNumberChangeProvider,
    BUILTIN_DECLARATION_SIGNATURE_MATCH_OPTIONS,
    BuiltinApiRule,
    BuiltinFieldRule,
    IssueInfo,
    IssueReason,
    NumberCategory,
} from '../core/NumericSemanticTypes';
import { NumericTypeClassifier } from '../core/NumericTypeClassifier';

export interface NumericSemanticContext {
    scene: Scene;
    rule: Rule;
    defaultSeverity: number;
    ruleDocPath: string;
    issuesMap: Map<string, IssueReport>;
    getEts2Sdks(): Sdk[] | undefined;
    getEts2SdkScene(): Scene | undefined;
    getBuiltinApiRules(): BuiltinApiRule[];
    getBuiltinFieldRules(): BuiltinFieldRule[];
    getClassFieldRes(): Map<ArkField, IssueInfo>;
    getCallGraph(): CallGraph;
    getDvfg(): DVFG;
    getDvfgBuilder(): DVFGBuilder;
    getVisited(): Set<ArkMethod>;
    resetCallDepth(): void;
    incrementCallDepth(): number;
}

export class NumericSemanticServices {
    private numericTypeClassifier = new NumericTypeClassifier();
    private localReferenceResolver = new NumericLocalReferenceResolver();
    private sdkApiChangeDetector?: SdkApiChangeDetector;
    private builtinApiChangeDetector?: BuiltinApiChangeDetector;
    private autofixBuilder?: NumericAutofixBuilder;
    private issueReporter?: NumericIssueReporter;
    private usageIssueEmitter?: NumericUsageIssueEmitter;
    private apiNumberUsageChecker?: ApiNumberUsageChecker;
    private arrayIndexUsageChecker?: NumericArrayIndexUsageChecker;
    private sourceUsageChecker?: NumericSourceUsageChecker;
    private sourceIssueEmitter?: NumericSourceIssueEmitter;
    private builtinNewArrayArgChecker?: BuiltinNewArrayArgChecker;
    private fieldUsageAnalyzer?: NumericFieldUsageAnalyzer;
    private parameterFlowAnalyzer?: NumericParameterFlowAnalyzer;
    private expressionUsageAnalyzer?: NumericExpressionUsageAnalyzer;
    private localUsageAnalyzer?: NumericLocalUsageAnalyzer;
    private apiNumberChangeProviders?: ApiNumberChangeProvider[];
    private valueUsageAnalyzer?: NumericValueUsageAnalyzer;
    private localIgnorePolicy = new NumericLocalIgnorePolicy();
    private sourceFileProvider = new NumericSourceFileProvider();

    constructor(private context: NumericSemanticContext) {}

    public clearSourceFiles(): void {
        this.sourceFileProvider.clear();
    }

    public getNumericTypeClassifier(): NumericTypeClassifier {
        return this.numericTypeClassifier;
    }

    public getSdkApiChangeDetector(): SdkApiChangeDetector {
        if (!this.sdkApiChangeDetector) {
            this.sdkApiChangeDetector = new SdkApiChangeDetector({
                scene: this.context.scene,
                ets2SdkScene: this.context.getEts2SdkScene(),
                ets2Sdks: this.context.getEts2Sdks(),
                isIntType: type => this.numericTypeClassifier.isIntType(type),
                isLongType: type => this.numericTypeClassifier.isLongType(type),
                isNumberLikeType: type => Utils.isNearlyNumberType(type),
            });
        }
        return this.sdkApiChangeDetector;
    }

    public getBuiltinApiChangeDetector(): BuiltinApiChangeDetector {
        if (!this.builtinApiChangeDetector) {
            this.builtinApiChangeDetector = new BuiltinApiChangeDetector({
                apiRules: this.context.getBuiltinApiRules(),
                fieldRules: this.context.getBuiltinFieldRules(),
                isSignatureMatched: (dynSignature, staSignature) => this.numericTypeClassifier.isEts1NumberEts2IntLongSignatureMatched(
                    dynSignature,
                    staSignature,
                    BUILTIN_DECLARATION_SIGNATURE_MATCH_OPTIONS
                ),
                isIntType: type => this.numericTypeClassifier.isIntType(type),
                isLongType: type => this.numericTypeClassifier.isLongType(type),
                isNumberLikeType: type => Utils.isNearlyNumberType(type),
            });
        }
        return this.builtinApiChangeDetector;
    }

    public getAutofixBuilder(): NumericAutofixBuilder {
        if (!this.autofixBuilder) {
            this.autofixBuilder = new NumericAutofixBuilder({
                scene: this.context.scene,
                getSourceFile: (field, issueStmt) => this.sourceFileProvider.getSourceFile(field, issueStmt),
            });
        }
        return this.autofixBuilder;
    }

    public getIssueReporter(): NumericIssueReporter {
        if (!this.issueReporter) {
            this.issueReporter = new NumericIssueReporter({
                rule: this.context.rule,
                defaultSeverity: this.context.defaultSeverity,
                ruleDocPath: this.context.ruleDocPath,
                issuesMap: this.context.issuesMap,
                getAutofixBuilder: () => this.getAutofixBuilder(),
                getActualIndexPosInStmt: stmt => this.getArrayIndexUsageChecker().getActualIndexPosInStmt(stmt),
            });
        }
        return this.issueReporter;
    }

    public getUsageIssueEmitter(): NumericUsageIssueEmitter {
        if (!this.usageIssueEmitter) {
            this.usageIssueEmitter = new NumericUsageIssueEmitter({
                getIssueReporter: () => this.getIssueReporter(),
                getClassFieldRes: () => this.context.getClassFieldRes(),
                resetCallDepth: () => this.context.resetCallDepth(),
                checkValueOnlyUsedAsIntLong: (stmt, value, hasChecked, numberCategory) =>
                    this.checkValueOnlyUsedAsIntLong(stmt, value, hasChecked, numberCategory),
                shouldIgnoreLocal: local => this.shouldIgnoreLocal(local),
                isNumberLikeValue: value => Utils.isNearlyNumberType(value.getType()),
            });
        }
        return this.usageIssueEmitter;
    }

    public getApiNumberChangeProviders(): ApiNumberChangeProvider[] {
        if (!this.apiNumberChangeProviders) {
            this.apiNumberChangeProviders = [
                new SdkNumberChangeProvider(this.getSdkApiChangeDetector()),
                new BuiltinNumberChangeProvider(this.getBuiltinApiChangeDetector(), this.getIssueReporter()),
            ];
        }
        return this.apiNumberChangeProviders;
    }

    public getApiNumberUsageChecker(): ApiNumberUsageChecker {
        if (!this.apiNumberUsageChecker) {
            this.apiNumberUsageChecker = new ApiNumberUsageChecker({
                getProviders: () => this.getApiNumberChangeProviders(),
                getUsageIssueEmitter: () => this.getUsageIssueEmitter(),
            });
        }
        return this.apiNumberUsageChecker;
    }

    public getSourceUsageChecker(): NumericSourceUsageChecker {
        if (!this.sourceUsageChecker) {
            this.sourceUsageChecker = new NumericSourceUsageChecker({
                getArrayIndexUsageChecker: () => this.getArrayIndexUsageChecker(),
                getSourceIssueEmitter: () => this.getSourceIssueEmitter(),
                resetCallDepth: () => this.context.resetCallDepth(),
                checkValueOnlyUsedAsIntLong: (stmt, value, hasChecked, numberCategory) =>
                    this.checkValueOnlyUsedAsIntLong(stmt, value, hasChecked, numberCategory),
                isLocalOnlyUsedAsIntLong: (stmt, local, hasChecked, numberCategory) =>
                    this.isLocalOnlyUsedAsIntLong(stmt, local, hasChecked, numberCategory),
                isAbstractExprOnlyUsedAsIntLong: (stmt, expr, hasChecked, numberCategory) =>
                    this.isAbstractExprOnlyUsedAsIntLong(stmt, expr, hasChecked, numberCategory),
                checkFieldRef: (fieldRef, currentClassSig, numberCategory, hasChecked) =>
                    this.checkFieldRef(fieldRef, currentClassSig, numberCategory, hasChecked),
                isNumberConstantActuallyFloat: constant => NumericLiteralUtils.isNumberConstantActuallyFloat(constant),
            });
        }
        return this.sourceUsageChecker;
    }

    public getArrayIndexUsageChecker(): NumericArrayIndexUsageChecker {
        if (!this.arrayIndexUsageChecker) {
            this.arrayIndexUsageChecker = new NumericArrayIndexUsageChecker({
                getSourceIssueEmitter: () => this.getSourceIssueEmitter(),
                resetCallDepth: () => this.context.resetCallDepth(),
                checkValueOnlyUsedAsIntLong: (stmt, value, hasChecked, numberCategory) =>
                    this.checkValueOnlyUsedAsIntLong(stmt, value, hasChecked, numberCategory),
                isFloatActuallyInt: constant => NumericLiteralUtils.isFloatActuallyInt(constant),
                isFromParameter: stmt => this.isFromParameter(stmt),
            });
        }
        return this.arrayIndexUsageChecker;
    }

    public getSourceIssueEmitter(): NumericSourceIssueEmitter {
        if (!this.sourceIssueEmitter) {
            this.sourceIssueEmitter = new NumericSourceIssueEmitter({
                getIssueReporter: () => this.getIssueReporter(),
                getClassFieldRes: () => this.context.getClassFieldRes(),
                shouldIgnoreLocal: local => this.shouldIgnoreLocal(local),
            });
        }
        return this.sourceIssueEmitter;
    }

    public getBuiltinNewArrayArgChecker(): BuiltinNewArrayArgChecker {
        if (!this.builtinNewArrayArgChecker) {
            this.builtinNewArrayArgChecker = new BuiltinNewArrayArgChecker({
                getBuiltinApiChangeDetector: () => this.getBuiltinApiChangeDetector(),
                getUsageIssueEmitter: () => this.getUsageIssueEmitter(),
            });
        }
        return this.builtinNewArrayArgChecker;
    }

    public getFieldUsageAnalyzer(): NumericFieldUsageAnalyzer {
        if (!this.fieldUsageAnalyzer) {
            this.fieldUsageAnalyzer = new NumericFieldUsageAnalyzer({
                scene: this.context.scene,
                getClassFieldRes: () => this.context.getClassFieldRes(),
                getBuiltinApiChangeDetector: () => this.getBuiltinApiChangeDetector(),
                getSdkApiChangeDetector: () => this.getSdkApiChangeDetector(),
                checkValueOnlyUsedAsIntLong: (stmt, value, hasChecked, numberCategory) =>
                    this.checkValueOnlyUsedAsIntLong(stmt, value, hasChecked, numberCategory),
                isLocalOnlyUsedAsIntLong: (stmt, local, hasChecked, numberCategory) =>
                    this.isLocalOnlyUsedAsIntLong(stmt, local, hasChecked, numberCategory),
                isClassDerivedFromArray: arkClass => this.localReferenceResolver.isClassDerivedFromArray(arkClass),
            });
        }
        return this.fieldUsageAnalyzer;
    }

    public getParameterFlowAnalyzer(): NumericParameterFlowAnalyzer {
        if (!this.parameterFlowAnalyzer) {
            this.parameterFlowAnalyzer = new NumericParameterFlowAnalyzer({
                cg: this.context.getCallGraph(),
                dvfg: this.context.getDvfg(),
                dvfgBuilder: this.context.getDvfgBuilder(),
                visited: this.context.getVisited(),
                incrementCallDepth: () => this.context.incrementCallDepth(),
                checkValueOnlyUsedAsIntLong: (stmt, value, hasChecked, numberCategory) =>
                    this.checkValueOnlyUsedAsIntLong(stmt, value, hasChecked, numberCategory),
                isLocalOnlyUsedAsIntLong: (stmt, local, hasChecked, numberCategory) =>
                    this.isLocalOnlyUsedAsIntLong(stmt, local, hasChecked, numberCategory),
            });
        }
        return this.parameterFlowAnalyzer;
    }

    public getExpressionUsageAnalyzer(): NumericExpressionUsageAnalyzer {
        if (!this.expressionUsageAnalyzer) {
            this.expressionUsageAnalyzer = new NumericExpressionUsageAnalyzer({
                scene: this.context.scene,
                getIssueReporter: () => this.getIssueReporter(),
                getBuiltinApiChangeDetector: () => this.getBuiltinApiChangeDetector(),
                getSdkApiChangeDetector: () => this.getSdkApiChangeDetector(),
                getNumericTypeClassifier: () => this.numericTypeClassifier,
                getLocalReferenceResolver: () => this.localReferenceResolver,
                checkValueOnlyUsedAsIntLong: (stmt, value, hasChecked, numberCategory) =>
                    this.checkValueOnlyUsedAsIntLong(stmt, value, hasChecked, numberCategory),
                isNumberConstantActuallyFloat: constant => NumericLiteralUtils.isNumberConstantActuallyFloat(constant),
                checkFieldRef: (fieldRef, currentClassSig, numberCategory, hasChecked) =>
                    this.checkFieldRef(fieldRef, currentClassSig, numberCategory, hasChecked),
                checkAllArgsOfParameter: (stmt, hasChecked, numberCategory) =>
                    this.checkAllArgsOfParameter(stmt, hasChecked, numberCategory),
                checkClosureFieldRef: (closureRef, hasChecked, numberCategory) =>
                    this.checkClosureFieldRef(closureRef, hasChecked, numberCategory),
            });
        }
        return this.expressionUsageAnalyzer;
    }

    public getLocalUsageAnalyzer(): NumericLocalUsageAnalyzer {
        if (!this.localUsageAnalyzer) {
            this.localUsageAnalyzer = new NumericLocalUsageAnalyzer({
                scene: this.context.scene,
                getIssueReporter: () => this.getIssueReporter(),
                getBuiltinApiChangeDetector: () => this.getBuiltinApiChangeDetector(),
                getSdkApiChangeDetector: () => this.getSdkApiChangeDetector(),
                getNumericTypeClassifier: () => this.numericTypeClassifier,
                getLocalReferenceResolver: () => this.localReferenceResolver,
                checkValueOnlyUsedAsIntLong: (stmt, value, hasChecked, numberCategory) =>
                    this.checkValueOnlyUsedAsIntLong(stmt, value, hasChecked, numberCategory),
            });
        }
        return this.localUsageAnalyzer;
    }

    public getLocalReferenceResolver(): NumericLocalReferenceResolver {
        return this.localReferenceResolver;
    }

    public checkValueOnlyUsedAsIntLong(stmt: Stmt, value: Value, hasChecked: Map<Local, IssueInfo>, numberCategory: NumberCategory): IssueReason {
        return this.getValueUsageAnalyzer().checkValueOnlyUsedAsIntLong(stmt, value, hasChecked, numberCategory);
    }

    private getValueUsageAnalyzer(): NumericValueUsageAnalyzer {
        if (!this.valueUsageAnalyzer) {
            this.valueUsageAnalyzer = new NumericValueUsageAnalyzer({
                isLocalOnlyUsedAsIntLong: (stmt, local, hasChecked, numberCategory) =>
                    this.isLocalOnlyUsedAsIntLong(stmt, local, hasChecked, numberCategory),
                isAbstractExprOnlyUsedAsIntLong: (stmt, expr, hasChecked, numberCategory) =>
                    this.isAbstractExprOnlyUsedAsIntLong(stmt, expr, hasChecked, numberCategory),
                isAbstractRefOnlyUsedAsIntLong: (stmt, ref, hasChecked, numberCategory) =>
                    this.isAbstractRefOnlyUsedAsIntLong(stmt, ref, hasChecked, numberCategory),
            });
        }
        return this.valueUsageAnalyzer;
    }

    private isLocalOnlyUsedAsIntLong(stmt: Stmt, local: Local, hasChecked: Map<Local, IssueInfo>, numberCategory: NumberCategory): IssueReason {
        return this.getLocalUsageAnalyzer().isLocalOnlyUsedAsIntLong(stmt, local, hasChecked, numberCategory);
    }

    private isAbstractExprOnlyUsedAsIntLong(stmt: Stmt, expr: AbstractExpr, hasChecked: Map<Local, IssueInfo>, numberCategory: NumberCategory): IssueReason {
        return this.getExpressionUsageAnalyzer().isAbstractExprOnlyUsedAsIntLong(stmt, expr, hasChecked, numberCategory);
    }

    private isAbstractRefOnlyUsedAsIntLong(stmt: Stmt, ref: AbstractRef, hasChecked: Map<Local, IssueInfo>, numberCategory: NumberCategory): IssueReason {
        return this.getExpressionUsageAnalyzer().isAbstractRefOnlyUsedAsIntLong(stmt, ref, hasChecked, numberCategory);
    }

    private checkFieldRef(
        fieldRef: AbstractFieldRef,
        currentClassSig: ClassSignature,
        numberCategory: NumberCategory,
        hasChecked: Map<Local, IssueInfo>
    ): IssueReason {
        return this.getFieldUsageAnalyzer().checkFieldRef(fieldRef, currentClassSig, numberCategory, hasChecked);
    }

    private checkAllArgsOfParameter(stmt: Stmt, hasChecked: Map<Local, IssueInfo>, numberCategory: NumberCategory): IssueReason {
        return this.getParameterFlowAnalyzer().checkAllArgsOfParameter(stmt, hasChecked, numberCategory);
    }

    private checkClosureFieldRef(closureRef: ClosureFieldRef, hasChecked: Map<Local, IssueInfo>, numberCategory: NumberCategory): IssueReason {
        return this.getParameterFlowAnalyzer().checkClosureFieldRef(closureRef, hasChecked, numberCategory);
    }

    private shouldIgnoreLocal(local: Local): boolean {
        return this.localIgnorePolicy.shouldIgnoreLocal(local);
    }

    private isFromParameter(stmt: Stmt): ArkParameterRef | undefined {
        return this.getParameterFlowAnalyzer().isFromParameter(stmt);
    }
}
