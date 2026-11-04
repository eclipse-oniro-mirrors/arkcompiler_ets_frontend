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

enum Color{GREEN, RED, BLUE}
enum Size{S, M, L, XL, XXL}

@interface MyAnno {
    testProperty1: string = 1
    testProperty2: boolean = "false"
    testProperty3: FixedArray<int> = [1.1, 3.14]
    testProperty4: Color = Size.L
    testProperty5: FixedArray<Size> = [Color.GREEN, Color.BLUE]
}


/* @@? 20:29 Error Semantic error ESE0318: Type 'Int' cannot be assigned to type 'String' */
/* @@? 21:30 Error Semantic error ESE0318: Type '"false"' cannot be assigned to type 'Boolean' */
/* @@? 22:39 Error Semantic error ESE0227: Array element at index 0 with type 'Double' is not compatible with the target array element type 'Int' */
/* @@? 22:44 Error Semantic error ESE0227: Array element at index 1 with type 'Double' is not compatible with the target array element type 'Int' */
/* @@? 23:28 Error Semantic error ESE0318: Type 'Size' cannot be assigned to type 'Color' */
/* @@? 24:40 Error Semantic error ESE0227: Array element at index 0 with type 'Color' is not compatible with the target array element type 'Size' */
/* @@? 24:53 Error Semantic error ESE0227: Array element at index 1 with type 'Color' is not compatible with the target array element type 'Size' */