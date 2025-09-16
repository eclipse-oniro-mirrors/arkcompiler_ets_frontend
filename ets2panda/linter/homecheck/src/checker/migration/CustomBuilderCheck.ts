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
    Scene,
    Local,
    Stmt,
    Type,
    ArkMethod,
    AliasType,
    AbstractInvokeExpr,
    Value,
    AstTreeUtils,
    ts,
    UnionType,
} from 'arkanalyzer';
import Logger, { LOG_MODULE_TYPE } from 'arkanalyzer/lib/utils/logger';
import { BaseChecker, BaseMetaData } from '../BaseChecker';
import { Rule, Defects, MatcherTypes, MatcherCallback, MethodMatcher } from '../../Index';
import { IssueReport } from '../../model/Defects';
import { RuleFix } from '../../model/Fix';
import { FixPosition, FixUtils } from '../../utils/common/FixUtils';
import { WarnInfo } from '../../utils/common/Utils';

const logger = Logger.getLogger(LOG_MODULE_TYPE.HOMECHECK, 'CustomBuilderCheck');
const gMetaData: BaseMetaData = {
    severity: 1,
    ruleDocPath: '',
    description: 'The CustomBuilder type parameter only accepts functions annotated with @Builder',
};

export class CustomBuilderCheck implements BaseChecker {
    readonly metaData: BaseMetaData = gMetaData;
    public rule: Rule;
    public defects: Defects[] = [];
    public issues: IssueReport[] = [];

    private buildMatcher: MethodMatcher = {
        matcherType: MatcherTypes.METHOD,
    };

    public registerMatchers(): MatcherCallback[] {
        const matchBuildCb: MatcherCallback = {
            matcher: this.buildMatcher,
            callback: this.check,
        };
        return [matchBuildCb];
    }

    public check = (target: ArkMethod): void => {
        const scene = target.getDeclaringArkFile().getScene();
        const stmts = target.getBody()?.getCfg().getStmts() ?? [];
        let locals = new Set<Local>();
        for (const stmt of stmts) {
            // 场景1：函数调用赋值给CustomBuilder类型的对象
            const local = this.isCallToBuilder(stmt, scene);
            if (local) {
                locals.add(local);
                continue;
            }
            const usage = this.isPassToCustomBuilder(stmt, locals);
            if (usage) {
                this.addIssueReport(usage.getDeclaringStmt()!, usage);
            }
        }
    };

    private isCallToBuilder(stmt: Stmt, scene: Scene): Local | undefined {
        if (!(stmt instanceof ArkAssignStmt)) {
            return undefined;
        }
        const leftOp = stmt.getLeftOp();
        if (!(leftOp instanceof Local)) {
            return undefined;
        }
        const rightOp = stmt.getRightOp();
        if (!(rightOp instanceof AbstractInvokeExpr)) {
            return undefined;
        }
        const method = scene.getMethod(rightOp.getMethodSignature());
        if (method && method.hasBuilderDecorator()) {
            return leftOp;
        }
        return undefined;
    }

    private isCustomBuilderTy(type: Type): boolean {
        const isRawCustomBuilderTy = (ty: Type): boolean => {
            return ty instanceof AliasType && ty.getName() === 'CustomBuilder';
        };
        return isRawCustomBuilderTy(type) || (type instanceof UnionType && type.getTypes().some(ty => isRawCustomBuilderTy(ty)));
    }

    private isPassToCustomBuilder(stmt: Stmt, locals: Set<Local>): Local | undefined {
        let res: Local | undefined = undefined;
        if (stmt instanceof ArkAssignStmt) {
            if (this.isCustomBuilderTy(stmt.getLeftOp().getType())) {
                const rightOp = stmt.getRightOp();
                if (rightOp instanceof Local && locals.has(rightOp)) {
                    res = rightOp;
                }
            }
        }
        if (res !== undefined) {
            return res;
        }
        const invokeExpr = stmt.getInvokeExpr();
        if (invokeExpr) {
            const paramTys = invokeExpr.getMethodSignature().getMethodSubSignature().getParameterTypes();
            const args = invokeExpr.getArgs();
            for (let i = 0; i < paramTys.length && i < args.length; ++i) {
                if (!this.isCustomBuilderTy(paramTys[i])) {
                    continue;
                }
                const arg = args[i];
                if (arg instanceof Local && locals.has(arg)) {
                    return arg;
                }
            }
        }
        return undefined;
    }

    private addIssueReport(stmt: Stmt, operand: Value): void {
        const severity = this.rule.alert ?? this.metaData.severity;
        const warnInfo = this.getLineAndColumn(stmt, operand);
        const problem = 'CustomBuilderTypeChanged';
        const desc = `${this.metaData.description} (${this.rule.ruleId.replace('@migration/', '')})`;
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
        const fixPosition: FixPosition = {
            startLine: warnInfo.line,
            startCol: warnInfo.startCol,
            endLine: -1,
            endCol: -1,
        };
        const ruleFix = this.generateRuleFix(fixPosition, stmt) ?? undefined;
        this.issues.push(new IssueReport(defects, ruleFix));
        if (ruleFix === undefined) {
            defects.fixable = false;
        }
    }

    private getLineAndColumn(stmt: Stmt, operand: Value): WarnInfo {
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

    private generateRuleFix(fixPosition: FixPosition, stmt: Stmt): RuleFix | null {
        let ruleFix: RuleFix = new RuleFix();
        const endPosition = this.getEndPositionOfStmt(stmt);
        if (endPosition) {
            fixPosition.endLine = endPosition.line;
            fixPosition.endCol = endPosition.col;
        }
        const arkFile = stmt.getCfg().getDeclaringMethod().getDeclaringArkFile();
        const sourceFile = AstTreeUtils.getASTNode(arkFile.getName(), arkFile.getCode());
        const range = FixUtils.getRangeWithAst(sourceFile, fixPosition);
        if (range === null) {
            return null;
        }
        ruleFix.range = range;
        const originalText = FixUtils.getSourceWithRange(sourceFile, range);
        if (originalText !== null) {
            ruleFix.text = this.generateReplaceText(sourceFile, originalText, fixPosition);
        } else {
            return null;
        }
        return ruleFix;
    }

    private getEndPositionOfStmt(stmt: Stmt): { line: number; col: number } | null {
        const allPositions = stmt.getOperandOriginalPositions();
        if (allPositions === undefined) {
            return null;
        }
        let res = { line: -1, col: -1 };
        allPositions.forEach(position => {
            if (position.getLastLine() > res.line) {
                res = { line: position.getLastLine(), col: position.getLastCol() };
                return;
            }
            if (position.getLastLine() === res.line && position.getLastCol() > res.col) {
                res = { line: position.getLastLine(), col: position.getLastCol() };
                return;
            }
        });
        return res;
    }

    private generateReplaceText(sourceFile: ts.SourceFile, originalText: string, fixPosition: FixPosition): string {
        // 已经是箭头函数的场景，无需任何处理
        if (originalText.includes('=>')) {
            return originalText;
        }

        // 非箭头函数包裹的函数调用，需要使用箭头函数包裹
        const eol = FixUtils.getEolSymbol(sourceFile, fixPosition.startLine);
        const startLineIndent = FixUtils.getIndentOfLine(sourceFile, fixPosition.startLine) ?? 0;
        const increaseSpaces = FixUtils.getIndentWidth(sourceFile, fixPosition.startLine);
        const space = ' ';

        let res = `() => {${eol}`;
        const originalLineStrs = originalText.split(eol);
        res += `${space.repeat(startLineIndent + increaseSpaces)}${originalLineStrs[0]}${eol}`;
        for (let index = 1; index < originalLineStrs.length; index++) {
            res += `${space.repeat(increaseSpaces)}${originalLineStrs[index]}${eol}`;
        }
        res += `${space.repeat(startLineIndent)}}`;
        return res;
    }
}
