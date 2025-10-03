/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

@interface MyAnno {
    testProperty1: string
    testProperty2: boolean
    testProperty3: int[]
    testProperty4: Color
    testProperty5: Size[]
}

enum Color{GREEN, RED, BLUE}
enum Size{S, M, L, XL, XXL}

class B{
    @MyAnno({
        testProperty1: 1,
        testProperty2: "false",
        testProperty3: [1.1, 3.14],
        testProperty4: Size.L,
        testProperty5: [Color.GREEN, Color.BLUE]
    })
    foo(){}
}


/* @@? 29:24 Error Semantic error ESE0318: Type 'Int' cannot be assigned to type 'String' */
/* @@? 30:24 Error Semantic error ESE0318: Type '"false"' cannot be assigned to type 'Boolean' */
/* @@? 31:25 Error Semantic error ESE0227: Array element at index 0 with type 'Double' is not compatible with the target array element type 'Int' */
/* @@? 31:30 Error Semantic error ESE0227: Array element at index 1 with type 'Double' is not compatible with the target array element type 'Int' */
/* @@? 32:24 Error Semantic error ESE0318: Type 'Size' cannot be assigned to type 'Color' */
/* @@? 33:25 Error Semantic error ESE0227: Array element at index 0 with type 'Color' is not compatible with the target array element type 'Size' */
/* @@? 33:38 Error Semantic error ESE0227: Array element at index 1 with type 'Color' is not compatible with the target array element type 'Size' */