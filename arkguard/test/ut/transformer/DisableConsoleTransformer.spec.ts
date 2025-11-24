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

import { assert, expect } from 'chai';
import { it, describe} from 'mocha';
import { IOptions } from '../../../src/configs/IOptions';
import { FileUtils } from '../../../src/utils/FileUtils';
import { wildcardTransformer } from '../../../src/utils/TransformUtil';

import ts from 'typescript';
import path from 'path';
import secharmony from '../../../src/transformers/layout/DisableConsoleTransformer';

describe('Tester cases for <DisableConsoleTransformer>', function() {
  it('should return null if mDisableConsole is false and mRemoveNoSideEffectsCalls is empty', () => {
    let options: IOptions | undefined = FileUtils.readFileAsJson(path.join(__dirname, "default_config.json"));
    assert.strictEqual(options !== undefined, true);
    const disableConsoleFactory = secharmony.transformerPlugin.createTransformerFactory(options as IOptions);
    expect(disableConsoleFactory).to.be.equal(null);
  });

  it('should return function if mDisableConsole is true', () => {
    let options: IOptions = {
      "mDisableConsole": true
    }
    const disableConsoleFactory = secharmony.transformerPlugin.createTransformerFactory(options as IOptions);
    expect(typeof disableConsoleFactory).to.be.equal('function');
  });

  it('should return function if mRemoveNoSideEffectsCalls.mRemovedCallNames is defined', () => {
    let options: IOptions = {
      "mRemoveNoSideEffectsCalls": {
          "mRemovedCallNames": ["class.method"]
      }
    }
    const disableConsoleFactory = secharmony.transformerPlugin.createTransformerFactory(options as IOptions);
    expect(typeof disableConsoleFactory).to.be.equal('function');
  });

  it('should return function if mRemoveNoSideEffectsCalls.mUniversalRemovedCallNames is defined', () => {
    let universalRemovedCallRegx = wildcardTransformer("class[\"property?\"]*");
    let universalRemovedCallRegExp =  new RegExp(`^${universalRemovedCallRegx}$`);
    let options: IOptions = {
      "mRemoveNoSideEffectsCalls": {
          "mRemovedCallNames": [],
          "mUniversalRemovedCallNames": [universalRemovedCallRegExp]
      }
    }
    const disableConsoleFactory = secharmony.transformerPlugin.createTransformerFactory(options as IOptions);
    expect(typeof disableConsoleFactory).to.be.equal('function');
  });

  it('should return origin node if isSourceFile is false', () => {
    let options: IOptions | undefined = FileUtils.readFileAsJson(path.join(__dirname, "obfuscate_identifier_config.json"));
    assert.strictEqual(options !== undefined, true);
    const disableConsoleFactory = secharmony.transformerPlugin.createTransformerFactory(options as IOptions);
    const blockNode: ts.Block = ts.factory.createBlock([]);
    let transformedResult: ts.TransformationResult<ts.Node> = ts.transform(blockNode, [disableConsoleFactory], {});
    expect(blockNode).to.be.equal(transformedResult.transformed[0]);
  });

  describe('test cases for visitAst', () => {
    let options: IOptions | undefined = FileUtils.readFileAsJson(path.join(__dirname, "obfuscate_identifier_config.json"));
    assert.strictEqual(options !== undefined, true);
    const disableConsoleFactory = secharmony.transformerPlugin.createTransformerFactory(options as IOptions);

    it('should return factory.createModuleBlock', () => {
      const fileContent = `
        module Test {
          console.log("test whether the Block if created by factory.createModuleBlock");
        }
      `;
      const sourceFile: ts.SourceFile = ts.createSourceFile('demo.ts', fileContent, ts.ScriptTarget.ES2015, true);
      let transformedResult: ts.TransformationResult<ts.Node> = ts.transform(sourceFile, [disableConsoleFactory], {});
      assert.strictEqual((((transformedResult.transformed[0] as ts.SourceFile).statements[0] as ts.FunctionDeclaration).body!.statements.length), 0);
    });

    it('should return factory.createCaseClause and factory.createDefaultClause', () => {
      const fileContent = `
        function workday(day: number): void {
          switch (day) {
            case 6:
            case 7:
              console.log("Not Workday");
            default:
              console.log("Workday");
          }
        }
      `;
      const sourceFile: ts.SourceFile = ts.createSourceFile('demo.ts', fileContent, ts.ScriptTarget.ES2015, true);
      let transformedResult: ts.TransformationResult<ts.Node> = ts.transform(sourceFile, [disableConsoleFactory], {});
      assert.strictEqual(
        (((transformedResult
          .transformed[0] as ts.SourceFile)
          .statements[0] as ts.FunctionDeclaration)
          .body!
          .statements[0] as ts.SwitchStatement)
          .caseBlock
          .clauses[1]
          .statements
          .length
          , 0);
      assert.strictEqual(
        (((transformedResult
          .transformed[0] as ts.SourceFile)
          .statements[0] as ts.FunctionDeclaration)
          .body!
          .statements[0] as ts.SwitchStatement)
          .caseBlock
          .clauses[2]
          .statements
          .length
          , 0);
    });
  });

  describe('test cases for isSimpleConsoleStatement', () => {
    let options: IOptions | undefined = FileUtils.readFileAsJson(path.join(__dirname, "obfuscate_identifier_config.json"));
    assert.strictEqual(options !== undefined, true);
    const disableConsoleFactory = secharmony.transformerPlugin.createTransformerFactory(options as IOptions);
    const universalCalls = wildcardTransformer("testClass?.property.*Log");
    const regUniversalCalls = new RegExp(`^${universalCalls}$`);
    options?.mRemoveNoSideEffectsCalls?.mUniversalRemovedCallNames?.push(regUniversalCalls);

    it('should return true beacause the content of fileContent is the support simple format', () => {
      const fileContent = `
        console.log('Hello, World!');
        console['log']('hello, world');
      `;
      const sourceFile: ts.SourceFile = ts.createSourceFile('demo.ts', fileContent, ts.ScriptTarget.ES2015, true);
      let transformedResult: ts.TransformationResult<ts.Node> = ts.transform(sourceFile, [disableConsoleFactory], {});
      assert.strictEqual((transformedResult.transformed[0] as ts.SourceFile).statements.length, 0);
    });

    it('should return false because there is no console keyword or other configured calls in the source file', () => {
      const fileContent = `
        [];
      `;
      const sourceFile: ts.SourceFile = ts.createSourceFile('demo.ts', fileContent, ts.ScriptTarget.ES2015, true);
      let transformedResult: ts.TransformationResult<ts.Node> = ts.transform(sourceFile, [disableConsoleFactory], {});
      assert.strictEqual(
        (((transformedResult
          .transformed[0] as ts.SourceFile)
          .statements[0] as ts.ExpressionStatement)
          .expression as ts.ArrayLiteralExpression)
          .elements
          .length
          , 0);
    });

    it('should return true because the content of fileContent is the configured format', () => {
      const fileContent = `
        testClass1.Log();
        testClass1['property']["Log"]();
        testClass2.property.firstLog();
        testClass3.property.LastLog();
      `;
      const sourceFile: ts.SourceFile = ts.createSourceFile('demo.ts', fileContent, ts.ScriptTarget.ES2015, true);
      let transformedResult: ts.TransformationResult<ts.Node> = ts.transform(sourceFile, [disableConsoleFactory], {});
      assert.strictEqual((transformedResult.transformed[0] as ts.SourceFile).statements.length, 0);
    });
  });
});