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

import { describe, it } from 'mocha';
import { assert, expect } from 'chai';
import { IOptions } from '../../../src/configs/IOptions';
import { FileUtils } from '../../../src/utils/FileUtils';
import { ArkObfuscatorForTest } from '../../../src/ArkObfuscatorForTest'
import secharmony, {
  transformerPlugin,
  historyNameCache,
  clearCaches,
 } from '../../../src/transformers/rename/RenameIdentifierTransformer';
import path from 'path';
import * as ts from 'typescript';
import {
  PropCollections,
  UnobfuscationCollections,
  LocalVariableCollections
} from '../../../src/utils/CommonCollections';

describe('Teste Cases for <RenameFileNameTransformer>.', function () {
  describe('Teste Cases for <createRenameIdentifierFactory>.', function () {
    it('should return null if mEnable is false',function () {
      let options: IOptions = {
        "mNameObfuscation": {
          "mEnable": false,
          "mRenameProperties": false,
          "mReservedProperties": []
        }
      };
      let renameIdentifierFactory = transformerPlugin.createTransformerFactory(options);
      expect(renameIdentifierFactory).to.be.null;
    })
    describe('Teste Cases for <renameTransformer>.', function () {
      let option: IOptions = {
        "mCompact": false,
        "mRemoveComments": false,
        "mOutputDir": "",
        "mDisableConsole": false,
        "mSimplify": false,
        "mNameObfuscation": {
            "mEnable": true,
            "mNameGeneratorType": 1,
            "mDictionaryList": [],
            "mRenameProperties": true,
            "mKeepStringProperty": false,
            "mTopLevel": false,
            "mReservedProperties": []
        },
        "mEnableSourceMap": true,
        "mEnableNameCache": true
      };
      const fileContent = `
      class A1{
        prop_5 = 5;
        constructor(public para1: number, private para2: string, protected para3: boolean, readonly para4: number, para5: string) {
          para5 = para5 + 1;
          let temp1 = para1;
          let temp2 = para2;
          let temp3 = para3;
          let temp4 = para4;
          this.prop_5 = para4;
        }
      }
      `;
      const fileContent1 = `
      class a{
        prop_5 = 5;
        constructor(public para1: number, private para2: string, protected para3: boolean, readonly para4: number, para5: string) {
          para5 = para5 + 1;
          let temp1 = para1;
          let temp2 = para2;
          let temp3 = para3;
          let temp4 = para4;
          this.prop_5 = para4;
        }
      }
      `;
      const fileContent2 = `
      import {A as B} from './file1.ts';
      export {C as D} from './file1.ts';
      `;

      const fileContentAvailable = `
      @Available({minApiVersion: '23'})
      export let numA = 111;
      @Available({minApiVersion: '23'})
      export let strA = "111";
      `;
      let transformer: ts.TransformerFactory<ts.Node>;

      it('should not transform parameter property when mRenameProperties is false',function () {
        let option1: IOptions = {
          "mCompact": false,
          "mRemoveComments": false,
          "mOutputDir": "",
          "mDisableConsole": false,
          "mSimplify": false,
          "mNameObfuscation": {
              "mEnable": true,
              "mNameGeneratorType": 1,
              "mDictionaryList": [],
              "mRenameProperties": false,
              "mKeepStringProperty": false,
              "mTopLevel": false,
              "mReservedProperties": []
          },
          "mEnableSourceMap": true,
          "mEnableNameCache": true
        };
        transformer = transformerPlugin.createTransformerFactory(option1);
        const sourceFile: ts.SourceFile = ts.createSourceFile('demo.ts', fileContent, ts.ScriptTarget.ES2015, true);
        let transformed = ts.transform(sourceFile, [transformer]);
        expect(
          ((((transformed
            .transformed[0] as ts.SourceFile)
            .statements[0] as ts.ClassDeclaration)
            .members[1] as ts.ConstructorDeclaration)
            .parameters[0]
            .name as ts.Identifier)
            .escapedText == 'para1')
            .to.be.true;
        expect(
          ((((transformed
            .transformed[0] as ts.SourceFile)
            .statements[0] as ts.ClassDeclaration)
            .members[1] as ts.ConstructorDeclaration)
            .parameters[1]
            .name as ts.Identifier)
            .escapedText == 'para2')
            .to.be.true;
        expect(
          ((((transformed
            .transformed[0] as ts.SourceFile)
            .statements[0] as ts.ClassDeclaration)
            .members[1] as ts.ConstructorDeclaration)
            .parameters[2]
            .name as ts.Identifier)
            .escapedText == 'para3')
            .to.be.true;
        expect(
          ((((transformed
            .transformed[0] as ts.SourceFile)
            .statements[0] as ts.ClassDeclaration)
            .members[1] as ts.ConstructorDeclaration)
            .parameters[3]
            .name as ts.Identifier)
            .escapedText == 'para4')
            .to.be.true;
        expect(
          ((((transformed
            .transformed[0] as ts.SourceFile)
            .statements[0] as ts.ClassDeclaration)
            .members[1] as ts.ConstructorDeclaration)
            .parameters[4]
            .name as ts.Identifier)
            .escapedText == 'a')
            .to.be.true;
      })

      it('should not transform parameter property when mRenameProperties is true',function () {
        transformer = transformerPlugin.createTransformerFactory(option);
        const sourceFile: ts.SourceFile = ts.createSourceFile('demo.ts', fileContent, ts.ScriptTarget.ES2015, true);
        let transformed = ts.transform(sourceFile, [transformer]);
        expect(
          ((((transformed
            .transformed[0] as ts.SourceFile)
            .statements[0] as ts.ClassDeclaration)
            .members[1] as ts.ConstructorDeclaration)
            .parameters[0].name as ts.Identifier)
            .escapedText == 'a')
            .to.be.true;
        expect(
          ((((transformed
            .transformed[0] as ts.SourceFile)
            .statements[0] as ts.ClassDeclaration)
            .members[1] as ts.ConstructorDeclaration)
            .parameters[1]
            .name as ts.Identifier)
            .escapedText == 'b')
            .to.be.true;
        expect(
          ((((transformed
            .transformed[0] as ts.SourceFile)
            .statements[0] as ts.ClassDeclaration)
            .members[1] as ts.ConstructorDeclaration)
            .parameters[2]
            .name as ts.Identifier)
            .escapedText == 'c')
            .to.be.true;
        expect(
          ((((transformed
            .transformed[0] as ts.SourceFile)
            .statements[0] as ts.ClassDeclaration)
            .members[1] as ts.ConstructorDeclaration)
            .parameters[3]
            .name as ts.Identifier)
            .escapedText == 'd')
            .to.be.true;
        expect(
          ((((transformed
            .transformed[0] as ts.SourceFile)
            .statements[0] as ts.ClassDeclaration)
            .members[1] as ts.ConstructorDeclaration)
            .parameters[4]
            .name as ts.Identifier)
            .escapedText == 'e')
            .to.be.true;
      })

      it('should use historyMangledName when originName is in historyMangledTable', function () {
        PropCollections.historyMangledTable.set('para1', 'test1');
        PropCollections.globalMangledTable.set('para1', 'test2');
        transformer = transformerPlugin.createTransformerFactory(option);
        const sourceFile: ts.SourceFile = ts.createSourceFile('demo.ts', fileContent, ts.ScriptTarget.ES2015, true);
        let transformed = ts.transform(sourceFile, [transformer]);
        expect(
          ((((transformed
            .transformed[0] as ts.SourceFile)
            .statements[0] as ts.ClassDeclaration)
            .members[1] as ts.ConstructorDeclaration)
            .parameters[0]
            .name as ts.Identifier)
            .escapedText == 'test1')
            .to.be.true;
        PropCollections.historyMangledTable.clear();
        PropCollections.globalMangledTable.clear();
      })

      it('should use historyMangledName when originName is in globalMangleTable', function () {
        PropCollections.globalMangledTable.set('para1', 'test2');
        transformer = transformerPlugin.createTransformerFactory(option);
        const sourceFile: ts.SourceFile = ts.createSourceFile('demo.ts', fileContent, ts.ScriptTarget.ES2015, true);
        let transformed = ts.transform(sourceFile, [transformer]);
        expect(
          ((((transformed
            .transformed[0] as ts.SourceFile)
            .statements[0] as ts.ClassDeclaration)
            .members[1] as ts.ConstructorDeclaration)
            .parameters[0]
            .name as ts.Identifier)
            .escapedText == 'test2')
            .to.be.true;
        PropCollections.globalMangledTable.clear();
      })

      it('should not obfuscate when originName is in property whitelist', function () {
        PropCollections.reservedProperties.add('para1');
        transformer = transformerPlugin.createTransformerFactory(option);
        const sourceFile: ts.SourceFile = ts.createSourceFile('demo.ts', fileContent, ts.ScriptTarget.ES2015, true);
        let transformed = ts.transform(sourceFile, [transformer]);
        expect(
          ((((transformed
            .transformed[0] as ts.SourceFile)
            .statements[0] as ts.ClassDeclaration)
            .members[1] as ts.ConstructorDeclaration)
            .parameters[0].name as ts.Identifier)
            .escapedText == 'para1')
            .to.be.true;
        PropCollections.reservedProperties.clear();
        PropCollections.globalMangledTable.clear();
      })

      it('should not obfuscated as names in ReservedProperty or ReservedLocalVariable or mangledPropsInNameCache', function () {
        PropCollections.reservedProperties.add('b');
        UnobfuscationCollections.reservedExportName.add('c');
        PropCollections.historyMangledTable.set('testorigin', 'd');
        transformer = transformerPlugin.createTransformerFactory(option);
        const sourceFile: ts.SourceFile = ts.createSourceFile('demo.ts', fileContent, ts.ScriptTarget.ES2015, true);
        let transformed = ts.transform(sourceFile, [transformer]);
        expect(
          ((((transformed
            .transformed[0] as ts.SourceFile)
            .statements[0] as ts.ClassDeclaration)
            .members[1] as ts.ConstructorDeclaration)
            .parameters[0]
            .name as ts.Identifier)
            .escapedText == 'a')
            .to.be.true;
        expect(
          ((((transformed
            .transformed[0] as ts.SourceFile)
            .statements[0] as ts.ClassDeclaration)
            .members[1] as ts.ConstructorDeclaration)
            .parameters[1]
            .name as ts.Identifier)
            .escapedText == 'e')
            .to.be.true;
        expect(
          ((((transformed
            .transformed[0] as ts.SourceFile)
            .statements[0] as ts.ClassDeclaration)
            .members[1] as ts.ConstructorDeclaration)
            .parameters[2]
            .name as ts.Identifier)
            .escapedText == 'f')
            .to.be.true;
        expect(
          ((((transformed
            .transformed[0] as ts.SourceFile)
            .statements[0] as ts.ClassDeclaration)
            .members[1] as ts.ConstructorDeclaration)
            .parameters[3]
            .name as ts.Identifier)
            .escapedText == 'g')
            .to.be.true;
        expect(
          ((((transformed
            .transformed[0] as ts.SourceFile)
            .statements[0] as ts.ClassDeclaration)
            .members[1] as ts.ConstructorDeclaration)
            .parameters[4]
            .name as ts.Identifier)
            .escapedText == 'h')
            .to.be.true;
        PropCollections.reservedProperties.clear();
        UnobfuscationCollections.reservedExportName.clear();
        PropCollections.globalMangledTable.clear();
        UnobfuscationCollections.reservedExportNameAndProp.clear();
        PropCollections.historyMangledTable.clear();
      })

      it('should not obfuscated as names in outer scope', function () {
        transformer = transformerPlugin.createTransformerFactory(option);
        const sourceFile: ts.SourceFile = ts.createSourceFile('demo.ts', fileContent1, ts.ScriptTarget.ES2015, true);
        let transformed = ts.transform(sourceFile, [transformer]);
        expect(
          ((((transformed
            .transformed[0] as ts.SourceFile)
            .statements[0] as ts.ClassDeclaration)
            .members[1] as ts.ConstructorDeclaration)
            .parameters[0]
            .name as ts.Identifier)
            .escapedText == 'b')
            .to.be.true;
        expect(
          ((((transformed
            .transformed[0] as ts.SourceFile)
            .statements[0] as ts.ClassDeclaration)
            .members[1] as ts.ConstructorDeclaration)
            .parameters[1]
            .name as ts.Identifier)
            .escapedText == 'c')
            .to.be.true;
        expect(
          ((((transformed
            .transformed[0] as ts.SourceFile)
            .statements[0] as ts.ClassDeclaration)
            .members[1] as ts.ConstructorDeclaration)
            .parameters[2]
            .name as ts.Identifier)
            .escapedText == 'd')
            .to.be.true;
        expect(
          ((((transformed
            .transformed[0] as ts.SourceFile)
            .statements[0] as ts.ClassDeclaration)
            .members[1] as ts.ConstructorDeclaration)
            .parameters[3]
            .name as ts.Identifier)
            .escapedText == 'e')
            .to.be.true;
        expect(
          ((((transformed
            .transformed[0] as ts.SourceFile)
            .statements[0] as ts.ClassDeclaration)
            .members[1] as ts.ConstructorDeclaration)
            .parameters[4]
            .name as ts.Identifier)
            .escapedText == 'f')
            .to.be.true;
        PropCollections.globalMangledTable.clear();
      })

      it('Only Enable Toplevel Obfuscation Test', () => {
        let options: IOptions = {
          "mNameObfuscation": {
            "mEnable": true,
            "mRenameProperties": false,
            "mReservedProperties": [],
            "mTopLevel": true
          }
        };
        assert.strictEqual(options !== undefined, true);
        const renameIdentifierFactory = secharmony.transformerPlugin.createTransformerFactory(options);
        const fileContent = `
          let a = 1;
          export let b = 1;
          import {c} from 'filePath';
        `;
        const textWriter = ts.createTextWriter('\n');
        let arkobfuscator = new ArkObfuscatorForTest();
        arkobfuscator.init(options);
        const sourceFile: ts.SourceFile = ts.createSourceFile('demo.ts', fileContent, ts.ScriptTarget.ES2015, true);
        let transformedResult: ts.TransformationResult<ts.Node> = ts.transform(sourceFile, [renameIdentifierFactory], {});
        let ast: ts.SourceFile = transformedResult.transformed[0] as ts.SourceFile;
        arkobfuscator.createObfsPrinter(ast.isDeclarationFile).writeFile(ast, textWriter, undefined);
        const actualContent = textWriter.getText();
        const expectContent = `
          let d = 1;
          export let b = 1;
          import {c} from 'filePath';
        `;
        assert.strictEqual(compareStringsIgnoreNewlines(actualContent, expectContent), true);
      })

      it('should return origin node if isSourceFile is false', () => {
        let options: IOptions | undefined = FileUtils.readFileAsJson(path.join(__dirname, "obfuscate_identifier_config.json"));
        assert.strictEqual(options !== undefined, true);
        const renameIdentifierFactory = secharmony.transformerPlugin.createTransformerFactory(options as IOptions);
        const blockFile: ts.Block = ts.factory.createBlock([]);
        let transformedResult: ts.TransformationResult<ts.Node> = ts.transform(blockFile, [renameIdentifierFactory], {});
        assert.strictEqual(transformedResult.transformed[0], blockFile);
      })

      it('noSymbolIdentifierTest: enable export obfuscation', function () {
        let option1: IOptions = {
          mCompact: false,
          mRemoveComments: false,
          mOutputDir: '',
          mDisableConsole: false,
          mSimplify: false,
          mNameObfuscation: {
            mEnable: true,
            mNameGeneratorType: 1,
            mDictionaryList: [],
            mRenameProperties: false,
            mKeepStringProperty: false,
            mTopLevel: false,
            mReservedProperties: [],
          },
          mExportObfuscation: true,
          mEnableSourceMap: false,
          mEnableNameCache: false,
        };
        transformer = transformerPlugin.createTransformerFactory(option1);
        const sourceFile: ts.SourceFile = ts.createSourceFile('demo.ts', fileContent2, ts.ScriptTarget.ES2015, true);
        let transformed = ts.transform(sourceFile, [transformer]);
        let ast: ts.SourceFile = transformed.transformed[0] as ts.SourceFile;
        const printer = ts.createPrinter();
        const transformedAst: string = printer.printFile(ast);
        expect(transformedAst === "import { A as B } from './file1.ts';\nexport { C as D } from './file1.ts';\n").to.be
          .true;
      });

      it('noSymbolIdentifierTest: enable export and toplevel obfuscation', function () {
        let option1: IOptions = {
          mCompact: false,
          mRemoveComments: false,
          mOutputDir: '',
          mDisableConsole: false,
          mSimplify: false,
          mNameObfuscation: {
            mEnable: true,
            mNameGeneratorType: 1,
            mDictionaryList: [],
            mRenameProperties: false,
            mKeepStringProperty: false,
            mTopLevel: true,
            mReservedProperties: [],
          },
          mExportObfuscation: true,
          mEnableSourceMap: false,
          mEnableNameCache: false,
        };
        transformer = transformerPlugin.createTransformerFactory(option1);
        const sourceFile: ts.SourceFile = ts.createSourceFile('demo.ts', fileContent2, ts.ScriptTarget.ES2015, true);
        let transformed = ts.transform(sourceFile, [transformer]);
        let ast: ts.SourceFile = transformed.transformed[0] as ts.SourceFile;
        const printer = ts.createPrinter();
        const transformedAst: string = printer.printFile(ast);
        expect(transformedAst === "import { c as a } from './file1.ts';\nexport { d as b } from './file1.ts';\n").to.be
          .true;
      });

      it('originalSymbolTest: import test', function () {
        const fileContent3 = `
          declare module 'testModule2' {
            import { noSymbolIdentifier2 as ni2 } from 'module2';
            export { ni2 };
          }
        `;
        let option: IOptions = {
          mCompact: false,
          mRemoveComments: false,
          mOutputDir: '',
          mDisableConsole: false,
          mSimplify: false,
          mNameObfuscation: {
            mEnable: true,
            mNameGeneratorType: 1,
            mDictionaryList: [],
            mRenameProperties: false,
            mKeepStringProperty: false,
            mTopLevel: true,
            mReservedProperties: [],
          },
          mExportObfuscation: true,
          mEnableSourceMap: false,
          mEnableNameCache: false
        };
        transformer = transformerPlugin.createTransformerFactory(option);
        const sourceFile: ts.SourceFile = ts.createSourceFile('demo.ts', fileContent3, ts.ScriptTarget.ES2015, true);
        let transformed = ts.transform(sourceFile, [transformer]);
        let ast: ts.SourceFile = transformed.transformed[0] as ts.SourceFile;
        const printer = ts.createPrinter();
        const transformedAst: string = printer.printFile(ast);
        expect(
          transformedAst ===
            "declare module 'testModule2' {\n    import { c as b } from 'module2';\n    export { b };\n}\n",
        ).to.be.true;
      });

      it('originalSymbolTest: export test', function () {
        const fileContent4 = `
          type ni2 = string;
          declare namespace ns {
            export { ni2 };
          }
        `;
        let option: IOptions = {
          mCompact: false,
          mRemoveComments: false,
          mOutputDir: '',
          mDisableConsole: false,
          mSimplify: false,
          mNameObfuscation: {
            mEnable: true,
            mNameGeneratorType: 1,
            mDictionaryList: [],
            mRenameProperties: false,
            mKeepStringProperty: false,
            mTopLevel: true,
            mReservedProperties: [],
          },
          mExportObfuscation: true,
          mEnableSourceMap: false,
          mEnableNameCache: false
        };
        transformer = transformerPlugin.createTransformerFactory(option);
        const sourceFile: ts.SourceFile = ts.createSourceFile('demo.ts', fileContent4, ts.ScriptTarget.ES2015, true);
        let transformed = ts.transform(sourceFile, [transformer]);
        let ast: ts.SourceFile = transformed.transformed[0] as ts.SourceFile;
        const printer = ts.createPrinter();
        const transformedAst: string = printer.printFile(ast);
        expect(
          transformedAst ===
            "type b = string;\ndeclare namespace a {\n    export { b };\n}\n",
        ).to.be.true;
      });

      it('originalSymbolTest: propertyName test', function () {
        const fileContent5 = `
          import { Symbol as sy } from 'typescript';
          let localVariable: number = 1;
          export { SourceFile sf } from 'typescript';
          export { localVariable as lv };
        `;
        let option: IOptions = {
          mCompact: false,
          mRemoveComments: false,
          mOutputDir: '',
          mDisableConsole: false,
          mSimplify: false,
          mNameObfuscation: {
            mEnable: true,
            mNameGeneratorType: 1,
            mDictionaryList: [],
            mRenameProperties: false,
            mKeepStringProperty: false,
            mTopLevel: true,
            mReservedProperties: [],
          },
          mExportObfuscation: true,
          mEnableSourceMap: false,
          mEnableNameCache: false,
        };
        transformer = transformerPlugin.createTransformerFactory(option);
        const sourceFile: ts.SourceFile = ts.createSourceFile('demo.ts', fileContent5, ts.ScriptTarget.ES2015, true);
        let transformed = ts.transform(sourceFile, [transformer]);
        let ast: ts.SourceFile = transformed.transformed[0] as ts.SourceFile;
        const printer = ts.createPrinter();
        const transformedAst: string = printer.printFile(ast);
        expect(
          transformedAst ===
            "import { f as a } from 'typescript';\nlet b: number = 1;\nexport { c, d } from 'typescript';\nexport { b as e };\n",
        ).to.be.true;
      });

      it('Test the option of mKeepParameterNames for declaration file', () => {
        let options: IOptions = {
          'mNameObfuscation': {
            'mEnable': true,
            'mRenameProperties': false,
            'mReservedProperties': [],
            'mTopLevel': false,
            'mKeepParameterNames': true
          }
        };
        assert.strictEqual(options !== undefined, true);
        const renameIdentifierFactory = secharmony.transformerPlugin.createTransformerFactory(options);
        const fileContent = `export declare function foo(para: number): void;`;
        const textWriter = ts.createTextWriter('\n');
        let arkobfuscator = new ArkObfuscatorForTest();
        arkobfuscator.init(options);
        const sourceFile: ts.SourceFile = ts.createSourceFile('demo.d.ts', fileContent, ts.ScriptTarget.ES2015, true);
        let transformedResult: ts.TransformationResult<ts.Node> = ts.transform(sourceFile, [renameIdentifierFactory], {});
        let ast: ts.SourceFile = transformedResult.transformed[0] as ts.SourceFile;
        arkobfuscator.createObfsPrinter(ast.isDeclarationFile).writeFile(ast, textWriter, undefined);
        const actualContent = textWriter.getText();
        const expectContent = `export declare function foo(para: number): void;`;
        assert.strictEqual(compareStringsIgnoreNewlines(actualContent, expectContent), true);
      })

      it('should obfuscate annotationDeclaration when top-level obfuscation is enable', () => {
        let options: IOptions = {
          'mNameObfuscation': {
            'mEnable': true,
            'mRenameProperties': false,
            'mReservedProperties': [],
            'mTopLevel': true,
            'mKeepParameterNames': false
          },
          'mAllowEtsAnnotations': true
        };
        assert.strictEqual(options !== undefined, true);
        const renameIdentifierFactory = secharmony.transformerPlugin.createTransformerFactory(options);
        const fileContent = `@interface __$$ETS_ANNOTATION$$__Anno1 {prop1: number = 1}`;
        const textWriter = ts.createTextWriter('\n');
        let arkobfuscator = new ArkObfuscatorForTest();
        arkobfuscator.init(options);
        const sourceFile: ts.SourceFile = ts.createSourceFile('demo.ts', fileContent, ts.ScriptTarget.ES2015, true, undefined, {
          'etsAnnotationsEnable': true
        }, true);
        let transformedResult: ts.TransformationResult<ts.Node> = ts.transform(sourceFile, [renameIdentifierFactory], {});
        let ast: ts.SourceFile = transformedResult.transformed[0] as ts.SourceFile;
        arkobfuscator.createObfsPrinter(ast.isDeclarationFile).writeFile(ast, textWriter, undefined);
        const actualContent = textWriter.getText();
        const expectContent = `@interface __$$ETS_ANNOTATION$$__a {
            prop1: number = 1;
        }`;
        assert.strictEqual(compareStringsIgnoreNewlines(actualContent, expectContent), true);
        PropCollections.clearPropsCollections();
      })

      it('should not obfuscate when annotationName is in top-level whitelist', function () {
        let options: IOptions = {
          'mNameObfuscation': {
            'mEnable': true,
            'mRenameProperties': false,
            'mReservedProperties': [],
            'mTopLevel': true,
            'mKeepParameterNames': false
          },
          'mAllowEtsAnnotations': true
        };
        assert.strictEqual(options !== undefined, true);
        PropCollections.reservedProperties.add('Anno1');
        const renameIdentifierFactory = secharmony.transformerPlugin.createTransformerFactory(options);
        const fileContent = `@interface __$$ETS_ANNOTATION$$__Anno1 {prop1: number = 1}`;
        const textWriter = ts.createTextWriter('\n');
        let arkobfuscator = new ArkObfuscatorForTest();
        arkobfuscator.init(options);
        const sourceFile: ts.SourceFile = ts.createSourceFile('demo.ts', fileContent, ts.ScriptTarget.ES2015, true, undefined, {
          'etsAnnotationsEnable': true
        }, true);
        let transformedResult: ts.TransformationResult<ts.Node> = ts.transform(sourceFile, [renameIdentifierFactory], {});
        let ast: ts.SourceFile = transformedResult.transformed[0] as ts.SourceFile;
        arkobfuscator.createObfsPrinter(ast.isDeclarationFile).writeFile(ast, textWriter, undefined);
        const actualContent = textWriter.getText();
        const expectContent = `@interface __$$ETS_ANNOTATION$$__Anno1 {
            prop1: number = 1;
        }`;
        assert.strictEqual(compareStringsIgnoreNewlines(actualContent, expectContent), true);
        PropCollections.clearPropsCollections();
      })

      it('should not obfuscate when annotationName is exported and not enable export-obfuscation', function () {
        let options: IOptions = {
          'mNameObfuscation': {
            'mEnable': true,
            'mRenameProperties': false,
            'mReservedProperties': [],
            'mTopLevel': true,
            'mKeepParameterNames': false
          },
          'mAllowEtsAnnotations': true,
          'mExportObfuscation': false,
        };
        assert.strictEqual(options !== undefined, true);
        const renameIdentifierFactory = secharmony.transformerPlugin.createTransformerFactory(options);
        const fileContent = `export @interface __$$ETS_ANNOTATION$$__Anno1 {prop1: number = 1}`;
        const textWriter = ts.createTextWriter('\n');
        let arkobfuscator = new ArkObfuscatorForTest();
        arkobfuscator.init(options);
        const sourceFile: ts.SourceFile = ts.createSourceFile('demo.ts', fileContent, ts.ScriptTarget.ES2015, true, undefined, {
          'etsAnnotationsEnable': true
        }, true);
        let transformedResult: ts.TransformationResult<ts.Node> = ts.transform(sourceFile, [renameIdentifierFactory], {});
        let ast: ts.SourceFile = transformedResult.transformed[0] as ts.SourceFile;
        arkobfuscator.createObfsPrinter(ast.isDeclarationFile).writeFile(ast, textWriter, undefined);
        const actualContent = textWriter.getText();
        const expectContent = `export @interface __$$ETS_ANNOTATION$$__Anno1 {
            prop1: number = 1;
        }`;
        assert.strictEqual(compareStringsIgnoreNewlines(actualContent, expectContent), true);
        PropCollections.clearPropsCollections();
      })

      it('should reserve available when enable obfuscation', function () {
        let options: IOptions = {
          'mNameObfuscation': {
            'mEnable': true,
            'mRenameProperties': false,
            'mReservedProperties': ['Available','minApiVersion'],
            'mTopLevel': true,
            'mKeepParameterNames': false
          },
          'mAllowEtsAnnotations': true,
          'mExportObfuscation': true,
        };
        assert.strictEqual(options !== undefined, true);
        const renameIdentifierFactory = secharmony.transformerPlugin.createTransformerFactory(options);
        const textWriter = ts.createTextWriter('\n');
        let arkobfuscator = new ArkObfuscatorForTest();
        arkobfuscator.init(options);
        const sourceFile: ts.SourceFile = ts.createSourceFile('demo.ts', fileContentAvailable, ts.ScriptTarget.ES2015, true, undefined, {
          'etsAnnotationsEnable': true
        }, true);
        let transformedResult: ts.TransformationResult<ts.Node> = ts.transform(sourceFile, [renameIdentifierFactory], {});
        let ast: ts.SourceFile = transformedResult.transformed[0] as ts.SourceFile;
        arkobfuscator.createObfsPrinter(ast.isDeclarationFile).writeFile(ast, textWriter, undefined);
        const actualContent = textWriter.getText();
        const expectContent = `
        @Available({minApiVersion: '23'})
        export let a = 111;
        @Available({minApiVersion: '23'})
        export let b = "111";
        `;
        assert.strictEqual(compareStringsIgnoreNewlines(actualContent, expectContent), true);
        PropCollections.clearPropsCollections();
      })

      it('should obfuscate when annotationName is exported and enable export-obfuscation', function () {
        let options: IOptions = {
          'mNameObfuscation': {
            'mEnable': true,
            'mRenameProperties': false,
            'mReservedProperties': [],
            'mTopLevel': true,
            'mKeepParameterNames': false
          },
          'mAllowEtsAnnotations': true,
          'mExportObfuscation': true,
        };
        assert.strictEqual(options !== undefined, true);
        const renameIdentifierFactory = secharmony.transformerPlugin.createTransformerFactory(options);
        const fileContent = `export @interface __$$ETS_ANNOTATION$$__Anno1 {prop1: number = 1}`;
        const textWriter = ts.createTextWriter('\n');
        let arkobfuscator = new ArkObfuscatorForTest();
        arkobfuscator.init(options);
        const sourceFile: ts.SourceFile = ts.createSourceFile('demo.ts', fileContent, ts.ScriptTarget.ES2015, true, undefined, {
          'etsAnnotationsEnable': true
        }, true);
        let transformedResult: ts.TransformationResult<ts.Node> = ts.transform(sourceFile, [renameIdentifierFactory], {});
        let ast: ts.SourceFile = transformedResult.transformed[0] as ts.SourceFile;
        arkobfuscator.createObfsPrinter(ast.isDeclarationFile).writeFile(ast, textWriter, undefined);
        const actualContent = textWriter.getText();
        const expectContent = `export @interface __$$ETS_ANNOTATION$$__a {
            prop1: number = 1;
        }`;
        assert.strictEqual(compareStringsIgnoreNewlines(actualContent, expectContent), true);
        PropCollections.clearPropsCollections();
      })
    })
  })
})

export function compareStringsIgnoreNewlines(str1: string, str2: string): boolean {
  const normalize = (str: string) => str.replace(/[\n\r\s]+/g, '');
  return normalize(str1) === normalize(str2);
}