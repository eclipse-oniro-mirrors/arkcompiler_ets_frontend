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

import { ArkClass, ArkFile, ArkMethod, AstTreeUtils, ExportInfo, ImportInfo, Stmt, ts } from 'arkanalyzer';
import { AIFix, FunctionFix, Range, RuleFix } from '../../model/Fix';

export type FixPosition = {
    startLine: number,
    startCol: number,
    endLine: number,
    endCol: number
};

export class FixUtils {

    public static getRangeStart(arkFile: ArkFile, codeNode: Stmt | ArkMethod | ArkClass | ExportInfo | ImportInfo): number {
        let lineNum = 0;
        let startColumn = 0;
        if (codeNode instanceof Stmt) {
            let originalPosition = codeNode.getOriginPositionInfo();
            lineNum = originalPosition.getLineNo();
            startColumn = originalPosition.getColNo();
        } else if (codeNode instanceof ArkMethod) {
            lineNum = codeNode.getLine() ?? 0;
            startColumn = codeNode.getColumn() ?? 0;
        } else if (codeNode instanceof ArkClass) {
            lineNum = codeNode.getLine() ?? 0;
            startColumn = codeNode.getColumn() ?? 0;
        } else if (codeNode instanceof ExportInfo) {
            let originalPosition = codeNode.getOriginTsPosition();
            lineNum = originalPosition.getLineNo();
            startColumn = originalPosition.getColNo();
        } else if (codeNode instanceof ImportInfo) {
            let originalPosition = codeNode.getOriginTsPosition();
            lineNum = originalPosition.getLineNo();
            startColumn = originalPosition.getColNo();
        }
        // 原文代码
        let code = arkFile.getCode();
        // 找到当前分割符所在行
        let lineBreak = this.getTextEof(code);
        let cnt = 0;
        if (lineBreak.length > 0) {
            for (let index = 1; index !== lineNum; index++) {
                cnt = code.indexOf(lineBreak, cnt + 1);
            }
        }
        let start = (cnt === 0 && startColumn === 1) ? 0 : (cnt + startColumn + 1);//对第一行第一列特殊处理，后续代码都是以0，所以需要+1
        return start;
    }

    // 根据输入的代码片段的起始、结束行列号信息，计算此代码片段在该文件中的起始偏移量、结束偏移量数据
    public static getRangeWithAst(sourceFile: ts.SourceFile, fixPosition: FixPosition): Range | null {
        if (fixPosition.startLine < 1 || fixPosition.startCol < 1 || fixPosition.endLine < 1 || fixPosition.endCol < 1) {
            return null;
        }
        const startNumber = ts.getPositionOfLineAndCharacter(sourceFile, fixPosition.startLine - 1, fixPosition.startCol - 1);
        const endNumber = ts.getPositionOfLineAndCharacter(sourceFile, fixPosition.endLine - 1, fixPosition.endCol - 1);
        return [startNumber, endNumber];
    }

    // 根据输入的起始行号信息，计算该行的起始偏移量、结束偏移量数据
    public static getLineRange(sourceFile: ts.SourceFile, lineNumber: number): Range | null {
        const lineStarts = sourceFile.getLineStarts();

        // 验证行号范围
        if (lineNumber < 1 || lineNumber > lineStarts.length) {
            return null;
        }

        const startPos = lineStarts[lineNumber - 1];
        let endPos: number;

        // 处理文件最后一行
        if (lineNumber === lineStarts.length) {
            endPos = sourceFile.text.length;
        } else {
            endPos = lineStarts[lineNumber] - 1;
        }
        return [startPos, endPos];
    }

    public static getLineRangeWithStartCol(sourceFile: ts.SourceFile, lineNumber: number, startCol: number): Range | null {
        const lineRange = this.getLineRange(sourceFile, lineNumber);
        if (lineRange === null) {
            return null;
        }
        const newStartPos = lineRange[0] + startCol - 1;
        const endPos = lineRange[1];
        if (newStartPos <= endPos) {
            return [newStartPos, endPos];
        }
        return null;
    }

    // 根据给定的起始、结束偏移量数据，获取此段代码片段的源码字符串，位置信息不合法则返回null
    public static getSourceWithRange(sourceFile: ts.SourceFile, range: Range): string | null {
        const start = range[0];
        const end = range[1];
        if (start < 0 || end > sourceFile.text.length || start > end) {
            return null;
        }
        return sourceFile.text.substring(start, end);
    }

    // 根据给定的行号，获取该行的源码字符串，行号不合法则返回null
    public static getLineText(sourceFile: ts.SourceFile, lineNumber: number): string | null {
        const range = this.getLineRange(sourceFile, lineNumber);
        if (range === null) {
            return null;
        }

        return sourceFile.text.substring(range[0], range[1]);
    }

    // 根据给定的行号，获取该行的换行符，获取失败则使用默认的'\n'换行符
    public static getEolSymbol(sourceFile: ts.SourceFile, lineNumber: number): string {
        let res = '\n';
        const lineStr = this.getLineText(sourceFile, lineNumber);
        if (lineStr === null) {
            return res;
        }

        const lfIndex = lineStr.indexOf('\n');
        if (lfIndex > 0 && lineStr[lfIndex - 1] === '\r') {
            res = '\r\n';
        }
        return res;
    }

    // 根据给定的行号，获取该行的缩进数量，采用空格缩进时为空格数量，采用tab缩进时为tab数量，行号不合法则返回null
    public static getIndentOfLine(sourceFile: ts.SourceFile, lineNumber: number): number | null {
        const lineStr = this.getLineText(sourceFile, lineNumber);
        if (lineStr === null) {
            return null;
        }

        const space = ' ';
        let res = 0;
        for (const char of lineStr) {
            if (char === space) {
                res++;
            } else {
                break;
            }
        }
        return res;
    }

    // 根据给定的行号，获取该行附近的缩进宽度，采用空格缩进时为空格数量，采用tab缩进时为tab数量，无法找到则返回2
    public static getIndentWidth(sourceFile: ts.SourceFile, lineNumber: number): number {
        const lineIndent = FixUtils.getIndentOfLine(sourceFile, lineNumber);
        let indentWidth = 0;

        // 从当前行向上寻找最近的缩进量
        let currLineIndent = lineIndent;
        let previousLine = lineNumber - 1;
        while (indentWidth <= 0 && previousLine > 0 && currLineIndent !== null) {
            const previousLineIndent = FixUtils.getIndentOfLine(sourceFile, previousLine);
            if (previousLineIndent !== null) {
                indentWidth = Math.abs(currLineIndent - previousLineIndent);
            }
            currLineIndent = previousLineIndent;
            previousLine--;
        }
        if (indentWidth > 0) {
            return indentWidth;
        }

        // 从当前行向下寻找最近的缩进量
        currLineIndent = lineIndent;
        let nextLine = lineNumber + 1;
        while (indentWidth <= 0 && nextLine < sourceFile.getLineStarts().length && currLineIndent !== null) {
            const nextLineIndent = FixUtils.getIndentOfLine(sourceFile, nextLine);
            if (nextLineIndent !== null) {
                indentWidth = Math.abs(nextLineIndent - currLineIndent);
            }
            currLineIndent = nextLineIndent;
            nextLine++;
        }
        if (indentWidth > 0) {
            return indentWidth;
        }
        return 2;
    }

    public static getTextEof(text: string): string {
        if (text.includes('\r\n')) {
            return '\r\n';
        } else if (text.includes('\n')) {
            return '\n';
        } else if (text.includes('\r')) {
            return '\r';
        } else {
            return '';
        }
    }

    public static isRuleFix(object: any): object is RuleFix {
        return typeof object === 'object' && 'range' in object && 'text' in object;
    }

    public static isFunctionFix(object: any): object is FunctionFix {
        return typeof object === 'object' && 'fix' in object;
    }

    public static isAIFix(object: any): object is AIFix {
        return typeof object === 'object' && 'text' in object;
    }

    public static hasOwnPropertyOwn(object: any, key: string): boolean {
        return typeof object === 'object' && key in object;
    }
}
