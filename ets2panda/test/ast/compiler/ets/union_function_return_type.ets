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

type int_nullish = int | null | undefined
function foo(): int_nullish { return 1; }

let lhs: int = 1
let rhs: () => int_nullish = foo
let x = lhs ?? rhs
let y: string = "neg test"
let z: Int = 5

let v0: Int|((() => Int|null|undefined)) = x
let v1: Int|((() => Int|null|undefined)) = z
let v2: Int|((() => Int|null|undefined)) = y
let v3: Int|((() => Int))|null|undefined = x
let v4: () => Int|null|undefined = z
let v5: (() => Int)|null|undefined = z

let v6: Int|((() => Int|null|undefined)) = () => 10
let v7: Int|((() => Int|null|undefined)) = () => null
let v8: Int|((() => Int|null|undefined)) = () => undefined

// nested function types
let v9: Int|((() => (() => Int))) = z
let v10: Int|((() => (() => Int))) = () => () => 20
let v11: Int|((() => (() => Int))) = y

// complex union types
let v12: (() => Int|null)|undefined|String = x
let v13: (() => Int|null)|undefined|String = "test"
let v14: (() => Int|null)|undefined|String = () => 30
let v15: (() => Int|null)|undefined|String = () => null
let v16: (() => Int|null)|undefined|String = undefined

// function signatures that have parameters
let v17: Int|((() => Int|null|undefined)) = () => 40
let v18: (p0: Int) => int|null|undefined = (a) => a
let v19: Int|((p0: Int) => Int|null|undefined) = 50
let v20: Int|((p0: Int) => Int|null|undefined) = (b) => b
let v21: Int|((p0: Int) => Int|null|undefined) = y

// different parameter types in function signatures
let v22: ((p0: String) => Int|null|undefined) = (s) => s.length
let v23: ((p0: Int|String) => Int|null|undefined) = (v) => v instanceof Int ? v : v.length
let v24: ((p0: Int|String) => Int|null|undefined) = (v) => null
let v25: ((p0: Int|String) => Int|null|undefined) = (v) => undefined
let v26: ((p0: Int|String) => Int|null|undefined) = 7

/* @@? 27:44 Error Semantic error ESE0318: Type 'String' cannot be assigned to type 'Int|((() => Int|null|undefined))' */
/* @@? 28:44 Error Semantic error ESE0318: Type 'Int|((() => Int|null|undefined))' cannot be assigned to type 'Int|((() => Int))|null|undefined' */
/* @@? 29:36 Error Semantic error ESE0318: Type 'Int' cannot be assigned to type '(() => Int|null|undefined)' */
/* @@? 30:38 Error Semantic error ESE0318: Type 'Int' cannot be assigned to type '((() => Int))|null|undefined' */
/* @@? 39:38 Error Semantic error ESE0318: Type 'String' cannot be assigned to type 'Int|((() => (() => Int)))' */
/* @@? 42:46 Error Semantic error ESE0318: Type 'Int|((() => Int|null|undefined))' cannot be assigned to type '((() => Int|null))|undefined|String' */
/* @@? 53:50 Error Semantic error ESE0318: Type 'String' cannot be assigned to type 'Int|(((p1: Int) => Int|null|undefined))' */
/* @@? 60:53 Error Semantic error ESE0318: Type 'Int' cannot be assigned to type '((p1: Int|String) => Int|null|undefined)' */
