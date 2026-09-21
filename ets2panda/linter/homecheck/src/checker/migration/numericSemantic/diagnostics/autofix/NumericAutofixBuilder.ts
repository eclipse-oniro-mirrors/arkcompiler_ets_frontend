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

interface ConflictCheckContext {
    currentStart: number;
    currentEnd: number;
    paramIndex: number;
}

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
                return this.generateRuleFixForTypedText(sourceFile, ruleFix.range, localString, numberCategory);
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
        return this.generateRuleFixForTypedText(sourceFile, ruleFix.range, localString, numberCategory);
    }

    private generateRuleFixForTypedText(
        sourceFile: ts.SourceFile,
        range: [number, number],
        localString: string,
        numberCategory: NumberCategory
    ): RuleFix | null {
        const declNode = this.findDeclarationNodeAtRange(sourceFile, range);
        if (declNode && declNode.type) {
            return this.generateTypeNodeReplaceFix(sourceFile, declNode.type, numberCategory);
        }
        const colonIndex = localString.indexOf(':');
        if (colonIndex === -1) {
            logger.error('Failed to getting text of the fix range info when generating auto fix info.');
            return null;
        }
        const namePart = localString.substring(0, colonIndex);
        const typePart = localString.substring(colonIndex + 1);
        if (NumericTypeAnnotationText.containsTypeToken(typePart, numberCategory)) {
            return null;
        }
        if (!NumericTypeAnnotationText.containsTypeToken(typePart, NumberCategory.number)) {
            return null;
        }
        const ruleFix = new RuleFix();
        ruleFix.range = range;
        ruleFix.text = `${namePart.trimEnd()}: ${NumericTypeAnnotationText.replaceTypeToken(typePart.trimStart(), NumberCategory.number, numberCategory)}`;
        return ruleFix;
    }

    private findDeclarationNodeAtRange(
        sourceFile: ts.SourceFile,
        range: [number, number]
    ): ts.ParameterDeclaration | ts.VariableDeclaration | null {
        let result: ts.ParameterDeclaration | ts.VariableDeclaration | null = null;
        const visit = (node: ts.Node): void => {
            if (result) {
                return;
            }
            const start = node.getStart(sourceFile);
            const end = node.getEnd();
            if (end < range[0] || start > range[1]) {
                return;
            }
            if (this.isDeclarationAtRange(node, sourceFile, range)) {
                result = node as ts.ParameterDeclaration | ts.VariableDeclaration;
                return;
            }
            ts.forEachChild(node, visit);
        };
        visit(sourceFile);
        return result;
    }

    private isDeclarationAtRange(
        node: ts.Node,
        sourceFile: ts.SourceFile,
        range: [number, number]
    ): boolean {
        if (!ts.isParameter(node) && !ts.isVariableDeclaration(node)) {
            return false;
        }
        if (!node.name) {
            return false;
        }
        return node.name.getStart(sourceFile) === range[0];
    }

    private generateTypeNodeReplaceFix(
        sourceFile: ts.SourceFile,
        typeNode: ts.TypeNode,
        numberCategory: NumberCategory
    ): RuleFix | null {
        const typeRange: [number, number] = [typeNode.getStart(sourceFile), typeNode.getEnd()];
        const originalText = FixUtils.getSourceWithRange(sourceFile, typeRange);
        if (!originalText) {
            return null;
        }
        if (NumericTypeAnnotationText.containsTypeToken(originalText, numberCategory)) {
            return null;
        }
        if (!NumericTypeAnnotationText.containsTypeToken(originalText, NumberCategory.number)) {
            return null;
        }
        const ruleFix = new RuleFix();
        ruleFix.range = typeRange;
        ruleFix.text = NumericTypeAnnotationText.replaceTypeToken(originalText, NumberCategory.number, numberCategory);
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
        const colonIndex = localString.indexOf(':');
        if (colonIndex === -1) {
            logger.error('Failed to getting text of the fix range info when generating auto fix info.');
            return null;
        }
        const namePart = localString.substring(0, colonIndex);
        const typePart = localString.substring(colonIndex + 1);
        if (NumericTypeAnnotationText.containsTypeToken(typePart, numberCategory)) {
            return null;
        }
        if (!NumericTypeAnnotationText.containsTypeToken(typePart, NumberCategory.number)) {
            return null;
        }
        ruleFix.text = `${namePart.trimEnd()}: ${NumericTypeAnnotationText.replaceTypeToken(typePart.trimStart(), NumberCategory.number, numberCategory)}`;
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

    public generateCompanionTypeAnnotationFixes(
        warnInfo: WarnInfo,
        numberCategory: NumberCategory,
        issueStmt?: Stmt
    ): RuleFix[] | null {
        const sourceFile = this.options.getSourceFile(undefined, issueStmt);
        if (!sourceFile) {
            return [];
        }
        const paramRange = FixUtils.getRangeWithAst(sourceFile, {
            startLine: warnInfo.line,
            startCol: warnInfo.startCol,
            endLine: warnInfo.line,
            endCol: warnInfo.endCol,
        });
        if (!paramRange) {
            return [];
        }
        return this.findCompanionTypeAnnotationFixes(sourceFile, paramRange, numberCategory);
    }

    private findCompanionTypeAnnotationFixes(
        sourceFile: ts.SourceFile,
        paramRange: [number, number],
        numberCategory: NumberCategory
    ): RuleFix[] | null {
        const arrowFunc = this.findEnclosingArrowFunction(sourceFile, paramRange);
        if (!arrowFunc) {
            return [];
        }
        const paramDecl = this.findDeclarationNodeAtRange(sourceFile, paramRange);
        if (!paramDecl || !ts.isParameter(paramDecl)) {
            return [];
        }
        const paramIndex = arrowFunc.parameters.indexOf(paramDecl);
        if (paramIndex < 0) {
            return [];
        }

        const varDecl = this.findAssignedVariableDeclaration(sourceFile, arrowFunc);

        if (this.hasSharedDeclarationConflict(sourceFile, arrowFunc, paramIndex, varDecl)) {
            return null;
        }

        const fixes: RuleFix[] = [];

        if (varDecl && varDecl.type) {
            const fix = this.createCompanionFixFromTypeReference(sourceFile, varDecl.type, paramIndex, numberCategory);
            if (fix) {
                this.addUniqueFix(fixes, fix);
            }
        }

        const returnFix = this.findReturnStatementCompanionFix(sourceFile, arrowFunc, paramIndex, numberCategory);
        if (returnFix) {
            this.addUniqueFix(fixes, returnFix);
        }

        const propertyFix = this.findObjectPropertyCompanionFix(sourceFile, arrowFunc, paramIndex, numberCategory);
        if (propertyFix) {
            this.addUniqueFix(fixes, propertyFix);
        }

        const classFieldFix = this.findClassFieldCompanionFix(sourceFile, arrowFunc, paramIndex, numberCategory);
        if (classFieldFix) {
            this.addUniqueFix(fixes, classFieldFix);
        }

        const outerArrowReturnFix = this.findOuterArrowReturnCompanionFix(sourceFile, arrowFunc, paramIndex, numberCategory);
        if (outerArrowReturnFix) {
            this.addUniqueFix(fixes, outerArrowReturnFix);
        }

        return fixes;
    }

    private addUniqueFix(fixes: RuleFix[], fix: RuleFix): void {
        const exists = fixes.some(f => f.range[0] === fix.range[0] && f.range[1] === fix.range[1]);
        if (!exists) {
            fixes.push(fix);
        }
    }

    private createCompanionFixFromTypeNode(
        sourceFile: ts.SourceFile,
        typeNode: ts.TypeNode,
        paramIndex: number,
        numberCategory: NumberCategory
    ): RuleFix | null {
        if (!ts.isFunctionTypeNode(typeNode)) {
            return null;
        }
        const correspondingParam = typeNode.parameters[paramIndex];
        if (!correspondingParam || !correspondingParam.type) {
            return null;
        }
        return this.generateTypeNodeReplaceFix(sourceFile, correspondingParam.type, numberCategory);
    }

    private createCompanionFixFromTypeReference(
        sourceFile: ts.SourceFile,
        typeNode: ts.TypeNode,
        paramIndex: number,
        numberCategory: NumberCategory
    ): RuleFix | null {
        if (ts.isFunctionTypeNode(typeNode)) {
            return this.createCompanionFixFromTypeNode(sourceFile, typeNode, paramIndex, numberCategory);
        }
        if (ts.isTypeReferenceNode(typeNode)) {
            const typeName = typeNode.typeName.getText(sourceFile);
            const typeAlias = this.findTypeAliasDeclaration(sourceFile, typeName);
            if (typeAlias && typeAlias.type) {
                return this.createCompanionFixFromTypeNode(sourceFile, typeAlias.type, paramIndex, numberCategory);
            }
        }
        return null;
    }

    private findReturnStatementCompanionFix(
        sourceFile: ts.SourceFile,
        arrowFunc: ts.ArrowFunction,
        paramIndex: number,
        numberCategory: NumberCategory
    ): RuleFix | null {
        const funcDecl = this.findEnclosingFunctionDeclaration(sourceFile, arrowFunc);
        if (!funcDecl || !funcDecl.type) {
            return null;
        }
        if (ts.isFunctionTypeNode(funcDecl.type)) {
            return this.createCompanionFixFromTypeNode(sourceFile, funcDecl.type, paramIndex, numberCategory);
        }
        if (ts.isTypeReferenceNode(funcDecl.type)) {
            const typeName = funcDecl.type.typeName.getText(sourceFile);
            const typeAlias = this.findTypeAliasDeclaration(sourceFile, typeName);
            if (typeAlias && typeAlias.type) {
                return this.createCompanionFixFromTypeNode(sourceFile, typeAlias.type, paramIndex, numberCategory);
            }
        }
        return null;
    }

    private findObjectPropertyCompanionFix(
        sourceFile: ts.SourceFile,
        arrowFunc: ts.ArrowFunction,
        paramIndex: number,
        numberCategory: NumberCategory
    ): RuleFix | null {
        const propAssignment = this.findEnclosingPropertyAssignment(sourceFile, arrowFunc);
        if (!propAssignment) {
            return null;
        }
        const propName = propAssignment.name.getText(sourceFile);
        const objLiteral = this.findEnclosingObjectLiteral(sourceFile, propAssignment);
        if (!objLiteral) {
            return null;
        }
        const varDecl = this.findVariableDeclarationForObjectLiteral(sourceFile, objLiteral);
        if (!varDecl || !varDecl.type) {
            return null;
        }
        if (ts.isTypeReferenceNode(varDecl.type)) {
            const typeName = varDecl.type.typeName.getText(sourceFile);
            return this.findPropertyFixInTypeReference(sourceFile, typeName, propName, paramIndex, numberCategory);
        }
        if (ts.isTypeLiteralNode(varDecl.type)) {
            return this.findPropertyFixInMembers(sourceFile, varDecl.type.members, propName, paramIndex, numberCategory);
        }
        return null;
    }

    private findPropertyFixInTypeReference(
        sourceFile: ts.SourceFile,
        typeName: string,
        propName: string,
        paramIndex: number,
        numberCategory: NumberCategory
    ): RuleFix | null {
        const interfaceDecl = this.findInterfaceDeclaration(sourceFile, typeName);
        if (interfaceDecl) {
            const fix = this.findPropertyFixInMembers(sourceFile, interfaceDecl.members, propName, paramIndex, numberCategory);
            if (fix) {
                return fix;
            }
        }
        const classDecl = this.findClassDeclaration(sourceFile, typeName);
        if (classDecl) {
            return this.findPropertyFixInMembers(sourceFile, classDecl.members, propName, paramIndex, numberCategory);
        }
        return null;
    }

    private findPropertyFixInMembers(
        sourceFile: ts.SourceFile,
        members: ts.NodeArray<ts.ObjectLiteralElementLike | ts.TypeElement | ts.ClassElement>,
        propName: string,
        paramIndex: number,
        numberCategory: NumberCategory
    ): RuleFix | null {
        for (const member of members) {
            const typeNode = this.getMemberTypeNode(member, sourceFile, propName);
            if (typeNode) {
                const fix = this.createCompanionFixFromTypeNode(sourceFile, typeNode, paramIndex, numberCategory);
                if (fix) {
                    return fix;
                }
            }
        }
        return null;
    }

    private getMemberTypeNode(
        member: ts.Node,
        sourceFile: ts.SourceFile,
        propName: string
    ): ts.TypeNode | null {
        let name: ts.Node | undefined;
        let type: ts.TypeNode | undefined;
        if (ts.isPropertySignature(member)) {
            name = member.name;
            type = member.type;
        } else if (ts.isPropertyDeclaration(member)) {
            name = member.name;
            type = member.type;
        }
        if (!name || !type || name.getText(sourceFile) !== propName) {
            return null;
        }
        return type;
    }

    private findEnclosingFunctionDeclaration(
        sourceFile: ts.SourceFile,
        arrowFunc: ts.ArrowFunction
    ): ts.FunctionDeclaration | ts.MethodDeclaration | null {
        let result: ts.FunctionDeclaration | ts.MethodDeclaration | null = null;
        const arrowStart = arrowFunc.getStart(sourceFile);
        const arrowEnd = arrowFunc.getEnd();
        const visit = (node: ts.Node): void => {
            const start = node.getStart(sourceFile);
            const end = node.getEnd();
            if (start > arrowStart || end < arrowEnd) {
                return;
            }
            if (ts.isFunctionDeclaration(node) || ts.isMethodDeclaration(node)) {
                result = node;
            }
            ts.forEachChild(node, visit);
        };
        visit(sourceFile);
        return result;
    }

    private findTypeAliasDeclaration(
        sourceFile: ts.SourceFile,
        name: string
    ): ts.TypeAliasDeclaration | null {
        let result: ts.TypeAliasDeclaration | null = null;
        const visit = (node: ts.Node): void => {
            if (result) {
                return;
            }
            if (ts.isTypeAliasDeclaration(node) && node.name.text === name) {
                result = node;
                return;
            }
            ts.forEachChild(node, visit);
        };
        visit(sourceFile);
        return result;
    }

    private findEnclosingPropertyAssignment(
        sourceFile: ts.SourceFile,
        arrowFunc: ts.ArrowFunction
    ): ts.PropertyAssignment | null {
        let result: ts.PropertyAssignment | null = null;
        const arrowStart = arrowFunc.getStart(sourceFile);
        const arrowEnd = arrowFunc.getEnd();
        const visit = (node: ts.Node): void => {
            const start = node.getStart(sourceFile);
            const end = node.getEnd();
            if (start > arrowStart || end < arrowEnd) {
                return;
            }
            if (ts.isPropertyAssignment(node)) {
                result = node;
            }
            ts.forEachChild(node, visit);
        };
        visit(sourceFile);
        return result;
    }

    private findEnclosingObjectLiteral(
        sourceFile: ts.SourceFile,
        propertyAssignment: ts.PropertyAssignment
    ): ts.ObjectLiteralExpression | null {
        let result: ts.ObjectLiteralExpression | null = null;
        const paStart = propertyAssignment.getStart(sourceFile);
        const paEnd = propertyAssignment.getEnd();
        const visit = (node: ts.Node): void => {
            const start = node.getStart(sourceFile);
            const end = node.getEnd();
            if (start > paStart || end < paEnd) {
                return;
            }
            if (ts.isObjectLiteralExpression(node)) {
                result = node;
            }
            ts.forEachChild(node, visit);
        };
        visit(sourceFile);
        return result;
    }

    private findVariableDeclarationForObjectLiteral(
        sourceFile: ts.SourceFile,
        objectLiteral: ts.ObjectLiteralExpression
    ): ts.VariableDeclaration | null {
        let result: ts.VariableDeclaration | null = null;
        const olStart = objectLiteral.getStart(sourceFile);
        const olEnd = objectLiteral.getEnd();
        const visit = (node: ts.Node): void => {
            if (result) {
                return;
            }
            if (ts.isVariableDeclaration(node) && node.initializer) {
                const initStart = node.initializer.getStart(sourceFile);
                const initEnd = node.initializer.getEnd();
                if (initStart === olStart && initEnd === olEnd) {
                    result = node;
                    return;
                }
            }
            ts.forEachChild(node, visit);
        };
        visit(sourceFile);
        return result;
    }

    private findInterfaceDeclaration(
        sourceFile: ts.SourceFile,
        name: string
    ): ts.InterfaceDeclaration | null {
        let result: ts.InterfaceDeclaration | null = null;
        const visit = (node: ts.Node): void => {
            if (result) {
                return;
            }
            if (ts.isInterfaceDeclaration(node) && node.name.text === name) {
                result = node;
                return;
            }
            ts.forEachChild(node, visit);
        };
        visit(sourceFile);
        return result;
    }

    private findEnclosingArrowFunction(
        sourceFile: ts.SourceFile,
        range: [number, number]
    ): ts.ArrowFunction | null {
        let result: ts.ArrowFunction | null = null;
        const visit = (node: ts.Node): void => {
            const start = node.getStart(sourceFile);
            const end = node.getEnd();
            if (start > range[0] || end < range[1]) {
                return;
            }
            if (ts.isArrowFunction(node) && start <= range[0] && end >= range[1]) {
                result = node;
            }
            ts.forEachChild(node, visit);
        };
        visit(sourceFile);
        return result;
    }

    private findAssignedVariableName(sourceFile: ts.SourceFile, funcNode: ts.ArrowFunction): string | null {
        let varName: string | null = null;
        const funcStart = funcNode.getStart(sourceFile);
        const funcEnd = funcNode.getEnd();
        const visit = (node: ts.Node): void => {
            if (varName) {
                return;
            }
            const name = this.getAssignedVarName(node, sourceFile, funcStart, funcEnd);
            if (name) {
                varName = name;
                return;
            }
            ts.forEachChild(node, visit);
        };
        visit(sourceFile);
        return varName;
    }

    private findAssignedVariableDeclaration(
        sourceFile: ts.SourceFile,
        funcNode: ts.ArrowFunction
    ): ts.VariableDeclaration | null {
        const funcStart = funcNode.getStart(sourceFile);
        const funcEnd = funcNode.getEnd();
        let result: ts.VariableDeclaration | null = null;
        const visit = (node: ts.Node): void => {
            if (result) {
                return;
            }
            if (ts.isVariableDeclaration(node) && node.initializer) {
                const initStart = node.initializer.getStart(sourceFile);
                const initEnd = node.initializer.getEnd();
                if (initStart === funcStart && initEnd === funcEnd) {
                    result = node;
                    return;
                }
            }
            ts.forEachChild(node, visit);
        };
        visit(sourceFile);
        if (result) {
            return result;
        }
        const varName = this.findAssignedVariableName(sourceFile, funcNode);
        if (varName) {
            return this.findVariableDeclarationByName(sourceFile, varName);
        }
        return null;
    }

    private getAssignedVarName(
        node: ts.Node,
        sourceFile: ts.SourceFile,
        funcStart: number,
        funcEnd: number
    ): string | null {
        if (ts.isVariableDeclaration(node) && node.initializer) {
            const initStart = node.initializer.getStart(sourceFile);
            const initEnd = node.initializer.getEnd();
            if (initStart <= funcStart && initEnd >= funcEnd && ts.isIdentifier(node.name)) {
                return node.name.getText(sourceFile);
            }
        }
        if (!ts.isBinaryExpression(node) || node.operatorToken.kind !== ts.SyntaxKind.EqualsToken) {
            return null;
        }
        if (!ts.isIdentifier(node.left)) {
            return null;
        }
        const rightStart = node.right.getStart(sourceFile);
        const rightEnd = node.right.getEnd();
        if (rightStart <= funcStart && rightEnd >= funcEnd) {
            return node.left.getText(sourceFile);
        }
        return null;
    }

    private findVariableDeclarationByName(
        sourceFile: ts.SourceFile,
        name: string
    ): ts.VariableDeclaration | null {
        let result: ts.VariableDeclaration | null = null;
        const visit = (node: ts.Node): void => {
            if (result) {
                return;
            }
            if (ts.isVariableDeclaration(node) &&
                node.name && ts.isIdentifier(node.name) &&
                node.name.getText(sourceFile) === name) {
                result = node;
                return;
            }
            ts.forEachChild(node, visit);
        };
        visit(sourceFile);
        return result;
    }

    private hasSharedDeclarationConflict(
        sourceFile: ts.SourceFile,
        currentArrowFunc: ts.ArrowFunction,
        paramIndex: number,
        varDecl: ts.VariableDeclaration | null
    ): boolean {
        const ctx: ConflictCheckContext = {
            currentStart: currentArrowFunc.getStart(sourceFile),
            currentEnd: currentArrowFunc.getEnd(),
            paramIndex,
        };

        if (this.hasExportConflict(sourceFile, currentArrowFunc)) {
            return true;
        }
        if (this.hasVariableAssignmentConflict(sourceFile, currentArrowFunc, ctx, varDecl)) {
            return true;
        }
        if (this.hasVariableTypeAliasConflict(sourceFile, currentArrowFunc, ctx, varDecl)) {
            return true;
        }
        if (this.hasTypeAliasConflict(sourceFile, currentArrowFunc, ctx)) {
            return true;
        }
        if (this.hasOuterArrowTypeAliasConflict(sourceFile, currentArrowFunc, ctx)) {
            return true;
        }
        if (this.hasInterfaceOrClassConflict(sourceFile, currentArrowFunc, ctx)) {
            return true;
        }
        return false;
    }

    private hasExportConflict(
        sourceFile: ts.SourceFile,
        currentArrowFunc: ts.ArrowFunction
    ): boolean {
        const enclosingClass = this.findEnclosingClassDeclaration(sourceFile, currentArrowFunc);
        if (enclosingClass && this.isExportedDeclaration(enclosingClass)) {
            return true;
        }
        const enclosingFunc = this.findEnclosingFunctionDeclaration(sourceFile, currentArrowFunc);
        return !!enclosingFunc && this.isExportedDeclaration(enclosingFunc);
    }

    private hasVariableAssignmentConflict(
        sourceFile: ts.SourceFile,
        currentArrowFunc: ts.ArrowFunction,
        ctx: ConflictCheckContext,
        varDecl: ts.VariableDeclaration | null
    ): boolean {
        if (!varDecl || !ts.isIdentifier(varDecl.name)) {
            return false;
        }
        if (this.isVariableExported(varDecl)) {
            return true;
        }
        return this.hasOtherAssignmentWithDivision(sourceFile, varDecl.name.getText(sourceFile), ctx);
    }

    private hasVariableTypeAliasConflict(
        sourceFile: ts.SourceFile,
        currentArrowFunc: ts.ArrowFunction,
        ctx: ConflictCheckContext,
        varDecl: ts.VariableDeclaration | null
    ): boolean {
        if (!varDecl || !varDecl.type || !ts.isTypeReferenceNode(varDecl.type)) {
            return false;
        }
        const typeName = varDecl.type.typeName.getText(sourceFile);
        return this.checkTypeAliasConflict(sourceFile, typeName, ctx);
    }

    private isVariableExported(varDecl: ts.VariableDeclaration): boolean {
        const declList = varDecl.parent;
        if (!ts.isVariableDeclarationList(declList)) {
            return false;
        }
        const varStmt = declList.parent;
        if (!ts.isVariableStatement(varStmt)) {
            return false;
        }
        return this.isExportedDeclaration(varStmt);
    }

    private hasTypeAliasConflict(
        sourceFile: ts.SourceFile,
        currentArrowFunc: ts.ArrowFunction,
        ctx: ConflictCheckContext
    ): boolean {
        const funcDecl = this.findEnclosingFunctionDeclaration(sourceFile, currentArrowFunc);
        if (!funcDecl || !funcDecl.type || !ts.isTypeReferenceNode(funcDecl.type)) {
            return false;
        }
        const typeName = funcDecl.type.typeName.getText(sourceFile);
        return this.checkTypeAliasConflict(sourceFile, typeName, ctx);
    }

    private hasOuterArrowTypeAliasConflict(
        sourceFile: ts.SourceFile,
        innerArrow: ts.ArrowFunction,
        ctx: ConflictCheckContext
    ): boolean {
        let parent: ts.Node | undefined = innerArrow.parent;
        while (parent) {
            if (ts.isArrowFunction(parent) && parent.type && ts.isTypeReferenceNode(parent.type)) {
                const typeName = parent.type.typeName.getText(sourceFile);
                if (this.checkTypeAliasConflict(sourceFile, typeName, ctx)) {
                    return true;
                }
            }
            parent = parent.parent;
        }
        return false;
    }

    private checkTypeAliasConflict(
        sourceFile: ts.SourceFile,
        typeName: string,
        ctx: ConflictCheckContext
    ): boolean {
        const typeAlias = this.findTypeAliasDeclaration(sourceFile, typeName);
        if (!typeAlias || !typeAlias.type || !ts.isFunctionTypeNode(typeAlias.type)) {
            return false;
        }
        if (this.isExportedDeclaration(typeAlias)) {
            return true;
        }
        return this.hasOtherReturnWithDivision(sourceFile, typeName, ctx);
    }

    private hasInterfaceOrClassConflict(
        sourceFile: ts.SourceFile,
        currentArrowFunc: ts.ArrowFunction,
        ctx: ConflictCheckContext
    ): boolean {
        const propAssignment = this.findEnclosingPropertyAssignment(sourceFile, currentArrowFunc);
        if (!propAssignment) {
            return false;
        }
        const propName = propAssignment.name.getText(sourceFile);
        const objLiteral = this.findEnclosingObjectLiteral(sourceFile, propAssignment);
        if (!objLiteral) {
            return false;
        }
        const varDecl = this.findVariableDeclarationForObjectLiteral(sourceFile, objLiteral);
        if (!varDecl || !varDecl.type || !ts.isTypeReferenceNode(varDecl.type)) {
            return false;
        }
        const typeName = varDecl.type.typeName.getText(sourceFile);
        return this.checkInterfaceConflict(sourceFile, typeName, propName, ctx) ||
            this.checkClassConflict(sourceFile, typeName, propName, ctx);
    }

    private checkInterfaceConflict(
        sourceFile: ts.SourceFile,
        typeName: string,
        propName: string,
        ctx: ConflictCheckContext
    ): boolean {
        const interfaceDecl = this.findInterfaceDeclaration(sourceFile, typeName);
        if (!interfaceDecl) {
            return false;
        }
        if (this.isExportedDeclaration(interfaceDecl)) {
            return true;
        }
        return this.hasOtherPropertyWithDivision(sourceFile, typeName, propName, ctx);
    }

    private checkClassConflict(
        sourceFile: ts.SourceFile,
        typeName: string,
        propName: string,
        ctx: ConflictCheckContext
    ): boolean {
        const classDecl = this.findClassDeclaration(sourceFile, typeName);
        if (!classDecl) {
            return false;
        }
        if (this.isExportedDeclaration(classDecl)) {
            return true;
        }
        return this.hasOtherPropertyWithDivision(sourceFile, typeName, propName, ctx);
    }

    private isExportedDeclaration(node: ts.Node): boolean {
        if (!node.modifiers) {
            return false;
        }
        return node.modifiers.some(m => m.kind === ts.SyntaxKind.ExportKeyword);
    }

    private checkArrowForDivision(
        sourceFile: ts.SourceFile,
        arrow: ts.ArrowFunction,
        ctx: ConflictCheckContext
    ): boolean {
        const arrowStart = arrow.getStart(sourceFile);
        const arrowEnd = arrow.getEnd();
        if (arrowStart === ctx.currentStart && arrowEnd === ctx.currentEnd) {
            return false;
        }
        return this.isParamUsedInDivision(sourceFile, arrow, ctx.paramIndex);
    }

    private hasOtherAssignmentWithDivision(
        sourceFile: ts.SourceFile,
        varName: string,
        ctx: ConflictCheckContext
    ): boolean {
        let found = false;
        const visit = (node: ts.Node): void => {
            if (found) {
                return;
            }
            if (this.isAssignmentToVar(node, sourceFile, varName)) {
                found = this.checkArrowForDivision(sourceFile, (node as ts.BinaryExpression).right as ts.ArrowFunction, ctx);
            }
            ts.forEachChild(node, visit);
        };
        visit(sourceFile);
        return found;
    }

    private isAssignmentToVar(node: ts.Node, sourceFile: ts.SourceFile, varName: string): node is ts.BinaryExpression {
        return ts.isBinaryExpression(node) &&
            node.operatorToken.kind === ts.SyntaxKind.EqualsToken &&
            ts.isIdentifier(node.left) &&
            node.left.getText(sourceFile) === varName &&
            ts.isArrowFunction(node.right);
    }

    private hasOtherReturnWithDivision(
        sourceFile: ts.SourceFile,
        typeName: string,
        ctx: ConflictCheckContext
    ): boolean {
        let found = false;
        const visit = (node: ts.Node): void => {
            if (found) {
                return;
            }
            if (this.isFunctionReturningType(node, sourceFile, typeName)) {
                found = this.hasReturnWithDivision(sourceFile, node as ts.FunctionDeclaration | ts.MethodDeclaration, ctx);
            }
            ts.forEachChild(node, visit);
        };
        visit(sourceFile);
        return found;
    }

    private hasReturnWithDivision(
        sourceFile: ts.SourceFile,
        funcDecl: ts.FunctionDeclaration | ts.MethodDeclaration,
        ctx: ConflictCheckContext
    ): boolean {
        const returnStmts = this.findAllReturnStatements(sourceFile, funcDecl);
        for (const returnStmt of returnStmts) {
            if (returnStmt.expression && ts.isArrowFunction(returnStmt.expression) &&
                this.checkArrowForDivision(sourceFile, returnStmt.expression, ctx)) {
                return true;
            }
        }
        return false;
    }

    private isFunctionReturningType(node: ts.Node, sourceFile: ts.SourceFile, typeName: string): boolean {
        return (ts.isFunctionDeclaration(node) || ts.isMethodDeclaration(node)) &&
            !!node.type && ts.isTypeReferenceNode(node.type) &&
            node.type.typeName.getText(sourceFile) === typeName;
    }

    private hasOtherPropertyWithDivision(
        sourceFile: ts.SourceFile,
        typeName: string,
        propName: string,
        ctx: ConflictCheckContext
    ): boolean {
        return this.scanObjectLiteralsForDivision(sourceFile, typeName, propName, ctx);
    }

    private scanObjectLiteralsForDivision(
        sourceFile: ts.SourceFile,
        typeName: string,
        propName: string,
        ctx: ConflictCheckContext
    ): boolean {
        let found = false;
        const visit = (node: ts.Node): void => {
            if (found) {
                return;
            }
            if (this.isTypedObjectLiteral(node, sourceFile, typeName)) {
                found = this.checkPropertiesForDivision(sourceFile, node as ts.VariableDeclaration, propName, ctx);
            }
            ts.forEachChild(node, visit);
        };
        visit(sourceFile);
        return found;
    }

    private isTypedObjectLiteral(node: ts.Node, sourceFile: ts.SourceFile, typeName: string): node is ts.VariableDeclaration {
        return ts.isVariableDeclaration(node) &&
            !!node.initializer && ts.isObjectLiteralExpression(node.initializer) &&
            !!node.type && ts.isTypeReferenceNode(node.type) &&
            node.type.typeName.getText(sourceFile) === typeName;
    }

    private checkPropertiesForDivision(
        sourceFile: ts.SourceFile,
        varDecl: ts.VariableDeclaration,
        propName: string,
        ctx: ConflictCheckContext
    ): boolean {
        const objLiteral = varDecl.initializer as ts.ObjectLiteralExpression;
        for (const prop of objLiteral.properties) {
            if (!this.isMatchingPropertyAssignment(prop, sourceFile, propName)) {
                continue;
            }
            const arrow = (prop as ts.PropertyAssignment).initializer as ts.ArrowFunction;
            if (this.checkArrowForDivision(sourceFile, arrow, ctx)) {
                return true;
            }
        }
        return false;
    }

    private isMatchingPropertyAssignment(prop: ts.ObjectLiteralElementLike, sourceFile: ts.SourceFile, propName: string): prop is ts.PropertyAssignment {
        return ts.isPropertyAssignment(prop) &&
            prop.name.getText(sourceFile) === propName &&
            ts.isArrowFunction(prop.initializer);
    }

    private findAllReturnStatements(
        sourceFile: ts.SourceFile,
        funcDecl: ts.FunctionDeclaration | ts.MethodDeclaration
    ): ts.ReturnStatement[] {
        const results: ts.ReturnStatement[] = [];
        const visit = (node: ts.Node): void => {
            if (ts.isReturnStatement(node)) {
                results.push(node);
            }
            ts.forEachChild(node, visit);
        };
        funcDecl.body?.forEachChild(visit);
        return results;
    }

    private isParamUsedInDivision(
        sourceFile: ts.SourceFile,
        arrowFunc: ts.ArrowFunction,
        paramIndex: number
    ): boolean {
        const param = arrowFunc.parameters[paramIndex];
        if (!param || !ts.isIdentifier(param.name)) {
            return false;
        }
        const paramName = param.name.getText(sourceFile);
        let found = false;
        const visit = (node: ts.Node): void => {
            if (found) {
                return;
            }
            if (ts.isBinaryExpression(node) &&
                (node.operatorToken.kind === ts.SyntaxKind.SlashToken ||
                 node.operatorToken.kind === ts.SyntaxKind.SlashEqualsToken)) {
                if (this.isIdentifierNamed(node.left, paramName) ||
                    this.isIdentifierNamed(node.right, paramName)) {
                    found = true;
                    return;
                }
            }
            ts.forEachChild(node, visit);
        };
        visit(arrowFunc.body);
        return found;
    }

    private isIdentifierNamed(expr: ts.Node, name: string): boolean {
        return ts.isIdentifier(expr) && expr.text === name;
    }

    private findClassDeclaration(
        sourceFile: ts.SourceFile,
        name: string
    ): ts.ClassDeclaration | null {
        let result: ts.ClassDeclaration | null = null;
        const visit = (node: ts.Node): void => {
            if (result) {
                return;
            }
            if (ts.isClassDeclaration(node) && node.name && node.name.text === name) {
                result = node;
                return;
            }
            ts.forEachChild(node, visit);
        };
        visit(sourceFile);
        return result;
    }

    private findEnclosingClassDeclaration(
        sourceFile: ts.SourceFile,
        arrowFunc: ts.ArrowFunction
    ): ts.ClassDeclaration | null {
        let result: ts.ClassDeclaration | null = null;
        const arrowStart = arrowFunc.getStart(sourceFile);
        const arrowEnd = arrowFunc.getEnd();
        const visit = (node: ts.Node): void => {
            const start = node.getStart(sourceFile);
            const end = node.getEnd();
            if (start > arrowStart || end < arrowEnd) {
                return;
            }
            if (ts.isClassDeclaration(node)) {
                result = node;
            }
            ts.forEachChild(node, visit);
        };
        visit(sourceFile);
        return result;
    }

    private findClassFieldCompanionFix(
        sourceFile: ts.SourceFile,
        arrowFunc: ts.ArrowFunction,
        paramIndex: number,
        numberCategory: NumberCategory
    ): RuleFix | null {
        const propDecl = this.findEnclosingPropertyDeclaration(sourceFile, arrowFunc);
        if (!propDecl || !propDecl.type) {
            return null;
        }
        return this.createCompanionFixFromTypeReference(sourceFile, propDecl.type, paramIndex, numberCategory);
    }

    private findEnclosingPropertyDeclaration(
        sourceFile: ts.SourceFile,
        arrowFunc: ts.ArrowFunction
    ): ts.PropertyDeclaration | null {
        let result: ts.PropertyDeclaration | null = null;
        const arrowStart = arrowFunc.getStart(sourceFile);
        const arrowEnd = arrowFunc.getEnd();
        const visit = (node: ts.Node): void => {
            const start = node.getStart(sourceFile);
            const end = node.getEnd();
            if (start > arrowStart || end < arrowEnd) {
                return;
            }
            if (ts.isPropertyDeclaration(node)) {
                result = node;
            }
            ts.forEachChild(node, visit);
        };
        visit(sourceFile);
        return result;
    }

    private findOuterArrowReturnCompanionFix(
        sourceFile: ts.SourceFile,
        innerArrow: ts.ArrowFunction,
        paramIndex: number,
        numberCategory: NumberCategory
    ): RuleFix | null {
        let parent: ts.Node | undefined = innerArrow.parent;
        while (parent) {
            if (ts.isArrowFunction(parent) && parent.type) {
                const fix = this.tryCreateCompanionFixFromArrowType(sourceFile, parent.type, paramIndex, numberCategory);
                if (fix) {
                    return fix;
                }
            }
            parent = parent.parent;
        }
        return null;
    }

    private tryCreateCompanionFixFromArrowType(
        sourceFile: ts.SourceFile,
        typeNode: ts.TypeNode,
        paramIndex: number,
        numberCategory: NumberCategory
    ): RuleFix | null {
        let unwrapped: ts.TypeNode = typeNode;
        while (ts.isParenthesizedTypeNode(unwrapped)) {
            unwrapped = unwrapped.type;
        }
        return this.createCompanionFixFromTypeReference(sourceFile, unwrapped, paramIndex, numberCategory);
    }
}
