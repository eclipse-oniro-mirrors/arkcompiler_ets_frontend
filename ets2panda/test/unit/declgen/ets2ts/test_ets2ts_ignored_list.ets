/**
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
'use static'
import preferences from '@ohos.data.preferences'
import { ValueType } from '@ohos.data.preferences'
import promptAction from '@ohos.promptAction'
import { LevelOrder } from '@ohos.promptAction'
import { Scene } from 'graphics3d.Scene'
import { A1 } from '../ets/expected/export_directives_1.txt'

@Component
export declare class C {
    @Require(LevelOrder)
    @State field: string;
}

export declare interface SceneService {
    getScene3d(): Scene | undefined | null;
}
// === Variable types (qualified) → ESObject ===
export let pref: preferences.Preferences = null!
export let val: preferences.ValueType = 42
export let opts: preferences.Options = null!
export let stor: preferences.StorageType = 0

// === Variable type (simple, no .X) → ESObject ===
export let prefSimple: preferences = null!

// === Function param/return → ESObject ===
export function getPref(key: string, defVal: preferences.ValueType): preferences.Preferences { return null! }
export function getOpts(): preferences.Options { return null! }

// === Rest parameter → Array<ESObject> ===
export function multiGet(...vals: preferences.ValueType[]): void {}

// === Class implements → ESObject ===
export class PrefStore implements preferences.Preferences {
  put(key: string, val: preferences.ValueType): void {}
  get(key: string, defVal: preferences.ValueType): preferences.ValueType { return null! }
  has(key: string): boolean { return false }
  delete(key: string): void {}
  clear(): void {}
  flush(): void {}
  getAll(): Object { return null! }
}

// === Interface extends → ESObject ===
export interface PrefOpts extends preferences.Options {
  extra: string;
}

// === Class extends enum → drop extends clause ===
export class StorClass extends preferences.StorageType {}

// === Class extends class (non-enum) → drop extends clause ===
export class MyLevelOrder extends promptAction.LevelOrder {}

// === Optional parameter → ESObject with ? ===
export function f4(opts?: preferences.Options): void {}

// === Public class property → ESObject ===
export class WithField {
  field: preferences.Preferences = null!
}

// === Interface method using SDK types → ESObject ===
export interface IMethod {
  foo(opts: preferences.Options): preferences.Preferences
  bar(): promptAction.Button
}

// === Re-export with alias from SDK → filtered ===
export { ValueType as VT } from '@ohos.data.preferences'

// === Union → ESObject | string ===
export let mixed: preferences.ValueType | string = ""

// === Type alias → ESObject ===
export type PrefValue = preferences.ValueType

// === Generic type argument → Promise<ESObject> ===
export let prom: Promise<preferences.Preferences> = null!

// === Array element → st.Array<ESObject> ===
export let arr: preferences.Preferences[] = []

// === Map value → st.Map<string, ESObject> ===
export let mp: Map<string, preferences.ValueType> = new Map()

// === Tuple element → [ESObject, string] ===
export let tup: [preferences.Preferences, string] = [null!, ""]

// === Function type → (x: ESObject) => void ===
export let fn: (x: preferences.Preferences) => void = () => {}

// === Type parameter constraint → <T extends ESObject> ===
export function f3<T extends preferences.Preferences>(t: T): T { return t }

// === Getter/Setter → get/set p(): ESObject ===
export class Accessor {
  private _p: preferences.Preferences = null!
  get p(): preferences.Preferences { return this._p }
  set p(v: preferences.Preferences) { this._p = v }
}

// === Nested generic → Promise<st.Map<string, ESObject>> ===
export let nested: Promise<Map<string, preferences.ValueType>> = null!

// === Nested array → st.Array<st.Array<ESObject>> ===
export let nestedArr: preferences.Preferences[][] = []

// === Multi SDK union → ESObject | ESObject ===
export let multiUnion: preferences.Preferences | preferences.ValueType = null!

// === All SDK tuple → [ESObject, ESObject] ===
export let allSdkTup: [preferences.Preferences, preferences.ValueType] = [null!, null!]

// === All SDK function type → (x: ESObject) => ESObject ===
export let sdkFn: (x: preferences.Preferences) => preferences.ValueType = () => null!

// === Export suppression ===
export { preferences }
export { preferences as prefs }
export default preferences
export { ValueType } from '@ohos.data.preferences'
export * from '@ohos.data.preferences'

// === Control: local types (NOT imports) → KEPT, NOT ESObject ===
export class LocalClass { val: number = 0 }
export let local: LocalClass = new LocalClass()
export type LocalAlias = string
export let localAlias: LocalAlias = ""

// Default import + qualified type ref (same path as preferences, different module)
export let toastOpts: promptAction.ShowToastOptions = null!
export let dialogOpts: promptAction.ShowDialogOptions = null!
export let btn: promptAction.Button = null!
export let levelMode: promptAction.LevelMode = 0
export let dialogCtrl: promptAction.DialogController = null!
export function showToast(opts: promptAction.ShowToastOptions): promptAction.ShowDialogSuccessResponse { return null! }
export function getButton(): promptAction.Button { return null! }
export class MyDialog implements promptAction.CommonController {
  close(): void {}
  getState(): promptAction.CommonState { return 0 }
}
export type MyButtons = promptAction.PromptActionDoubleButtons
export let paPromise: Promise<promptAction.ShowDialogOptions> = null!
export let paMultiUnion: promptAction.LevelMode | promptAction.CommonState = 0
export { promptAction }
export { promptAction as pa }
export { ShowDialogOptions } from '@ohos.promptAction'
export * from '@ohos.promptAction'

// === Mixed export clause: ignored specifier filtered, normal import survives ===
export { ValueType, A1 }

// === Method/function type parameters shadow the import name → kept ===
export class GenericMethods {
  m<ValueType>(x: ValueType): ValueType { return x }
}
export interface IGenericMethod {
  m<ValueType>(x: ValueType): ValueType
}
export function genericFn<ValueType>(v: ValueType): ValueType { return v }

// === Class/interface type parameters shadow the import name → kept ===
export class Container<ValueType> {
  v: ValueType = null!
}
export interface IContainer<ValueType> {
  v: ValueType
}

// === Named import from ignored SDK → ESObject; namespace-local shadowing → kept ===
export type A = ValueType
export namespace ns {
  export class ValueType {
    constructor() {}
    foo(): void {}
  };

  export const someVarWithConflictName: ValueType = new ValueType();
}
