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

let good: ValueArray<int> = [1, 2, 3];

class C {}
enum E { A, B }

let err1: ValueArray<Object> = [];
let err2: ValueArray<string> = [];
let err3: ValueArray<E> = [];
let err4: ValueArray<C> = [];
let err5: ValueArray<int|Object> = [];
let err6: ValueArray<any> = [];
let err7: ValueArray<void> = [];

let vint: ValueArray<int>;
let vint1: ValueArray<Int>;
let vdouble: ValueArray<double>;
let vboolean: ValueArray<boolean>;
let vbyte: ValueArray<byte>;
let vshort: ValueArray<short>;
let vlong: ValueArray<long>;
let vfloat: ValueArray<float>;
let vchar: ValueArray<char>;

/* @@? 21:11 Error Semantic error ESE1547180: ValueArray requires a primitive type argument: numeric, boolean or char type. */
/* @@? 21:32 Error Semantic error ESE0301: Can't resolve array type */
/* @@? 22:11 Error Semantic error ESE1547180: ValueArray requires a primitive type argument: numeric, boolean or char type. */
/* @@? 22:32 Error Semantic error ESE0301: Can't resolve array type */
/* @@? 23:11 Error Semantic error ESE1547180: ValueArray requires a primitive type argument: numeric, boolean or char type. */
/* @@? 23:27 Error Semantic error ESE0301: Can't resolve array type */
/* @@? 24:11 Error Semantic error ESE1547180: ValueArray requires a primitive type argument: numeric, boolean or char type. */
/* @@? 24:27 Error Semantic error ESE0301: Can't resolve array type */
/* @@? 25:11 Error Semantic error ESE1547180: ValueArray requires a primitive type argument: numeric, boolean or char type. */
/* @@? 25:36 Error Semantic error ESE0301: Can't resolve array type */
/* @@? 26:11 Error Semantic error ESE1547180: ValueArray requires a primitive type argument: numeric, boolean or char type. */
/* @@? 26:22 Error Semantic error ESE153545: 'any' type annotation is forbidden */
/* @@? 26:29 Error Semantic error ESE0301: Can't resolve array type */
/* @@? 27:11 Error Semantic error ESE1547180: ValueArray requires a primitive type argument: numeric, boolean or char type. */
/* @@? 27:30 Error Semantic error ESE0301: Can't resolve array type */