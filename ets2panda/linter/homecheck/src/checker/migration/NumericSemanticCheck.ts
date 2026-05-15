/*
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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
    ArkField,
    ArkMethod,
    CallGraph,
    DVFGBuilder,
    Scene,
} from 'arkanalyzer/lib';
import Logger, { LOG_MODULE_TYPE } from 'arkanalyzer/lib/utils/logger';
import { BaseChecker, BaseMetaData } from '../BaseChecker';
import { Defects, MatcherCallback, Rule, Utils } from '../../Index';
import { IssueReport } from '../../model/Defects';
import { DVFG } from 'arkanalyzer/lib/VFG/DVFG';
import { GlobalCallGraphHelper } from './Utils';
import { Language } from 'arkanalyzer/lib/core/model/ArkFile';
import { ArkClass } from 'arkanalyzer';
import { Sdk } from 'arkanalyzer/lib/Config';
import { ModifierType } from 'arkanalyzer/lib/core/model/ArkBaseModel';
import { SdkUtils } from '../../utils/common/SDKUtils';
import { ClassCategory } from 'arkanalyzer/lib/core/model/ArkClass';
import { ApiNumberUsageChecker } from './numericSemantic/analysis/api/ApiNumberUsageChecker';
import { BuiltinDeclarationRuleProvider } from './numericSemantic/providers/builtin/declaration/BuiltinDeclarationRuleProvider';
import { BuiltinNewArrayArgChecker } from './numericSemantic/providers/builtin/runtime/BuiltinNewArrayArgChecker';
import { NumericSemanticServices } from './numericSemantic/orchestration/NumericSemanticServices';
import { NumericTypeClassifier } from './numericSemantic/core/NumericTypeClassifier';
import { NumericSourceUsageChecker } from './numericSemantic/analysis/source/NumericSourceUsageChecker';
import {
    BUILTIN_DECLARATION_SIGNATURE_MATCH_OPTIONS,
    BuiltinApiRule,
    BuiltinFieldRule,
    IssueInfo,
    RuleOptions,
} from './numericSemantic/core/NumericSemanticTypes';

const logger = Logger.getLogger(LOG_MODULE_TYPE.HOMECHECK, 'NumericSemanticCheck');
const gMetaData: BaseMetaData = {
    severity: 1,
    ruleDocPath: '',
    description: '',
};

export class NumericSemanticCheck implements BaseChecker {
    readonly metaData: BaseMetaData = gMetaData;
    public rule: Rule;
    public defects: Defects[] = [];
    public issues: IssueReport[] = [];
    private scene: Scene;
    private ets2Sdks?: Sdk[];
    private ets2SdkScene?: Scene;
    private cg: CallGraph;
    private dvfg: DVFG;
    private dvfgBuilder: DVFGBuilder;
    private visited: Set<ArkMethod> = new Set();
    private callDepth = 0;
    private builtinApiRules: BuiltinApiRule[] = [];
    private builtinFieldRules: BuiltinFieldRule[] = [];
    private classFieldRes: Map<ArkField, IssueInfo> = new Map<ArkField, IssueInfo>();
    private issuesMap: Map<string, IssueReport> = new Map<string, IssueReport>();
    private services?: NumericSemanticServices;

    public registerMatchers(): MatcherCallback[] {
        const matchBuildCb: MatcherCallback = {
            matcher: undefined,
            callback: this.check,
        };
        return [matchBuildCb];
    }

    public check = (scene: Scene): void => {
        this.services = undefined;
        this.scene = scene;
        const ruleOptions = this.rule.option?.[0] as RuleOptions | undefined;
        const builtinDeclarationRules = this.getBuiltinDeclarationRuleProvider(ruleOptions).getDeduplicatedDeclarationRules();
        this.builtinApiRules = ruleOptions?.disableDefaultBuiltinApis ? [] : builtinDeclarationRules.apiRules;
        this.builtinFieldRules = ruleOptions?.disableDefaultBuiltinApis ? [] : builtinDeclarationRules.fieldRules;

        // 为ets2的SDK单独生成scene，用于sdk检查时进行匹配使用，单独scene可以避免与源码的scene进行干扰
        let ets2Sdks = ruleOptions?.ets2Sdks ?? SdkUtils.getEts2SdksWithSdkRelativePath(this.scene.getProjectSdkMap());
        if (ets2Sdks && ets2Sdks.length > 0) {
            this.ets2Sdks = ets2Sdks;
            this.ets2SdkScene = Utils.generateSceneForEts2SDK(ets2Sdks);
        }

        this.cg = GlobalCallGraphHelper.getCGInstance(scene);

        this.dvfg = new DVFG(this.cg);
        this.dvfgBuilder = new DVFGBuilder(this.dvfg, scene);
        this.services = this.createServices();

        for (let arkFile of scene.getFiles()) {
            // 此规则仅对arkts1.2进行检查，仅对要将arkts1.1迁移到arkts1.2的文件进行number转int的检查和自动修复
            if (arkFile.getLanguage() !== Language.ARKTS1_2) {
                continue;
            }
            // 用于记录与issue相关的文件的tsc信息，避免每次新增issue时重复创建，提升性能。每次遍历新文件时清空map，节省内存。
            this.getServices().clearSourceFiles();
            const defaultMethod = arkFile.getDefaultClass().getDefaultArkMethod();
            if (defaultMethod) {
                this.dvfgBuilder.buildForSingleMethod(defaultMethod);
            }
            for (let clazz of arkFile.getClasses()) {
                this.processClass(clazz);
            }
            for (let namespace of arkFile.getAllNamespacesUnderThisFile()) {
                for (let clazz of namespace.getClasses()) {
                    this.processClass(clazz);
                }
            }
        }

        this.issues = Array.from(this.issuesMap.values());
    };

    public processClass(arkClass: ArkClass): void {
        if (arkClass.getCategory() === ClassCategory.ENUM || arkClass.getCategory() === ClassCategory.TYPE_LITERAL) {
            // Enum类型的class不需要处理，仅有statint函数，一定不涉及SDK调用，整型字面量不能进行浮点字面量的修改，也不涉及类型注解修改
            // TYPE_LITERAL类型的class不需要处理，仅作为type使用，该class内无方法，仅有field的定义，且field无初始化语句，仅设定类型
            return;
        }
        this.classFieldRes = new Map<ArkField, IssueInfo>();
        // 查找全部method，包含constructor、%instInit，%statInit等
        for (let mtd of arkClass.getMethods(true)) {
            this.processArkMethod(mtd);
        }
    }

    public processArkMethod(target: ArkMethod): void {
        const stmts = target.getBody()?.getCfg().getStmts() ?? [];
        // 场景1：需要检查的sdk调用语句，该stmt为sink点
        for (const stmt of stmts) {
            try {
                this.getApiNumberUsageChecker().checkInStmt(stmt);
            } catch (e) {
                logger.error(`Error checking sdk called in stmt: ${stmt.toString()}, method: ${target.getSignature().toString()}, error: ${e}`);
            }
        }

        try {
            this.getBuiltinNewArrayArgChecker().checkInMethod(stmts);
        } catch (e) {
            logger.error(`Error checking builtin array constructor args, method: ${target.getSignature().toString()}, error: ${e}`);
        }

        // 场景2：需要检查整型字面量或除法出现的stmt，该stmt为sink点。场景2在场景1之后执行，优先让SDK调用来决定变量的类型为int、long、number，剩余的场景2处理，避免issue之间的冲突
        if (target.isGenerated()) {
            // statInit、instInit等方法不进行检查，不主动对类属性的类型进行检查，因为类属性的使用范围很广，很难找全，仅对涉及的1/2这种进行告警，自动修复为1.0/2.0
            try {
                this.getSourceUsageChecker().checkFieldInitializerWithIntLiteral(target);
            } catch (e) {
                logger.error(`Error checking init method with numeric literal, method: ${target.getSignature().toString()}, error: ${e}`);
            }
        } else {
            for (const stmt of stmts) {
                try {
                    this.getSourceUsageChecker().checkStmtContainsNumericLiteral(stmt);
                } catch (e) {
                    logger.error(
                        `Error checking stmt with numeric literal, stmt: ${stmt.toString()}, method: ${target.getSignature().toString()}, error: ${e}`
                    );
                }
            }
        }

        // 场景3：需要检查array的index，该stmt为sink点
        for (const stmt of stmts) {
            try {
                this.getSourceUsageChecker().checkArrayIndexInStmt(stmt);
            } catch (e) {
                logger.error(`Error checking array index in stmt: ${stmt.toString()}, method: ${target.getSignature().toString()}, error: ${e}`);
            }
        }

        // 场景4：async方法检查返回值，与返回值相关的所有变量为number类型，同时替换之前对变量的修复为int/long的告警
        if (target.containsModifier(ModifierType.ASYNC)) {
            try {
                this.getSourceUsageChecker().checkAsyncReturnStmts(target.getReturnStmt());
            } catch (e) {
                logger.error(`Error checking async method return operands, method: ${target.getSignature().toString()}, error: ${e}`);
            }
        }
    }

    private createServices(): NumericSemanticServices {
        return new NumericSemanticServices({
            scene: this.scene,
            rule: this.rule,
            defaultSeverity: this.metaData.severity,
            ruleDocPath: this.metaData.ruleDocPath,
            issuesMap: this.issuesMap,
            getEts2Sdks: () => this.ets2Sdks,
            getEts2SdkScene: () => this.ets2SdkScene,
            getBuiltinApiRules: () => this.builtinApiRules,
            getBuiltinFieldRules: () => this.builtinFieldRules,
            getClassFieldRes: () => this.classFieldRes,
            getCallGraph: () => this.cg,
            getDvfg: () => this.dvfg,
            getDvfgBuilder: () => this.dvfgBuilder,
            getVisited: () => this.visited,
            resetCallDepth: (): void => {
                this.callDepth = 0;
            },
            incrementCallDepth: () => ++this.callDepth,
        });
    }

    private getServices(): NumericSemanticServices {
        if (!this.services) {
            this.services = this.createServices();
        }
        return this.services;
    }

    private getApiNumberUsageChecker(): ApiNumberUsageChecker {
        return this.getServices().getApiNumberUsageChecker();
    }

    private getSourceUsageChecker(): NumericSourceUsageChecker {
        return this.getServices().getSourceUsageChecker();
    }

    private getBuiltinNewArrayArgChecker(): BuiltinNewArrayArgChecker {
        return this.getServices().getBuiltinNewArrayArgChecker();
    }

    private getBuiltinDeclarationRuleProvider(ruleOptions?: RuleOptions): BuiltinDeclarationRuleProvider {
        const numericTypeClassifier = new NumericTypeClassifier();
        return new BuiltinDeclarationRuleProvider({
            scene: this.scene,
            ruleOptions,
            isSignatureMatched: (dynSignature, staSignature) => numericTypeClassifier.isEts1NumberEts2IntLongSignatureMatched(
                dynSignature,
                staSignature,
                BUILTIN_DECLARATION_SIGNATURE_MATCH_OPTIONS
            ),
            getIntLongCategoryFromType: type => numericTypeClassifier.getIntLongCategoryFromType(type),
        });
    }

}
