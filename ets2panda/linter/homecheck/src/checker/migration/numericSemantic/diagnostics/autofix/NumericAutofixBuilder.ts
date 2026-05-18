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
    ArkAssignStmt,
    ArkField,
    ClassSignature,
    Local,
    Scene,
    Stmt,
    TEMP_LOCAL_PREFIX,
    Value,
} from 'arkanalyzer/lib';
import {
    ArkArrayRef,
    EnumValueType,
    ts,
} from 'arkanalyzer';
import { NumberConstant } from 'arkanalyzer/lib/core/base/Constant';
import Logger, { LOG_MODULE_TYPE } from 'arkanalyzer/lib/utils/logger';
import { RuleFix } from '../../../../../Index';
import { FixUtils } from '../../../../../utils/common/FixUtils';
import { WarnInfo } from '../../../../../utils/common/Utils';
import { COLON, ENDS_WITH_EQUALS, QUESTION_MARK, UNDEFINED_PART } from '../../../../../utils/common/ArrayIndexConstants';
import { IssueReason, NumberCategory } from '../../core/NumericSemanticTypes';
import { NumericLiteralUtils } from '../../core/NumericLiteralUtils';
import { NumericTypeAnnotationText } from '../../core/NumericTypeAnnotationText';

const logger = Logger.getLogger(LOG_MODULE_TYPE.HOMECHECK, 'NumericAutofixBuilder');

interface NumericAutofixBuilderOptions {
    scene: Scene;
    getSourceFile(field?: ArkField, issueStmt?: Stmt): ts.SourceFile | null;
}

export interface ApiFunctionReturnRuleFixResult {
    warnInfo: WarnInfo;
    fix: RuleFix;
}

export interface ApiFunctionParamRuleFixResult {
    warnInfo: WarnInfo;
    fix: RuleFix;
}

type ReturnTypeFixNode = ts.ArrowFunction | ts.FunctionExpression | ts.FunctionDeclaration | ts.MethodDeclaration;

export class NumericAutofixBuilder {
    constructor(private options: NumericAutofixBuilderOptions) {}

    public generateApiArgRuleFix(
        warnInfo: WarnInfo,
        issueReason: IssueReason,
        numberCategory: NumberCategory,
        issueStmt?: Stmt,
        value?: Value,
        field?: ArkField
    ): RuleFix | null {
        const sourceFile = this.options.getSourceFile(field, issueStmt);
        if (!sourceFile) {
            return null;
        }
        if (field) {
            return this.generateRuleFixForFieldDefine(sourceFile, warnInfo, numberCategory);
        }

        if (issueReason === IssueReason.OnlyUsedAsIntLong) {
            return this.generateRuleFixForLocalDefine(sourceFile, warnInfo, numberCategory);
        }
        return this.generateCastRuleFix(sourceFile, warnInfo, numberCategory, value);
    }

    public generateApiFunctionReturnRuleFix(
        callbackWarnInfo: WarnInfo,
        numberCategory: NumberCategory,
        issueStmt?: Stmt
    ): ApiFunctionReturnRuleFixResult | null {
        const sourceFile = this.options.getSourceFile(undefined, issueStmt);
        if (!sourceFile) {
            return null;
        }
        const callbackRange = this.getCallbackRange(sourceFile, callbackWarnInfo);
        if (!callbackRange) {
            return null;
        }
        const functionNode = this.findFunctionLikeNodeInRange(sourceFile, callbackRange);
        if (!functionNode) {
            return null;
        }
        if (functionNode.type) {
            return this.generateFunctionReturnTypeReplaceFix(sourceFile, callbackWarnInfo.filePath, functionNode.type, numberCategory);
        }
        return this.generateFunctionReturnTypeInsertFix(sourceFile, callbackWarnInfo.filePath, functionNode, numberCategory);
    }

    public generateApiMethodReturnRuleFix(
        returnWarnInfo: WarnInfo,
        numberCategory: NumberCategory,
        issueStmt?: Stmt
    ): ApiFunctionReturnRuleFixResult | null {
        const sourceFile = this.options.getSourceFile(undefined, issueStmt);
        if (!sourceFile) {
            return null;
        }
        const returnRange = FixUtils.getRangeWithAst(sourceFile, {
            startLine: returnWarnInfo.line,
            startCol: returnWarnInfo.startCol,
            endLine: returnWarnInfo.endLine ?? returnWarnInfo.line,
            endCol: returnWarnInfo.endCol,
        });
        if (!returnRange) {
            return null;
        }
        const functionNode = this.findEnclosingReturnTypeFixNode(sourceFile, returnRange);
        if (!functionNode) {
            return null;
        }
        if (functionNode.type) {
            return this.generateFunctionReturnTypeReplaceFix(sourceFile, returnWarnInfo.filePath, functionNode.type, numberCategory);
        }
        return this.generateFunctionReturnTypeInsertFix(sourceFile, returnWarnInfo.filePath, functionNode, numberCategory);
    }

    public generateApiFunctionParamRuleFix(
        callbackWarnInfo: WarnInfo,
        paramIndex: number,
        numberCategory: NumberCategory,
        issueStmt?: Stmt
    ): ApiFunctionParamRuleFixResult | null {
        const sourceFile = this.options.getSourceFile(undefined, issueStmt);
        if (!sourceFile) {
            return null;
        }
        const callbackRange = this.getCallbackRange(sourceFile, callbackWarnInfo);
        if (!callbackRange) {
            return null;
        }
        const functionNode = this.findFunctionLikeNodeInRange(sourceFile, callbackRange);
        if (!functionNode) {
            return null;
        }
        const parameter = functionNode.parameters[paramIndex];
        if (!parameter) {
            return null;
        }
        if (parameter.type) {
            return this.generateFunctionParamTypeReplaceFix(sourceFile, callbackWarnInfo.filePath, parameter.type, numberCategory);
        }
        return this.generateFunctionParamTypeInsertFix(sourceFile, callbackWarnInfo.filePath, parameter, numberCategory);
    }

    public generateApiReturnOrFieldRuleFix(warnInfo: WarnInfo, numberCategory: NumberCategory, issueStmt?: Stmt, field?: ArkField): RuleFix | null {
        const sourceFile = this.options.getSourceFile(field, issueStmt);
        if (!sourceFile) {
            return null;
        }
        if (field) {
            return this.generateRuleFixForFieldDefine(sourceFile, warnInfo, numberCategory);
        }

        const isOptionalField = this.isOptionalFieldAccess(issueStmt);
        return this.generateRuleFixForLocalDefine(sourceFile, warnInfo, numberCategory, isOptionalField);
    }

    private getCallbackRange(sourceFile: ts.SourceFile, warnInfo: WarnInfo): [number, number] | null {
        return FixUtils.getRangeWithAst(sourceFile, {
            startLine: warnInfo.line,
            startCol: warnInfo.startCol,
            endLine: warnInfo.endLine ?? warnInfo.line,
            endCol: warnInfo.endCol,
        });
    }

    private findFunctionLikeNodeInRange(
        sourceFile: ts.SourceFile,
        range: [number, number]
    ): ts.ArrowFunction | ts.FunctionExpression | null {
        let res: ts.ArrowFunction | ts.FunctionExpression | null = null;
        const visit = (node: ts.Node): void => {
            const start = node.getStart(sourceFile);
            const end = node.getEnd();
            if (end < range[0] || start > range[1]) {
                return;
            }
            if ((ts.isArrowFunction(node) || ts.isFunctionExpression(node)) && start >= range[0] && end <= range[1]) {
                if (!res || end - start < res.getEnd() - res.getStart(sourceFile)) {
                    res = node;
                }
            }
            ts.forEachChild(node, visit);
        };
        visit(sourceFile);
        return res;
    }

    private findEnclosingReturnTypeFixNode(sourceFile: ts.SourceFile, range: [number, number]): ReturnTypeFixNode | null {
        let res: ReturnTypeFixNode | null = null;
        const visit = (node: ts.Node): void => {
            const start = node.getStart(sourceFile);
            const end = node.getEnd();
            if (range[0] < start || range[1] > end) {
                return;
            }
            if (this.isReturnTypeFixNode(node)) {
                if (!res || end - start < res.getEnd() - res.getStart(sourceFile)) {
                    res = node;
                }
            }
            ts.forEachChild(node, visit);
        };
        visit(sourceFile);
        return res;
    }

    private isReturnTypeFixNode(node: ts.Node): node is ReturnTypeFixNode {
        return ts.isArrowFunction(node) ||
            ts.isFunctionExpression(node) ||
            ts.isFunctionDeclaration(node) ||
            ts.isMethodDeclaration(node);
    }

    private generateFunctionReturnTypeReplaceFix(
        sourceFile: ts.SourceFile,
        filePath: string,
        typeNode: ts.TypeNode,
        numberCategory: NumberCategory
    ): ApiFunctionReturnRuleFixResult | null {
        const range: [number, number] = [typeNode.getStart(sourceFile), typeNode.getEnd()];
        const originalText = FixUtils.getSourceWithRange(sourceFile, range);
        if (originalText === null || NumericTypeAnnotationText.containsTypeToken(originalText, numberCategory)) {
            return null;
        }
        if (!NumericTypeAnnotationText.containsTypeToken(originalText, NumberCategory.number)) {
            return null;
        }
        const fix = new RuleFix();
        fix.range = range;
        fix.text = NumericTypeAnnotationText.replaceTypeToken(originalText, NumberCategory.number, numberCategory);
        return {
            warnInfo: this.getWarnInfoFromRange(sourceFile, filePath, range),
            fix,
        };
    }

    private generateFunctionParamTypeReplaceFix(
        sourceFile: ts.SourceFile,
        filePath: string,
        typeNode: ts.TypeNode,
        numberCategory: NumberCategory
    ): ApiFunctionParamRuleFixResult | null {
        const range: [number, number] = [typeNode.getStart(sourceFile), typeNode.getEnd()];
        const originalText = FixUtils.getSourceWithRange(sourceFile, range);
        if (originalText === null || NumericTypeAnnotationText.containsTypeToken(originalText, numberCategory)) {
            return null;
        }
        if (!NumericTypeAnnotationText.containsTypeToken(originalText, NumberCategory.number)) {
            return null;
        }
        const fix = new RuleFix();
        fix.range = range;
        fix.text = NumericTypeAnnotationText.replaceTypeToken(originalText, NumberCategory.number, numberCategory);
        return {
            warnInfo: this.getWarnInfoFromRange(sourceFile, filePath, range),
            fix,
        };
    }

    private generateFunctionParamTypeInsertFix(
        sourceFile: ts.SourceFile,
        filePath: string,
        parameter: ts.ParameterDeclaration,
        numberCategory: NumberCategory
    ): ApiFunctionParamRuleFixResult | null {
        if (!ts.isIdentifier(parameter.name)) {
            return null;
        }
        const insertPos = parameter.questionToken?.getEnd() ?? parameter.name.getEnd();
        const fix = new RuleFix();
        fix.range = [insertPos, insertPos];
        fix.text = `: ${numberCategory}`;
        return {
            warnInfo: this.getWarnInfoFromRange(sourceFile, filePath, fix.range),
            fix,
        };
    }

    private generateFunctionReturnTypeInsertFix(
        sourceFile: ts.SourceFile,
        filePath: string,
        functionNode: ReturnTypeFixNode,
        numberCategory: NumberCategory
    ): ApiFunctionReturnRuleFixResult | null {
        const insertPos = ts.isArrowFunction(functionNode) ?
            functionNode.equalsGreaterThanToken.getStart(sourceFile) :
            functionNode.body?.getStart(sourceFile);
        if (insertPos === undefined) {
            return null;
        }
        const fix = new RuleFix();
        fix.range = [insertPos, insertPos];
        fix.text = `: ${numberCategory} `;
        return {
            warnInfo: this.getWarnInfoFromRange(sourceFile, filePath, fix.range),
            fix,
        };
    }

    private getWarnInfoFromRange(sourceFile: ts.SourceFile, filePath: string, range: [number, number]): WarnInfo {
        const start = sourceFile.getLineAndCharacterOfPosition(range[0]);
        const end = sourceFile.getLineAndCharacterOfPosition(range[1]);
        return {
            line: start.line + 1,
            startCol: start.character + 1,
            endLine: end.line + 1,
            endCol: end.character + 1,
            filePath,
        };
    }

    public generateIntConstantIndexRuleFix(warnInfo: WarnInfo, issueStmt: Stmt, constant: NumberConstant): RuleFix | null {
        if (!NumericLiteralUtils.isFloatActuallyInt(constant)) {
            return null;
        }
        const sourceFile = this.options.getSourceFile(undefined, issueStmt);
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

    public generateNumericLiteralRuleFix(warnInfo: WarnInfo, issueReason: IssueReason, issueStmt?: Stmt, value?: Value, field?: ArkField): RuleFix | null {
        const sourceFile = this.options.getSourceFile(field, issueStmt);
        if (!sourceFile) {
            return null;
        }

        if (field) {
            if (issueReason === IssueReason.OnlyUsedAsIntLong) {
                return this.generateRuleFixForFieldDefine(sourceFile, warnInfo, NumberCategory.int);
            }
            return this.generateRuleFixForFieldDefine(sourceFile, warnInfo, NumberCategory.number);
        }

        if (this.shouldFixValueLiteralDirectly(value)) {
            return this.generateLiteralValueRuleFix(sourceFile, warnInfo, value);
        }

        // 非整型字面量
        // warnInfo中对于变量声明语句的位置信息只包括变量名，不包括变量声明时的类型注解位置，此处获取变量名后到行尾的字符串信息，替换‘: number’ 或增加 ‘: int’
        if (issueReason === IssueReason.OnlyUsedAsIntLong) {
            return this.generateRuleFixForLocalDefine(sourceFile, warnInfo, NumberCategory.int);
        }
        return this.generateRuleFixForLocalDefine(sourceFile, warnInfo, NumberCategory.number);
    }

    private generateRuleFixForLocalDefine(sourceFile: ts.SourceFile, warnInfo: WarnInfo, numberCategory: NumberCategory, isOptional?: boolean): RuleFix | null {
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

        // 场景1：变量或函数入参，无类型注解的场景，直接在localString后面添加': int'，同时考虑可选参数即'?:'
        if (!restString.trimStart().startsWith(COLON) && !restString.trimStart().startsWith(QUESTION_MARK)) {
            const ruleFix = new RuleFix();
            ruleFix.range = localRange;
            const localString = FixUtils.getSourceWithRange(sourceFile, ruleFix.range);
            if (!localString) {
                logger.error('Failed to getting text of the fix range info when generating auto fix info.');
                return null;
            }
            if (localString.includes(COLON)) {
                return this.generateRuleFixForTypedText(ruleFix.range, localString, numberCategory);
            }
            ruleFix.text = isOptional ? `${localString}: ${numberCategory}${UNDEFINED_PART}` : `${localString}: ${numberCategory}`;
            if (restString.trimStart().startsWith(ENDS_WITH_EQUALS)) {
                ruleFix.text = `(${ruleFix.text})`;
            }
            return ruleFix;
        }

        return this.generateRuleFixForTypedLocal(sourceFile, localRange, restString, numberCategory);
    }

    private generateRuleFixForTypedLocal(
        sourceFile: ts.SourceFile,
        localRange: [number, number],
        restString: string,
        numberCategory: NumberCategory
    ): RuleFix | null {
        // 场景2：变量或函数入参，有类型注解的场景，需要将类型注解替换成新的类型，同时考虑可选参数即'?:'
        const match = restString.match(/^(\s*\??\s*:[^=,);]+)([\s\S]*)$/);
        if (match === null || match.length < 3) {
            return null;
        }
        // 如果需要替换成number，但是已经存在类型注解number，则返回null，不需要告警和自动修复
        if (NumericTypeAnnotationText.containsTypeToken(match[1], numberCategory)) {
            return null;
        }
        const ruleFix = new RuleFix();
        ruleFix.range = [localRange[0], localRange[1] + match[1].length];
        const localString = FixUtils.getSourceWithRange(sourceFile, ruleFix.range);
        if (!localString) {
            logger.error('Failed to getting text of the fix range info when generating auto fix info.');
            return null;
        }
        return this.generateRuleFixForTypedText(ruleFix.range, localString, numberCategory);
    }

    private generateRuleFixForTypedText(range: [number, number], localString: string, numberCategory: NumberCategory): RuleFix | null {
        const parts = localString.split(':');
        if (parts.length !== 2) {
            logger.error('Failed to getting text of the fix range info when generating auto fix info.');
            return null;
        }
        if (NumericTypeAnnotationText.containsTypeToken(parts[1], numberCategory)) {
            return null;
        }
        if (!NumericTypeAnnotationText.containsTypeToken(parts[1], NumberCategory.number)) {
            // 原码含有类型注解但是其类型中不含number，无法进行替换
            return null;
        }
        const ruleFix = new RuleFix();
        ruleFix.range = range;
        ruleFix.text = `${parts[0].trimEnd()}: ${NumericTypeAnnotationText.replaceTypeToken(parts[1].trimStart(), NumberCategory.number, numberCategory)}`;
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

        if (this.isTypedFieldText(fullValueString)) {
            return this.generateRuleFixForTypedField(sourceFile, fullRange, fullValueString, numberCategory);
        }
        return this.generateRuleFixForInferredField(sourceFile, fullRange, fullValueString, numberCategory);
    }

    private isTypedFieldText(fullValueString: string): boolean {
        return /^([^=;]+:[^=;]+)([\s\S]*)$/.test(fullValueString);
    }

    private generateRuleFixForTypedField(
        sourceFile: ts.SourceFile,
        fullRange: [number, number],
        fullValueString: string,
        numberCategory: NumberCategory
    ): RuleFix | null {
        // 场景1：对于类属性private a: number 或 private a: number = xxx, fullValueString为private开始到行尾的内容，需要替换为private a: int
        const match = fullValueString.match(/^([^=;]+:[^=;]+)([\s\S]*)$/);
        if (match === null || match.length <= 2) {
            return null;
        }
        const ruleFix = new RuleFix();
        ruleFix.range = [fullRange[0], fullRange[0] + match[1].length];
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
        if (NumericTypeAnnotationText.containsTypeToken(parts[1], numberCategory)) {
            // 判断field是否已经有正确的类型注解
            return null;
        }
        if (!NumericTypeAnnotationText.containsTypeToken(parts[1], NumberCategory.number)) {
            // 原码含有类型注解但是其类型中不含number，无法进行替换
            return null;
        }
        ruleFix.text = `${parts[0].trimEnd()}: ${NumericTypeAnnotationText.replaceTypeToken(parts[1].trimStart(), NumberCategory.number, numberCategory)}`;
        return ruleFix;
    }

    private generateRuleFixForInferredField(
        sourceFile: ts.SourceFile,
        fullRange: [number, number],
        fullValueString: string,
        numberCategory: NumberCategory
    ): RuleFix | null {
        // 场景2：对于private a = 123，originalText为private开始到行尾的内容，需要替换为private a: int = 123
        const match = fullValueString.match(/^([^=;]+)([\s\S]*)$/);
        if (match === null || match.length <= 2) {
            // 正常情况下不会走到此处，因为field一定有类型注解或初始化值来确定其类型
            return null;
        }
        const ruleFix = new RuleFix();
        ruleFix.range = [fullRange[0], fullRange[0] + match[1].trimEnd().length];
        const originalText = FixUtils.getSourceWithRange(sourceFile, ruleFix.range);
        if (!originalText) {
            logger.error('Failed to getting text of the fix range info when generating auto fix info.');
            return null;
        }
        ruleFix.text = `${originalText}: ${numberCategory}`;
        return ruleFix;
    }

    private generateCastRuleFix(sourceFile: ts.SourceFile, warnInfo: WarnInfo, numberCategory: NumberCategory, value?: Value): RuleFix | null {
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
        if (value === undefined) {
            logger.error('Missing issue SDK arg when generating auto fix info.');
            return null;
        }
        const transStr = this.getTransStr(numberCategory);
        if (!transStr) {
            return null;
        }

        const ruleFix = new RuleFix();
        ruleFix.range = range;
        const fixText = this.getCastRuleFixText(value, valueString, transStr);
        if (fixText === null) {
            return null;
        }
        ruleFix.text = fixText;
        return ruleFix;
    }

    private getTransStr(numberCategory: NumberCategory): string | null {
        if (numberCategory === NumberCategory.int) {
            return '.toInt()';
        }
        if (numberCategory === NumberCategory.long) {
            return '.toLong()';
        }
        logger.error(`Have not support number category ${numberCategory} yet.`);
        return null;
    }

    private getCastRuleFixText(value: Value, valueString: string, transStr: string): string | null {
        if (!(value instanceof Local)) {
            return `(${valueString})${transStr}`;
        }
        if (!value.getName().startsWith(TEMP_LOCAL_PREFIX)) {
            return `${valueString}${transStr}`;
        }
        const declaringStmt = value.getDeclaringStmt();
        if (declaringStmt === null) {
            return `(${valueString})${transStr}`;
        }
        if (!(declaringStmt instanceof ArkAssignStmt)) {
            logger.error('Temp local declaring stmt must be assign stmt.');
            return null;
        }
        const rightOp = declaringStmt.getRightOp();
        if (rightOp instanceof AbstractInvokeExpr || rightOp instanceof AbstractFieldRef || rightOp instanceof ArkArrayRef) {
            return `${valueString}${transStr}`;
        }
        return `(${valueString})${transStr}`;
    }

    private isOptionalFieldAccess(issueStmt?: Stmt): boolean | undefined {
        if (!(issueStmt instanceof ArkAssignStmt)) {
            return undefined;
        }
        const rightOp = issueStmt.getRightOp();
        if (!(rightOp instanceof AbstractFieldRef)) {
            return undefined;
        }
        const fieldSig = rightOp.getFieldSignature();
        const declaringSig = fieldSig.getDeclaringSignature();
        if (!(declaringSig instanceof ClassSignature)) {
            return undefined;
        }
        const baseClass = this.options.scene.getClass(declaringSig);
        const baseField = baseClass?.getField(fieldSig);
        return !!baseField?.getQuestionToken();
    }

    private shouldFixValueLiteralDirectly(value?: Value): boolean {
        return (value instanceof Local && value.getName().startsWith(TEMP_LOCAL_PREFIX) && value.getType() instanceof EnumValueType) ||
            value instanceof NumberConstant;
    }

    private generateLiteralValueRuleFix(sourceFile: ts.SourceFile, warnInfo: WarnInfo, value?: Value): RuleFix | null {
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

        if (value instanceof NumberConstant) {
            // 场景1：对整型字面量进行自动修复，转成浮点字面量，例如1->1.0
            if (NumericLiteralUtils.isNumberConstantActuallyFloat(value)) {
                // 无需修复
                return null;
            }
            ruleFix.text = NumericLiteralUtils.createFixTextForIntLiteral(value.getValue());
            return ruleFix;
        }

        // 场景2：对enum.A这样的枚举类型进行自动修复成enum.A.valueOf().toDouble()
        const valueStr = FixUtils.getSourceWithRange(sourceFile, range);
        if (valueStr === null) {
            logger.error('Failed to getting enum source code with range info.');
            return null;
        }
        ruleFix.text = NumericLiteralUtils.createFixTextForEnumValue(valueStr);
        return ruleFix;
    }
}
