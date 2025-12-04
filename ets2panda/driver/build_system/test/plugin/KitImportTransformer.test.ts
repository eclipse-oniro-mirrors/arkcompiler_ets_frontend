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

import * as path from 'path';
import { AliasConfig, ArkTS } from '../../src/types';
jest.mock('../../src/logger', () => ({
    Logger: {
        getInstance: jest.fn(() => ({
            log: jest.fn(),
            error: jest.fn(),
            warn: jest.fn(),
        })),
    },
}));
jest.mock('../../src/pre_define', () => ({
    DYNAMIC_PREFIX: 'dynamic/',
    KIT_CONFIGS_PATH_FROM_SDK: './plugin'
}));
import { KitImportTransformer } from '../../src/plugins/KitImportTransformer';
import { DYNAMIC_PREFIX } from '../../src/pre_define';

export const arktsMock: Partial<ArkTS> = {
    isEtsScript: jest.fn((node) => true),
    isETSImportDeclaration: jest.fn((node) => true),
    isImportSpecifier: jest.fn(() => true),
    factory: {
        createImportSpecifier: jest.fn((id) => ({ imported: id })),
        createIdentifier: jest.fn((name) => ({ name })),
        createLiteral: jest.fn((str) => ({ str })),
        createImportDeclaration: jest.fn((source, specifiers) => ({
            kind: 'ImportDeclaration',
            source,
            specifiers,
        })),
        updateEtsScript: jest.fn((oldNode, newStatements) => ({
            ...oldNode,
            statements: newStatements,
        })),
        createEtsScript: jest.fn(() => ({ kind: 'EtsScript', statements: [] })),
        createStringLiteral: jest.fn((s) => ({ str: s })),
    },
    Es2pandaImportKinds: {
      IMPORT_KINDS_ALL: 0,
    },
    Es2pandaImportFlags: {
        IMPORT_FLAGS_NONE: 0,
    }
};

const mockAliasConfig: Record<string, Record<string, AliasConfig>> = {
    har: {
        'dynamic@kit.abilityKit': { isStatic: false, originalAPIName: '@kit.abilityKit' }
    }
};

const mockProgram = {};

describe('KitImportTransformer', () => {

    it('should transform kit import to dynamic/@ohos.* imports', () => {
        const transformer = new KitImportTransformer(arktsMock as ArkTS, mockProgram, './test', mockAliasConfig);

        const astInput = {
            statements: [
                {
                    source: { str: 'dynamic@kit.abilityKit' },
                    specifiers: [
                        { imported: { name: 'foo' } },
                        { imported: { name: 'bar' } }
                    ],
                },
            ],
        };

        const result = transformer.transform(astInput as any);
        expect(result.statements[0]).toEqual({
            kind: 'ImportDeclaration',
            source: { str: 'dynamic/@ohos.app.ability.common' },
            specifiers: [{ imported: { name: 'foo' } }]
        });

        expect(result.statements[1]).toEqual({
            kind: 'ImportDeclaration',
            source: { str: 'dynamic/@ohos.app.ability.ConfigurationConstant' },
            specifiers: [{ imported: { name: 'bar' } }]
        });
    });

    it('should not transform non-kit imports', () => {
        const transformer = new KitImportTransformer(arktsMock as ArkTS, mockProgram, './test', mockAliasConfig);

        const astInput = {
            statements: [
                {
                    source: { str: '@ohos.hilog' },
                    specifiers: [{ imported: { name: 'hilog' } }],
                },
            ],
        };

        const result = transformer.transform(astInput as any);
        expect(result.statements).toEqual(astInput.statements);
    });

    it('should skip unknown kit imports', () => {
        const transformer = new KitImportTransformer(arktsMock as ArkTS, mockProgram, './test', mockAliasConfig);

        const astInput = {
            statements: [
                {
                    source: { str: '@kit.unknownKit' },
                    specifiers: [{ imported: { name: 'unknown' } }],
                },
            ],
        };

        const result = transformer.transform(astInput as any);
        expect(result.statements).toEqual(astInput.statements);
    });
});
