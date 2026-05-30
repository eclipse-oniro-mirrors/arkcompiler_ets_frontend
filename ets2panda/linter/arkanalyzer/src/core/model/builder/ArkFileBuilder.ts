/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

import fs from 'fs';
import path from 'path';
import ts from 'ohos-typescript';
import { ArkFile, Language } from '../ArkFile';
import { ArkNamespace } from '../ArkNamespace';
import Logger, { LOG_MODULE_TYPE } from '../../../utils/logger';
import { buildDefaultArkClassFromArkFile, buildNormalArkClassFromArkFile } from './ArkClassBuilder';
import { buildArkMethodFromArkClass } from './ArkMethodBuilder';
import { buildImportInfo } from './ArkImportBuilder';
import {
    buildExportAssignment,
    buildExportDeclaration,
    buildExportInfo,
    buildExportTypeAliasDeclaration,
    buildExportVariableStatement,
    isExported,
} from './ArkExportBuilder';
import { buildArkNamespace, mergeNameSpaces } from './ArkNamespaceBuilder';
import { ArkClass, ClassCategory } from '../ArkClass';
import { ArkMethod } from '../ArkMethod';
import { LineColPosition } from '../../base/Position';
import { ETS_COMPILER_OPTIONS } from '../../common/EtsConst';
import { FileSignature } from '../ArkSignature';
import { ARKTS_STATIC_MARK } from '../../common/Const';

const logger = Logger.getLogger(LOG_MODULE_TYPE.ARKANALYZER, 'ArkFileBuilder');

export const notStmtOrExprKind = [
    'ModuleDeclaration',
    'ClassDeclaration',
    'InterfaceDeclaration',
    'EnumDeclaration',
    'ExportDeclaration',
    'ExportAssignment',
    'MethodDeclaration',
    'Constructor',
    'FunctionDeclaration',
    'GetAccessor',
    'SetAccessor',
    'ArrowFunction',
    'FunctionExpression',
    'MethodSignature',
    'ConstructSignature',
    'CallSignature',
];

/**
 * Entry of building ArkFile instance
 *
 * @param arkFile
 * @returns
 */
export function buildArkFileFromFile(absoluteFilePath: string, projectDir: string, arkFile: ArkFile, projectName: string): void {
    arkFile.setFilePath(absoluteFilePath);
    arkFile.setProjectDir(projectDir);

    const fileSignature = new FileSignature(projectName, path.relative(projectDir, absoluteFilePath));
    arkFile.setFileSignature(fileSignature);

    arkFile.setCode(fs.readFileSync(arkFile.getFilePath(), 'utf8'));
    const sourceFile = ts.createSourceFile(arkFile.getName(), arkFile.getCode(), ts.ScriptTarget.Latest, true, undefined, ETS_COMPILER_OPTIONS);
    genDefaultArkClass(arkFile, sourceFile);
    buildArkFile(arkFile, sourceFile);
}

/**
 * Building ArkFile instance
 *
 * @param arkFile
 * @param astRoot
 * @returns
 */
function buildArkFile(arkFile: ArkFile, astRoot: ts.SourceFile): void {
    const statements = astRoot.statements;
    const namespaces: ArkNamespace[] = [];
    statements.forEach(child => {
        if (!buildArkFileStatement(arkFile, astRoot, child, namespaces)) {
            logger.trace('Child joined default method of arkFile: ', ts.SyntaxKind[child.kind]);
        }
    });

    const mergedNameSpaces = mergeNameSpaces(namespaces);
    mergedNameSpaces.forEach(mergedNameSpace => {
        arkFile.addNamespace(mergedNameSpace);
        if (mergedNameSpace.isExport()) {
            const linCol = new LineColPosition(mergedNameSpace.getLine(), mergedNameSpace.getColumn());
            arkFile.addExportInfo(buildExportInfo(mergedNameSpace, arkFile, linCol));
        }
    });
}

function buildArkFileStatement(arkFile: ArkFile, astRoot: ts.SourceFile, child: ts.Statement, namespaces: ArkNamespace[]): boolean {
    return handleNamespaceDeclaration(arkFile, astRoot, child, namespaces) ||
        handleClassDeclaration(arkFile, astRoot, child) ||
        handleMethodDeclaration(arkFile, astRoot, child) ||
        handleFunctionDeclaration(arkFile, astRoot, child) ||
        handleImportDeclaration(arkFile, astRoot, child) ||
        handleExportDeclaration(arkFile, astRoot, child) ||
        handleLanguageDirective(arkFile, child);
}

function handleNamespaceDeclaration(arkFile: ArkFile, astRoot: ts.SourceFile, child: ts.Statement, namespaces: ArkNamespace[]): boolean {
    if (!ts.isModuleDeclaration(child)) {
        return false;
    }

    const ns: ArkNamespace = new ArkNamespace();
    ns.setDeclaringArkFile(arkFile);
    buildArkNamespace(child, arkFile, ns, astRoot);
    namespaces.push(ns);
    if (ns.isExported()) {
        arkFile.addExportInfo(buildExportInfo(ns, arkFile, LineColPosition.buildFromNode(child, astRoot)));
    }
    return true;
}

function handleClassDeclaration(arkFile: ArkFile, astRoot: ts.SourceFile, child: ts.Statement): boolean {
    if (!ts.isClassDeclaration(child) && !ts.isInterfaceDeclaration(child) && !ts.isEnumDeclaration(child) && !ts.isStructDeclaration(child)) {
        return false;
    }

    const cls: ArkClass = getMergeableInterfaceClass(child, arkFile) ?? new ArkClass();
    buildNormalArkClassFromArkFile(child, arkFile, cls, astRoot);
    arkFile.addArkClass(cls);
    if (cls.isExported()) {
        arkFile.addExportInfo(buildExportInfo(cls, arkFile, LineColPosition.buildFromNode(child, astRoot)));
    }
    return true;
}

function handleMethodDeclaration(arkFile: ArkFile, astRoot: ts.SourceFile, child: ts.Statement): boolean {
    if (!ts.isMethodDeclaration(child)) {
        return false;
    }

    logger.trace('This is a MethodDeclaration in ArkFile.');
    buildDefaultClassMethod(arkFile, astRoot, child);
    return true;
}

function handleFunctionDeclaration(arkFile: ArkFile, astRoot: ts.SourceFile, child: ts.Statement): boolean {
    if (!ts.isFunctionDeclaration(child)) {
        return false;
    }

    buildDefaultClassMethod(arkFile, astRoot, child);
    return true;
}

function buildDefaultClassMethod(arkFile: ArkFile, astRoot: ts.SourceFile, child: ts.FunctionDeclaration | ts.MethodDeclaration): void {
    const mthd: ArkMethod = new ArkMethod();
    buildArkMethodFromArkClass(child, arkFile.getDefaultClass(), mthd, astRoot);
    if (mthd.isExported()) {
        arkFile.addExportInfo(buildExportInfo(mthd, arkFile, LineColPosition.buildFromNode(child, astRoot)));
    }
}

function handleImportDeclaration(arkFile: ArkFile, astRoot: ts.SourceFile, child: ts.Statement): boolean {
    if (!ts.isImportEqualsDeclaration(child) && !ts.isImportDeclaration(child)) {
        return false;
    }

    const importInfos = buildImportInfo(child, astRoot, arkFile);
    importInfos?.forEach(element => {
        element.setDeclaringArkFile(arkFile);
        arkFile.addImportInfo(element);
    });
    return true;
}

function handleExportDeclaration(arkFile: ArkFile, astRoot: ts.SourceFile, child: ts.Statement): boolean {
    if (ts.isExportDeclaration(child)) {
        buildExportDeclaration(child, astRoot, arkFile).forEach(item => arkFile.addExportInfo(item));
        return true;
    }
    if (ts.isExportAssignment(child)) {
        buildExportAssignment(child, astRoot, arkFile).forEach(item => arkFile.addExportInfo(item));
        return true;
    }
    if (ts.isVariableStatement(child) && isExported(child.modifiers)) {
        buildExportVariableStatement(child, astRoot, arkFile).forEach(item => arkFile.addExportInfo(item));
        return true;
    }
    if (ts.isTypeAliasDeclaration(child) && isExported(child.modifiers)) {
        buildExportTypeAliasDeclaration(child, astRoot, arkFile).forEach(item => arkFile.addExportInfo(item));
        return true;
    }
    return false;
}

function handleLanguageDirective(arkFile: ArkFile, child: ts.Statement): boolean {
    if (!ts.isExpressionStatement(child) || !ts.isStringLiteral(child.expression)) {
        return false;
    }

    if (child.expression.text.trim() === ARKTS_STATIC_MARK) {
        arkFile.setLanguage(Language.ARKTS1_2);
    }
    return true;
}

function getMergeableInterfaceClass(node: ts.Node, arkFile: ArkFile): ArkClass | null {
    if (!ts.isInterfaceDeclaration(node) || !node.name) {
        return null;
    }
    const existingClass = arkFile.getClassWithName(node.name.text);
    if (!existingClass || existingClass.getCategory() !== ClassCategory.INTERFACE) {
        return null;
    }
    return existingClass;
}

function genDefaultArkClass(arkFile: ArkFile, astRoot: ts.SourceFile): void {
    let defaultClass = new ArkClass();

    buildDefaultArkClassFromArkFile(arkFile, defaultClass, astRoot);
    arkFile.setDefaultClass(defaultClass);
    arkFile.addArkClass(defaultClass);
}
