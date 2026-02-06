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

type ItemGeneratorFunc<T> = (item: T, index: int) => void;

type KeyGeneratorFunc<T> = (item: T, index: int) => string;

function ForEach<T>(arr: Array<T>, itemGenerator: ItemGeneratorFunc<T>, keyGenerator?: KeyGeneratorFunc<T>){

}

function test() {
    let colors: string[] = ["Red", "Orange", "Yellow", "Green", "Pink", "Grey", "Blue", "Brown"];
    ForEach(colors, (color: string, index: number) => {
    })
}

/* @@? 26:5 Error Semantic error ESE0127: No matching call signature for ForEach(Array<String>, ((p1: String, p2: Double) => void)) */
/* @@? 26:21 Error Semantic error ESE0046: Type '((p1: String, p2: Double) => void)' is not compatible with type '((p1: String, p2: Int) => void)' at index 2 */