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
    ArkAssignStmt,
    ArkIfStmt,
    ArkInstanceFieldRef,
    ArkInstanceInvokeExpr,
    ArkMethod,
    ClassType,
    FunctionType,
    Local,
    Stmt,
    Value,
    Scene,
    ArkNewExpr,
    ArkClass,
    ClassSignature,
    ArkReturnStmt,
    ArkInvokeStmt, ArkStaticInvokeExpr,
} from 'arkanalyzer';
import Logger, { LOG_MODULE_TYPE } from 'arkanalyzer/lib/utils/logger';
import { BaseChecker, BaseMetaData } from '../BaseChecker';
import { Rule, Defects, MatcherTypes, MethodMatcher, MatcherCallback } from '../../Index';
import { IssueReport } from '../../model/Defects';
import { WarnInfo } from '../../utils/common/Utils';
import {ArkPtrInvokeExpr} from "arkanalyzer/lib";
import {Language} from "arkanalyzer/lib/core/model/ArkFile";

const logger = Logger.getLogger(LOG_MODULE_TYPE.HOMECHECK, 'ThisBindCheck');

const ARKTS_RULE_ID = '@migration/arkts-instance-method-bind-this';
const arktsMetaData: BaseMetaData = {
    severity: 1,
    ruleDocPath: '',
    description: "Instance method shall bind the 'this' by default",
};

const ARKUI_RULE_ID = '@migration/arkui-buildparam-passing';
const arkuiMetaData: BaseMetaData = {
    severity: 1,
    ruleDocPath: '',
    description:
        'The execution context of the function annotated with @Builder is determined at the time of declaration. Please check the code carefully to ensure the correct function context',
};
const LOCAL_BUILDER_PREFIX = 'The @LocalBuilder decorator is not supported, it will be transformed to @Builder decorator. ';

export class ThisBindCheck implements BaseChecker {
    readonly metaData: BaseMetaData = arktsMetaData;
    public rule: Rule;
    public defects: Defects[] = [];
    public issues: IssueReport[] = [];

    private methodMatcher: MethodMatcher = {
        matcherType: MatcherTypes.METHOD,
    };

    public registerMatchers(): MatcherCallback[] {
        const methodCb: MatcherCallback = {
            matcher: this.methodMatcher,
            callback: this.check,
        };
        return [methodCb];
    }

    public check = (targetMtd: ArkMethod): void => {
        if (targetMtd.getLanguage() !== Language.ARKTS1_2) {
            return;
        }
        const file = targetMtd.getDeclaringArkFile();
        if (file.getName().endsWith('.test.ets') || file.getName().endsWith('.ts') || file.getName().endsWith('.js')) {
            return;
        }
        const scene = file.getScene();
        const stmts = targetMtd.getBody()?.getCfg().getStmts() ?? [];
        for (let i = 0; i < stmts.length; ++i) {
            const stmt = stmts[i];
            // const method = a.foo
            if (!(stmt instanceof ArkAssignStmt)) {
                continue;
            }
            const rightOp = stmt.getRightOp();
            if (!(rightOp instanceof ArkInstanceFieldRef)) {
                continue;
            }
            const base = rightOp.getBase();
            const classTy = base.getType();
            if (!(classTy instanceof ClassType)) {
                continue;
            }
            let isArrowType = false;
            if (rightOp.getFieldSignature().getType() instanceof ClassType) {
                if (rightOp.getFieldSignature().getType().toString().includes('Function')) {
                    isArrowType = true;
                }
            }
            let isFuncType = false;
            if (rightOp.getFieldSignature().getType() instanceof FunctionType) {
                isFuncType = true;
            }
            if (!(isArrowType || isFuncType)) {
                continue;
            }

            const klass = scene.getClass(classTy.getClassSignature());
            let isBuilder = false;
            let isLocalBuilder = false;
            if (klass && isFuncType) {
                const method = klass?.getMethodWithName(rightOp.getFieldName());
                if (!method || !method.getCfg() || !this.useThisInBody(method)) {
                    continue;
                }
                isBuilder = method.hasBuilderDecorator();
                isLocalBuilder = method.hasDecorator('LocalBuilder');
            }
            const leftOp = stmt.getLeftOp();

            if (i + 1 >= stmts.length || !this.hasBindThis(leftOp, stmt)) {
                this.addIssueReport(stmt, isBuilder, isLocalBuilder, base, scene);
            }
        }
    };

    private useThisInBody(method: ArkMethod): boolean {
        const thisInstance = (method.getThisInstance() as Local)!;
        const usedStmts = thisInstance.getUsedStmts();
        if (method.getName() !== 'constructor') {
            return usedStmts.length > 0;
        }
        // constructor方法一定会有return this语句，此句若为ArkAnalyzer为constructor方法自动生成，则不在检查范围内
        for (const stmt of usedStmts) {
            if (stmt instanceof ArkReturnStmt && stmt.getOriginPositionInfo().getLineNo() <= 0) {
                continue;
            }
            return true;
        }
        return false;
    }

    private isSafeUse(v: Value): boolean {
        if (!(v instanceof Local)) {
            return false;
        }

        const users = v.getUsedStmts();
        if (users.length === 0) {
            return true;
        }
        for (const user of users) {
            if (user instanceof ArkIfStmt) {
                const cond = user.getConditionExpr();
                if (v !== cond.getOp1() && v !== cond.getOp2()) {
                    return false;
                }
            } else if(user instanceof ArkAssignStmt) {
                if (user.getRightOp() instanceof ArkPtrInvokeExpr || user.getRightOp() instanceof ArkStaticInvokeExpr) {
                    return false;
                }
            } else if (user instanceof ArkReturnStmt) {
                return false;
            } else if (user instanceof ArkInvokeStmt) {
                return false;
            }
        }
        return true;
    }

    private hasBindThis(base: Value, next: Stmt): boolean {
        if (!(next instanceof ArkAssignStmt)) {
            return false;
        }
        const rightOp = next.getRightOp();
        const txt = next.getOriginalText();
        if (rightOp instanceof ArkInstanceFieldRef && txt?.includes('.bind')) {
            // const method = a.foo.bind()
            return true;
        }
        return false;
    }

    private addIssueReport(stmt: ArkAssignStmt, isBuilder: boolean, isLocalBuilder: boolean, operand: Value, scene: Scene): void {
        if ((isBuilder || isLocalBuilder) && this.isAssignToBuilderParam(stmt, scene)) {
            this.reportArkUIIssue(stmt, operand, isLocalBuilder);
        } else {
            this.reportArkTSIssue(stmt, operand);
        }
    }

    private isAssignToBuilderParam(assign: ArkAssignStmt, scene: Scene): boolean {
        /**
         *  class CA {
         *    @Builder builder() { ... }
         *    build() {
         *      Column() { CB({ content: this.builder }) }
         *    }
         *  }
         *  class CB {
         *    @BuilderParam content: () => void
         *  }
         *
         * ==================================================
         *  class %AC2$CA.build {
         *      constructor() { ... }
         *      %instInit() {
         *          %0 = this.builder
         *          this.content = %0
         *      }
         *  }
         *  class CA {
         *    ...
         *    build() {
         *      ...
         *      %3 = new %AC2$CA.build
         *      %3.constructor()
         *      %4 = new CB
         *      %4.constructor(%3)
         *      ...
         *    }
         *    ...
         *  }
         */
        const currentMethod = assign.getCfg().getDeclaringMethod();
        if (currentMethod.getName() !== '%instInit') {
            return false;
        }
        const currentClass = currentMethod.getDeclaringArkClass();
        if (!currentClass.isAnonymousClass()) {
            return false;
        }
        const currentClassSig = currentClass.getSignature();

        const leftOp = assign.getLeftOp();
        if (!(leftOp instanceof Local)) {
            return false;
        }
        const usedStmts = leftOp.getUsedStmts();
        if (usedStmts.length !== 1) {
            return false;
        }
        const usedStmt = usedStmts[0];
        if (!(usedStmt instanceof ArkAssignStmt)) {
            return false;
        }
        const target = usedStmt.getLeftOp();
        if (!(target instanceof ArkInstanceFieldRef)) {
            return false;
        }
        const baseTy = target.getBase().getType();
        if (!(baseTy instanceof ClassType)) {
            return false;
        }
        if ((baseTy as ClassType).getClassSignature() !== currentClassSig) {
            return false;
        }
        const fieldName = target.getFieldName();

        const declaringClassName = currentClassSig.getDeclaringClassName();
        if (declaringClassName === currentClass.getName()) {
            return false;
        }
        const declaringClass = currentClass.getDeclaringArkFile().getClassWithName(declaringClassName);
        if (!declaringClass) {
            return false;
        }
        let targetClassSig = this.findDefinitionOfAnonymousClass(declaringClass, currentClassSig);
        if (!targetClassSig) {
            return false;
        }
        const targetClass = scene.getClass(targetClassSig);
        if (!targetClass) {
            return false;
        }
        const arkField = targetClass.getFieldWithName(fieldName);
        if (!arkField) {
            return false;
        }
        return arkField.hasBuilderParamDecorator();
    }

    private findDefinitionOfAnonymousClass(declaringClass: ArkClass, anonymousClassSig: ClassSignature): ClassSignature | undefined {
        for (const m of declaringClass.getMethods()) {
            const stmts = m.getBody()?.getCfg().getStmts() ?? [];
            for (const stmt of stmts) {
                if (!(stmt instanceof ArkAssignStmt)) {
                    continue;
                }
                const rightOp = stmt.getRightOp();
                if (!(rightOp instanceof ArkNewExpr)) {
                    continue;
                }
                if (rightOp.getClassType().getClassSignature() !== anonymousClassSig) {
                    continue;
                }
                const local = stmt.getLeftOp() as Local;
                const classSignature = this.processUsedStmts(local, anonymousClassSig);
                if (!classSignature) {
                    continue;
                }
                return classSignature;
            }
        }
        return undefined;
    }

    private processUsedStmts(local: Local, anonymousClassSig: ClassSignature): ClassSignature | null {
        for (const usedStmt of local.getUsedStmts()) {
            const invoke = usedStmt.getInvokeExpr();
            if (!invoke) {
                continue;
            }
            const sig = invoke.getMethodSignature();
            if (sig.getMethodSubSignature().getMethodName() !== 'constructor') {
                continue;
            }
            if (sig.getDeclaringClassSignature() === anonymousClassSig) {
                continue;
            }
            return sig.getDeclaringClassSignature();
        }
        return null;
    }

    private reportArkTSIssue(stmt: ArkAssignStmt, operand: Value): void {
        const severity = this.rule.alert ?? this.metaData.severity;
        const warnInfo = this.getLineAndColumn(stmt, operand);
        const problem = 'DefaultBindThis';
        const desc = `${this.metaData.description} (${this.rule.ruleId.replace('@migration/', '')})`;
        let defects = new Defects(
            warnInfo.line,
            warnInfo.startCol,
            warnInfo.endCol,
            problem,
            desc,
            severity,
            ARKTS_RULE_ID,
            warnInfo.filePath,
            this.metaData.ruleDocPath,
            true,
            false,
            false
        );
        this.issues.push(new IssueReport(defects, undefined));
    }

    private reportArkUIIssue(stmt: ArkAssignStmt, operand: Value, isLocalBuilder: boolean): void {
        const severity = this.rule.alert ?? arkuiMetaData.severity;
        const warnInfo = this.getLineAndColumn(stmt, operand);
        const problem = 'BuilderParamContextChanged';
        const desc = `${isLocalBuilder ? LOCAL_BUILDER_PREFIX : ''}${arkuiMetaData.description} (${ARKUI_RULE_ID.replace('@migration/', '')})`;
        let defects = new Defects(
            warnInfo.line,
            warnInfo.startCol,
            warnInfo.endCol,
            problem,
            desc,
            severity,
            ARKUI_RULE_ID,
            warnInfo.filePath,
            arkuiMetaData.ruleDocPath,
            true,
            false,
            false
        );
        this.issues.push(new IssueReport(defects, undefined));
    }

    private getLineAndColumn(stmt: ArkAssignStmt, operand: Value): WarnInfo {
        const arkFile = stmt.getCfg().getDeclaringMethod().getDeclaringArkFile();
        const originPosition = stmt.getOperandOriginalPosition(operand);
        if (arkFile && originPosition) {
            const originPath = arkFile.getFilePath();
            const line = originPosition.getFirstLine();
            const startCol = originPosition.getFirstCol();
            const endCol = startCol;
            return { line, startCol, endCol, filePath: originPath };
        } else {
            logger.debug('ArkFile is null.');
        }
        return { line: -1, startCol: -1, endCol: -1, filePath: '' };
    }
}
