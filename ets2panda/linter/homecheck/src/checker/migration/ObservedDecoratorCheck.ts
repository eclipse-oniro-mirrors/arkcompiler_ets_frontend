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

import path from 'path';
import {
    AbstractInvokeExpr,
    ArkAssignStmt,
    ArkClass,
    ArkField,
    ArkNewExpr,
    ArkReturnStmt,
    AstTreeUtils,
    ClassType,
    fileSignatureCompare,
    Local,
    Scene,
    Type,
} from 'arkanalyzer';
import { ClassCategory } from 'arkanalyzer/lib/core/model/ArkClass';
import Logger, { LOG_MODULE_TYPE } from 'arkanalyzer/lib/utils/logger';
import { BaseChecker, BaseMetaData } from '../BaseChecker';
import { ClassMatcher, Defects, MatcherCallback, MatcherTypes, Rule } from '../../Index';
import { IssueReport } from '../../model/Defects';
import { RuleFix } from '../../model/Fix';
import { FixUtils } from '../../utils/common/FixUtils';
import { WarnInfo } from '../../utils/common/Utils';

const logger = Logger.getLogger(LOG_MODULE_TYPE.HOMECHECK, 'ObservedDecoratorCheck');
const gMetaData: BaseMetaData = {
    severity: 1,
    ruleDocPath: '',
    description: '',
};

const DECORATOR_SET: Set<string> = new Set<string>([
    'State',
    'Prop',
    'Link',
    'Provide',
    'Consume',
    'LocalStorageProp',
    'LocalStorageLink',
    'StorageProp',
    'StorageLink',
]);

// TODO: 需要考虑type alias、union type、intersection type中涉及class的场景
export class ObservedDecoratorCheck implements BaseChecker {
    readonly metaData: BaseMetaData = gMetaData;
    public rule: Rule;
    public defects: Defects[] = [];
    public issues: IssueReport[] = [];

    private clsMatcher: ClassMatcher = {
        matcherType: MatcherTypes.CLASS,
        category: [ClassCategory.STRUCT],
    };

    public registerMatchers(): MatcherCallback[] {
        const matchClsCb: MatcherCallback = {
            matcher: this.clsMatcher,
            callback: this.check,
        };
        return [matchClsCb];
    }

    public check = (arkClass: ArkClass): void => {
        const scene = arkClass.getDeclaringArkFile().getScene();
        const projectName = arkClass.getDeclaringArkFile().getProjectName();
        for (const field of arkClass.getFields()) {
            if (!field.getDecorators().some(d => DECORATOR_SET.has(d.getKind()))) {
                continue;
            }
            // usedClasses用于记录field的初始化中涉及的所有class
            let usedClasses: Set<ArkClass> = new Set();
            // ArkAnalyzer此处有问题，若field的类型注解为unclear type，会用右边的替换左边的。
            const fieldType = field.getType();
            // 此处仅对field为class类型进行检查，包含class和interface，非class类型不在本规则检查范围之内
            if (!(fieldType instanceof ClassType)) {
                continue;
            }
            const initializers = field.getInitializer();

            // field无初始化的场景，对field的类型进行检查
            if (initializers.length === 0) {
                const fieldTypeClass = scene.getClass(fieldType.getClassSignature());
                if (fieldTypeClass === null || fieldTypeClass.getCategory() !== ClassCategory.CLASS) {
                    continue;
                }
                usedClasses.add(fieldTypeClass);
                this.handleAllUsedClasses(field, usedClasses);
                continue;
            }

            let canFindAllTargets = true;
            let locals: Set<Local> = new Set();
            // field的初始化语句的最后一句，一定是将右边的value赋值给field，此处仍然判断一次，排除其他场景或者初始化语句为空的场景
            const lastStmt = initializers[initializers.length - 1];
            if (!(lastStmt instanceof ArkAssignStmt)) {
                continue;
            }
            const start = lastStmt.getRightOp();
            // 直接对属性进行常量赋值属于这种场景
            if (!(start instanceof Local)) {
                continue;
            }
            locals.add(start);
            for (const stmt of initializers.slice(0, -1).reverse()) {
                if (!(stmt instanceof ArkAssignStmt)) {
                    continue;
                }

                const leftOp = stmt.getLeftOp();
                const rightOp = stmt.getRightOp();
                if (!(leftOp instanceof Local)) {
                    continue;
                }
                if (!locals.has(leftOp)) {
                    continue;
                }
                if (rightOp instanceof Local) {
                    locals.add(rightOp);
                } else if (rightOp instanceof ArkNewExpr) {
                    // 此处需要区分field = new cls()和field = {}两种场景，查找完毕需继续遍历stmts以解析条件表达式造成的多赋值场景
                    canFindAllTargets = canFindAllTargets && this.handleNewExpr(scene, fieldType, rightOp, usedClasses, projectName);
                } else if (rightOp instanceof AbstractInvokeExpr) {
                    canFindAllTargets = canFindAllTargets && this.handleInvokeExpr(scene, fieldType, rightOp, usedClasses, projectName);
                } else {
                    // 对应场景为使用条件表达式cond ? 123 : 456赋值时
                    continue;
                }
            }

            this.handleAllUsedClasses(field, usedClasses);

            if (!canFindAllTargets) {
                const pos = this.getFieldPos(field);
                const description = this.generateIssueDescription(field, null, false);
                this.addIssueReport(pos, description);
            }
        }
    };

    private handleAllUsedClasses(field: ArkField, usedClasses: Set<ArkClass>): void {
        // issueClasses用于记录usedClasses以及他们的所有父类
        let issueClasses: Set<ArkClass> = new Set();
        for (const cls of usedClasses) {
            issueClasses.add(cls);
            this.getAllSuperClasses(cls, superCls => superCls.getCategory() === ClassCategory.CLASS && issueClasses.add(superCls));
        }

        for (const target of issueClasses) {
            if (target.hasDecorator('Observed')) {
                continue;
            }
            const pos = this.getClassPos(target);
            const description = this.generateIssueDescription(field, target);
            const ruleFix = this.generateRuleFix(pos, target) ?? undefined;
            this.addIssueReport(pos, description, ruleFix);
        }
    }

    // 此处需要区分field = new cls()和field = {}两种场景
    // 对于field = new cls()场景，需要查找此右边class的所有父class
    // 对于field = {}场景，需要查找左边field类型为class时的所有父class
    private handleNewExpr(scene: Scene, fieldType: Type, rightOp: ArkNewExpr, targets: Set<ArkClass>, projectName: string): boolean {
        const target = scene.getClass(rightOp.getClassType().getClassSignature());
        if (target === null) {
            return false;
        }

        // class为非本项目的内容时，表示调用到三方库、SDK等内容，不再继续进行查找
        if (target.getDeclaringArkFile().getProjectName() !== projectName) {
            return true;
        }

        if (!target.isAnonymousClass()) {
            // 理论上来说ArkNewExpr中的class一定ClassCategory.CLASS，此处仍然显式的检查一次
            if (target.getCategory() !== ClassCategory.CLASS) {
                return true;
            }
            targets.add(target);
            return true;
        }

        // 处理匿名类场景，若右边为object literal时，需考虑左边是什么类型注解，将涉及的class找出
        if (target.getCategory() !== ClassCategory.OBJECT) {
            return true;
        }
        if (!(fieldType instanceof ClassType)) {
            return true;
        }
        const fieldClass = scene.getClass(fieldType.getClassSignature());
        if (fieldClass === null) {
            return false;
        }
        // fieldClass为非本项目的内容时，表示调用到三方库、SDK等内容，不再继续进行查找
        if (fieldClass.getDeclaringArkFile().getProjectName() !== projectName) {
            return true;
        }
        if (fieldClass.getCategory() !== ClassCategory.CLASS) {
            return true;
        }
        targets.add(fieldClass);
        return true;
    }

    // 遍历此处的调用方法的所有return stmts，查找class
    // 此处需要区分返回值为class和object literal两种场景
    // 对于返回值为class的场景，需要查找此class的所有父class
    // 对于存在返回值为object literal的场景，需要查找左边field类型为class时的所有父class
    private handleInvokeExpr(scene: Scene, fieldType: Type, invokeExpr: AbstractInvokeExpr, targets: Set<ArkClass>, projectName: string): boolean {
        let canFindAllTargets = true;
        const callMethod = scene.getMethod(invokeExpr.getMethodSignature());
        if (callMethod === null) {
            return false;
        }
        // callMethod为非本项目的内容时，表示调用到三方库、SDK等内容，不再继续进行查找
        if (callMethod.getDeclaringArkFile().getProjectName() !== projectName) {
            return true;
        }
        const stmts = callMethod.getBody()?.getCfg().getStmts();
        if (stmts === undefined) {
            return false;
        }
        for (const stmt of stmts) {
            if (!(stmt instanceof ArkReturnStmt)) {
                continue;
            }
            const opType = stmt.getOp().getType();
            if (!(opType instanceof ClassType)) {
                continue;
            }
            const returnClass = scene.getClass(opType.getClassSignature());
            if (returnClass === null) {
                canFindAllTargets = false;
                continue;
            }
            if (returnClass.getCategory() === ClassCategory.CLASS) {
                targets.add(returnClass);
            } else if (returnClass.getCategory() === ClassCategory.OBJECT) {
                if (!(fieldType instanceof ClassType)) {
                    continue;
                }
                const leftClass = scene.getClass(fieldType.getClassSignature());
                if (leftClass === null) {
                    canFindAllTargets = false;
                    continue;
                }
                if (leftClass.getCategory() === ClassCategory.CLASS) {
                    targets.add(leftClass);
                }
            }
        }
        return canFindAllTargets;
    }

    // 采用广度优先遍历方式，逐层获取该class的所有父类，一直查找到基类
    // arkanalyzer getAllHeritageClasses有点问题，对于未能推出来的父类会忽略，不加入列表中返回。
    private getAllSuperClasses(arkClass: ArkClass, callback: (value: ArkClass) => void): void {
        let superClasses: Set<ArkClass> = new Set();
        const classes = arkClass.getAllHeritageClasses();
        while (classes.length > 0) {
            const superCls = classes.shift()!;
            const superSuperCls = superCls.getAllHeritageClasses();
            callback(superCls);

            if (superSuperCls.length > 0) {
                classes.push(...superSuperCls);
            }
        }
    }

    private generateIssueDescription(field: ArkField, issueClass: ArkClass | null, canFindAllTargets: boolean = true): string {
        if (issueClass === null || !canFindAllTargets) {
            return `can not find all classes, please check this field manually (arkui-data-observation)`;
        }
        const fieldLine = field.getOriginPosition().getLineNo();
        const fieldColumn = field.getOriginPosition().getColNo();

        const fieldFileSig = field.getDeclaringArkClass().getDeclaringArkFile().getFileSignature();
        const issueClassSig = issueClass.getDeclaringArkFile().getFileSignature();
        let res = `but it's not be annotated by @Observed (arkui-data-observation)`;
        if (fileSignatureCompare(fieldFileSig, issueClassSig)) {
            res = `Class ${issueClass.getName()} is used by state property in [${fieldLine}, ${fieldColumn}], ` + res;
        } else {
            const filePath = path.normalize(fieldFileSig.getFileName());
            res = `Class ${issueClass.getName()} is used by state property in file ${filePath} [${fieldLine}, ${fieldColumn}], ` + res;
        }
        return res;
    }

    private getClassPos(cls: ArkClass): WarnInfo {
        const arkFile = cls.getDeclaringArkFile();
        if (arkFile) {
            const originPath = path.normalize(arkFile.getFilePath());
            const line = cls.getLine();
            const startCol = cls.getColumn();
            const endCol = startCol;
            return { line, startCol, endCol, filePath: originPath };
        } else {
            logger.debug('ArkFile is null.');
            return { line: -1, startCol: -1, endCol: -1, filePath: '' };
        }
    }

    private getFieldPos(field: ArkField): WarnInfo {
        const arkFile = field.getDeclaringArkClass().getDeclaringArkFile();
        const pos = field.getOriginPosition();
        if (arkFile && pos) {
            const originPath = arkFile.getFilePath();
            const line = pos.getLineNo();
            const startCol = pos.getColNo();
            const endCol = startCol;
            return { line, startCol, endCol, filePath: originPath };
        } else {
            logger.debug('ArkFile is null.');
            return { line: -1, startCol: -1, endCol: -1, filePath: '' };
        }
    }

    private addIssueReport(warnInfo: WarnInfo, description: string, ruleFix?: RuleFix): void {
        const problem = 'DataObservationNeedObserved';
        const severity = this.rule.alert ?? this.metaData.severity;
        let defects = new Defects(
            warnInfo.line,
            warnInfo.startCol,
            warnInfo.endCol,
            problem,
            description,
            severity,
            this.rule.ruleId,
            warnInfo.filePath,
            this.metaData.ruleDocPath,
            true,
            false,
            true
        );
        this.issues.push(new IssueReport(defects, ruleFix));
        if (ruleFix === undefined) {
            defects.fixable = false;
        }
    }

    private generateRuleFix(warnInfo: WarnInfo, targetClass: ArkClass): RuleFix | null {
        const arkFile = targetClass.getDeclaringArkFile();
        const sourceFile = AstTreeUtils.getASTNode(arkFile.getName(), arkFile.getCode());
        const startLineRange = FixUtils.getLineRange(sourceFile, warnInfo.line);
        if (startLineRange === null) {
            return null;
        }

        const ruleFix = new RuleFix();
        ruleFix.range = startLineRange;

        const startLineStr = FixUtils.getSourceWithRange(sourceFile, startLineRange);
        if (startLineStr === null) {
            return null;
        }

        const eol = FixUtils.getEolSymbol(sourceFile, warnInfo.line);
        const startLineIndent = FixUtils.getIndentOfLine(sourceFile, warnInfo.line);
        if (startLineIndent === null) {
            return null;
        }
        const space = ' ';
        ruleFix.text = `@Observed${eol}${space.repeat(startLineIndent)}${startLineStr}`;
        return ruleFix;
    }
}
