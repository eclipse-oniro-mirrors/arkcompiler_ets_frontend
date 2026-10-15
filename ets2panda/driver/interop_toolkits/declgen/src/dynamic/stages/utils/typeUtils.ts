/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import * as ts from 'typescript';
import { Extension } from '../../utils/extension';
import * as path from 'node:path';

export const JSVALUE: string = 'Any';
export const ESOBJECT: string = 'ESObject';
export const ARRAY_TYPE: string = 'Array';
export const STRING: string = 'string';
export const ETS_KEYWORDS: ReadonlySet<string> = new Set(['Class', 'ESObject', 'MethodType']);

const INCOMPATIBLE_BUILTINS = new Set([
  'Object',
  'Function',
  'Resource',
  'Symbol',
  'SharedArrayBuffer',
  'PropertyDecorator',
  'Transferable',
  'InstanceType',
  'ESObject',
]);

const INCOMPATIBLE_SDKS = new Set<string>([
  '@ohos.ability.wantConstant',
  '@ohos.annotation',
  '@ohos.animator',
  '@ohos.app.ability.InsightIntentDecorator',
  '@ohos.app.ability.common',
  '@ohos.app.ability.errorManager',
  '@ohos.app.ability.insightIntentProvider',
  '@ohos.application.testRunner',
  '@ohos.arkui.ArcAlphabetIndexer',
  '@ohos.arkui.ArcList',
  '@ohos.arkui.ArcScrollBar',
  '@ohos.arkui.ArcSwiper',
  '@ohos.arkui.Prefetcher',
  '@ohos.arkui.StateManagement',
  '@ohos.arkui.UIContext',
  '@ohos.arkui.WithEnv',
  '@ohos.arkui.advanced.ArcButton',
  '@ohos.arkui.advanced.Chip',
  '@ohos.arkui.advanced.ChipGroup',
  '@ohos.arkui.advanced.ChipGroupV2',
  '@ohos.arkui.advanced.ChipV2',
  '@ohos.arkui.advanced.ComposeListItem',
  '@ohos.arkui.advanced.ComposeListItemV2',
  '@ohos.arkui.advanced.ComposeTitleBar',
  '@ohos.arkui.advanced.ComposeTitleBarV2',
  '@ohos.arkui.advanced.Counter',
  '@ohos.arkui.advanced.CounterV2',
  '@ohos.arkui.advanced.DatePickerComponent',
  '@ohos.arkui.advanced.Dialog',
  '@ohos.arkui.advanced.DialogV2',
  '@ohos.arkui.advanced.DownloadFileButton',
  '@ohos.arkui.advanced.EditableTitleBar',
  '@ohos.arkui.advanced.EditableTitleBarV2',
  '@ohos.arkui.advanced.ExceptionPrompt',
  '@ohos.arkui.advanced.ExceptionPromptV2',
  '@ohos.arkui.advanced.Filter',
  '@ohos.arkui.advanced.FoldSplitContainer',
  '@ohos.arkui.advanced.FormMenu',
  '@ohos.arkui.advanced.GridObjectSortComponent',
  '@ohos.arkui.advanced.MultiNavigation',
  '@ohos.arkui.advanced.Popup',
  '@ohos.arkui.advanced.PopupV2',
  '@ohos.arkui.advanced.ProgressButton',
  '@ohos.arkui.advanced.ProgressButtonV2',
  '@ohos.arkui.advanced.SegmentButton',
  '@ohos.arkui.advanced.SegmentButtonV2',
  '@ohos.arkui.advanced.SelectTitleBar',
  '@ohos.arkui.advanced.SelectionMenu',
  '@ohos.arkui.advanced.SplitLayout',
  '@ohos.arkui.advanced.SubHeader',
  '@ohos.arkui.advanced.SubHeaderV2',
  '@ohos.arkui.advanced.SwipeRefresher',
  '@ohos.arkui.advanced.SwipeRefresherV2',
  '@ohos.arkui.advanced.TabTitleBar',
  '@ohos.arkui.advanced.ToolBar',
  '@ohos.arkui.advanced.ToolBarV2',
  '@ohos.arkui.advanced.TreeView',
  '@ohos.arkui.advanced.TreeViewV2',
  '@ohos.arkui.componentSnapshot',
  '@ohos.arkui.components.ArkLazyColumnLayout',
  '@ohos.arkui.components.ArkLazyWaterFlowLayout',
  '@ohos.arkui.components.ContainerReader',
  '@ohos.arkui.components.SelectionContainer',
  '@ohos.arkui.dialog',
  '@ohos.arkui.dragController',
  '@ohos.arkui.drawableDescriptor',
  '@ohos.arkui.intelligence.imageGeneration',
  '@ohos.arkui.lazyLayoutAlgorithm',
  '@ohos.arkui.layoutAlgorithm',
  '@ohos.arkui.modifier',
  '@ohos.arkui.node',
  '@ohos.arkui.observer',
  '@ohos.arkui.shape',
  '@ohos.arkui.theme',
  '@ohos.arkui.uiMaterial',
  '@ohos.atomicservice.AtomicServiceNavigation',
  '@ohos.atomicservice.AtomicServiceSearch',
  '@ohos.atomicservice.AtomicServiceTabs',
  '@ohos.atomicservice.AtomicServiceWeb',
  '@ohos.atomicservice.InterstitialDialogAction',
  '@ohos.atomicservice.NavPushPathHelper',
  '@ohos.base',
  '@ohos.buffer',
  '@ohos.bundle',
  '@ohos.continuation.continuationManager',
  '@ohos.data.UdmfComponents',
  '@ohos.data.unifiedDataChannel',
  '@ohos.document',
  '@ohos.distributedMissionManager',
  '@ohos.effectKit',
  '@ohos.fastbuffer',
  '@ohos.file.backup',
  '@ohos.file.fs',
  '@ohos.fileio',
  '@ohos.graphics.displaySync',
  '@ohos.graphics.drawing',
  '@ohos.graphics.scene',
  '@ohos.graphics.uiEffect',
  '@ohos.hiviewdfx.jsLeakWatcher',
  '@ohos.i18n',
  '@ohos.inputMethodList',
  '@ohos.measure',
  '@ohos.multimedia.audio',
  '@ohos.multimedia.avCastPicker',
  '@ohos.multimedia.avInputCastPicker',
  '@ohos.multimedia.avVolumePanel',
  '@ohos.nearlink.remoteDevice',
  '@ohos.pluginComponent',
  '@ohos.promptAction',
  '@ohos.security.asset',
  '@ohos.security.cert',
  '@ohos.security.huks',
  '@ohos.taskpool',
  '@ohos.userIAM.userAuth',
  '@ohos.userIAM.userAuthIcon',
  '@ohos.util.json',
  '@ohos.window',
  '@ohos.window.floatView',
  '@ohos.worker',
  '@ohos.app.ability.EmbeddableUIAbility',
  '@ohos.app.ability.UIAbility',
  '@ohos.app.ability.appRecovery',
  '@ohos.app.ability.kioskManager',
  '@ohos.app.form.formProvider',
  '@ohos.net.vpn',
  '@ohos.notificationExtensionSubscription',
  '@ohos.notificationManager',
  '@ohos.security.CryptoExtensionAbility',
  '@ohos.file.photoAccessHelper',
  '@ohos.file.PhotoPickerComponent',
  '@ohos.file.RecentPhotoComponent',
  '@ohos.file.picker',
  '@ohos.file.sendablePhotoAccessHelper',
  '@ohos.file.AlbumPickerComponent', 
  '@ohos.atomicservice.HalfScreenLaunchComponent',
  '@ohos.application.appManager',
  '@ohos.app.ability.UIExtensionContentSession',
  '@ohos.app.ability.autoFillManager',
  '@ohos.advertising.AdComponent',
  '@ohos.advertising.AutoAdComponent',
  '@ohos.web.webview',
  '@ohos.app.form.LiveFormExtensionAbility',
  '@ohos.data.sendableRelationalStore',
  '@ohos.data.sendablePreferences',
  '@ohos.graphics.sendableColorSpaceManager',
  '@ohos.sendableResourceManager',
  '@ohos.multimedia.sendableImage',
  '@ohos.print',
  '@ohos.enterprise.common',
  '@ohos.resourceManage',
  '@ohos.advertising.AdsServiceExtensionAbility',
  '@ohos.advertising',
  '@ohos.FusionConnectivity.PartnerAgentExtensionAbility',
  '@ohos.settings',
  '@ohos.contact',
  '@ohos.pasteboard',  
  '@ohos.application.AccessibilityExtensionAbility',
  '@ohos.ability.featureAbility',
  '@ohos.app.ability.appManager',
  '@ohos.app.form.FormEditExtensionAbility',
  '@ohos.arkui.advanced.InnerFullScreenLaunchComponent',
  '@ohos.arkui.advanced.FullScreenLaunchComponent',
  '@ohos.app.ability.sendableContextManager',
  '@ohos.inputMethodEngine',
  '@ohos.arkui.components.ArkDynamicLayout',
  '@ohos.arkui.components.ArkLazyDynamicLayout',
  '@ohos.PiPWindow',
  '@ohos.multimedia.movingphotoview',
  '@ohos.file.photoAccessHelper',
  '@ohos.font',
  '@ohos.net.socket',
  '@ohos.net.vpnExtension',
  '@arkts.lang',
  '@arkts.collections',
  '@arkts.utils',
  '@ohos.enterprise.systemManager'
]);

const STA_RESERVED_KEYWORDS = new Set<string>([
  'abstract',
  'enum',
  'let',
  'this',
  'as',
  'export',
  'native',
  'throw',
  'async',
  'extends',
  'new',
  'true',
  'await',
  'false',
  'null',
  'try',
  'break',
  'final',
  'overload',
  'typeof',
  'case',
  'for',
  'override',
  'undefined',
  'class',
  'function',
  'private',
  'while',
  'const',
  'if',
  'protected',
  'constructor',
  'implements',
  'public',
  'continue',
  'import',
  'return',
  'default',
  'in',
  'static',
  'do',
  'instanceof',
  'switch',
  'else',
  'interface',
  'super',
]);

const STA_HARD_KEYWORDS = new Set<string>([
  'Any',
  'bigint',
  'BigInt',
  'boolean',
  'Boolean',
  'byte',
  'Byte',
  'char',
  'Char',
  'double',
  'Double',
  'float',
  'Float',
  'int',
  'Int',
  'long',
  'Long',
  'number',
  'Number',
  'Object',
  'object',
  'short',
  'Short',
  'string',
  'String',
  'void',
]);

export const FINAL_CLASS: string[] = [
  'ArgumentOutOfRangeError',
  'ArrayIndexOutOfBoundsError',
  'ArrayStoreError',
  'ArrayType',
  'ArrayValue',
  'ArithmeticError',
  'AtomicFlag',
  'Boolean',
  'BooleanType',
  'BooleanValue',
  'Byte',
  'ByteType',
  'ByteValue',
  'CallBodyDefault',
  'CallBodyFunction',
  'CallBodyMethod',
  'CallBodyErasedFunction',
  'Char',
  'CharType',
  'CharValue',
  'Class',
  'ClassCastError',
  'ClassType',
  'ClassValue',
  'Chrono',
  'Console',
  'Coroutine',
  'CoroutineExtras',
  'DebuggerAPI',
  'Double',
  'DoubleType',
  'DoubleValue',
  'EnumConstant',
  'EnumType',
  'Field',
  'FieldCreator',
  'FinalizableWeakRef',
  'FinalizationRegistry',
  'Float',
  'FloatType',
  'FloatValue',
  'GC',
  'IllegalArgumentError',
  'InterfaceType',
  'Int',
  'IntIntValue',
  'IntType',
  'JSError',
  'JSRuntime',
  'JSValue',
  'LambdaType',
  'LambdaValue',
  'Logger',
  'Long',
  'LongType',
  'LongValue',
  'Method',
  'MethodCreator',
  'MethodType',
  'NegativeArraySizeError',
  'NoDataError',
  'NullType',
  'NullValue',
  'Parameter',
  'ParameterCreator',
  'Promise',
  'Runtime',
  'RuntimeError',
  'Short',
  'ShortType',
  'ShortValue',
  'String',
  'StringBuilder',
  'StringIndexOutOfBoundsError',
  'StringType',
  'StringValue',
  'TupleType',
  'Types',
  'UnionCase',
  'UnionType',
  'UnsupportedOperationError',
  'UndefinedType',
  'UndefinedValue',
  'Void',
  'VoidType',
  'VoidValue',
  'WeakRef',
  'Error',
];

export const UTILITY_TYPES = new Map<string, string>([
  ['Capitalize', STRING],
  ['Uncapitalize', STRING],
  ['Uppercase', STRING],
  ['Lowercase', STRING],
  ['ConstructorParameters', ARRAY_TYPE],
  ['Exclude', JSVALUE],
  ['Extract', JSVALUE],
  ['InstanceType', JSVALUE],
  ['NoInfer', JSVALUE],
  ['Omit', JSVALUE],
  ['OmitThisParameter', JSVALUE],
  ['Parameters', ARRAY_TYPE],
  ['Pick', JSVALUE],
  ['ThisParameterType', JSVALUE],
  ['ThisType', JSVALUE],
  ['ReturnType', JSVALUE],
  ['NonNullable', JSVALUE],
]);

export enum INTEROP_NAMESPACE {
  Dynamic = 'es',
  Static = 'st',
}
export enum INTEROP_TYPE {
  Array = 'Array',
  Map = 'Map',
  Set = 'Set',
  STValue = 'STValue',
}
export const INTEROP_SDK_NAME = '@ohos.lang.interop';

export function isReservedKeyword(word: string): boolean {
  return STA_RESERVED_KEYWORDS.has(word);
}

export function isHardKeyword(word: string): boolean {
  return STA_HARD_KEYWORDS.has(word);
}

export function isHardOrReservedKeyword(word: string): boolean {
  return isReservedKeyword(word) || isHardKeyword(word);
}

export const LIMIT_DECORATOR: string[] = ['Sendable', 'Concurrent'];

export function isUtilityType(node: ts.TypeReferenceNode): boolean {
  return ts.isIdentifier(node.typeName) && UTILITY_TYPES.has(node.typeName.text);
}

export function createJSValueTypeNode(context: ts.TransformationContext): ts.TypeReferenceNode {
  const identifier = context.factory.createIdentifier(JSVALUE);
  return context.factory.createTypeReferenceNode(identifier, undefined);
}

export function createJSValueArrayTypeNode(context: ts.TransformationContext): ts.ArrayTypeNode {
  const jsValueTypeNode = createJSValueTypeNode(context);
  return context.factory.createArrayTypeNode(jsValueTypeNode);
}

export function createJSValueArrayTypeReferenceNode(context: ts.TransformationContext): ts.TypeReferenceNode {
  const jsValueTypeNode = createJSValueTypeNode(context);
  const arrayIdentifier = context.factory.createIdentifier(ARRAY_TYPE);
  return context.factory.createTypeReferenceNode(arrayIdentifier, [jsValueTypeNode]);
}

export function cloneTypeParameters(
  context: ts.TransformationContext,
  typeParameters: ts.NodeArray<ts.TypeParameterDeclaration> | undefined
): ts.NodeArray<ts.TypeParameterDeclaration> | undefined {
  if (typeParameters === undefined) {
    return undefined;
  }
  const cloned = typeParameters.map(
    (tp): ts.TypeParameterDeclaration =>
      context.factory.createTypeParameterDeclaration(undefined, tp.name, tp.constraint, tp.default)
  );
  return context.factory.createNodeArray(cloned);
}

export function isIncompatibleBuiltinType(type: ts.TypeReferenceNode): boolean {
  const typeName = type.typeName;
  if (typeName !== undefined && ts.isIdentifier(typeName)) {
    return INCOMPATIBLE_BUILTINS.has(typeName.text);
  }
  return false;
}

export function isIncompatibleSDK(sdkName: string): boolean {
  return INCOMPATIBLE_SDKS.has(sdkName);
}

export function getBaseNameFromFilePath(filePath: string): string {
  if (filePath.endsWith(Extension.DTS)) {
    return path.basename(filePath, Extension.DTS);
  } else if (filePath.endsWith(Extension.DETS)) {
    return path.basename(filePath, Extension.DETS);
  }
  return path.basename(filePath);
}

export function isNodeParentRestParameter(node: ts.Node): boolean {
  if (ts.isTypeReferenceNode(node) || ts.isArrayTypeNode(node) || ts.isTupleTypeNode(node)) {
    // Stages rebuild ancestors via visitEachChild, leaving `node.parent` unset on synthetic nodes.
    // Walk back to the original parsed node, whose `.parent` is populated by the binder.
    const parent = ts.getOriginalNode(node).parent;
    if (parent && ts.isParameter(parent)) {
      return !!parent.dotDotDotToken;
    }
  }
  return false;
}
