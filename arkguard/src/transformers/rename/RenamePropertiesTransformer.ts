/**
 * Copyright (c) 2023-2026 Huawei Device Co., Ltd.
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
  isComputedPropertyName,
  isConstructorDeclaration,
  isElementAccessExpression,
  isIdentifier,
  isNumericLiteral,
  isPrivateIdentifier,
  isStringLiteralLike,
  setParentRecursive,
  visitEachChild,
  isSourceFile,
  isIndexedAccessTypeNode,
  isLiteralTypeNode,
  isUnionTypeNode,
  canHaveIllegalDecorators,
  getIllegalDecorators,
  getAnnotationsFromIllegalDecorators,
} from 'typescript';

import type {
  ComputedPropertyName,
  Expression,
  Identifier,
  Node,
  TransformationContext,
  Transformer,
  TransformerFactory,
  ClassDeclaration,
  ClassExpression,
  StructDeclaration,
  PropertyName,
  StringLiteral,
  LiteralTypeNode,
  TypeNode
} from 'typescript';

import { annotationPrefix } from '../../common/type';
import type {IOptions} from '../../configs/IOptions';
import type { INameObfuscationOption } from '../../configs/INameObfuscationOption';
import type {TransformPlugin} from '../TransformPlugin';
import {TransformerOrder} from '../TransformPlugin';
import {NodeUtils} from '../../utils/NodeUtils';
import { ArkObfuscator, performancePrinter } from '../../ArkObfuscator';
import { endSingleFileEvent, startSingleFileEvent } from '../../utils/PrinterUtils';
import { EventList } from '../../utils/PrinterTimeAndMemUtils';
import {
  isInPropertyWhitelist,
  isReservedProperty,
  needToRecordProperty
} from '../../utils/TransformUtil';
import {
  classInfoInMemberMethodCache,
  ensureGlobalGenerator,
  globalGenerator,
  nameCache
} from './RenameIdentifierTransformer';
import { FileUtils } from '../../ArkObfuscator';
import { UpdateMemberMethodName } from '../../utils/NameCacheUtil';
import { PropCollections, UnobfuscationCollections } from '../../utils/CommonCollections';
import { MemoryDottingDefine } from '../../utils/MemoryDottingDefine';

namespace secharmony {
  /**
   * Rename Properties Transformer
   *
   * @param option obfuscation options
   */
  const createRenamePropertiesFactory = function (option: IOptions): TransformerFactory<Node> | null {
    let profile: INameObfuscationOption | undefined = option?.mNameObfuscation;
    let shouldPrintKeptNames: boolean = !!(option.mUnobfuscationOption?.mPrintKeptNames);

    if (!profile || !profile.mEnable || !profile.mRenameProperties) {
      return null;
    }

    // Property mangling uses the same name generator as identifier obfuscation. If only this
    // transformer runs (e.g. unit tests), RenameIdentifierTransformer never created globalGenerator.
    ensureGlobalGenerator(profile.mNameGeneratorType, {});

    return renamePropertiesFactory;

    function renamePropertiesFactory(context: TransformationContext): Transformer<Node> {
      let currentFileType: string | undefined = undefined;
      return renamePropertiesTransformer;

      function renamePropertiesTransformer(node: Node): Node {
        if (isSourceFile(node) && ArkObfuscator.isKeptCurrentFile) {
          return node;
        }
        currentFileType = isSourceFile(node) ? FileUtils.getFileSuffix(node.fileName).ext : undefined;
        const recordInfo = ArkObfuscator.recordStage(MemoryDottingDefine.PROPERTY_OBFUSCATION);
        startSingleFileEvent(EventList.PROPERTY_OBFUSCATION, performancePrinter.timeSumPrinter);

        startSingleFileEvent(EventList.RENAME_PROPERTIES);
        let ret: Node = renameProperties(node);
        endSingleFileEvent(EventList.RENAME_PROPERTIES);

        startSingleFileEvent(EventList.UPDATE_MEMBER_METHOD_NAME);
        UpdateMemberMethodName(nameCache, PropCollections.globalMangledTable, classInfoInMemberMethodCache);
        endSingleFileEvent(EventList.UPDATE_MEMBER_METHOD_NAME);

        startSingleFileEvent(EventList.SET_PARENT_RECURSIVE);
        let parentNodes = setParentRecursive(ret, true);
        endSingleFileEvent(EventList.SET_PARENT_RECURSIVE);

        endSingleFileEvent(EventList.PROPERTY_OBFUSCATION, performancePrinter.timeSumPrinter);
        ArkObfuscator.stopRecordStage(recordInfo);
        return parentNodes;
      }

      function renameProperties(node: Node): Node {
        if (NodeUtils.isObjectLiteralInAnnotation(node, currentFileType)) {
          return node;
        }

        if (canHaveIllegalDecorators(node)) {
          const illegalDecorators = getIllegalDecorators(node);
          const annotationDecorators = getAnnotationsFromIllegalDecorators(node);
          if (!illegalDecorators?.length && !annotationDecorators?.length) {
            return visitEachChild(node, renameProperties, context);
          }
          const reservedDecorators = factory.createNodeArray([
            ...(illegalDecorators ?? []),
            ...(annotationDecorators ?? []),
          ]);
          let updated = visitEachChild(node, renameProperties, context);
          const visitedReservedDecorators = factory.createNodeArray(
            reservedDecorators.map(decorator => visitEachChild(decorator, renameProperties, context))
          );
          return factory.updateIllegalDecorators(updated, visitedReservedDecorators);
        }

        if (!NodeUtils.isPropertyNode(node)) {
          return visitEachChild(node, renameProperties, context);
        }

        if (isElementAccessExpression(node.parent)) {
          return renameElementAccessProperty(node);
        }

        if (isIndexedAccessTypeNode(node.parent)) {
          return renameIndexedAccessProperty(node);
        }

        if (isComputedPropertyName(node)) {
          return renameComputedProperty(node);
        }

        return renameProperty(node, false);
      }

      function renameElementAccessProperty(node: Node): Node {
        if (isStringLiteralLike(node)) {
          return renameProperty(node, false);
        }
        return visitEachChild(node, renameProperties, context);
      }

      function renameIndexedAccessProperty(node: Node): Node {
        if (NodeUtils.isStringLiteralTypeNode(node)) {
          let prop = renameProperty((node as LiteralTypeNode).literal, false);
          if (prop !== (node as LiteralTypeNode).literal) {
            return factory.createLiteralTypeNode(prop as StringLiteral);
          }
          return visitEachChild(node, renameProperties, context);
        }

        if (!isUnionTypeNode(node)) {
          return visitEachChild(node, renameProperties, context);
        }

        let isChanged: boolean = false;
        const elemTypes = node.types.map((elemType) => {
          if (!elemType || !NodeUtils.isStringLiteralTypeNode(elemType)) {
            return elemType;
          }
          const prop = renameProperty((elemType as LiteralTypeNode).literal, false);
          if (prop !== (elemType as LiteralTypeNode).literal) {
            isChanged = true;
            return factory.createLiteralTypeNode(prop as StringLiteral);
          }
          return elemType;
        });
        if (isChanged) {
          return factory.createUnionTypeNode(elemTypes);
        }
        return visitEachChild(node, renameProperties, context);
      }

      function renameComputedProperty(node: ComputedPropertyName): ComputedPropertyName {
        if (isStringLiteralLike(node.expression) || isNumericLiteral(node.expression)) {
          let prop: Node = renameProperty(node.expression, true);
          if (prop !== node.expression) {
            return factory.createComputedPropertyName(prop as Expression);
          }
        }

        if (isIdentifier(node.expression)) {
          return node;
        }

        return visitEachChild(node, renameProperties, context);
      }

      function renameProperty(node: Node, computeName: boolean): Node {
        if (!NodeUtils.isPropertyNameType(node)) {
          return visitEachChild(node, renameProperties, context);
        }

        if (isStringLiteralLike(node) && profile?.mKeepStringProperty) {
          if (shouldPrintKeptNames) {
            needToRecordProperty(node.text, UnobfuscationCollections.unobfuscatedPropMap);
          }
          return node;
        }

        let original: string = node.text;
        if (isInPropertyWhitelist(original, UnobfuscationCollections.unobfuscatedPropMap, shouldPrintKeptNames)) {
          return node;
        }

        let mangledName: string = original;
        let name: string | undefined = original.startsWith(annotationPrefix) ? original.substring(annotationPrefix.length) : undefined;
        if (name) {
          mangledName = getAnnotationMangledNameWithPrefix(name);
        } else {
          mangledName = getPropertyMangledName(original);
        }

        if (isStringLiteralLike(node)) {
          return factory.createStringLiteral(mangledName);
        }

        /**
         * source demo:
         * class A {
         *   123 = 1; // it is NumericLiteral
         *   [456] = 2; // it is NumericLiteral within ComputedPropertyName
         * }
         * obfuscation result:
         * class A {
         *   a = 1;
         *   ['b'] = 2;
         * }
         */
        if (isNumericLiteral(node)) {
          return computeName ? factory.createStringLiteral(mangledName) : factory.createIdentifier(mangledName);
        }

        if (isIdentifier(node) || isNumericLiteral(node)) {
          return factory.createIdentifier(mangledName);
        }

        return factory.createPrivateIdentifier('#' + mangledName);
      }

      function getPropertyMangledName(original: string): string {
        let mangledName: string = getMangledName(original);
        return mangledName;
      }

      function getAnnotationMangledNameWithPrefix(original: string): string {
        let mangledName: string = `${annotationPrefix}${getMangledName(original)}`;
        return mangledName;
      }

      function getMangledName(original: string): string {
        const historyName: string = PropCollections.historyMangledTable?.get(original);
        let mangledName: string = historyName ? historyName : PropCollections.globalMangledTable.get(original);
        while (!mangledName) {
          let tmpName = globalGenerator.getName();
          if (isReservedProperty(tmpName) ||
            tmpName === original) {
            continue;
          }

          // For incremental compilation, preventing generated names from conflicting with existing global name.
          if (PropCollections.globalMangledNamesInCache.has(tmpName)) {
            continue;
          }

          mangledName = tmpName;
        }
        PropCollections.globalMangledTable.set(original, mangledName);
        return mangledName;
      }
    }
  };

  export let transformerPlugin: TransformPlugin = {
    'name': 'renamePropertiesPlugin',
    'order': TransformerOrder.RENAME_PROPERTIES_TRANSFORMER,
    'createTransformerFactory': createRenamePropertiesFactory
  };
}

export = secharmony;
