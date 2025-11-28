/*
 * Copyright (c) 2022-2025 Huawei Device Co., Ltd.
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

// Example 1: Basic Usage
// Unresolved reference globalThis
globalThis.obj
globalThis?.obj;
globalThis.abc = 200;

// Example 2: Usage in Return statement
function circleArea(r: number): number {
  return globalThis.pi * r * r; // Unresolved reference globalThis
}

// Example 3: Assignment
const Assignment: number = globalThis.obj.prop; // Unresolved reference globalThis

// Example 4: Reflect API usage
Reflect.get(globalThis, 'CFE') // Unresolved reference globalThis

// Example 5: Alias usage
const root = globalThis; // Unresolved reference globalThis

root.settings.flag

root?.mode?.enabled

//Example 6: Usage of linked with bind
function show(): number {
  console.log('')
  return 4
}
const reslt: number = show.bind(globalThis)(); // Unresolved reference globalThis

// Example 7: Std Method Call
globalThis.setTimeout(() => { }, 1000) // Unresolved reference globalThis

// Example 9: Template String
const template = `${globalThis.abc}` // Unresolved reference globalThis

// Example 10: Usage in a Class
class MyClass {
  globa = globalThis; // Unresolved reference globalThis

  method():void {
    return globalThis.perf // Unresolved reference globalThis
  }

  static statmethod():void {
    return globalThis.navi // Unresolved reference globalThis
  }
}

// Example 11: Object keys
const a:string[] = Object.keys(globalThis) // Unresolved reference globalThis
