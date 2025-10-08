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

import * as fs from 'fs';
import * as path from 'path';

import { AliasConfig, ArkTS } from '../types';
import {
    Logger,
    LogDataFactory
} from '../logger';
import { ErrorCode } from '../error_code';
import {
  DYNAMIC_PREFIX,
  KIT_CONFIGS_PATH_FROM_SDK,
} from '../pre_define';

export class KitImportTransformer {

    private arkts: ArkTS;
    private extraImports: ArkTS['ETSImportDeclaration'][] = [];
    private sdkAliasConfig: Record<string, Record<string, AliasConfig>>;
    private buildSdkPath: string;
    private program: object;
    private logger: Logger;

    constructor(arkts: ArkTS, program: object, buildSdkPath: string, aliasMap: Record<string, Record<string, AliasConfig>>) {
        this.arkts = arkts;
        this.buildSdkPath = buildSdkPath;
        this.sdkAliasConfig = aliasMap;
        this.program = program;
        this.logger = Logger.getInstance();
    }

    public transform(astNode: ArkTS['AstNode']): ArkTS['AstNode'] {
        if (!this.arkts.isEtsScript(astNode)) {
            return astNode;
        }

        const newStatements: ArkTS['AstNode'][] = [];
        const dynamicAliasNames = new Set(this.getDynamicAliasNames());
        if (astNode.statements.length === 0) {
            return astNode;
        }
        for (const stmt of astNode.statements) {
            if (this.arkts.isETSImportDeclaration(stmt) && dynamicAliasNames.has(stmt.source?.str)) {
                this.splitKitImport(stmt);
                continue;
            }
            newStatements.push(stmt);
        }

        const finalStatements = [...this.extraImports, ...newStatements];

        return this.arkts.factory.updateEtsScript(astNode, finalStatements);
    }

    private splitKitImport(importNode: ArkTS['ETSImportDeclaration']): void {
        const kitName = importNode.source.str;
        const symbolsJson = this.loadKitSymbolsJson(kitName);
        if (!symbolsJson) {
            return;
        }
    
        const groupedSymbols = this.groupImportSpecifiersBySource(importNode, symbolsJson, kitName);
        this.generateSplitImportDeclarations(groupedSymbols);
    }
    
    private loadKitSymbolsJson(kitName: string): unknown | null {
        let jsonFileName: string = this.getOriginalNameByAlias(kitName);
        if (jsonFileName === '') {
            this.logger.printError(LogDataFactory.newInstance(
                ErrorCode.BUILDSYSTEM_PLUGIN_ALIAS_CONFIG_PARSING_FAIL,
                `json file: '${jsonFileName}' not found in kit config contents`
            ));
            return null;
        }
        const configPath = path.resolve(this.buildSdkPath, KIT_CONFIGS_PATH_FROM_SDK, `${jsonFileName}.json`);
    
        if (!fs.existsSync(configPath)) {
            this.logger.printError(LogDataFactory.newInstance(
                ErrorCode.BUILDSYSTEM_PLUGIN_ALIAS_CONFIG_PARSING_FAIL,
                `Kit config file not found for ${kitName}`,
                configPath
            ));
            return null;
        }
    
        try {
            return JSON.parse(fs.readFileSync(configPath, 'utf-8'));
        } catch (error) {
            this.logger.printError(LogDataFactory.newInstance(
                ErrorCode.BUILDSYSTEM_PLUGIN_ALIAS_CONFIG_PARSING_FAIL,
                `Failed to parse kit config JSON for ${kitName}`,
                error instanceof Error ? error.message : String(error)
            ));
            return null;
        }
    }

    private groupImportSpecifiersBySource(importNode: ArkTS['ETSImportDeclaration'], symbolsJson: unknown, kitName: string): Map<string, string[]> {
        const grouped = new Map<string, string[]>();
    
        for (const specifier of importNode.specifiers) {
            if (!this.arkts.isImportSpecifier(specifier)) {
                continue;
            }
    
            const symbolName = specifier.imported?.name;
            if (!symbolName) {
                continue;
            }
    
            const typedSymbols = (symbolsJson as { symbols: Record<string, { source: string }> });
            const symbolEntry = typedSymbols.symbols?.[symbolName];
            if (!symbolEntry?.source) {
                this.logger.printWarn(`Symbol '${symbolName}' not found in ${kitName}.json`);
                continue;
            }
    
            const sourcePath = DYNAMIC_PREFIX + symbolEntry.source.replace(/\.d\.ts$/, '');
            if (!grouped.has(sourcePath)) {
                grouped.set(sourcePath, []);
            }
            grouped.get(sourcePath)!.push(symbolName);
        }
    
        return grouped;
    }

    private generateSplitImportDeclarations(groupedSymbols: Map<string, string[]>): void {
        for (const [source, names] of groupedSymbols.entries()) {
            const specifiers = names.map(name =>
                this.arkts.factory.createImportSpecifier(
                    this.arkts.factory.createIdentifier(name),
                    this.arkts.factory.createIdentifier(name)
                )
            );
    
            const importDecl = this.arkts.factory.createImportDeclaration(
                this.arkts.factory.createStringLiteral(source),
                specifiers,
                this.arkts.Es2pandaImportKinds.IMPORT_KINDS_VALUE,
                this.program,
                this.arkts.Es2pandaImportFlags.IMPORT_FLAGS_NONE
            );
            this.extraImports.push(importDecl);
        }
    }

    private getDynamicAliasNames(): Set<string> {
        const dynamicAliasNames = new Set<string>();
    
        if (Object.keys(this.sdkAliasConfig).length === 0) {
            return dynamicAliasNames;
        }
    
        for (const innerMap of Object.values(this.sdkAliasConfig)) {
            for (const [aliasName, aliasConfig] of Object.entries(innerMap)) {
                if (!aliasConfig.originalAPIName.startsWith('@kit')) {
                    continue;
                }
                if (!aliasConfig.isStatic) {
                    dynamicAliasNames.add(aliasName);
                }
            }
        }
        return dynamicAliasNames;
    }

    private getOriginalNameByAlias(aliasName: string): string {
        for (const innerMap of Object.values(this.sdkAliasConfig)) {
            if (aliasName in innerMap) {
                return innerMap[aliasName].originalAPIName;
            }
        }
        return '';
    }
}
