/**
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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
import { TransformerManager } from '../../../src/transformers/TransformerManager';
import { assert, expect } from 'chai';
import { FileUtils } from '../../../src/utils/FileUtils';
import path from 'path';
import { IOptions } from '../../../src/configs/IOptions';
import fs from 'fs';
import ts, {
  TransformationResult,
  factory,
  transform,
  SourceFile,
  Node,
  IndexedAccessTypeNode,
  LiteralTypeNode,
  setParentRecursive,
  UnionTypeNode
} from 'typescript';
import secharmony from '../../../src/transformers/rename/RenamePropertiesTransformer';
import { ArkObfuscator } from '../../../src/ArkObfuscator';
import { ArkObfuscatorForTest } from '../../../src/ArkObfuscatorForTest'
import { PropCollections } from '../../../src/utils/CommonCollections';


describe('Tester Cases for <RenamePropertiesTransformer>.', function () {
  describe('Tester Cases for <renameIndexedAccessProperty>.', function () {
    let options: IOptions;
    beforeEach(() => {
      options = {
        mNameObfuscation: {
          mEnable: true,
          mNameGeneratorType: 1,
          mRenameProperties: true,
          mKeepStringProperty: false,
          mReservedProperties: []
        },
      }
    });

    it('should rename string property in indexedAccessType when using prop+strProp', function () {
      ArkObfuscator.isKeptCurrentFile = false;
      const renamePropertiesFactory = secharmony.transformerPlugin.createTransformerFactory(options as IOptions);
      let sourcefile = factory.createIndexedAccessTypeNode(
        factory.createTypeReferenceNode(
          factory.createIdentifier("T"),
          undefined
        ),
        factory.createLiteralTypeNode(factory.createStringLiteral("X"))
      )
      let parentNodes = setParentRecursive(sourcefile, true);
      let transfomedResult: TransformationResult<Node> = transform(parentNodes, [renamePropertiesFactory], {});
      let node = transfomedResult.transformed[0] as IndexedAccessTypeNode;
      expect((node.indexType as LiteralTypeNode).literal).not.to.be.equal((parentNodes.indexType as LiteralTypeNode).literal);
    });

    it('should not rename string property in indexedAccessType when using prop', function () {
      ArkObfuscator.isKeptCurrentFile = false;
      options = {
        mNameObfuscation: {
          mEnable: true,
          mNameGeneratorType: 1,
          mRenameProperties: true,
          mKeepStringProperty: true,
          mReservedProperties: []
        },
      };
      const renamePropertiesFactory = secharmony.transformerPlugin.createTransformerFactory(options as IOptions);
      let sourcefile = factory.createIndexedAccessTypeNode(
        factory.createTypeReferenceNode(
          factory.createIdentifier("T"),
          undefined
        ),
        factory.createLiteralTypeNode(factory.createStringLiteral("X"))
      )
      let parentNodes = setParentRecursive(sourcefile, true);
      let transfomedResult: TransformationResult<Node> = transform(parentNodes, [renamePropertiesFactory], {});
      let node = transfomedResult.transformed[0] as IndexedAccessTypeNode;
      expect((node.indexType as LiteralTypeNode).literal).to.be.equal((parentNodes.indexType as LiteralTypeNode).literal);
    });

    it('should not rename numeric property in indexedAccessType when using prop+strProp', function () {
      ArkObfuscator.isKeptCurrentFile = false;
      options = {
        mNameObfuscation: {
          mEnable: true,
          mNameGeneratorType: 1,
          mRenameProperties: true,
          mKeepStringProperty: false,
          mReservedProperties: []
        },
      };
      const renamePropertiesFactory = secharmony.transformerPlugin.createTransformerFactory(options as IOptions);
      let sourcefile = factory.createIndexedAccessTypeNode(
        factory.createTypeReferenceNode(
          factory.createIdentifier("T"),
          undefined
        ),
        factory.createLiteralTypeNode(factory.createNumericLiteral(1))
      )
      let parentNodes = setParentRecursive(sourcefile, true);
      let transfomedResult: TransformationResult<Node> = transform(parentNodes, [renamePropertiesFactory], {});
      let node = transfomedResult.transformed[0] as IndexedAccessTypeNode;
      expect((node.indexType as LiteralTypeNode).literal).to.be.equal((parentNodes.indexType as LiteralTypeNode).literal);
    });

    it('should rename string property in unionType of indexedAccessType when using prop+strProp', function () {
      ArkObfuscator.isKeptCurrentFile = false;
      options = {
        mNameObfuscation: {
          mEnable: true,
          mNameGeneratorType: 1,
          mRenameProperties: true,
          mKeepStringProperty: false,
          mReservedProperties: []
        },
      };
      const renamePropertiesFactory = secharmony.transformerPlugin.createTransformerFactory(options as IOptions);
      let sourcefile = factory.createIndexedAccessTypeNode(
        factory.createTypeReferenceNode(
          factory.createIdentifier("T"),
          undefined
        ),
        factory.createUnionTypeNode([
          factory.createLiteralTypeNode(factory.createStringLiteral("U")),
          factory.createLiteralTypeNode(factory.createStringLiteral("V"))
        ])
      )
      let parentNodes = setParentRecursive(sourcefile, true);
      let transfomedResult: TransformationResult<Node> = transform(parentNodes, [renamePropertiesFactory], {});
      let node = transfomedResult.transformed[0] as IndexedAccessTypeNode;
      expect(((node.indexType as UnionTypeNode).types[0] as LiteralTypeNode).literal).not.to.be.equal(((parentNodes.indexType as UnionTypeNode).types[0] as LiteralTypeNode).literal);
      expect(((node.indexType as UnionTypeNode).types[1] as LiteralTypeNode).literal).not.to.be.equal(((parentNodes.indexType as UnionTypeNode).types[1] as LiteralTypeNode).literal);
    });

    it('should not rename string property in unionType of indexedAccessType when using prop', function () {
      ArkObfuscator.isKeptCurrentFile = false;
      options = {
        mNameObfuscation: {
          mEnable: true,
          mNameGeneratorType: 1,
          mRenameProperties: true,
          mKeepStringProperty: true,
          mReservedProperties: []
        },
      };
      const renamePropertiesFactory = secharmony.transformerPlugin.createTransformerFactory(options as IOptions);
      let sourcefile = factory.createIndexedAccessTypeNode(
        factory.createTypeReferenceNode(
          factory.createIdentifier("T"),
          undefined
        ),
        factory.createUnionTypeNode([
          factory.createLiteralTypeNode(factory.createStringLiteral("U")),
          factory.createLiteralTypeNode(factory.createStringLiteral("V"))
        ])
      )
      let parentNodes = setParentRecursive(sourcefile, true);
      let transfomedResult: TransformationResult<Node> = transform(parentNodes, [renamePropertiesFactory], {});
      let node = transfomedResult.transformed[0] as IndexedAccessTypeNode;
      expect(((node.indexType as UnionTypeNode).types[0] as LiteralTypeNode).literal).to.be.equal(((parentNodes.indexType as UnionTypeNode).types[0] as LiteralTypeNode).literal);
      expect(((node.indexType as UnionTypeNode).types[1] as LiteralTypeNode).literal).to.be.equal(((parentNodes.indexType as UnionTypeNode).types[1] as LiteralTypeNode).literal);
    });

    it('should not rename numeric property in unionType of indexedAccessType when using prop+strProp', function () {
      ArkObfuscator.isKeptCurrentFile = false;
      options = {
        mNameObfuscation: {
          mEnable: true,
          mNameGeneratorType: 1,
          mRenameProperties: true,
          mKeepStringProperty: false,
          mReservedProperties: []
        },
      };
      const renamePropertiesFactory = secharmony.transformerPlugin.createTransformerFactory(options as IOptions);
      let sourcefile = factory.createIndexedAccessTypeNode(
        factory.createTypeReferenceNode(
          factory.createIdentifier("T"),
          undefined
        ),
        factory.createUnionTypeNode([
          factory.createLiteralTypeNode(factory.createNumericLiteral(1)),
          factory.createLiteralTypeNode(factory.createNumericLiteral(2))
        ])
      )
      let parentNodes = setParentRecursive(sourcefile, true);
      let transfomedResult: TransformationResult<Node> = transform(parentNodes, [renamePropertiesFactory], {});
      let node = transfomedResult.transformed[0] as IndexedAccessTypeNode;
      expect(((node.indexType as UnionTypeNode).types[0] as LiteralTypeNode).literal).to.be.equal(((parentNodes.indexType as UnionTypeNode).types[0] as LiteralTypeNode).literal);
      expect(((node.indexType as UnionTypeNode).types[1] as LiteralTypeNode).literal).to.be.equal(((parentNodes.indexType as UnionTypeNode).types[1] as LiteralTypeNode).literal);
    });

    it('should not rename property in annotation decl and callsite when using prop', function () {
      ArkObfuscator.isKeptCurrentFile = false;
      options = {
        mNameObfuscation: {
          mEnable: true,
          mNameGeneratorType: 1,
          mRenameProperties: true,
          mKeepStringProperty: false,
          mReservedProperties: []
        },
      };
      const renameIdentifierFactory = secharmony.transformerPlugin.createTransformerFactory(options as IOptions);
      const fileContent = `
        @interface __$$ETS_ANNOTATION$$__Anno1 {
          prop1: number = 1;
        }
        @__$$ETS_ANNOTATION$$__Anno1({prop: 1})
        class myClass1 {
          prop1: number = 1;
        }
        let var1: myClass1 = new myClass1();
        let var2 = var1.prop1;
      `;
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
      const expectContent = `
        @interface __$$ETS_ANNOTATION$$__Anno1 {
            prop1: number = 1;
        }
        @__$$ETS_ANNOTATION$$__Anno1({ prop: 1 })
        class myClass1 {
            g: number = 1;
        }
        let var1: myClass1 = new myClass1();
        let var2 = var1.g;
      `;
      assert.strictEqual(compareStringsIgnoreNewlines(actualContent, expectContent), true);
      PropCollections.clearPropsCollections();
    });

    it('should not rename object literal property inside source-code annotation in .ets', function () {
      ArkObfuscator.isKeptCurrentFile = false;
      options = {
        mNameObfuscation: {
          mEnable: true,
          mNameGeneratorType: 1,
          mRenameProperties: true,
          mKeepStringProperty: false,
          mReservedProperties: []
        },
      };
      const renameIdentifierFactory = secharmony.transformerPlugin.createTransformerFactory(options as IOptions);
      const fileContent = `
        annotation Anno2 {
          value: number;
        }
        @Anno2({prop: 1})
        class MyClass2 {
          field1: number = 1;
        }
      `;
      const textWriter = ts.createTextWriter('\n');
      let arkobfuscator = new ArkObfuscatorForTest();
      arkobfuscator.init(options);
      const sourceFile: ts.SourceFile = ts.createSourceFile('demo.ets', fileContent, ts.ScriptTarget.ES2015, true, undefined, {
        'etsAnnotationsEnable': true
      }, false);
      let transformedResult: ts.TransformationResult<ts.Node> = ts.transform(sourceFile, [renameIdentifierFactory], {});
      let ast: ts.SourceFile = transformedResult.transformed[0] as ts.SourceFile;
      arkobfuscator.createObfsPrinter(ast.isDeclarationFile).writeFile(ast, textWriter, undefined);
      const actualContent = textWriter.getText();
      const expectContent = `
        annotation;
        Anno2;
        {
            value: number;
        }
        @Anno2({ prop: 1 })
        class MyClass2 {
            g: number = 1;
        }
      `;
      assert.strictEqual(compareStringsIgnoreNewlines(actualContent, expectContent), true);
      PropCollections.clearPropsCollections();
    });

    it('should preserve and visit illegalDecorators on function declaration with annotation usage', function () {
      ArkObfuscator.isKeptCurrentFile = false;
      options = {
        mNameObfuscation: {
          mEnable: true,
          mNameGeneratorType: 1,
          mRenameProperties: true,
          mKeepStringProperty: false,
          mReservedProperties: []
        },
      };
      const renameIdentifierFactory = secharmony.transformerPlugin.createTransformerFactory(options as IOptions);
      const fileContent = `
        annotation Anno3 {
          value: string;
        }
        @Anno3({x: 1})
        function foo() {}
      `;
      const textWriter = ts.createTextWriter('\n');
      let arkobfuscator = new ArkObfuscatorForTest();
      arkobfuscator.init(options);
      const sourceFile: ts.SourceFile = ts.createSourceFile('demo.ets', fileContent, ts.ScriptTarget.ES2015, true, undefined, {
        'etsAnnotationsEnable': true
      }, false);
      let transformedResult: ts.TransformationResult<ts.Node> = ts.transform(sourceFile, [renameIdentifierFactory], {});
      let ast: ts.SourceFile = transformedResult.transformed[0] as ts.SourceFile;
      arkobfuscator.createObfsPrinter(ast.isDeclarationFile).writeFile(ast, textWriter, undefined);
      const actualContent = textWriter.getText();
      const expectContent = `
        annotation;
        Anno3;
        {
            value: string;
        }
        @Anno3({ x: 1 })
        function foo() { }
      `;
      assert.strictEqual(compareStringsIgnoreNewlines(actualContent, expectContent), true);
      PropCollections.clearPropsCollections();
    });

    it('should rename annotation called as prop when using prop obfuscation', function () {
      ArkObfuscator.isKeptCurrentFile = false;
      options = {
        mNameObfuscation: {
          mEnable: true,
          mNameGeneratorType: 1,
          mRenameProperties: true,
          mKeepStringProperty: false,
          mReservedProperties: []
        },
      };
      const renameIdentifierFactory = secharmony.transformerPlugin.createTransformerFactory(options as IOptions);
      const fileContent = `
        @ns.__$$ETS_ANNOTATION$$__Anno1({prop: 1})
        class myClass1 {}
      `;
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
      const expectContent = `
        @ns.__$$ETS_ANNOTATION$$__g({ prop: 1 })
        class myClass1 {}
      `;
      assert.strictEqual(compareStringsIgnoreNewlines(actualContent, expectContent), true);
      PropCollections.clearPropsCollections();
    });
  });
});

function compareStringsIgnoreNewlines(str1: string, str2: string): boolean {
  const normalize = (str: string) => str.replace(/[\n\r\s]+/g, '');
  return normalize(str1) === normalize(str2);
}
