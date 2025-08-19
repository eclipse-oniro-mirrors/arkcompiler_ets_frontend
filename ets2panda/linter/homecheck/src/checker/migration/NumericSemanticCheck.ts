/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
    ArkConditionExpr,
    ArkField,
    ArkIfStmt,
    ArkInstanceFieldRef,
    ArkInstanceInvokeExpr,
    ArkInvokeStmt,
    ArkMethod,
    ArkNormalBinopExpr,
    ArkParameterRef,
    ArkTypeOfExpr,
    ArkUnopExpr,
    ArrayType,
    BooleanType,
    CallGraph,
    ClassSignature,
    ClassType,
    ClosureFieldRef,
    CONSTRUCTOR_NAME,
    DVFGBuilder,
    FileSignature,
    FullPosition,
    GlobalRef,
    INSTANCE_INIT_METHOD_NAME,
    LexicalEnvType,
    Local,
    MethodSignature,
    NAME_DELIMITER,
    NamespaceSignature,
    NormalBinaryOperator,
    Scene,
    STATIC_INIT_METHOD_NAME,
    Stmt,
    TEMP_LOCAL_PREFIX,
    Type,
    UnaryOperator,
    UnionType,
    UnknownType,
    Value,
} from 'arkanalyzer/lib';
import Logger, { LOG_MODULE_TYPE } from 'arkanalyzer/lib/utils/logger';
import { BaseChecker, BaseMetaData } from '../BaseChecker';
import { Defects, MatcherCallback, Rule, RuleFix, Utils } from '../../Index';
import { IssueReport } from '../../model/Defects';
import { DVFG, DVFGNode } from 'arkanalyzer/lib/VFG/DVFG';
import { CALL_DEPTH_LIMIT, getLineAndColumn, GlobalCallGraphHelper } from './Utils';
import { Language } from 'arkanalyzer/lib/core/model/ArkFile';
import { NullConstant, NumberConstant, StringConstant, UndefinedConstant } from 'arkanalyzer/lib/core/base/Constant';
import {
    AliasType,
    ArkArrayRef,
    ArkClass,
    ArkFile,
    ArkReturnStmt,
    AstTreeUtils,
    EnumValueType,
    NumberType,
    ts,
    TupleType,
    UnclearReferenceType,
} from 'arkanalyzer';
import { FixUtils } from '../../utils/common/FixUtils';
import { Sdk } from 'arkanalyzer/lib/Config';
import path from 'path';
import { ModifierType } from 'arkanalyzer/lib/core/model/ArkBaseModel';
import { WarnInfo } from '../../utils/common/Utils';
import { SdkUtils } from '../../utils/common/SDKUtils';
import { ClassCategory } from 'arkanalyzer/lib/core/model/ArkClass';
import { ArkAwaitExpr } from 'arkanalyzer/lib/core/base/Expr';

const logger = Logger.getLogger(LOG_MODULE_TYPE.HOMECHECK, 'NumericSemanticCheck');
const gMetaData: BaseMetaData = {
    severity: 1,
    ruleDocPath: '',
    description: '',
};

enum NumberCategory {
    int = 'int',
    long = 'long',
    number = 'number',
}

enum RuleCategory {
    SDKIntType = 'sdk-api-num2int',
    NumericLiteral = 'arkts-numeric-semantic',
    ArrayIndex = 'arkts-array-index-expr-type',
}

enum IssueReason {
    OnlyUsedAsIntLong = 'only-used-as-int-or-long',
    UsedWithOtherType = 'not-only-used-as-int-or-long',
    CannotFindAll = 'cannot-find-all',
    RelatedWithNonETS2 = 'related-with-non-ets2',
    ActuallyIntConstant = 'actually-int-constant',
    Other = 'other',
}

interface IssueInfo {
    issueReason: IssueReason;
    numberCategory: NumberCategory;
}

interface RuleOptions {
    ets2Sdks?: Sdk[];
}

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
    private classFieldRes: Map<ArkField, IssueInfo> = new Map<ArkField, IssueInfo>();
    private issuesMap: Map<string, IssueReport> = new Map<string, IssueReport>();
    private sourceFiles: Map<FileSignature, ts.SourceFile> = new Map<FileSignature, ts.SourceFile>();

    public registerMatchers(): MatcherCallback[] {
        const matchBuildCb: MatcherCallback = {
            matcher: undefined,
            callback: this.check,
        };
        return [matchBuildCb];
    }

    public check = (scene: Scene): void => {
        this.scene = scene;

        // 为ets2的SDK单独生成scene，用于sdk检查时进行匹配使用，单独scene可以避免与源码的scene进行干扰
        let ets2Sdks = (this.rule.option[0] as RuleOptions | undefined)?.ets2Sdks ?? SdkUtils.getEts2SdksWithSdkRelativePath(this.scene.getProjectSdkMap());
        if (ets2Sdks && ets2Sdks.length > 0) {
            this.ets2Sdks = ets2Sdks;
            this.ets2SdkScene = Utils.generateSceneForEts2SDK(ets2Sdks);
        }

        this.cg = GlobalCallGraphHelper.getCGInstance(scene);

        this.dvfg = new DVFG(this.cg);
        this.dvfgBuilder = new DVFGBuilder(this.dvfg, scene);

        for (let arkFile of scene.getFiles()) {
            // 此规则仅对arkts1.2进行检查，仅对要将arkts1.1迁移到arkts1.2的文件进行number转int的检查和自动修复
            if (arkFile.getLanguage() !== Language.ARKTS1_2) {
                continue;
            }
            // 用于记录与issue相关的文件的tsc信息，避免每次新增issue时重复创建，提升性能。每次遍历新文件时清空map，节省内存。
            this.sourceFiles = new Map<FileSignature, ts.SourceFile>();
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
                this.checkSdkArgsInStmt(stmt);
                this.checkSdkReturnValueInStmt(stmt);
                this.checkSdkFieldValueInStmt(stmt);
            } catch (e) {
                logger.error(`Error checking sdk called in stmt: ${stmt.toString()}, method: ${target.getSignature().toString()}, error: ${e}`);
            }
        }

        // 场景2：需要检查整型字面量或除法出现的stmt，该stmt为sink点。场景2在场景1之后执行，优先让SDK调用来决定变量的类型为int、long、number，剩余的场景2处理，避免issue之间的冲突
        if (target.isGenerated()) {
            // statInit、instInit等方法不进行检查，不主动对类属性的类型进行检查，因为类属性的使用范围很广，很难找全，仅对涉及的1/2这种进行告警，自动修复为1.0/2.0
            try {
                this.checkFieldInitializerWithIntLiteral(target);
            } catch (e) {
                logger.error(`Error checking init method with numeric literal, method: ${target.getSignature().toString()}, error: ${e}`);
            }
        } else {
            for (const stmt of stmts) {
                try {
                    this.checkStmtContainsNumericLiteral(stmt);
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
                this.checkArrayIndexInStmt(stmt);
            } catch (e) {
                logger.error(`Error checking array index in stmt: ${stmt.toString()}, method: ${target.getSignature().toString()}, error: ${e}`);
            }
        }
    }

    private checkSdkArgsInStmt(stmt: Stmt): void {
        const intArgs = this.getSDKIntLongArgs(stmt);
        if (intArgs === null || intArgs.size === 0) {
            return;
        }

        // res用于存放检查过程中所有找到的Local变量，记录这些变量是否均仅当做int使用，若是则可以设置成int类型，跨函数场景下可能包含其他method中的Local变量
        const res = new Map<Local, IssueInfo>();
        this.callDepth = 0;
        for (const [arg, category] of intArgs) {
            const issueReason = this.checkValueOnlyUsedAsIntLong(stmt, arg, res, category);
            if (issueReason !== IssueReason.OnlyUsedAsIntLong) {
                this.addIssueReportForSDKArg(RuleCategory.SDKIntType, category, issueReason, true, stmt, arg);
            }
        }
        res.forEach((issueInfo, local) => {
            if (this.shouldIgnoreLocal(local)) {
                return;
            }
            const declaringStmt = local.getDeclaringStmt();
            if (declaringStmt !== null && issueInfo.issueReason === IssueReason.OnlyUsedAsIntLong) {
                this.addIssueReportForSDKArg(
                    RuleCategory.SDKIntType,
                    issueInfo.numberCategory,
                    issueInfo.issueReason,
                    true,
                    declaringStmt,
                    local,
                    undefined,
                    stmt
                );
            }
        });
        this.classFieldRes.forEach((fieldInfo, field) => {
            if (fieldInfo.issueReason === IssueReason.OnlyUsedAsIntLong || fieldInfo.issueReason === IssueReason.UsedWithOtherType) {
                // 如果能明确判断出field是int或非int，则添加类型注解int或number，其他找不全的场景不变
                this.addIssueReportForSDKArg(RuleCategory.SDKIntType, fieldInfo.numberCategory, fieldInfo.issueReason, true, undefined, undefined, field, stmt);
            }
        });
    }

    private checkSdkReturnValueInStmt(stmt: Stmt): void {
        if (!(stmt instanceof ArkAssignStmt)) {
            return;
        }
        const rightOp = stmt.getRightOp();
        if (!(rightOp instanceof AbstractInvokeExpr)) {
            return;
        }
        const numberCategory = this.checkSDKReturnType(rightOp);
        if (!numberCategory) {
            return;
        }

        const res = new Map<Local, IssueInfo>();
        this.callDepth = 0;
        const leftOp = stmt.getLeftOp();
        this.checkValueOnlyUsedAsIntLong(stmt, leftOp, res, numberCategory);
        res.forEach((issueInfo, local) => {
            if (this.shouldIgnoreLocal(local)) {
                return;
            }
            const declaringStmt = local.getDeclaringStmt();
            if (declaringStmt !== null) {
                this.addIssueReportForSDKReturnOrField(
                    RuleCategory.SDKIntType,
                    issueInfo.numberCategory,
                    issueInfo.issueReason,
                    declaringStmt,
                    local,
                    undefined,
                    stmt
                );
            }
        });
        this.classFieldRes.forEach((fieldInfo, field) => {
            if (fieldInfo.issueReason === IssueReason.OnlyUsedAsIntLong || fieldInfo.issueReason === IssueReason.UsedWithOtherType) {
                // 如果能明确判断出field是int或非int，则添加类型注解int或number，其他找不全的场景不变
                this.addIssueReportForSDKReturnOrField(
                    RuleCategory.SDKIntType,
                    fieldInfo.numberCategory,
                    fieldInfo.issueReason,
                    undefined,
                    undefined,
                    field,
                    stmt
                );
            }
        });
    }

    private checkSdkFieldValueInStmt(stmt: Stmt): void {
        if (!(stmt instanceof ArkAssignStmt)) {
            return;
        }
        const rightOp = stmt.getRightOp();
        if (!(rightOp instanceof AbstractFieldRef)) {
            return;
        }
        const numberCategory = this.checkSDKFieldType(rightOp);
        if (!numberCategory) {
            return;
        }
        const res = new Map<Local, IssueInfo>();
        this.callDepth = 0;
        const leftOp = stmt.getLeftOp();
        if (!Utils.isNearlyNumberType(leftOp.getType())) {
            return;
        }
        this.checkValueOnlyUsedAsIntLong(stmt, leftOp, res, numberCategory);
        res.forEach((issueInfo, local) => {
            if (this.shouldIgnoreLocal(local)) {
                return;
            }
            const declaringStmt = local.getDeclaringStmt();
            if (declaringStmt !== null) {
                this.addIssueReportForSDKReturnOrField(
                    RuleCategory.SDKIntType,
                    issueInfo.numberCategory,
                    issueInfo.issueReason,
                    declaringStmt,
                    local,
                    undefined,
                    stmt
                );
            }
        });
        this.classFieldRes.forEach((fieldInfo, field) => {
            if (fieldInfo.issueReason === IssueReason.OnlyUsedAsIntLong || fieldInfo.issueReason === IssueReason.UsedWithOtherType) {
                // 如果能明确判断出field是int或非int，则添加类型注解int或number，其他找不全的场景不变
                this.addIssueReportForSDKReturnOrField(
                    RuleCategory.SDKIntType,
                    fieldInfo.numberCategory,
                    fieldInfo.issueReason,
                    undefined,
                    undefined,
                    field,
                    stmt
                );
            }
        });
    }

    private checkFieldInitializerWithIntLiteral(method: ArkMethod): void {
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
            if (!(leftOp instanceof AbstractFieldRef) || !Utils.isNearlyNumberType(leftOp.getType())) {
                continue;
            }
            const rightOp = stmt.getRightOp();
            if (rightOp instanceof Local && rightOp.getName().startsWith(TEMP_LOCAL_PREFIX)) {
                // 类属性的初始化语句使用Local赋值，且Local为临时变量，则可能涉及除法运算
                // 整型字面量参与除法运算的告警和自动修复信息在检查过程中就已生成，无需在此处额外生成
                this.checkValueOnlyUsedAsIntLong(stmt, rightOp, new Map<Local, IssueInfo>(), NumberCategory.int);
                this.checkFieldRef(
                    leftOp,
                    stmt.getCfg().getDeclaringMethod().getDeclaringArkClass().getSignature(),
                    NumberCategory.int,
                    new Map<Local, IssueInfo>()
                );
            }
            if (rightOp instanceof NumberConstant && !this.isNumberConstantActuallyFloat(rightOp)) {
                this.checkFieldRef(
                    leftOp,
                    stmt.getCfg().getDeclaringMethod().getDeclaringArkClass().getSignature(),
                    NumberCategory.int,
                    new Map<Local, IssueInfo>()
                );
            }
        }
        this.classFieldRes.forEach((fieldInfo, field) => {
            this.addIssueReport(RuleCategory.NumericLiteral, fieldInfo.numberCategory, fieldInfo.issueReason, true, undefined, undefined, field);
        });
    }

    private checkStmtContainsNumericLiteral(stmt: Stmt): void {
        const res = new Map<Local, IssueInfo>();
        this.callDepth = 0;

        // 场景1：先判断是否涉及除法运算
        if (stmt instanceof ArkAssignStmt) {
            const leftOp = stmt.getLeftOp();
            const rightOp = stmt.getRightOp();
            if (leftOp instanceof Local && rightOp instanceof ArkNormalBinopExpr && rightOp.getOperator() === NormalBinaryOperator.Division) {
                if (this.isLocalAssigned2Array(leftOp)) {
                    // local为临时变量，用于给数组元素赋值的场景，不在本规则的实现范围内，归另一处的规则开发实现
                    return;
                }
                if (!Utils.isNearlyNumberType(leftOp.getType())) {
                    // 对左值进行检查决定是否对其添加类型注解int或number，如果不是number相关类型则无需继续进行检查
                    return;
                }
                this.checkValueOnlyUsedAsIntLong(stmt, stmt.getLeftOp(), res, NumberCategory.number);
                // 因为如果let a10 = a1/2; a10 = a2/3;第1句能判断a10为number，则不会继续后面的检查，所以需要额外对除法表达式的op1和op2进行number类型注解的补充
                this.isAbstractExprOnlyUsedAsIntLong(stmt, rightOp, res, NumberCategory.number);
                res.forEach((issueInfo, local) => {
                    if (this.shouldIgnoreLocal(local)) {
                        return;
                    }
                    const declaringStmt = local.getDeclaringStmt();
                    if (declaringStmt === null) {
                        return;
                    }
                    // 无论local的判定结果是什么，均需要进行自动修复类型注解为int或者number
                    this.addIssueReport(RuleCategory.NumericLiteral, issueInfo.numberCategory, issueInfo.issueReason, true, declaringStmt, local);
                });
                this.classFieldRes.forEach((fieldInfo, field) => {
                    if (fieldInfo.issueReason === IssueReason.OnlyUsedAsIntLong || fieldInfo.issueReason === IssueReason.UsedWithOtherType) {
                        // 如果能明确判断出field是int或非int，则添加类型注解int或number，其他找不全的场景不变
                        this.addIssueReport(RuleCategory.NumericLiteral, fieldInfo.numberCategory, fieldInfo.issueReason, true, undefined, undefined, field);
                    }
                });
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
            // local为临时变量，用于给数组元素赋值的场景，不在本规则的实现范围内，归另一处的规则开发实现
            return;
        }
        if (!Utils.isNearlyNumberType(leftOp.getType())) {
            // 对左值进行检查决定是否对其添加类型注解int或number，如果不是number相关类型则无需继续进行检查
            return;
        }

        if (rightOp instanceof NumberConstant && !this.isNumberConstantActuallyFloat(rightOp)) {
            // 整型字面量直接赋值给左值，判断左值在生命周期内是否仅作为int使用，并且判断左值是否继续赋值给其他变量，其他变量是否也可以定义为int
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
        res.forEach((issueInfo, local) => {
            if (this.shouldIgnoreLocal(local)) {
                return;
            }
            const declaringStmt = local.getDeclaringStmt();
            if (declaringStmt === null) {
                return;
            }
            // 无论local的判定结果是什么，均需要进行自动修复类型注解为int或者number
            this.addIssueReport(RuleCategory.NumericLiteral, issueInfo.numberCategory, issueInfo.issueReason, true, declaringStmt, local);
        });
        this.classFieldRes.forEach((fieldInfo, field) => {
            if (fieldInfo.issueReason === IssueReason.OnlyUsedAsIntLong || fieldInfo.issueReason === IssueReason.UsedWithOtherType) {
                // 如果能明确判断出field是int或非int，则添加类型注解int或number，其他找不全的场景不变
                this.addIssueReport(RuleCategory.NumericLiteral, fieldInfo.numberCategory, fieldInfo.issueReason, true, undefined, undefined, field);
            }
        });
    }

    private checkArrayIndexInStmt(stmt: Stmt): void {
        const res = new Map<Local, IssueInfo>();
        this.callDepth = 0;
        const index = this.getIndexValue(stmt);
        if (index === null) {
            return;
        }
        // 对于index为1.0、2.0这种number constant，需要告警并自动修复成1、2
        if (index instanceof NumberConstant && this.isFloatActuallyInt(index)) {
            this.addIssueReport(RuleCategory.ArrayIndex, NumberCategory.number, IssueReason.ActuallyIntConstant, true, stmt, index);
            return;
        }
        const issueReason = this.checkValueOnlyUsedAsIntLong(stmt, index, res, NumberCategory.int);
        if (issueReason !== IssueReason.OnlyUsedAsIntLong) {
            // 若index原先非int，则获取的数组元素应该是undefined，不可以对其进行强转int，否则对原始代码的语义有修改
            this.addIssueReport(RuleCategory.ArrayIndex, NumberCategory.number, issueReason, false, stmt, index);
        }
        res.forEach((issueInfo, local) => {
            if (this.shouldIgnoreLocal(local)) {
                return;
            }
            const declaringStmt = local.getDeclaringStmt();
            if (declaringStmt !== null) {
                this.addIssueReport(RuleCategory.ArrayIndex, issueInfo.numberCategory, issueInfo.issueReason, true, declaringStmt, local);
            }
        });
        this.classFieldRes.forEach((fieldInfo, field) => {
            if (fieldInfo.issueReason === IssueReason.OnlyUsedAsIntLong) {
                // 如果能明确判断出field是int，则添加类型注解int，其他找不全的场景不变
                this.addIssueReport(RuleCategory.ArrayIndex, NumberCategory.int, fieldInfo.issueReason, true, undefined, undefined, field);
            }
        });
    }

    private getFieldRefActualArrayRef(stmt: Stmt): ArkInstanceFieldRef | null {
        const fieldRef = stmt.getFieldRef();
        if (fieldRef === undefined || !(fieldRef instanceof ArkInstanceFieldRef)) {
            return null;
        }
        const fieldBaseType = fieldRef.getBase().getType();
        if (!(fieldBaseType instanceof UnionType)) {
            return null;
        }
        let containArray = false;
        for (const t of fieldBaseType.getTypes()) {
            if (t instanceof ArrayType) {
                containArray = true;
                break;
            }
        }
        if (!containArray) {
            return null;
        }
        const fieldName = fieldRef.getFieldName();
        if (fieldName === 'length') {
            return null;
        }
        return fieldRef;
    }

    private getActualIndexPosInStmt(stmt: Stmt): FullPosition {
        // 处理array定义时为unionType，例如string[] | undefined，array的index访问语句被错误表示成ArkInstanceFieldRef的场景
        // 由于IR表示在此处的能力限制，此处判断unionType中包含arrayType，且fieldRef的名字非length，取第4个操作数的位置为实际index的位置
        const fieldRef = this.getFieldRefActualArrayRef(stmt);
        if (!fieldRef) {
            return FullPosition.DEFAULT;
        }
        const positions = stmt.getOperandOriginalPositions();
        if (!positions || positions.length !== 4) {
            return FullPosition.DEFAULT;
        }
        return positions[3];
    }

    private getIndexValue(stmt: Stmt): Value | null {
        // 处理stmt的uses中的array的index访问语句
        const arrayRef = stmt.getArrayRef();
        if (arrayRef !== undefined) {
            return arrayRef.getIndex();
        }

        // 处理array的index访问出现在赋值语句左边的情况
        if (stmt.getDef() instanceof ArkArrayRef) {
            return (stmt.getDef() as ArkArrayRef).getIndex();
        }

        // 处理array定义时为unionType，例如string[] | undefined，array的index访问语句被错误表示成ArkInstanceFieldRef的场景
        // 由于IR表示在此处的能力限制，此处判断unionType中包含arrayType，且fieldRef的名字非length，且在local或global中能找到同名的情况
        const methodBody = stmt.getCfg().getDeclaringMethod().getBody();
        if (methodBody === undefined) {
            return null;
        }

        const fieldRef = this.getFieldRefActualArrayRef(stmt);
        if (!fieldRef) {
            return null;
        }
        let index = methodBody.getLocals().get(fieldRef.getFieldName());
        if (index !== undefined) {
            return index;
        }
        const global = methodBody.getUsedGlobals()?.get(fieldRef.getFieldName());
        if (global === undefined || !(global instanceof GlobalRef)) {
            return null;
        }
        return global.getRef();
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

    private shouldIgnoreLocal(local: Local): boolean {
        // 临时变量没有源码的定义语句，无需自动修复类型注解
        if (local.getName().startsWith(TEMP_LOCAL_PREFIX)) {
            return true;
        }
        const declaringStmt = local.getDeclaringStmt();
        // 闭包变量的定义在外层函数，在外层函数处修复，无需在此处修复
        if (declaringStmt instanceof ArkAssignStmt && declaringStmt.getRightOp() instanceof ClosureFieldRef) {
            return true;
        }

        // 对于for (const i of arr)这样的写法，不能为i添加类型注解
        if (declaringStmt instanceof ArkAssignStmt) {
            const rightOp = declaringStmt.getRightOp();
            if (!(rightOp instanceof ArkCastExpr)) {
                return false;
            }
            const castOp = rightOp.getOp();
            if (!(castOp instanceof Local)) {
                return false;
            }
            const castOpDeclaring = castOp.getDeclaringStmt();
            if (!(castOpDeclaring instanceof ArkAssignStmt)) {
                return false;
            }
            const castOpRight = castOpDeclaring.getRightOp();
            if (!(castOpRight instanceof ArkInstanceFieldRef)) {
                return false;
            }
            const fieldSig = castOpRight.getFieldSignature();
            if (fieldSig.getFieldName() === 'value') {
                const declaringSig = fieldSig.getDeclaringSignature();
                if (declaringSig instanceof ClassSignature && declaringSig.getClassName() === 'IteratorYieldResult') {
                    return true;
                }
            }
        }
        return false;
    }

    private isStmtContainsIntLiteral(stmt: Stmt): boolean {
        const uses = stmt.getUses();
        for (const use of uses) {
            if (use instanceof NumberConstant && !this.isNumberConstantActuallyFloat(use)) {
                return true;
            }
        }
        return false;
    }

    private checkAllLocalsAroundLocal(stmt: Stmt, local: Local, hasChecked: Map<Local, IssueInfo>, numberCategory: NumberCategory): void {
        const issueReason = this.isLocalOnlyUsedAsIntLong(stmt, local, hasChecked, NumberCategory.int);
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
            const operator = rightOp.getOperator();

            if (operator === NormalBinaryOperator.LogicalAnd || operator === NormalBinaryOperator.LogicalOr) {
                // 整型字面量参与||、&&运算，不会影响左值的类型，不处理，直接退出
                return;
            }

            const op1 = rightOp.getOp1();
            const op2 = rightOp.getOp2();

            if (operator === NormalBinaryOperator.Division) {
                this.checkAllLocalsAroundLocal(declaringStmt, leftOp, hasChecked, numberCategory);
                return;
            }
            if (
                operator === NormalBinaryOperator.Addition ||
                operator === NormalBinaryOperator.Subtraction ||
                operator === NormalBinaryOperator.Multiplication ||
                operator === NormalBinaryOperator.Exponentiation ||
                operator === NormalBinaryOperator.NullishCoalescing
            ) {
                // 整型字面量参与+、-、*、**、??二元运算，左值的类型取决于另外一个操作数的类型，若其为int则左值可以为int，若其为number则左值为number
                const op1Res = this.checkValueOnlyUsedAsIntLong(stmt, op1, hasChecked, numberCategory);
                const op2Res = this.checkValueOnlyUsedAsIntLong(stmt, op2, hasChecked, numberCategory);
                if (op1Res === IssueReason.OnlyUsedAsIntLong && op2Res === IssueReason.OnlyUsedAsIntLong) {
                    this.checkAllLocalsAroundLocal(declaringStmt, leftOp, hasChecked, numberCategory);
                    return;
                }
                hasChecked.set(leftOp, { issueReason: IssueReason.UsedWithOtherType, numberCategory: NumberCategory.number });
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
            return;
        }
        logger.error(`Need to handle new type of expr: ${rightOp.toString()}`);
        return;
    }

    // 语句为sdk的调用且形参有int或long类型，找出所有int类型形参的实参
    private getSDKIntLongArgs(stmt: Stmt): Map<Value, NumberCategory> | null {
        let invokeExpr = stmt.getInvokeExpr();
        if (invokeExpr === undefined) {
            return null;
        }
        const callMethod = this.scene.getMethod(invokeExpr.getMethodSignature());
        if (callMethod === null || !SdkUtils.isMethodFromSdk(callMethod)) {
            return null;
        }

        const args = invokeExpr.getArgs();

        // 根据找到的对应arkts1.1中的SDK接口匹配到对应在arkts1.2中的SDK接口
        const ets2SdkSignature = this.getEts2SdkSignatureWithEts1Method(callMethod, args, true);
        if (ets2SdkSignature === null) {
            return null;
        }
        const params = ets2SdkSignature.getMethodSubSignature().getParameters();
        if (params.length < args.length) {
            return null;
        }
        const res: Map<Value, NumberCategory> = new Map<Value, NumberCategory>();
        args.forEach((arg, index) => {
            if (this.isIntType(params[index].getType()) && !this.isIntType(arg.getType())) {
                res.set(arg, NumberCategory.int);
            } else if (this.isLongType(params[index].getType()) && !this.isLongType(arg.getType())) {
                res.set(arg, NumberCategory.long);
            }
        });
        if (res.size === 0) {
            return null;
        }
        return res;
    }

    private checkSDKReturnType(invokeExpr: AbstractInvokeExpr): NumberCategory | null {
        const callMethod = this.scene.getMethod(invokeExpr.getMethodSignature());
        if (callMethod === null || !SdkUtils.isMethodFromSdk(callMethod)) {
            return null;
        }
        const args = invokeExpr.getArgs();

        // 根据找到的对应arkts1.1中的SDK接口匹配到对应在arkts1.2中的SDK接口
        const ets2SdkSignature = this.getEts2SdkSignatureWithEts1Method(callMethod, args, false);
        if (ets2SdkSignature === null) {
            return null;
        }
        const returnType = ets2SdkSignature.getType();
        if (this.isLongType(returnType)) {
            return NumberCategory.long;
        }
        if (this.isIntType(returnType)) {
            return NumberCategory.int;
        }
        return null;
    }

    private checkSDKFieldType(fieldRef: AbstractFieldRef): NumberCategory | null {
        if (!SdkUtils.isFieldFromSdk(fieldRef) || !Utils.isNearlyNumberType(fieldRef.getType())) {
            return null;
        }
        const ets1SdkFileSig = fieldRef.getFieldSignature().getDeclaringSignature().getDeclaringFileSignature();
        const ets2SdkFileSig = new FileSignature(ets1SdkFileSig.getProjectName(), ets1SdkFileSig.getFileName().replace('.d.ts', '.d.ets'));
        const ets2SdkFileSigBak = new FileSignature(ets1SdkFileSig.getProjectName(), ets1SdkFileSig.getFileName());
        const ets2SdkFile = this.ets2SdkScene?.getFile(ets2SdkFileSig) ?? this.ets2SdkScene?.getFile(ets2SdkFileSigBak);
        if (!ets2SdkFile) {
            return null;
        }
        let ets2Field = SdkUtils.getSdkField(ets2SdkFile, fieldRef);
        if (!ets2Field) {
            return null;
        }
        if (this.isIntType(ets2Field.getType())) {
            return NumberCategory.int;
        }
        if (this.isLongType(ets2Field.getType())) {
            return NumberCategory.long;
        }
        return null;
    }

    private matchEts1NumberEts2IntLongReturnSig(ets2Sigs: MethodSignature[], ets1Sig: MethodSignature): MethodSignature | null {
        const ets1Params = ets1Sig.getMethodSubSignature().getParameters();
        for (const ets2Sig of ets2Sigs) {
            let allParamMatched = true;
            const ets2Params = ets2Sig.getMethodSubSignature().getParameters();
            if (ets2Params.length !== ets1Params.length) {
                continue;
            }
            for (let i = 0; i < ets1Params.length; i++) {
                const ets2ParamType = ets2Params[i].getType();
                const ets1ParamType = ets1Params[i].getType();
                if (
                    ets2ParamType === ets1ParamType ||
                    (ets1ParamType instanceof NumberType && (this.isIntType(ets2ParamType) || this.isLongType(ets2ParamType)))
                ) {
                    continue;
                }
                allParamMatched = false;
                break;
            }
            if (allParamMatched) {
                const returnType = ets2Sig.getType();
                if (this.isLongType(returnType) || this.isIntType(returnType)) {
                    return ets2Sig;
                }
            }
        }
        return null;
    }

    private matchEts1NumberEts2IntLongMethodSig(ets2Sigs: MethodSignature[], ets1Sig: MethodSignature): MethodSignature | null {
        let intSDKMatched: MethodSignature | null = null;
        const ets1Params = ets1Sig.getMethodSubSignature().getParameters();
        for (const ets2Sig of ets2Sigs) {
            let isInt = false;
            let isLong = false;
            const ets2Params = ets2Sig.getMethodSubSignature().getParameters();
            if (ets2Params.length !== ets1Params.length) {
                continue;
            }
            for (let i = 0; i < ets1Params.length; i++) {
                const ets2ParamType = ets2Params[i].getType();
                const ets1ParamType = ets1Params[i].getType();
                if (ets2ParamType === ets1ParamType) {
                    continue;
                }
                if (this.isIntType(ets2ParamType) && ets1ParamType instanceof NumberType) {
                    isInt = true;
                    continue;
                }
                if (this.isLongType(ets2ParamType) && ets1ParamType instanceof NumberType) {
                    isLong = true;
                    continue;
                }
                isInt = false;
                isLong = false;
            }
            if (isLong) {
                return ets2Sig;
            }
            if (isInt) {
                intSDKMatched = ets2Sig;
            }
        }
        return intSDKMatched;
    }

    // checkArg = true is for checking SDK arg with int or long; otherwise is for checking SDK return with int or long
    private getEts2SdkSignatureWithEts1Method(ets1SDK: ArkMethod, args: Value[], checkArg: boolean, exactMatch: boolean = true): MethodSignature | null {
        const ets2Sdks = this.ets2Sdks;
        if (ets2Sdks === undefined || ets2Sdks.length === 0) {
            return null;
        }

        const ets1SigMatched = SdkUtils.getSdkMatchedSignature(ets1SDK, args);
        if (ets1SigMatched === null) {
            return null;
        }

        const ets1SdkFileSig = ets1SDK.getDeclaringArkFile().getFileSignature();
        const ets2SdkFileSig = new FileSignature(ets1SdkFileSig.getProjectName(), ets1SdkFileSig.getFileName().replace('.d.ts', '.d.ets'));
        const ets2SdkFileSigBak = new FileSignature(ets1SdkFileSig.getProjectName(), ets1SdkFileSig.getFileName());
        const ets2SdkFile = this.ets2SdkScene?.getFile(ets2SdkFileSig) ?? this.ets2SdkScene?.getFile(ets2SdkFileSigBak);
        if (!ets2SdkFile) {
            return null;
        }
        const ets2SdkMethod = this.getEts2SdkWithEts1SdkInfo(ets2SdkFile, ets1SDK);
        if (ets2SdkMethod === null) {
            return null;
        }
        const declareSigs = ets2SdkMethod.getDeclareSignatures();
        if (declareSigs === null) {
            return null;
        }
        if (!exactMatch && declareSigs.length === 1) {
            return declareSigs[0];
        }
        if (checkArg) {
            return this.matchEts1NumberEts2IntLongMethodSig(declareSigs, ets1SigMatched);
        }
        return this.matchEts1NumberEts2IntLongReturnSig(declareSigs, ets1SigMatched);
    }

    private getEts2SdkWithEts1SdkInfo(ets2File: ArkFile, ets1SDK: ArkMethod): ArkMethod | null {
        const ets1Class = ets1SDK.getDeclaringArkClass();
        const ets1Namespace = ets1Class.getDeclaringArkNamespace();
        if (ets1Namespace === undefined) {
            return ets2File.getClassWithName(ets1Class.getName())?.getMethodWithName(ets1SDK.getName()) ?? null;
        }
        return ets2File.getNamespaceWithName(ets1Namespace.getName())?.getClassWithName(ets1Class.getName())?.getMethodWithName(ets1SDK.getName()) ?? null;
    }

    // 判断类型是否为int，当前ArkAnalyzer对于int的表示应该是name为int的AliasType或UnclearReferenceType
    private isIntType(checkType: Type): boolean {
        if (checkType instanceof AliasType || checkType instanceof UnclearReferenceType) {
            if (checkType.getName() === NumberCategory.int) {
                return true;
            }
        }
        // 函数返回值的Promise<int>其实也是int类型
        if (checkType instanceof UnclearReferenceType && checkType.getName() === 'Promise') {
            const gTypes = checkType.getGenericTypes();
            for (const gType of gTypes) {
                if (this.isIntType(gType)) {
                    return true;
                }
            }
        }
        if (checkType instanceof ClassType && checkType.getClassSignature().getClassName() === 'Promise') {
            const gTypes = checkType.getRealGenericTypes();
            if (gTypes === undefined) {
                return false;
            }
            for (const gType of gTypes) {
                if (this.isIntType(gType)) {
                    return true;
                }
            }
        }
        return false;
    }

    // 判断类型是否为ilong，当前ArkAnalyzer对于long的表示应该是name为long的AliasType或UnclearReferenceType
    private isLongType(checkType: Type): boolean {
        if (checkType instanceof AliasType || checkType instanceof UnclearReferenceType) {
            if (checkType.getName() === NumberCategory.long) {
                return true;
            }
            // 函数返回值的Promise<long>其实也是long类型
            if (checkType instanceof UnclearReferenceType && checkType.getName() === 'Promise') {
                const gTypes = checkType.getGenericTypes();
                for (const gType of gTypes) {
                    if (this.isLongType(gType)) {
                        return true;
                    }
                }
            }
            if (checkType instanceof ClassType && checkType.getClassSignature().getClassName() === 'Promise') {
                const gTypes = checkType.getRealGenericTypes();
                if (gTypes === undefined) {
                    return false;
                }
                for (const gType of gTypes) {
                    if (this.isLongType(gType)) {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    // 此处value作为函数入参、数组下标、a/b，因为三地址码原则的限制，只可能是Local和NumberConstant类型，其他value的类型均不可能存在
    private checkValueOnlyUsedAsIntLong(stmt: Stmt, value: Value, hasChecked: Map<Local, IssueInfo>, numberCategory: NumberCategory): IssueReason {
        if (stmt.getCfg().getDeclaringMethod().getLanguage() !== Language.ARKTS1_2) {
            return IssueReason.RelatedWithNonETS2;
        }
        if (value instanceof NumberConstant) {
            if (this.isNumberConstantActuallyFloat(value)) {
                return IssueReason.UsedWithOtherType;
            }
            return IssueReason.OnlyUsedAsIntLong;
        }
        if (value instanceof UndefinedConstant || value instanceof NullConstant) {
            // 对于用null或undefined赋值的场景，认为未进行初始化，还需其他赋值语句进行检查
            return IssueReason.OnlyUsedAsIntLong;
        }
        if (value instanceof StringConstant) {
            // 存在将‘100%’，‘auto’等赋值给numberType的情况，可能是ArkAnalyzer对左值的推导有错误，左值应该是联合类型
            return IssueReason.UsedWithOtherType;
        }
        if (value instanceof Local) {
            return this.isLocalOnlyUsedAsIntLong(stmt, value, hasChecked, numberCategory);
        }
        if (value instanceof AbstractExpr) {
            return this.isAbstractExprOnlyUsedAsIntLong(stmt, value, hasChecked, numberCategory);
        }
        if (value instanceof AbstractRef) {
            return this.isAbstractRefOnlyUsedAsIntLong(stmt, value, hasChecked, numberCategory);
        }
        logger.error(`Need to handle new value type: ${value.getType().getTypeString()}`);
        return IssueReason.Other;
    }

    private isNumberConstantActuallyFloat(constant: NumberConstant): boolean {
        const valueStr = constant.getValue();
        if (valueStr.includes('.') && !valueStr.includes('e')) {
            // 数字字面量非科学计数的写法，并且有小数点，则一定是浮点数，1.0也认为是float
            return true;
        }
        const num = Number(constant.getValue());
        if (isNaN(num)) {
            // 超大数字字面量转换后是NaN，按照number处理
            return true;
        }
        return !Number.isInteger(num);
    }

    // 判断number constant是否为1.0、2.0这种可以转成1、2的整型形式
    private isFloatActuallyInt(constant: NumberConstant): boolean {
        const parts = constant.getValue().split('.');
        if (parts.length !== 2) {
            return false;
        }
        return /^0+$/.test(parts[1]);
    }

    // 根据local的类型初步判断结果
    // 有些场景直接返回检查结果，不再继续检查，例如：类型为枚举、未知类型、与number无关的复杂类型等
    // 有些场景需要继续根据local的使用进行判断，例如：与number有关的类型等
    private checkResWithLocalType(local: Local, stmt: Stmt): IssueReason | null {
        const localType = local.getType();
        if (!Utils.isNearlyNumberType(localType) && !(localType instanceof BooleanType)) {
            // 对于联合类型仅包含number和null、undefined，可以认为是OK的，需要进一步根据local的使用情况进行判断
            // 对于return a || b, arkanalyzer会认为return op是boolean类型，其实是a的类型或b的类型，此处应该是number，需要正常继续解析表达式a || b
            if (localType instanceof UnknownType || localType instanceof UnclearReferenceType) {
                // 类型推导失败为unknownType或UnclearReferenceType
                if (stmt instanceof ArkAssignStmt && stmt.getRightOp() instanceof ArkArrayRef && (stmt.getRightOp() as ArkArrayRef).getIndex() === local) {
                    // class field初始化为函数指针，导致匿名函数中使用到的闭包变量未识别，其类型为unknownType，需要继续进行查找
                    return null;
                }
                return IssueReason.CannotFindAll;
            }
            if (localType instanceof EnumValueType) {
                // local是枚举类型的值，无法改变枚举类型的定义，当做number使用
                return IssueReason.UsedWithOtherType;
            }
            // 剩余情况有aliasType、classType、函数指针、genericType等复杂场景，不再继续判断，直接返回UsedWithOtherType
            logger.trace(`Local type is not number, local: ${local.getName()}, local type: ${local.getType().getTypeString()}`);
            return IssueReason.UsedWithOtherType;
        }
        return null;
    }

    private isLocalOnlyUsedAsIntLong(stmt: Stmt, local: Local, hasChecked: Map<Local, IssueInfo>, numberCategory: NumberCategory): IssueReason {
        const currentInfo = hasChecked.get(local);
        // hasChecked map中已有此local，若原先为int，现在为long则使用long替换，其余情况不改动，直接返回，避免死循环
        if (currentInfo) {
            if (currentInfo.numberCategory === NumberCategory.int && numberCategory === NumberCategory.long) {
                hasChecked.set(local, { issueReason: IssueReason.OnlyUsedAsIntLong, numberCategory: NumberCategory.long });
            }
            return IssueReason.OnlyUsedAsIntLong;
        }
        // 在之前的语句检测中已查找过此local并生成相应的issue，直接根据issue的内容返回结果，如果issue中是int，检查的是long，则结果为long
        const currentIssue = this.getLocalIssueFromIssueList(local, stmt);
        if (currentIssue && currentIssue.fix instanceof RuleFix) {
            const issueReason = this.getIssueReasonFromDefectInfo(currentIssue.defect);
            const issueCategory = this.getNumberCategoryFromFixInfo(currentIssue.fix as RuleFix);
            if (issueReason !== null && issueCategory !== null) {
                if (issueReason !== IssueReason.OnlyUsedAsIntLong) {
                    hasChecked.set(local, { issueReason: issueReason, numberCategory: numberCategory });
                    return issueReason;
                }
                if (numberCategory === NumberCategory.long) {
                    hasChecked.set(local, { issueReason: issueReason, numberCategory: numberCategory });
                } else {
                    hasChecked.set(local, { issueReason: issueReason, numberCategory: issueCategory });
                }
                return issueReason;
            }
        }

        if (stmt.getCfg().getDeclaringMethod().getLanguage() !== Language.ARKTS1_2) {
            hasChecked.set(local, { issueReason: IssueReason.RelatedWithNonETS2, numberCategory: NumberCategory.number });
            return IssueReason.RelatedWithNonETS2;
        }

        // 先将value加入map中，默认设置成false，避免后续递归查找阶段出现死循环，最后再根据查找结果绝对是否重新设置成true
        hasChecked.set(local, { issueReason: IssueReason.Other, numberCategory: NumberCategory.number });

        const resWithLocalType = this.checkResWithLocalType(local, stmt);
        if (resWithLocalType) {
            if (resWithLocalType === IssueReason.OnlyUsedAsIntLong) {
                hasChecked.set(local, { issueReason: resWithLocalType, numberCategory: numberCategory });
            } else {
                hasChecked.set(local, { issueReason: resWithLocalType, numberCategory: NumberCategory.number });
            }
            return resWithLocalType;
        }

        let checkStmts: Stmt[] = [];
        const declaringStmt = local.getDeclaringStmt();
        if (declaringStmt === null) {
            // 无定义语句的local可能来自于全局变量或import变量，需要根据import信息查找其原始local
            // 也可能是内层匿名类中使用到的外层函数中的变量，在内存类属性初始化时无定义语句
            const declaringMethod = stmt.getCfg().getDeclaringMethod();
            const newLocal =
                this.getLocalFromOuterMethod(local, declaringMethod) ??
                this.getLocalFromGlobal(local, declaringMethod) ??
                this.getLocalFromImportInfo(local, declaringMethod);
            if (newLocal === null) {
                // local非来自于import，确实是缺少定义语句，或者是从非1.2文件import，直接返回false，因为就算是能确认local仅当做int使用，也找不到定义语句去修改类型注解为int，所以后续检查都没有意义
                logger.error(`Missing declaring stmt, local: ${local.getName()}`);
                return hasChecked.get(local)!.issueReason;
            }
            const declaringStmt = newLocal.getDeclaringStmt();
            if (declaringStmt === null) {
                // local变量未找到定义语句，直接返回false，因为就算是能确认local仅当做int使用，也找不到定义语句去修改类型注解为int，所以后续检查都没有意义
                logger.error(`Missing declaring stmt, local: ${local.getName()}`);
                hasChecked.set(local, { issueReason: IssueReason.CannotFindAll, numberCategory: NumberCategory.number });
                return IssueReason.CannotFindAll;
            }
            hasChecked.delete(local);
            return this.isLocalOnlyUsedAsIntLong(declaringStmt, newLocal, hasChecked, numberCategory);
        }
        // declaringStmt存在，但是是export let a定义的全局变量并对外export，也认为是number，不作为int使用，因为其使用范围可能很广，无法找全
        const declaringMethod = declaringStmt.getCfg().getDeclaringMethod();
        if (declaringMethod.isDefaultArkMethod()) {
            const exportInfo = declaringMethod.getDeclaringArkFile().getExportInfoBy(local.getName());
            if (exportInfo !== undefined) {
                const arkExport = exportInfo.getArkExport();
                if (arkExport instanceof Local) {
                    hasChecked.set(local, { issueReason: IssueReason.UsedWithOtherType, numberCategory: NumberCategory.number });
                    return IssueReason.UsedWithOtherType;
                }
            }
        }

        checkStmts.push(declaringStmt);
        local.getUsedStmts().forEach(s => {
            if (s !== stmt) {
                checkStmts.push(s);
            }
        });
        // usedStmts中不会记录local为leftOp的stmt，在此处进行补充
        declaringStmt
            .getCfg()
            .getStmts()
            .forEach(s => {
                if (s === declaringStmt || !(s instanceof ArkAssignStmt) || s.getLeftOp() !== local) {
                    return;
                }
                checkStmts.push(s);
            });

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

    private checkRelatedStmtForLocal(stmt: Stmt, local: Local, hasChecked: Map<Local, IssueInfo>, numberCategory: NumberCategory): IssueInfo {
        if (stmt instanceof ArkAssignStmt && stmt.getLeftOp() === local) {
            const issueReason = this.checkValueOnlyUsedAsIntLong(stmt, stmt.getRightOp(), hasChecked, numberCategory);
            if (issueReason === IssueReason.OnlyUsedAsIntLong) {
                return { issueReason, numberCategory };
            } else {
                return { issueReason, numberCategory: NumberCategory.number };
            }
        }
        // 当前检查的local位于赋值语句的右边，若参与除法运算则看做double类型使用，若作为SDK入参依据SDK定义，其余运算、赋值等处理不会影响其自身从int -> number，所以不处理
        if (stmt instanceof ArkAssignStmt && stmt.getLeftOp() !== local) {
            const rightOp = stmt.getRightOp();
            if (rightOp instanceof ArkNormalBinopExpr && rightOp.getOperator() === NormalBinaryOperator.Division) {
                return { issueReason: IssueReason.UsedWithOtherType, numberCategory: NumberCategory.number };
            }
            if (rightOp instanceof AbstractInvokeExpr) {
                const res = this.checkLocalUsedAsSDKArg(rightOp, local, hasChecked);
                if (res !== null) {
                    return res;
                }
            }
            return { issueReason: IssueReason.OnlyUsedAsIntLong, numberCategory };
        }
        if (stmt instanceof ArkInvokeStmt) {
            // 函数调用语句，local作为实参或base，除作为SDK入参之外，其余场景不会影响其值的变化，不会导致int被重新赋值为number使用
            const res = this.checkLocalUsedAsSDKArg(stmt.getInvokeExpr(), local, hasChecked);
            if (res !== null) {
                return res;
            }
            return { issueReason: IssueReason.OnlyUsedAsIntLong, numberCategory };
        }
        if (stmt instanceof ArkReturnStmt || stmt instanceof ArkIfStmt) {
            // return语句，local作为返回值，不会影响其值的变化，不会导致int被重新赋值为number使用
            // 条件判断语句，local作为condition expr的op1或op2，进行二元条件判断，不会影响其值的变化，不会导致int被重新赋值为number使用
            return { issueReason: IssueReason.OnlyUsedAsIntLong, numberCategory };
        }
        logger.error(`Need to check new type of stmt: ${stmt.toString()}, method: ${stmt.getCfg().getDeclaringMethod().getSignature().toString()}`);
        return { issueReason: IssueReason.Other, numberCategory: NumberCategory.number };
    }

    // 判断local是否是SDK invoke expr的入参，且其类型是int或long，否则返回null
    private checkLocalUsedAsSDKArg(expr: AbstractInvokeExpr, local: Local, hasChecked: Map<Local, IssueInfo>): IssueInfo | null {
        const method = this.scene.getMethod(expr.getMethodSignature());
        if (method === null) {
            if (expr instanceof ArkInstanceInvokeExpr && Utils.isNearlyPrimitiveType(expr.getBase().getType())) {
                // 调用方法为builtIn方法，但因为类型推导失败，导致获取的方法签名为%unk/%unk
                return null;
            }
            logger.trace(`Failed to find method: ${expr.getMethodSignature().toString()}`);
            return null;
        }
        const args = expr.getArgs();
        if (SdkUtils.isMethodFromSdk(method)) {
            const ets2SDKSig = this.getEts2SdkSignatureWithEts1Method(method, args, true);
            if (ets2SDKSig === null) {
                return null;
            }
            const argIndex = expr.getArgs().indexOf(local);
            if (argIndex < 0 || argIndex >= expr.getArgs().length) {
                return null;
            }
            const params = ets2SDKSig.getMethodSubSignature().getParameters();
            const currLocal = hasChecked.get(local);
            if (this.isIntType(params[argIndex].getType())) {
                if (currLocal === undefined) {
                    return { issueReason: IssueReason.OnlyUsedAsIntLong, numberCategory: NumberCategory.int };
                }
                if (currLocal.numberCategory === NumberCategory.long) {
                    return { issueReason: IssueReason.OnlyUsedAsIntLong, numberCategory: NumberCategory.long };
                }
                return { issueReason: IssueReason.OnlyUsedAsIntLong, numberCategory: NumberCategory.int };
            }
            if (this.isLongType(params[argIndex].getType())) {
                return { issueReason: IssueReason.OnlyUsedAsIntLong, numberCategory: NumberCategory.long };
            }
        }
        return null;
    }

    private getLocalFromGlobal(local: Local, method: ArkMethod): Local | null {
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

    private getLocalFromImportInfo(local: Local, method: ArkMethod): Local | null {
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

    // 对于method中的let obj: Obj = {aa: a}的对象字面量，其中使用到a变量为method中的local或global，当前ArkAnalyzer未能对其进行识别和表示，此处手动查找
    // 对于class的field为lambda函数定义的函数指针的场景，lambda函数中使用到的外层的变量，当前ArkAnalyzer未能对其进行识别和表示，此处手动查找
    private getLocalFromOuterMethod(local: Local, method: ArkMethod): Local | null {
        const outerMethod = method.getOuterMethod();
        if (outerMethod) {
            const newLocal = outerMethod.getBody()?.getLocals().get(local.getName());
            if (newLocal) {
                if (newLocal.getDeclaringStmt()) {
                    return newLocal;
                } else {
                    return this.getLocalFromOuterMethod(newLocal, outerMethod);
                }
            }
        }

        const clazz = method.getDeclaringArkClass();
        return this.findLocalFromOuterClass(local, clazz);
    }

    private findLocalFromOuterClass(local: Local, objectClass: ArkClass): Local | null {
        if (objectClass.getCategory() !== ClassCategory.INTERFACE && objectClass.getCategory() !== ClassCategory.OBJECT) {
            // 此查找仅涉及对象字面量中直接使用变量的场景，其余场景不涉及
            return null;
        }
        // 根据class的名字获取外层method的名字和其对应的class，例如'%AC3$%dflt.%outer111$%outer11$outer1'表示default class中的outer111$%outer11$outer1 method
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

    private isAbstractExprOnlyUsedAsIntLong(stmt: Stmt, expr: AbstractExpr, hasChecked: Map<Local, IssueInfo>, numberCategory: NumberCategory): IssueReason {
        if (expr instanceof ArkNormalBinopExpr) {
            if (expr.getOperator() === NormalBinaryOperator.Division) {
                const op1 = expr.getOp1();
                const op2 = expr.getOp2();
                if (op1 instanceof NumberConstant && !this.isNumberConstantActuallyFloat(op1)) {
                    this.addIssueReport(RuleCategory.NumericLiteral, NumberCategory.number, IssueReason.UsedWithOtherType, true, stmt, op1);
                } else if (op1 instanceof Local) {
                    hasChecked.set(op1, { issueReason: IssueReason.UsedWithOtherType, numberCategory: NumberCategory.number });
                }
                if (op2 instanceof NumberConstant && !this.isNumberConstantActuallyFloat(op2)) {
                    this.addIssueReport(RuleCategory.NumericLiteral, NumberCategory.number, IssueReason.UsedWithOtherType, true, stmt, op2);
                } else if (op2 instanceof Local) {
                    hasChecked.set(op2, { issueReason: IssueReason.UsedWithOtherType, numberCategory: NumberCategory.number });
                }
                return IssueReason.UsedWithOtherType;
            }
            const isOp1Int = this.checkValueOnlyUsedAsIntLong(stmt, expr.getOp1(), hasChecked, numberCategory);
            const isOp2Int = this.checkValueOnlyUsedAsIntLong(stmt, expr.getOp2(), hasChecked, numberCategory);
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
        if (expr instanceof AbstractInvokeExpr) {
            const method = this.scene.getMethod(expr.getMethodSignature());
            if (method === null) {
                logger.trace(`Failed to find method: ${expr.getMethodSignature().toString()}`);
                return IssueReason.Other;
            }
            if (SdkUtils.isMethodFromSdk(method)) {
                const ets2SDKSig = this.getEts2SdkSignatureWithEts1Method(method, expr.getArgs(), false);
                if (ets2SDKSig === null) {
                    return IssueReason.UsedWithOtherType;
                }
                if (this.isIntType(ets2SDKSig.getType()) || this.isLongType(ets2SDKSig.getType())) {
                    return IssueReason.OnlyUsedAsIntLong;
                }
                return IssueReason.UsedWithOtherType;
            }
            if (method.getLanguage() !== Language.ARKTS1_2) {
                return IssueReason.RelatedWithNonETS2;
            }
            const returnStmt = method.getReturnStmt();
            for (const s of returnStmt) {
                if (!(s instanceof ArkReturnStmt)) {
                    continue;
                }
                const res = this.checkValueOnlyUsedAsIntLong(s, s.getOp(), hasChecked, numberCategory);
                if (res !== IssueReason.OnlyUsedAsIntLong) {
                    return res;
                }
            }
            return IssueReason.OnlyUsedAsIntLong;
        }
        if (expr instanceof ArkAwaitExpr) {
            const promise = expr.getPromise();
            if (promise instanceof Local) {
                const declaringStmt = promise.getDeclaringStmt();
                if (declaringStmt === null || !(declaringStmt instanceof ArkAssignStmt)) {
                    logger.error('Missing or wrong declaringStmt for await promise');
                    return IssueReason.CannotFindAll;
                }
                return this.checkValueOnlyUsedAsIntLong(declaringStmt, declaringStmt.getRightOp(), hasChecked, numberCategory);
            }
            logger.error(`Need to handle new type of promise: ${promise.getType().toString()}`);
            return IssueReason.Other;
        }
        if (expr instanceof ArkCastExpr) {
            return this.checkValueOnlyUsedAsIntLong(stmt, expr.getOp(), hasChecked, numberCategory);
        }
        if (expr instanceof ArkUnopExpr) {
            if (expr.getOperator() === UnaryOperator.Neg || expr.getOperator() === UnaryOperator.BitwiseNot) {
                return this.checkValueOnlyUsedAsIntLong(stmt, expr.getOp(), hasChecked, numberCategory);
            }
            if (expr.getOperator() === UnaryOperator.LogicalNot) {
                // let a = 1; let b = !a，不会导致a产生int到number的变化
                return IssueReason.OnlyUsedAsIntLong;
            }
            logger.error(`Need to handle new type of unary operator: ${expr.getOperator().toString()}`);
            return IssueReason.Other;
        }
        // 剩余的expr的类型不应该出现在这里，如果出现了表示有场景未考虑到，打印日志记录，进行补充
        logger.error(`Need to handle new type of expr: ${expr.toString()}`);
        return IssueReason.Other;
    }

    private isAbstractRefOnlyUsedAsIntLong(stmt: Stmt, ref: AbstractRef, hasChecked: Map<Local, IssueInfo>, numberCategory: NumberCategory): IssueReason {
        if (ref instanceof ArkArrayRef) {
            // 使用数组中某元素进行赋值的场景很复杂，需要判断index的具体值，需要判断数组中的队应元素的全部使用场景，当前不做检查，直接返回false
            return IssueReason.CannotFindAll;
        }
        if (ref instanceof AbstractFieldRef) {
            return this.checkFieldRef(ref, stmt.getCfg().getDeclaringMethod().getDeclaringArkClass().getSignature(), numberCategory, hasChecked);
        }
        if (ref instanceof ArkParameterRef) {
            return this.checkAllArgsOfParameter(stmt, hasChecked, numberCategory);
        }
        if (ref instanceof ClosureFieldRef) {
            return this.checkClosureFieldRef(ref, hasChecked, numberCategory);
        }
        // 其他ref类型经分析不应该出现在此处，若存在输出日志，通过分析日志信息进行补充处理，包括：ArkCaughtExceptionRef, GlobalRef, ArkThisRef
        logger.error(`Need to check new type of ref in stmt: ${stmt.toString()}`);
        return IssueReason.Other;
    }

    private checkFieldRef(
        fieldRef: AbstractFieldRef,
        currentClassSig: ClassSignature,
        numberCategory: NumberCategory,
        hasChecked: Map<Local, IssueInfo>
    ): IssueReason {
        if (SdkUtils.isFieldFromSdk(fieldRef)) {
            const ets2FieldType = this.checkSDKFieldType(fieldRef);
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
        const baseClass = this.scene.getClass(fieldBase);
        if (baseClass === null) {
            return IssueReason.CannotFindAll;
        }
        if (baseClass.getLanguage() !== Language.ARKTS1_2) {
            return IssueReason.RelatedWithNonETS2;
        }
        if (
            baseClass.getCategory() === ClassCategory.ENUM ||
            baseClass.getCategory() === ClassCategory.OBJECT ||
            baseClass.getCategory() === ClassCategory.INTERFACE
        ) {
            // 如果是使用enum枚举类型进行赋值，不能修改为int，只能是number
            // 如果是使用object对象字面量类型进行赋值，arkts1.1和1.2规定左边一定需要声明具体interface，其中一定写明number类型，不能修改为int
            return IssueReason.UsedWithOtherType;
        }
        if (baseClass.getSignature().toString() !== currentClassSig.toString()) {
            return IssueReason.CannotFindAll;
        }
        const field = baseClass.getField(fieldRef.getFieldSignature());
        if (field === null) {
            return IssueReason.CannotFindAll;
        }
        const existRes = this.classFieldRes.get(field);
        if (existRes !== undefined) {
            return existRes.issueReason;
        }
        if (!Utils.isNearlyNumberType(refType)) {
            if (refType instanceof UnknownType) {
                const res = IssueReason.CannotFindAll;
                this.classFieldRes.set(field, { issueReason: res, numberCategory: NumberCategory.number });
                return res;
            }
            const res = IssueReason.UsedWithOtherType;
            this.classFieldRes.set(field, { issueReason: res, numberCategory: NumberCategory.number });
            return res;
        }
        if (field.containsModifier(ModifierType.PRIVATE)) {
            // 如果属性有setter方法，则无法找全其赋值的地方，无法判断是否为int，保守方式判定为number
            // 如果属性有getter方法，则无法找全其使用的地方，如果有用作除法运算，则应该是number，保守方式判定为number
            if (this.fieldWithSetter(field, baseClass) || this.fieldWithGetter(field, baseClass)) {
                const res = IssueReason.CannotFindAll;
                this.classFieldRes.set(field, { issueReason: res, numberCategory: NumberCategory.number });
                return res;
            }
            if (field.containsModifier(ModifierType.READONLY)) {
                // 先写入默认值，避免后续查找时出现死循环，得到结果后再进行替换
                this.classFieldRes.set(field, { issueReason: IssueReason.OnlyUsedAsIntLong, numberCategory: numberCategory });
                const res = this.checkReadonlyFieldInitializer(field, baseClass, numberCategory, hasChecked);
                if (res === IssueReason.OnlyUsedAsIntLong) {
                    this.classFieldRes.set(field, { issueReason: res, numberCategory: numberCategory });
                } else {
                    this.classFieldRes.set(field, { issueReason: res, numberCategory: NumberCategory.number });
                }
                return res;
            }
            this.classFieldRes.set(field, { issueReason: IssueReason.OnlyUsedAsIntLong, numberCategory: numberCategory });
            const res = this.checkPrivateField(field, baseClass, numberCategory, hasChecked);
            if (res === IssueReason.OnlyUsedAsIntLong) {
                this.classFieldRes.set(field, { issueReason: res, numberCategory: numberCategory });
            } else {
                this.classFieldRes.set(field, { issueReason: res, numberCategory: NumberCategory.number });
            }
            return res;
        }
        // 此处若想充分解析，需要在整个项目中找到该field的所有使用到的地方，效率很低，且很容易找漏，当前不做检查，直接返回false
        const res = IssueReason.CannotFindAll;
        this.classFieldRes.set(field, { issueReason: res, numberCategory: NumberCategory.number });
        return res;
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
        // readonly field只允许在构造函数、staticInit、instInit三处中的一处进行初始化
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
                return this.checkValueOnlyUsedAsIntLong(stmt, stmt.getRightOp(), hasChecked, numberCategory);
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

    // 当前仅查找当前field的fieldRef在左边与fieldRef在右边的场景，其余均不检查，认为cannot find all
    private checkFieldUsedInStmt(field: ArkField, stmt: Stmt, numberCategory: NumberCategory, hasChecked: Map<Local, IssueInfo>): IssueReason | null {
        if (stmt instanceof ArkAssignStmt) {
            const leftOp = stmt.getLeftOp();
            const rightOp = stmt.getRightOp();
            if (leftOp instanceof AbstractFieldRef) {
                if (this.isFieldRefMatchArkField(leftOp, field)) {
                    return this.checkValueOnlyUsedAsIntLong(stmt, rightOp, hasChecked, numberCategory);
                }
                return null;
            }
            if (rightOp instanceof AbstractFieldRef) {
                if (this.isFieldRefMatchArkField(rightOp, field)) {
                    if (leftOp instanceof Local && leftOp.getName().startsWith(TEMP_LOCAL_PREFIX)) {
                        return this.isLocalOnlyUsedAsIntLong(stmt, leftOp, hasChecked, numberCategory);
                    }
                    return IssueReason.OnlyUsedAsIntLong;
                }
                return null;
            }
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
                if (!(stmt instanceof ArkReturnStmt)) {
                    continue;
                }
                const op = stmt.getOp();
                if (op instanceof Local) {
                    const opDeclaringStmt = op.getDeclaringStmt();
                    if (!(opDeclaringStmt instanceof ArkAssignStmt)) {
                        continue;
                    }
                    const rightOp = opDeclaringStmt.getRightOp();
                    if (!(rightOp instanceof ArkInstanceFieldRef)) {
                        continue;
                    }
                    if (field.getName() === rightOp.getFieldName()) {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    private checkAllArgsOfParameter(stmt: Stmt, hasChecked: Map<Local, IssueInfo>, numberCategory: NumberCategory): IssueReason {
        let checkAll = { value: true };
        let visited: Set<Stmt> = new Set();
        const result = this.checkFromStmt(stmt, hasChecked, numberCategory, checkAll, visited);
        if (!checkAll.value) {
            return IssueReason.CannotFindAll;
        }
        return result;
    }

    private checkClosureFieldRef(closureRef: ClosureFieldRef, hasChecked: Map<Local, IssueInfo>, numberCategory: NumberCategory): IssueReason {
        const closureBase = closureRef.getBase();
        const baseType = closureBase.getType();
        if (!(baseType instanceof LexicalEnvType)) {
            // 此场景应该不可能出现，如果出现说明IR解析错误
            logger.error(`ClosureRef base must be LexicalEnvType, but here is ${baseType.getTypeString()}`);
            return IssueReason.Other;
        }
        const outerLocal = baseType.getClosures().filter(local => local.getName() === closureRef.getFieldName());
        if (outerLocal.length !== 1) {
            logger.error('Failed to find the local from outer method of the closure local.');
            return IssueReason.Other;
        }
        const declaringStmt = outerLocal[0].getDeclaringStmt();
        if (declaringStmt === null) {
            logger.error('Failed to find the declaring stmt of the local from outer method of the closure local.');
            return IssueReason.Other;
        }
        return this.isLocalOnlyUsedAsIntLong(declaringStmt, outerLocal[0], hasChecked, numberCategory);
    }

    private checkFromStmt(
        stmt: Stmt,
        hasChecked: Map<Local, IssueInfo>,
        numberCategory: NumberCategory,
        checkAll: { value: boolean },
        visited: Set<Stmt>
    ): IssueReason {
        const method = stmt.getCfg().getDeclaringMethod();
        if (!this.visited.has(method)) {
            this.dvfgBuilder.buildForSingleMethod(method);
            this.visited.add(method);
        }

        const node = this.dvfg.getOrNewDVFGNode(stmt);
        let workList: DVFGNode[] = [node];
        while (workList.length > 0) {
            const current = workList.shift()!;
            const currentStmt = current.getStmt();
            if (visited.has(currentStmt)) {
                continue;
            }
            visited.add(currentStmt);

            const paramRef = this.isFromParameter(currentStmt);
            if (paramRef) {
                const paramIdx = paramRef.getIndex();
                const callsites = this.cg.getInvokeStmtByMethod(currentStmt.getCfg().getDeclaringMethod().getSignature());
                this.processCallsites(callsites);
                const argMap = this.collectCallSiteArgs(paramIdx, callsites);
                this.callDepth++;
                if (this.callDepth > CALL_DEPTH_LIMIT) {
                    checkAll.value = false;
                    return IssueReason.CannotFindAll;
                }
                for (const [callSite, arg] of argMap) {
                    const res = this.checkValueOnlyUsedAsIntLong(callSite, arg, hasChecked, numberCategory);
                    if (res !== IssueReason.OnlyUsedAsIntLong) {
                        return res;
                    }
                }
                return IssueReason.OnlyUsedAsIntLong;
            }
        }
        return IssueReason.Other;
    }

    private processCallsites(callsites: Stmt[]): void {
        callsites.forEach(cs => {
            const declaringMtd = cs.getCfg().getDeclaringMethod();
            if (!this.visited.has(declaringMtd)) {
                this.dvfgBuilder.buildForSingleMethod(declaringMtd);
                this.visited.add(declaringMtd);
            }
        });
    }

    private isFromParameter(stmt: Stmt): ArkParameterRef | undefined {
        if (!(stmt instanceof ArkAssignStmt)) {
            return undefined;
        }
        const rightOp = stmt.getRightOp();
        if (rightOp instanceof ArkParameterRef) {
            return rightOp;
        }
        return undefined;
    }

    private collectCallSiteArgs(argIdx: number, callsites: Stmt[]): Map<Stmt, Value> {
        const argMap = new Map<Stmt, Value>();
        callsites.forEach(callsite => {
            const arg = callsite.getInvokeExpr()!.getArg(argIdx);
            if (arg !== undefined) {
                argMap.set(callsite, arg);
            }
        });
        return argMap;
    }

    private getIssueReasonFromDefectInfo(defect: Defects): IssueReason | null {
        const issueProblem = defect.problem;
        // 一定要将IssueReason.UsedWithOtherType放在IssueReason.OnlyUsedAsIntLong之前判断，因为他俩有包含关系，位置调换会导致错误
        if (issueProblem.includes(IssueReason.UsedWithOtherType)) {
            return IssueReason.UsedWithOtherType;
        }
        if (issueProblem.includes(IssueReason.OnlyUsedAsIntLong)) {
            return IssueReason.OnlyUsedAsIntLong;
        }
        if (issueProblem.includes(IssueReason.RelatedWithNonETS2)) {
            return IssueReason.RelatedWithNonETS2;
        }
        if (issueProblem.includes(IssueReason.CannotFindAll)) {
            return IssueReason.CannotFindAll;
        }
        if (issueProblem.includes(IssueReason.Other)) {
            return IssueReason.Other;
        }
        return null;
    }

    private getNumberCategoryFromFixInfo(fix: RuleFix): NumberCategory | null {
        const fixText = fix.text;
        let match = fix.text.match(/^([^=;]+:[^=;]+)([\s\S]*)$/);
        if (match === null || match.length < 2) {
            return null;
        }
        if (match[1].includes(NumberCategory.int)) {
            return NumberCategory.int;
        }
        if (match[1].includes(NumberCategory.long)) {
            return NumberCategory.long;
        }
        if (match[1].includes(NumberCategory.number)) {
            return NumberCategory.number;
        }
        return null;
    }

    private getFieldIssueFromIssueList(field: ArkField): IssueReport | null {
        const filePath = field.getDeclaringArkClass().getDeclaringArkFile().getFilePath();
        const position: WarnInfo = {
            line: field.getOriginPosition().getLineNo(),
            startCol: field.getOriginPosition().getColNo(),
            endCol: field.getOriginPosition().getColNo(),
            filePath: field.getDeclaringArkClass().getDeclaringArkFile().getFilePath(),
        };
        const mapKey = `${filePath}%${position.line}%${position.startCol}%${position.endCol}%${this.rule.ruleId}`;
        return this.issuesMap.get(mapKey) ?? null;
    }

    private getLocalIssueFromIssueList(local: Local, stmt: Stmt): IssueReport | null {
        const filePath = stmt.getCfg().getDeclaringMethod().getDeclaringArkFile().getFilePath();
        const position = getLineAndColumn(stmt, local, true);
        const mapKey = `${filePath}%${position.line}%${position.startCol}%${position.endCol}%${this.rule.ruleId}`;
        return this.issuesMap.get(mapKey) ?? null;
    }

    private getWarnInfo(field?: ArkField, issueStmt?: Stmt, value?: Value): WarnInfo | null {
        let warnInfo: WarnInfo | null = null;

        if (field === undefined) {
            if (issueStmt && value) {
                warnInfo = getLineAndColumn(issueStmt, value, true);
                if (warnInfo.line === -1) {
                    // 可能是因为获取array index时，array是联合类型导致index未推导成功，checker里面额外去body里找local替换index
                    // 但是获取index的position信息时，需要使用原始的index去stmt中查找位置
                    const actualPosition = this.getActualIndexPosInStmt(issueStmt);
                    const originPath = issueStmt.getCfg().getDeclaringMethod().getDeclaringArkFile().getFilePath();
                    warnInfo = {
                        line: actualPosition.getFirstLine(),
                        startCol: actualPosition.getFirstCol(),
                        endLine: actualPosition.getLastLine(),
                        endCol: actualPosition.getLastCol(),
                        filePath: originPath,
                    };
                }
            } else {
                logger.error('Missing stmt or value when adding issue.');
                return warnInfo;
            }
        } else {
            warnInfo = {
                line: field.getOriginPosition().getLineNo(),
                startCol: field.getOriginPosition().getColNo(),
                endCol: field.getOriginPosition().getColNo(),
                filePath: field.getDeclaringArkClass().getDeclaringArkFile().getFilePath(),
            };
        }
        if (warnInfo.line === -1) {
            if (issueStmt) {
                logger.error(`failed to get position info of value in issue stmt: ${issueStmt.toString()}`);
            } else if (field) {
                logger.error(`failed to get position info of field: ${field.getSignature().toString()}`);
            } else {
                logger.error(`failed to get position info`);
            }
            return null;
        }
        return warnInfo;
    }

    private getProblem(ruleCategory: RuleCategory, reason: IssueReason): string | null {
        if (ruleCategory === RuleCategory.SDKIntType) {
            return 'SDKIntType-' + reason;
        }
        if (ruleCategory === RuleCategory.NumericLiteral) {
            return 'NumericLiteral-' + reason;
        }
        if (ruleCategory === RuleCategory.ArrayIndex) {
            return 'IndexIntType-' + reason;
        }
        logger.error(`Have not support rule ${ruleCategory} yet.`);
        return null;
    }

    private getDesc(ruleCategory: RuleCategory, reason: IssueReason, couldAutofix: boolean): string | null {
        if (ruleCategory === RuleCategory.NumericLiteral) {
            if (reason === IssueReason.OnlyUsedAsIntLong) {
                return `It is used as ${NumberCategory.int} (${ruleCategory})`;
            }
            return `It is used as ${NumberCategory.number} (${ruleCategory})`;
        }
        if (ruleCategory === RuleCategory.ArrayIndex) {
            if (reason === IssueReason.OnlyUsedAsIntLong) {
                return `It is used as ${NumberCategory.int} (${ruleCategory})`;
            }
            if (reason === IssueReason.ActuallyIntConstant) {
                return `The number constant could be changed to int constant (${ruleCategory})`;
            }
            if (couldAutofix) {
                return `It is used as ${NumberCategory.number} (${ruleCategory})`;
            }
            return `The array index is used as ${NumberCategory.number}, please check if it's ok (${ruleCategory})`;
        }
        logger.error(`Have not support rule ${ruleCategory} yet.`);
        return null;
    }

    private shouldSkipDuplicatedIssue(numberCategory: NumberCategory, field?: ArkField, value?: Value, issueStmt?: Stmt): boolean {
        // 添加新的issue之前需要检查一下已有issue，避免重复issue，或2个issue之间冲突，一个issue要改为int，一个issue要改为long
        let currentIssue: IssueReport | null = null;
        let issueCategory: NumberCategory | null = null;
        if (field !== undefined) {
            currentIssue = this.getFieldIssueFromIssueList(field);
            if (currentIssue) {
                issueCategory = this.getNumberCategoryFromFixInfo(currentIssue.fix as RuleFix);
            }
        } else if (value instanceof Local) {
            if (issueStmt) {
                currentIssue = this.getLocalIssueFromIssueList(value, issueStmt);
                if (currentIssue && currentIssue.fix) {
                    issueCategory = this.getNumberCategoryFromFixInfo(currentIssue.fix as RuleFix);
                }
            }
        }
        if (currentIssue && issueCategory) {
            const issueReason = this.getIssueReasonFromDefectInfo(currentIssue.defect);
            if (issueReason === null) {
                return false;
            }
            if (issueReason !== IssueReason.OnlyUsedAsIntLong) {
                return true;
            }
            if (issueCategory !== NumberCategory.long && numberCategory === NumberCategory.long) {
                // 删除掉之前的修复为int的，用本次即将add的新的issue替代
                this.issuesMap.delete(this.getIssuesMapKey(currentIssue.defect.mergeKey));
                return false;
            } else {
                // 已有的issue已经足够进行自动修复处理，无需重复添加
                return true;
            }
        }
        return false;
    }

    private getIssuesMapKey(mergeKey: string): string {
        const lastIndex = mergeKey.lastIndexOf('%');
        return mergeKey.substring(0, lastIndex);
    }

    private addIssueReportForSDKArg(
        ruleCategory: RuleCategory,
        numberCategory: NumberCategory,
        reason: IssueReason,
        couldAutofix: boolean,
        issueStmt?: Stmt,
        value?: Value,
        field?: ArkField,
        usedStmt?: Stmt
    ): void {
        const severity = this.rule.alert ?? this.metaData.severity;
        let warnInfo = this.getWarnInfo(field, issueStmt, value);
        let problem = this.getProblem(ruleCategory, reason);
        if (!warnInfo || !problem) {
            return;
        }
        let desc: string;
        if (reason === IssueReason.OnlyUsedAsIntLong) {
            if (usedStmt) {
                desc = `It has relationship with the arg of SDK API in ${this.getUsedStmtDesc(usedStmt, issueStmt)} and only used as ${numberCategory}, should be defined as ${numberCategory} (${ruleCategory})`;
            } else {
                logger.error('Missing used stmt when getting issue description');
                return;
            }
        } else {
            desc = `The arg of SDK API should be ${numberCategory} here (${ruleCategory})`;
        }

        const shouldSkip = this.shouldSkipDuplicatedIssue(numberCategory, field, value, issueStmt);
        if (shouldSkip) {
            return;
        }

        let defects = new Defects(
            warnInfo.line,
            warnInfo.startCol,
            warnInfo.endCol,
            problem,
            desc,
            severity,
            this.rule.ruleId,
            warnInfo.filePath,
            this.metaData.ruleDocPath,
            true,
            false,
            couldAutofix
        );

        if (!couldAutofix) {
            this.issuesMap.set(this.getIssuesMapKey(defects.mergeKey), new IssueReport(defects, undefined));
            return;
        }
        const autofix = this.generateSDKArgRuleFix(warnInfo, reason, numberCategory, issueStmt, value, field);
        if (autofix === null) {
            // 此规则必须修复，若autofix为null，则表示无需修复，不添加issue
            return;
        } else {
            this.issuesMap.set(this.getIssuesMapKey(defects.mergeKey), new IssueReport(defects, autofix));
        }
        return;
    }

    private addIssueReportForSDKReturnOrField(
        ruleCategory: RuleCategory,
        numberCategory: NumberCategory,
        reason: IssueReason,
        issueStmt?: Stmt,
        value?: Value,
        field?: ArkField,
        usedStmt?: Stmt
    ): void {
        const severity = this.rule.alert ?? this.metaData.severity;
        let warnInfo = this.getWarnInfo(field, issueStmt, value);
        let problem = this.getProblem(ruleCategory, reason);
        if (!warnInfo || !problem) {
            return;
        }
        const shouldSkip = this.shouldSkipDuplicatedIssue(numberCategory, field, value, issueStmt);
        if (shouldSkip) {
            return;
        }

        let desc: string;
        if (reason === IssueReason.OnlyUsedAsIntLong) {
            if (usedStmt) {
                desc = `It has relationship with the SDK API in ${this.getUsedStmtDesc(usedStmt, issueStmt)} and only used as ${numberCategory}, should be defined as ${numberCategory} (${ruleCategory})`;
            } else {
                logger.error('Missing used stmt when getting issue description');
                return;
            }
        } else {
            desc = `It is used as number (${ruleCategory})`;
        }

        let defects = new Defects(
            warnInfo.line,
            warnInfo.startCol,
            warnInfo.endCol,
            problem,
            desc,
            severity,
            this.rule.ruleId,
            warnInfo.filePath,
            this.metaData.ruleDocPath,
            true,
            false,
            true
        );

        const autofix = this.generateSDKReturnOrFieldRuleFix(warnInfo, numberCategory, issueStmt, field);
        if (autofix === null) {
            // 此规则必须修复，若autofix为null，则表示无需修复，不添加issue
            return;
        } else {
            this.issuesMap.set(this.getIssuesMapKey(defects.mergeKey), new IssueReport(defects, autofix));
        }
        return;
    }

    private addIssueReport(
        ruleCategory: RuleCategory,
        numberCategory: NumberCategory,
        reason: IssueReason,
        couldAutofix: boolean,
        issueStmt?: Stmt,
        value?: Value,
        field?: ArkField
    ): void {
        const severity = this.rule.alert ?? this.metaData.severity;
        let warnInfo = this.getWarnInfo(field, issueStmt, value);
        let problem = this.getProblem(ruleCategory, reason);
        let desc = this.getDesc(ruleCategory, reason, couldAutofix);
        if (!warnInfo || !problem || !desc) {
            return;
        }

        const shouldSkip = this.shouldSkipDuplicatedIssue(numberCategory, field, value, issueStmt);
        if (shouldSkip) {
            return;
        }

        let defects = new Defects(
            warnInfo.line,
            warnInfo.startCol,
            warnInfo.endCol,
            problem,
            desc,
            severity,
            this.rule.ruleId,
            warnInfo.filePath,
            this.metaData.ruleDocPath,
            true,
            false,
            couldAutofix
        );

        if (!couldAutofix) {
            this.issuesMap.set(this.getIssuesMapKey(defects.mergeKey), new IssueReport(defects, undefined));
            return;
        }
        if (ruleCategory === RuleCategory.NumericLiteral) {
            const autofix = this.generateNumericLiteralRuleFix(warnInfo, reason, issueStmt, value, field);
            if (autofix === null) {
                // 此规则必须修复，若autofix为null，则表示无需修复，不添加issue
                return;
            }
            this.issuesMap.set(this.getIssuesMapKey(defects.mergeKey), new IssueReport(defects, autofix));
            return;
        }
        if (ruleCategory === RuleCategory.ArrayIndex) {
            if (reason === IssueReason.ActuallyIntConstant && issueStmt && value instanceof NumberConstant) {
                const autofix = this.generateIntConstantIndexRuleFix(warnInfo, issueStmt, value);
                if (autofix === null) {
                    defects.fixable = false;
                    this.issuesMap.set(this.getIssuesMapKey(defects.mergeKey), new IssueReport(defects, undefined));
                } else {
                    this.issuesMap.set(this.getIssuesMapKey(defects.mergeKey), new IssueReport(defects, autofix));
                }
            } else {
                const autofix = this.generateNumericLiteralRuleFix(warnInfo, reason, issueStmt, value, field);
                if (autofix === null) {
                    // 此规则必须修复，若autofix为null，则表示无需修复，不添加issue
                    return;
                }
                this.issuesMap.set(this.getIssuesMapKey(defects.mergeKey), new IssueReport(defects, autofix));
            }
            return;
        }
    }

    private getUsedStmtDesc(usedStmt: Stmt, issueStmt?: Stmt): string {
        const issueFile = issueStmt?.getCfg().getDeclaringMethod().getDeclaringArkFile();
        const usedFile = usedStmt.getCfg().getDeclaringMethod().getDeclaringArkFile();
        const line = usedStmt.getOriginPositionInfo().getLineNo();
        if (issueFile && issueFile !== usedFile) {
            return `${path.normalize(usedFile.getName())}: ${line}`;
        }
        return `line ${line}`;
    }

    private getSourceFile(field?: ArkField, issueStmt?: Stmt): ts.SourceFile | null {
        let arkFile: ArkFile;
        if (field) {
            arkFile = field.getDeclaringArkClass().getDeclaringArkFile();
        } else if (issueStmt) {
            arkFile = issueStmt.getCfg().getDeclaringMethod().getDeclaringArkFile();
        } else {
            logger.error('Missing both issue stmt and field when generating auto fix info.');
            return null;
        }
        let sourceFile = this.sourceFiles.get(arkFile.getFileSignature());
        if (!sourceFile) {
            sourceFile = AstTreeUtils.getASTNode(arkFile.getName(), arkFile.getCode());
            this.sourceFiles.set(arkFile.getFileSignature(), sourceFile);
        }
        return sourceFile;
    }

    private generateRuleFixForLocalDefine(sourceFile: ts.SourceFile, warnInfo: WarnInfo, numberCategory: NumberCategory): RuleFix | null {
        // warnInfo中对于变量声明语句的位置信息只包括变量名，不包括变量声明时的类型注解位置
        // 此处先获取变量名后到行尾的字符串信息，判断是替换‘: number’ 或增加 ‘: int’
        const localRange = FixUtils.getRangeWithAst(sourceFile, {
            startLine: warnInfo.line,
            startCol: warnInfo.startCol,
            endLine: warnInfo.line,
            endCol: warnInfo.endCol,
        });
        const restRange = FixUtils.getLineRangeWithStartCol(sourceFile, warnInfo.line, warnInfo.endCol);
        if (!localRange || !restRange) {
            logger.error('Failed to getting range info of issue file when generating auto fix info.');
            return null;
        }
        const restString = FixUtils.getSourceWithRange(sourceFile, restRange);
        if (!restString) {
            logger.error('Failed to getting text of the fix range info when generating auto fix info.');
            return null;
        }

        // 场景1：变量或函数入参，无类型注解的场景，直接在localString后面添加': int'
        if (!restString.trimStart().startsWith(':')) {
            let ruleFix = new RuleFix();
            ruleFix.range = localRange;
            const localString = FixUtils.getSourceWithRange(sourceFile, ruleFix.range);
            if (!localString) {
                logger.error('Failed to getting text of the fix range info when generating auto fix info.');
                return null;
            }
            ruleFix.text = `${localString}: ${numberCategory}`;
            return ruleFix;
        }
        // 场景2：变量或函数入参，有类型注解的场景，需要将类型注解替换成新的类型
        const match = restString.match(/^(\s*:[^,)=;]+)([\s\S]*)$/);
        if (match === null || match.length < 3) {
            return null;
        }
        // 如果需要替换成number，但是已经存在类型注解number，则返回null，不需要告警和自动修复
        if (match[1].includes(numberCategory)) {
            return null;
        }
        let ruleFix = new RuleFix();
        ruleFix.range = [localRange[0], localRange[1] + match[1].length];
        const localString = FixUtils.getSourceWithRange(sourceFile, ruleFix.range);
        if (!localString) {
            logger.error('Failed to getting text of the fix range info when generating auto fix info.');
            return null;
        }
        const parts = localString.split(':');
        if (parts.length !== 2) {
            logger.error('Failed to getting text of the fix range info when generating auto fix info.');
            return null;
        }
        if (!parts[1].includes(NumberCategory.number)) {
            // 原码含有类型注解但是其类型中不含number，无法进行替换
            return null;
        }
        ruleFix.text = `${parts[0].trimEnd()}: ${parts[1].trimStart().replace(NumberCategory.number, numberCategory)}`;
        return ruleFix;
    }

    private generateRuleFixForFieldDefine(sourceFile: ts.SourceFile, warnInfo: WarnInfo, numberCategory: NumberCategory): RuleFix | null {
        // warnInfo中对于field的endCol与startCol一样，均为filed首列位置，包含修饰符位置，这里autofix采用整行替换方式进行
        const fullRange = FixUtils.getLineRangeWithStartCol(sourceFile, warnInfo.line, warnInfo.startCol);
        if (fullRange === null) {
            logger.error('Failed to getting range info of issue file when generating auto fix info.');
            return null;
        }
        const fullValueString = FixUtils.getSourceWithRange(sourceFile, fullRange);
        if (fullValueString === null) {
            logger.error('Failed to getting text of the fix range info when generating auto fix info.');
            return null;
        }

        const ruleFix = new RuleFix();
        // 场景1：对于类属性private a: number 或 private a: number = xxx, fullValueString为private开始到行尾的内容，需要替换为private a: int
        let match = fullValueString.match(/^([^=;]+:[^=;]+)([\s\S]*)$/);
        if (match !== null && match.length > 2) {
            if (match[1].includes(numberCategory)) {
                // 判断field是否已经有正确的类型注解
                return null;
            }
            ruleFix.range = [fullRange[0], fullRange[0] + match[1].length];
            const originalText = FixUtils.getSourceWithRange(sourceFile, ruleFix.range);
            if (!originalText) {
                logger.error('Failed to getting text of the fix range info when generating auto fix info.');
                return null;
            }
            if (!originalText.includes(NumberCategory.number)) {
                // 原码含有类型注解但是其类型中不含number，无法进行替换
                return null;
            }
            ruleFix.text = originalText.replace(NumberCategory.number, numberCategory);
            return ruleFix;
        }
        // 场景2：对于private a = 123，originalText为private开始到行尾的内容，需要替换为private a: int = 123
        match = fullValueString.match(/^([^=;]+)([\s\S]*)$/);
        if (match !== null && match.length > 2) {
            ruleFix.range = [fullRange[0], fullRange[0] + match[1].trimEnd().length];
            const originalText = FixUtils.getSourceWithRange(sourceFile, ruleFix.range);
            if (!originalText) {
                logger.error('Failed to getting text of the fix range info when generating auto fix info.');
                return null;
            }
            ruleFix.text = `${originalText}: ${numberCategory}`;
            return ruleFix;
        }
        // 正常情况下不会走到此处，因为field一定有类型注解或初始化值来确定其类型
        return null;
    }

    private generateSDKArgRuleFix(
        warnInfo: WarnInfo,
        issueReason: IssueReason,
        numberCategory: NumberCategory,
        issueStmt?: Stmt,
        value?: Value,
        field?: ArkField
    ): RuleFix | null {
        const sourceFile = this.getSourceFile(field, issueStmt);
        if (!sourceFile) {
            return null;
        }
        if (field) {
            return this.generateRuleFixForFieldDefine(sourceFile, warnInfo, numberCategory);
        }

        if (issueReason === IssueReason.OnlyUsedAsIntLong) {
            return this.generateRuleFixForLocalDefine(sourceFile, warnInfo, numberCategory);
        }
        // 强转场景，获取到对应位置信息，在其后添加'.toInt()'或'.toLong()'
        let endLine = warnInfo.line;
        if (warnInfo.endLine !== undefined) {
            endLine = warnInfo.endLine;
        }
        const range = FixUtils.getRangeWithAst(sourceFile, {
            startLine: warnInfo.line,
            startCol: warnInfo.startCol,
            endLine: endLine,
            endCol: warnInfo.endCol,
        });
        if (range === null) {
            logger.error('Failed to getting range info of issue file when generating auto fix info.');
            return null;
        }
        const valueString = FixUtils.getSourceWithRange(sourceFile, range);
        if (valueString === null) {
            logger.error('Failed to getting text of the fix range info when generating auto fix info.');
            return null;
        }
        const ruleFix = new RuleFix();
        ruleFix.range = range;
        if (value === undefined) {
            logger.error('Missing issue SDK arg when generating auto fix info.');
            return null;
        }
        let transStr: string;
        if (numberCategory === NumberCategory.int) {
            transStr = '.toInt()';
        } else if (numberCategory === NumberCategory.long) {
            transStr = '.toLong()';
        } else {
            logger.error(`Have not support number category ${numberCategory} yet.`);
            return null;
        }

        if (value instanceof Local) {
            if (!value.getName().startsWith(TEMP_LOCAL_PREFIX)) {
                ruleFix.text = `${valueString}${transStr}`;
                return ruleFix;
            }
            const declaringStmt = value.getDeclaringStmt();
            if (declaringStmt === null) {
                ruleFix.text = `(${valueString})${transStr}`;
                return ruleFix;
            }
            if (declaringStmt instanceof ArkAssignStmt) {
                const rightOp = declaringStmt.getRightOp();
                if (rightOp instanceof AbstractInvokeExpr || rightOp instanceof AbstractFieldRef || rightOp instanceof ArkArrayRef) {
                    ruleFix.text = `${valueString}${transStr}`;
                    return ruleFix;
                }
                ruleFix.text = `(${valueString})${transStr}`;
                return ruleFix;
            }
            logger.error('Temp local declaring stmt must be assign stmt.');
            return null;
        } else {
            ruleFix.text = `(${valueString})${transStr}`;
            return ruleFix;
        }
    }

    private generateSDKReturnOrFieldRuleFix(warnInfo: WarnInfo, numberCategory: NumberCategory, issueStmt?: Stmt, field?: ArkField): RuleFix | null {
        const sourceFile = this.getSourceFile(field, issueStmt);
        if (!sourceFile) {
            return null;
        }
        if (field) {
            return this.generateRuleFixForFieldDefine(sourceFile, warnInfo, numberCategory);
        }
        return this.generateRuleFixForLocalDefine(sourceFile, warnInfo, numberCategory);
    }

    private generateIntConstantIndexRuleFix(warnInfo: WarnInfo, issueStmt: Stmt, constant: NumberConstant): RuleFix | null {
        if (!this.isFloatActuallyInt(constant)) {
            return null;
        }
        const sourceFile = this.getSourceFile(undefined, issueStmt);
        if (!sourceFile) {
            return null;
        }
        const range = FixUtils.getRangeWithAst(sourceFile, {
            startLine: warnInfo.line,
            startCol: warnInfo.startCol,
            endLine: warnInfo.line,
            endCol: warnInfo.endCol,
        });
        if (range === null) {
            logger.error('Failed to getting range info of issue file when generating auto fix info.');
            return null;
        }
        const ruleFix = new RuleFix();
        ruleFix.range = range;
        const parts = constant.getValue().split('.');
        if (parts.length !== 2) {
            return null;
        }
        ruleFix.text = parts[0];
        return ruleFix;
    }

    private generateNumericLiteralRuleFix(warnInfo: WarnInfo, issueReason: IssueReason, issueStmt?: Stmt, value?: Value, field?: ArkField): RuleFix | null {
        const sourceFile = this.getSourceFile(field, issueStmt);
        if (!sourceFile) {
            return null;
        }

        if (field) {
            if (issueReason === IssueReason.OnlyUsedAsIntLong) {
                return this.generateRuleFixForFieldDefine(sourceFile, warnInfo, NumberCategory.int);
            } else {
                return this.generateRuleFixForFieldDefine(sourceFile, warnInfo, NumberCategory.number);
            }
        }

        if (value instanceof NumberConstant) {
            // 对整型字面量进行自动修复，转成浮点字面量，例如1->1.0
            if (this.isNumberConstantActuallyFloat(value)) {
                // 无需修复
                return null;
            }
            if (warnInfo.endLine === undefined) {
                // 按正常流程不应该存在此场景
                logger.error('Missing end line info in warnInfo when generating auto fix info.');
                return null;
            }
            const range = FixUtils.getRangeWithAst(sourceFile, {
                startLine: warnInfo.line,
                startCol: warnInfo.startCol,
                endLine: warnInfo.endLine,
                endCol: warnInfo.endCol,
            });
            if (range === null) {
                logger.error('Failed to getting range info of issue file when generating auto fix info.');
                return null;
            }
            const ruleFix = new RuleFix();
            ruleFix.range = range;
            ruleFix.text = value.getValue() + '.0';
            return ruleFix;
        }
        // 非整型字面量
        // warnInfo中对于变量声明语句的位置信息只包括变量名，不包括变量声明时的类型注解位置，此处获取变量名后到行尾的字符串信息，替换‘: number’ 或增加 ‘: int’
        if (issueReason === IssueReason.OnlyUsedAsIntLong) {
            return this.generateRuleFixForLocalDefine(sourceFile, warnInfo, NumberCategory.int);
        }
        return this.generateRuleFixForLocalDefine(sourceFile, warnInfo, NumberCategory.number);
    }
}
