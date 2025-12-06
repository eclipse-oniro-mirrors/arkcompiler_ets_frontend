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

import ts from 'typescript';
import { shouldKeepCurFileParamerters, shouldKeepParameter } from '../../../src/utils/KeepParameterUtils';
import { INameObfuscationOption } from '../../../src/configs/INameObfuscationOption';
import assert from 'assert';
import { MangledSymbolInfo } from '../../../src/common/type';
import { TypeUtils } from '../../../src/utils/TypeUtils';
import { NodeUtils } from '../../../src/utils/NodeUtils';
import { PropCollections } from '../../../src/ArkObfuscator';
import { nodeSymbolMap } from '../../../src/utils/ScopeAnalyzer';

describe('Tester Cases for shouldKeepCurFileParamerters.', function () {
  it('Tester 1-1: shouldKeepCurFileParamerters returns true', function () {
    let path = '/user/local/tester.d.ts';
    let content = 'export declare let a: number';
    const ast = ts.createSourceFile(path, content, ts.ScriptTarget.ES2015, true);
    const config: INameObfuscationOption = {
      'mEnable': true,
      'mNameGeneratorType': 1,
      'mDictionaryList': [],
      'mRenameProperties': false,
      'mKeepStringProperty': true,
      'mTopLevel': false,
      'mKeepParameterNames': true,
      'mReservedProperties': []
    };
    const actual = shouldKeepCurFileParamerters(ast, config);
    assert.strictEqual(actual, true);
  });

  it('Tester 1-2: When mKeepParameterNames is disabled, shouldKeepCurFileParamerters returns false', function () {
    let path = '/user/local/tester.d.ts';
    let content = 'export declare let a: number';
    const ast = ts.createSourceFile(path, content, ts.ScriptTarget.ES2015, true);
    const config: INameObfuscationOption = {
      'mEnable': true,
      'mNameGeneratorType': 1,
      'mDictionaryList': [],
      'mRenameProperties': false,
      'mKeepStringProperty': true,
      'mTopLevel': false,
      'mKeepParameterNames': false,
      'mReservedProperties': []
    };
    const actual = shouldKeepCurFileParamerters(ast, config);
    assert.strictEqual(actual, false);
  });

  it('Tester 1-3: When file is not declaration file, shouldKeepCurFileParamerters returns false', function () {
    let path = '/user/local/tester.ts';
    let content = 'export declare let a: number';
    const ast = ts.createSourceFile(path, content, ts.ScriptTarget.ES2015, true);
    const config: INameObfuscationOption = {
      'mEnable': true,
      'mNameGeneratorType': 1,
      'mDictionaryList': [],
      'mRenameProperties': false,
      'mKeepStringProperty': true,
      'mTopLevel': false,
      'mKeepParameterNames': true,
      'mReservedProperties': []
    };
    const actual = shouldKeepCurFileParamerters(ast, config);
    assert.strictEqual(actual, false);
  });
});

describe('Tester Cases for shouldKeepParameter.', function () {
  let path = '';
  let config: INameObfuscationOption;
  this.beforeAll(() => {
    path = '/user/local/tester.d.ts';
    config = {
      'mEnable': true,
      'mNameGeneratorType': 1,
      'mDictionaryList': [],
      'mRenameProperties': false,
      'mKeepStringProperty': true,
      'mTopLevel': false,
      'mKeepParameterNames': true,
      'mReservedProperties': []
    }
  });
  it('Tester 2-1: When node is undefined, shouldKeepParameter returns false', function () {
    let content = 'export declare let a: number';
    const ast = ts.createSourceFile(path, content, ts.ScriptTarget.ES2015, true);
    let node: ts.Identifier = undefined;
    let mangledSymbolNames: Map<ts.Symbol, MangledSymbolInfo> = new Map<ts.Symbol, MangledSymbolInfo>();
    const checker: ts.TypeChecker = TypeUtils.createChecker(ast);
    const actual = shouldKeepParameter(node, config, mangledSymbolNames, checker);
    assert.strictEqual(actual, false);
  });

  it('Tester 2-2: When node is not parameter, shouldKeepParameter returns false', function () {
    let content = 'export declare let a: number';
    const ast = ts.createSourceFile(path, content, ts.ScriptTarget.ES2015, true);
    let node = (ast.statements[0] as ts.VariableStatement).declarationList.declarations[0].name;
    let mangledSymbolNames: Map<ts.Symbol, MangledSymbolInfo> = new Map<ts.Symbol, MangledSymbolInfo>();
    const checker: ts.TypeChecker = TypeUtils.createChecker(ast);
    // @ts-ignore
    const actual = shouldKeepParameter(node, config, mangledSymbolNames, checker);
    assert.strictEqual(actual, false);
  });

  it('Tester 2-3: When node is parameter has no parent node, shouldKeepParameter returns false', function () {
    let content = 'export declare function foo(para1: number): void';
    const ast = ts.createSourceFile(path, content, ts.ScriptTarget.ES2015, true);
    const customNode = ts.createParameter(undefined, undefined, 'a');
    let mangledSymbolNames: Map<ts.Symbol, MangledSymbolInfo> = new Map<ts.Symbol, MangledSymbolInfo>();
    const checker: ts.TypeChecker = TypeUtils.createChecker(ast);
    const actual = shouldKeepParameter(customNode, config, mangledSymbolNames, checker);
    assert.strictEqual(actual, false);
  });

  it('Tester 2-4: When node is parameter, shouldKeepParameter returns true', function () {
    let content = 'export declare function foo(para1: number): void';
    const ast = ts.createSourceFile(path, content, ts.ScriptTarget.ES2015, true);
    let node = (ast.statements[0] as ts.FunctionDeclaration).parameters[0];
    let mangledSymbolNames: Map<ts.Symbol, MangledSymbolInfo> = new Map<ts.Symbol, MangledSymbolInfo>();
    const checker: ts.TypeChecker = TypeUtils.createChecker(ast);
    const actual = shouldKeepParameter(node, config, mangledSymbolNames, checker);
    assert.strictEqual(actual, true);
  });

  it('Tester 2-5: When node is TypeParameter, shouldKeepParameter returns true', function () {
    let content = 'export declare function foo<T>(para1: number): void';
    const ast = ts.createSourceFile(path, content, ts.ScriptTarget.ES2015, true);
    let node = (ast.statements[0] as ts.FunctionDeclaration).typeParameters[0] as ts.TypeParameterDeclaration;
    const checker: ts.TypeChecker = TypeUtils.createChecker(ast);
    let mangledSymbolNames: Map<ts.Symbol, MangledSymbolInfo> = new Map<ts.Symbol, MangledSymbolInfo>();
    const actual = shouldKeepParameter(node, config, mangledSymbolNames, checker);
    assert.strictEqual(actual, true);
  });

  it('Tester 2-6: When node is parameter and function name is not kept, shouldKeepParameter returns false', function () {
    let content = 'export declare function foo<T>(para1: number): void';
    const ast = ts.createSourceFile(path, content, ts.ScriptTarget.ES2015, true);
    let node: ts.ParameterDeclaration = (ast.statements[0] as ts.FunctionDeclaration).parameters[0];
    const checker: ts.TypeChecker = TypeUtils.createChecker(ast);
    const parentNode: ts.FunctionDeclaration = node.parent as ts.FunctionDeclaration;
    assert.strictEqual(!!parentNode, true);
    const functionName: ts.Identifier = parentNode.name as ts.Identifier;
    assert.strictEqual(!!functionName, true);
    const sym: ts.Symbol = NodeUtils.findSymbolOfIdentifier(checker, functionName, nodeSymbolMap) as ts.Symbol;
    let mangledSymbolNames: Map<ts.Symbol, MangledSymbolInfo> = new Map<ts.Symbol, MangledSymbolInfo>();
    mangledSymbolNames.set(sym, { mangledName: 'a', originalNameWithScope: '' });
    const actual = shouldKeepParameter(node, config, mangledSymbolNames, checker);
    assert.strictEqual(actual, false);
  });

  it('Tester 2-7: When node is parameter and function name is kept, shouldKeepParameter returns true', function () {
    let content = 'export declare function foo<T>(para1: number): void';
    const ast = ts.createSourceFile(path, content, ts.ScriptTarget.ES2015, true);
    let node: ts.ParameterDeclaration = (ast.statements[0] as ts.FunctionDeclaration).parameters[0];
    const checker: ts.TypeChecker = TypeUtils.createChecker(ast);
    const parentNode: ts.FunctionDeclaration = node.parent as ts.FunctionDeclaration;
    assert.strictEqual(!!parentNode, true);
    const functionName: ts.Identifier = parentNode.name as ts.Identifier;
    assert.strictEqual(!!functionName, true);
    const sym: ts.Symbol = NodeUtils.findSymbolOfIdentifier(checker, functionName, nodeSymbolMap) as ts.Symbol;
    let mangledSymbolNames: Map<ts.Symbol, MangledSymbolInfo> = new Map<ts.Symbol, MangledSymbolInfo>();
    mangledSymbolNames.set(sym, { mangledName: 'foo', originalNameWithScope: '' });
    const actual = shouldKeepParameter(node, config, mangledSymbolNames, checker);
    assert.strictEqual(actual, true);
  });

  it('Tester 2-8: When node is parameter and function name is kept, shouldKeepParameter returns true', function () {
    let content = 'export declare function foo<T>(para1: number): void';
    const ast = ts.createSourceFile(path, content, ts.ScriptTarget.ES2015, true);
    let node: ts.ParameterDeclaration = (ast.statements[0] as ts.FunctionDeclaration).parameters[0];
    const checker: ts.TypeChecker = TypeUtils.createChecker(ast);
    const parentNode: ts.FunctionDeclaration = node.parent as ts.FunctionDeclaration;
    assert.strictEqual(!!parentNode, true);
    const functionName: ts.Identifier = parentNode.name as ts.Identifier;
    assert.strictEqual(!!functionName, true);
    const sym: ts.Symbol = NodeUtils.findSymbolOfIdentifier(checker, functionName, nodeSymbolMap) as ts.Symbol;
    let mangledSymbolNames: Map<ts.Symbol, MangledSymbolInfo> = new Map<ts.Symbol, MangledSymbolInfo>();
    mangledSymbolNames.set(sym, { mangledName: 'foo', originalNameWithScope: '' });
    const actual = shouldKeepParameter(node, config, mangledSymbolNames, checker);
    assert.strictEqual(actual, true);
  });

  it('Tester 2-9: When node is method and property obf is not enabled, shouldKeepParameter returns true', function () {
    let content = `declare class A {
      constructor(para1: number);
      method(para2: number): void;
      }`;
    const ast = ts.createSourceFile(path, content, ts.ScriptTarget.ES2015, true);
    let method: ts.MethodDeclaration = (ast.statements[0] as ts.ClassDeclaration).members[1] as ts.MethodDeclaration;
    let node: ts.ParameterDeclaration = method.parameters[0];
    const checker: ts.TypeChecker = TypeUtils.createChecker(ast);
    const methodName: ts.Identifier = method.name as ts.Identifier;
    assert.strictEqual(!!methodName, true);
    let mangledSymbolNames: Map<ts.Symbol, MangledSymbolInfo> = new Map<ts.Symbol, MangledSymbolInfo>();
    const actual = shouldKeepParameter(node, config, mangledSymbolNames, checker);
    assert.strictEqual(actual, true);
  });

  it('Tester 2-10: When node is method and method name is not kept, shouldKeepParameter returns false', function () {
    let content = `declare class A {
      constructor(para11: number);
      method1(para12: number): void;
      }`;
    const ast = ts.createSourceFile(path, content, ts.ScriptTarget.ES2015, true);
    let method: ts.MethodDeclaration = (ast.statements[0] as ts.ClassDeclaration).members[1] as ts.MethodDeclaration;
    let node: ts.ParameterDeclaration = method.parameters[0];
    const checker: ts.TypeChecker = TypeUtils.createChecker(ast);
    const methodName: ts.Identifier = method.name as ts.Identifier;
    assert.strictEqual(!!methodName, true);
    const customConfig: INameObfuscationOption = JSON.parse(JSON.stringify(config));
    // @ts-ignore
    customConfig['mRenameProperties'] = true;
    let mangledSymbolNames: Map<ts.Symbol, MangledSymbolInfo> = new Map<ts.Symbol, MangledSymbolInfo>();
    const actual = shouldKeepParameter(node, customConfig, mangledSymbolNames, checker);
    assert.strictEqual(actual, false);
  });

  it('Tester 2-11: When node is method and method name is kept, shouldKeepParameter returns true', function () {
    let content = `declare class A {
      constructor(para1: number);
      method(para2: number): void;
      }`;
    const ast = ts.createSourceFile(path, content, ts.ScriptTarget.ES2015, true);
    let method: ts.MethodDeclaration = (ast.statements[0] as ts.ClassDeclaration).members[1] as ts.MethodDeclaration;
    let node: ts.ParameterDeclaration = method.parameters[0];
    const checker: ts.TypeChecker = TypeUtils.createChecker(ast);
    const methodName: ts.Identifier = method.name as ts.Identifier;
    assert.strictEqual(!!methodName, true);
    let mangledSymbolNames: Map<ts.Symbol, MangledSymbolInfo> = new Map<ts.Symbol, MangledSymbolInfo>();
    PropCollections.reservedProperties.add('method');
    const customConfig: INameObfuscationOption = JSON.parse(JSON.stringify(config));
    // @ts-ignore
    customConfig['mRenameProperties'] = true;
    const actual = shouldKeepParameter(node, customConfig, mangledSymbolNames, checker);
    assert.strictEqual(actual, true);
  });

  it('Tester 2-12: When node is constructor and class name is not kept, shouldKeepParameter returns false', function () {
    let content = `declare class A {
      constructor(para1: number);
      method(para2: number): void;
      }`;
    const ast = ts.createSourceFile(path, content, ts.ScriptTarget.ES2015, true);
    let construcotr: ts.ConstructorDeclaration = (ast.statements[0] as ts.ClassDeclaration).members[0] as ts.ConstructorDeclaration;
    let node: ts.ParameterDeclaration = construcotr.parameters[0];
    assert.strictEqual(!!node, true);
    const checker: ts.TypeChecker = TypeUtils.createChecker(ast);
    const className: ts.Identifier = construcotr.parent.name as ts.Identifier;
    assert.strictEqual(!!className, true);
    const sym: ts.Symbol = NodeUtils.findSymbolOfIdentifier(checker, className, nodeSymbolMap) as ts.Symbol;
    let mangledSymbolNames: Map<ts.Symbol, MangledSymbolInfo> = new Map<ts.Symbol, MangledSymbolInfo>();
    mangledSymbolNames.set(sym, { mangledName: 'a', originalNameWithScope: '' });
    const actual = shouldKeepParameter(node, config, mangledSymbolNames, checker);
    assert.strictEqual(actual, false);
  });

  it('Tester 2-13: When node is constructor and class name is kept, shouldKeepParameter returns true', function () {
    let content = `declare class A {
      constructor(para1: number);
      method(para2: number): void;
      }`;
    const ast = ts.createSourceFile(path, content, ts.ScriptTarget.ES2015, true);
    let construcotr: ts.ConstructorDeclaration = (ast.statements[0] as ts.ClassDeclaration).members[0] as ts.ConstructorDeclaration;
    const className: ts.Identifier = construcotr.parent.name as ts.Identifier;
    assert.strictEqual(!!className, true);
    let node: ts.ParameterDeclaration = construcotr.parameters[0];
    const checker: ts.TypeChecker = TypeUtils.createChecker(ast);
    const sym: ts.Symbol = NodeUtils.findSymbolOfIdentifier(checker, className, nodeSymbolMap) as ts.Symbol;
    let mangledSymbolNames: Map<ts.Symbol, MangledSymbolInfo> = new Map<ts.Symbol, MangledSymbolInfo>();
    mangledSymbolNames.set(sym, { mangledName: 'A', originalNameWithScope: '' });
    const actual = shouldKeepParameter(node, config, mangledSymbolNames, checker);
    assert.strictEqual(actual, true);
  });

  it('Tester 2-14: When node is MethodSignature, shouldKeepParameter returns false', function () {
    let content = 'export declare function foo(para1: number): void';
    const ast = ts.createSourceFile(path, content, ts.ScriptTarget.ES2015, true);
    const customIdentifier = ts.createIdentifier('a1');
    const customParaNode = ts.createParameter(undefined, undefined, customIdentifier);
    // @ts-ignore
    customIdentifier.parent = customParaNode;
    const customMethodSig = ts.createMethodSignature(undefined, [customParaNode], undefined, '', undefined);
    // @ts-ignore
    customParaNode.parent = customMethodSig;
    let mangledSymbolNames: Map<ts.Symbol, MangledSymbolInfo> = new Map<ts.Symbol, MangledSymbolInfo>();
    const checker: ts.TypeChecker = TypeUtils.createChecker(ast);
    const actual = shouldKeepParameter(customParaNode, config, mangledSymbolNames, checker);
    assert.strictEqual(actual, false);
  });

  it('Tester 2-15: When node is method and name is stringType, shouldKeepParameter returns true', function () {
    let content = `declare class A {
      constructor(para1: number);
      ['strProp'](para2: number): void;
      }`;
    const ast = ts.createSourceFile(path, content, ts.ScriptTarget.ES2015, true);
    let method: ts.MethodDeclaration = (ast.statements[0] as ts.ClassDeclaration).members[1] as ts.MethodDeclaration;
    let node: ts.ParameterDeclaration = method.parameters[0];
    const checker: ts.TypeChecker = TypeUtils.createChecker(ast);
    const methodName: ts.Identifier = method.name as ts.Identifier;
    assert.strictEqual(!!methodName, true);
    let mangledSymbolNames: Map<ts.Symbol, MangledSymbolInfo> = new Map<ts.Symbol, MangledSymbolInfo>();
    PropCollections.reservedProperties.add('strProp');
    const customConfig: INameObfuscationOption = JSON.parse(JSON.stringify(config));
    // @ts-ignore
    customConfig['mRenameProperties'] = true;
    const actual = shouldKeepParameter(node, customConfig, mangledSymbolNames, checker);
    assert.strictEqual(actual, true);
  });

  it('Tester 2-16: When node is method and the type is unexpected, shouldKeepParameter returns true', function () {
    let content = `declare class A {
      constructor(para1: number);
      ['strProp' + 'prop'](para2: number): void;
      }`;
    const ast = ts.createSourceFile(path, content, ts.ScriptTarget.ES2015, false);
    let method: ts.MethodDeclaration = (ast.statements[0] as ts.ClassDeclaration).members[1] as ts.MethodDeclaration;
    let node: ts.ParameterDeclaration = method.parameters[0];
    const checker: ts.TypeChecker = TypeUtils.createChecker(ast);
    const methodName: ts.Identifier = method.name as ts.Identifier;
    assert.strictEqual(!!methodName, true);
    let mangledSymbolNames: Map<ts.Symbol, MangledSymbolInfo> = new Map<ts.Symbol, MangledSymbolInfo>();
    PropCollections.reservedProperties.add('strProp');
    const customConfig: INameObfuscationOption = JSON.parse(JSON.stringify(config));
    // @ts-ignore
    customConfig['mRenameProperties'] = true;
    const actual = shouldKeepParameter(node, customConfig, mangledSymbolNames, checker);
    assert.strictEqual(actual, false);
  });

  it('Tester 2-17: When node is setAccessor and setAccessor name is not kept, shouldKeepParameter returns false', function () {
    let content = `declare class A {
      constructor(para1: number);
      set method11(para2: number): void;
      }`;
    const ast = ts.createSourceFile(path, content, ts.ScriptTarget.ES2015, true);
    let method: ts.MethodDeclaration = (ast.statements[0] as ts.ClassDeclaration).members[1] as ts.MethodDeclaration;
    let node: ts.ParameterDeclaration = method.parameters[0];
    const checker: ts.TypeChecker = TypeUtils.createChecker(ast);
    const methodName: ts.Identifier = method.name as ts.Identifier;
    assert.strictEqual(!!methodName, true);
    const customConfig: INameObfuscationOption = JSON.parse(JSON.stringify(config));
    // @ts-ignore
    customConfig['mRenameProperties'] = true;
    let mangledSymbolNames: Map<ts.Symbol, MangledSymbolInfo> = new Map<ts.Symbol, MangledSymbolInfo>();
    const actual = shouldKeepParameter(node, customConfig, mangledSymbolNames, checker);
    assert.strictEqual(actual, false);
  });

  it('Tester 2-18: When node is setAccessor and setAccessor name is kept, shouldKeepParameter returns true', function () {
    let content = `declare class A {
      constructor(para1: number);
      set method12(para2: number): void;
      }`;
    const ast = ts.createSourceFile(path, content, ts.ScriptTarget.ES2015, true);
    let method: ts.MethodDeclaration = (ast.statements[0] as ts.ClassDeclaration).members[1] as ts.MethodDeclaration;
    let node: ts.ParameterDeclaration = method.parameters[0];
    const checker: ts.TypeChecker = TypeUtils.createChecker(ast);
    const methodName: ts.Identifier = method.name as ts.Identifier;
    assert.strictEqual(!!methodName, true);
    let mangledSymbolNames: Map<ts.Symbol, MangledSymbolInfo> = new Map<ts.Symbol, MangledSymbolInfo>();
    PropCollections.reservedProperties.add('method12');
    const customConfig: INameObfuscationOption = JSON.parse(JSON.stringify(config));
    // @ts-ignore
    customConfig['mRenameProperties'] = true;
    const actual = shouldKeepParameter(node, customConfig, mangledSymbolNames, checker);
    assert.strictEqual(actual, true);
  });
});
