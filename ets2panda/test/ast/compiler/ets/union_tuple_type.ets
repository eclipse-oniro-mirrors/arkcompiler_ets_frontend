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

let lhs: int = 1
let rhs: [int_nullish, Int, Int] = [0, 0, 0]
let x = lhs ?? rhs
let y: String = "neg test"

let v0: Int|[Int|null|undefined, Int, Int] = x
let v1: Int|[Int, Int, Int]|null|undefined = x
let v2: Int|[Int|null|undefined, Int, Int] = y

// tuple with union types in elements
let v3: Int|[Int|null, String|undefined] = [30, "test"]
let v4: Int|[Int|null, String|undefined] = [null, undefined]
let v5: Int|[Int|null, String|undefined] = y

// nested tuple with union types in elements
let v6: Int|[[Int|null, Int], [String, String|undefined]] = [[null, 2], ["a", undefined]]
let v7: Int|[[Int|null, Int], [String, String|undefined]] = y

// tuple with union types in union types (main test scenario)
let v8: Int|[(Int|String), Int] = ["test", 10]
let v9: Int|[(Int|String), (Int|Boolean)] = ["test", true]
let v10: Int|[(Int|String|null), (Int|Boolean|undefined)] = [null, undefined]
let v11: Int|[(Int|(String|Boolean)), (Int|String)] = [true, "test"]
let v12: Int|[(Int|String), Int] = y

// complex nested union and tuple types
let v13: Int|[[Int|String, Int|Boolean], [String|null, Int|undefined]] = [["test", true], [null, 20]]
let v14: (Int|[String, Int])|(Boolean|[(String|undefined), (Int|null)]) = true
let v15: (Int|[String, Int])|(Boolean|[(String|undefined), (Int|null)]) = 100
let v16: (Int|[String, Int])|(Boolean|[(String|undefined), (Int|null)]) = y

// deeply nested union and tuple types
let v17: Int|[(Int|(String|[(Boolean|undefined), Int])), Int] = [ [true, 10], 20 ]
let v18: Int|[(Int|String), (Int|(Boolean|[(String|null), Int]))] = [ "test", [null, 30] ]
let v19: Int|[(Int|(String|[(Boolean|undefined), Int])), Int] = y

/* @@? 24:46 Error Semantic error ESE0318: Type 'Int|[Int|null|undefined, Int, Int]' cannot be assigned to type 'Int|[Int, Int, Int]|null|undefined' */
/* @@? 25:46 Error Semantic error ESE0318: Type 'String' cannot be assigned to type 'Int|[Int|null|undefined, Int, Int]' */
/* @@? 30:44 Error Semantic error ESE0318: Type 'String' cannot be assigned to type 'Int|[Int|null, String|undefined]' */
/* @@? 34:61 Error Semantic error ESE0318: Type 'String' cannot be assigned to type 'Int|[[Int|null, Int], [String, String|undefined]]' */
/* @@? 41:36 Error Semantic error ESE0318: Type 'String' cannot be assigned to type 'Int|[Int|String, Int]' */
/* @@? 47:75 Error Semantic error ESE0318: Type 'String' cannot be assigned to type 'Int|[String, Int]|Boolean|[String|undefined, Int|null]' */
/* @@? 52:65 Error Semantic error ESE0318: Type 'String' cannot be assigned to type 'Int|[Int|String|[Boolean|undefined, Int], Int]' */
