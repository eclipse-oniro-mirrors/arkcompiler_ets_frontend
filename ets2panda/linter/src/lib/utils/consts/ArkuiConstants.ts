/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

export const DOUBLE_DOLLAR_IDENTIFIER = '$$';
export const THIS_IDENTIFIER = 'this';
export const ATTRIBUTE_SUFFIX = 'Attribute';
export const INSTANCE_IDENTIFIER = 'instance';
export const COMMON_METHOD_IDENTIFIER = 'CommonMethod';
export const APPLY_STYLES_IDENTIFIER = 'applyStyles';
export const STATE_STYLES = 'stateStyles';
export const VALUE_IDENTIFIER = 'value';
export const INDENT_STEP = 2;
export const MAKE_OBSERVED = 'makeObserved';
export const NEW_PROP_DECORATOR_SUFFIX = 'Ref';

export enum CustomInterfaceName {
  Extend = 'Extend',
  LocalBuilder = 'LocalBuilder',
  Styles = 'Styles',
  AnimatableExtend = 'AnimatableExtend',
  Memo = 'Memo',
  Observed = 'Observed',
  CustomLayout = 'CustomLayout',
  CustomStyles = 'CustomStyles',
  Repeat = 'Repeat',
  WrappedBuilder = 'WrappedBuilder',
  wrapBuilder = 'wrapBuilder',
  BuilderNode = 'BuilderNode'
}

export enum StorageTypeName {
  LocalStorage = 'LocalStorage',
  AppStorage = 'AppStorage',
  PersistentStorage = 'PersistentStorage',
  PersistenceV2 = 'PersistenceV2'
}

export enum PropDecoratorName {
  Prop = 'Prop',
  StorageProp = 'StorageProp',
  LocalStorageProp = 'LocalStorageProp'
}

export enum PropFunctionName {
  Prop = 'prop',
  SetAndProp = 'setAndProp'
}

export enum BuilderNodeFunctionName {
  Build = 'build',
  Update = 'update'
}

export const observedDecoratorName: Set<string> = new Set([
  'State',
  'Prop',
  'Link',
  'Provide',
  'Consume',
  'LocalStorageProp',
  'LocalStorageLink',
  'StorageProp',
  'StorageLink',
  'Track'
]);

export const skipImportDecoratorName: Set<string> = new Set([
  'Extend',
  'Styles',
  'Sendable',
  'Concurrent',
  'LocalBuilder',
  'Prop',
  'StorageProp',
  'LocalStorageProp'
]);

export const serializationTypeFlags: Set<ts.TypeFlags> = new Set([
  ts.TypeFlags.String,
  ts.TypeFlags.Number,
  ts.TypeFlags.Boolean,
  ts.TypeFlags.StringLiteral,
  ts.TypeFlags.NumberLiteral,
  ts.TypeFlags.BooleanLiteral,
  ts.TypeFlags.Undefined,
  ts.TypeFlags.Null
]);

export const serializationTypeName: Set<string> = new Set([
  'Date',
  'number',
  'boolean',
  'string',
  'undefined',
  'null',
  'Date[]',
  'number[]',
  'boolean[]',
  'string[]',
  'undefined[]',
  'null[]',
  'Set<Date>',
  'Set<number>',
  'Set<boolean>',
  'Set<string>',
  'Set<undefined>',
  'Set<null>',
  'Map<Date, Date>',
  'Map<Date, number>',
  'Map<Date, boolean>',
  'Map<Date, string>',
  'Map<Date, undefined>',
  'Map<Date, null>',
  'Map<number, Date>',
  'Map<number, number>',
  'Map<number, boolean>',
  'Map<number, string>',
  'Map<number, undefined>',
  'Map<number, null>',
  'Map<boolean, Date>',
  'Map<boolean, number>',
  'Map<boolean, boolean>',
  'Map<boolean, string>',
  'Map<boolean, undefined>',
  'Map<boolean, null>',
  'Map<string, Date>',
  'Map<string, number>',
  'Map<string, boolean>',
  'Map<string, string>',
  'Map<string, undefined>',
  'Map<string, null>',
  'Map<undefined, Date>',
  'Map<undefined, number>',
  'Map<undefined, boolean>',
  'Map<undefined, string>',
  'Map<undefined, undefined>',
  'Map<undefined, null>',
  'Map<null, Date>',
  'Map<null, number>',
  'Map<null, boolean>',
  'Map<null, string>',
  'Map<null, undefined>',
  'Map<null, null>'
]);

export const customLayoutFunctionName: Set<string> = new Set(['onMeasureSize', 'onPlaceChildren']);

export const ENTRY_DECORATOR_NAME = 'Entry';
export const ENTRY_STORAGE_PROPERITY = 'storage';
export const LOCAL_STORAGE_TYPE_NAME = 'LocalStorage';
export const GET_LOCAL_STORAGE_FUNC_NAME = '__get_local_storage__';

export const PROVIDE_DECORATOR_NAME = 'Provide';
export const PROVIDE_ALIAS_PROPERTY_NAME = 'alias';
export const PROVIDE_ALLOW_OVERRIDE_PROPERTY_NAME = 'allowOverride';

export const VIRTUAL_SCROLL_IDENTIFIER = 'virtualScroll';
export const DISABLE_VIRTUAL_SCROLL_IDENTIFIER = 'disableVirtualScroll';

export const ARKUI_MODULE = '@kit.ArkUI';
export const STATE_MANAGEMENT_MODULE = '@ohos.arkui.StateManagement';

export const BUILDERNODE_D_TS = 'BuilderNode.d.ts';
export const NESTING_BUILDER_SUPPORTED = 'nestingBuilderSupported';

export const COMMON_TS_ETS_API_D_TS = 'common_ts_ets_api.d.ts';
export const UI_STATE_MANAGEMENT_D_TS = '@ohos.arkui.StateManagement.d.ts';
export const PERSIST_PROP_FUNC_NAME = 'persistProp';
export const PERSIST_PROPS_FUNC_NAME = 'persistProps';
export const GLOBAL_CONNECT_FUNC_NAME = 'globalConnect';
export const CONNECT_FUNC_NAME = 'connect';

export const USE_STATIC_STATEMENT = 'use static';
