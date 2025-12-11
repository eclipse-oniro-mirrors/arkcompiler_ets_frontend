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

import {
  factory,
  isBlock,
  isCallExpression,
  isCaseClause,
  isDefaultClause,
  isElementAccessExpression,
  isExpressionStatement,
  isIdentifier,
  isModuleBlock,
  isPropertyAccessExpression,
  isSourceFile,
  setParentRecursive,
  visitEachChild,
  getOriginalNode
} from 'typescript';

import type {
  Block,
  CaseClause,
  DefaultClause,
  Expression,
  LeftHandSideExpression,
  ModuleBlock,
  Node,
  NodeArray,
  SourceFile,
  Statement,
  TransformationContext,
  Transformer,
  TransformerFactory
} from 'typescript';

import type {IOptions} from '../../configs/IOptions';
import type {TransformPlugin} from '../TransformPlugin';
import {TransformerOrder} from '../TransformPlugin';
import { NodeUtils } from '../../utils/NodeUtils';
import { ArkObfuscator, performancePrinter } from '../../ArkObfuscator';
import { EventList, endSingleFileEvent, startSingleFileEvent } from '../../utils/PrinterUtils';
import { isMatchWildcard } from '../../utils/TransformUtil';
import { MemoryDottingDefine } from '../../utils/MemoryDottingDefine';

namespace secharmony {
  export let transformerPlugin: TransformPlugin = {
    'name': 'disableConsolePlugin',
    'order': TransformerOrder.DISABLE_CONSOLE_TRANSFORMER,
    'createTransformerFactory': createDisableConsoleFactory
  };

  export function createDisableConsoleFactory(option: IOptions): TransformerFactory<Node> | null {
    if (!option.mDisableConsole &&
      !option.mRemoveNoSideEffectsCalls?.mRemovedCallNames?.length &&
      !option.mRemoveNoSideEffectsCalls?.mUniversalRemovedCallNames?.length) {
      return null;
    }

    const removedCallNamesForQuickCheck = !!option.mRemoveNoSideEffectsCalls?.mRemovedCallNames?.length ?
      new Set(option.mRemoveNoSideEffectsCalls.mRemovedCallNames) :
      new Set();

    return disableConsoleFactory;

    function disableConsoleFactory(context: TransformationContext): Transformer<Node> {
      return transformer;

      function transformer(node: Node): Node {
        if (!isSourceFile(node) || NodeUtils.isDeclarationFile(node)) {
          return node;
        }

        const recordInfo = ArkObfuscator.recordStage(MemoryDottingDefine.REMOVE_CONSOLE);
        startSingleFileEvent(EventList.REMOVE_CONSOLE, performancePrinter.timeSumPrinter);
        let resultAst: Node = visitAst(node);
        let parentNodes = setParentRecursive(resultAst, true);
        endSingleFileEvent(EventList.REMOVE_CONSOLE, performancePrinter.timeSumPrinter);
        ArkObfuscator.stopRecordStage(recordInfo);
        return parentNodes;
      }

      /**
       * delete console log print expression, only support simple format like:
       *  - console.xxx();
       *  - console['xxx']();
       * @param node
       */
      function visitAst(node: Node): Node {
        const visitedAst = visitEachChild(node, visitAst, context);

        if (!(isSourceFile(node) || isBlock(node) || isModuleBlock(node) || isCaseClause(node) || isDefaultClause(node))) {
          return visitedAst;
        }

        //@ts-ignore
        const deletedStatements: Statement[] = deleteConsoleStatement(visitedAst.statements);

        if (isSourceFile(node)) {
          return factory.updateSourceFile(node, deletedStatements);
        } else if (isBlock(node)) {
          return factory.createBlock(deletedStatements, true);
        } else if (isModuleBlock(node)) {
          return factory.createModuleBlock(deletedStatements);
        } else if (isCaseClause(node)) {
          return factory.createCaseClause(node.expression, deletedStatements);
        } else {
          return factory.createDefaultClause(deletedStatements);
        }
      }

      function deleteConsoleStatement(statements: NodeArray<Statement>): Statement[] {
        const reservedStatements: Statement[] = [];
        statements.forEach((child) => {
          if (!isStatementToRemove(child)) {
            reservedStatements.push(child);
          }
        });

        return reservedStatements;
      }

      function isStatementToRemove(statement: Statement): boolean {
        let node: Node = getOriginalNode(statement);

        if (!isExpressionStatement(node)) {
          return false;
        }

        if (!node.expression || !isCallExpression(node.expression)) {
          return false;
        }

        const expressionCalled: LeftHandSideExpression = node.expression.expression;
        if (!expressionCalled) {
          return false;
        }

        if (option.mDisableConsole && isSimpleConsoleExpression(expressionCalled)) {
          return true;
        }

        if ((!!option.mRemoveNoSideEffectsCalls?.mRemovedCallNames?.length ||
          !!option.mRemoveNoSideEffectsCalls?.mUniversalRemovedCallNames?.length) &&
          isMatchedNoSideEffectsCallsExpression(expressionCalled)) {
          return true;
        }

        return false;
      }

      function isSimpleConsoleExpression(callExpression: Expression): boolean {
        if (isPropertyAccessExpression(callExpression) && callExpression.expression) {
          if (isIdentifier(callExpression.expression) && callExpression.expression.text === 'console') {
            return true;
          }
        }

        if (isElementAccessExpression(callExpression) && callExpression.expression) {
          if (isIdentifier(callExpression.expression) && callExpression.expression.text === 'console') {
            return true;
          }
        }

        return false;
      }

      function isMatchedNoSideEffectsCallsExpression(callExpression: Expression): boolean {
        if (isPropertyOrElementAccessChain(callExpression)) {
          let accessExpressionText: string = callExpression.getText();
          return isInRemoveNoSideEffectsCalls(accessExpressionText);
        }

        return false;
      }

      function isPropertyOrElementAccessChain(accessExpression: Expression): boolean {
        let currentExpression = accessExpression;
        while (!isIdentifier(currentExpression)) {
          if ((isPropertyAccessExpression(currentExpression) || isElementAccessExpression(currentExpression)) && currentExpression.expression) {
            currentExpression = currentExpression.expression;
          } else {
            return false;
          }
        }
        return true;
      }

      function isInRemoveNoSideEffectsCalls(callName: string): boolean {
        return removedCallNamesForQuickCheck.has(callName) ||
          isMatchWildcard(option.mRemoveNoSideEffectsCalls.mUniversalRemovedCallNames, callName);
      }
    }
  }
}

export = secharmony;
