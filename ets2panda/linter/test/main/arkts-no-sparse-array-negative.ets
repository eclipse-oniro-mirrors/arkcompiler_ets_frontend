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

// Example 1: Basic usage (not given type)
let b = []; // The array type needs to be deduced from the context.

// Example 2: Omitted elements are not supported in arrays.
let a = [1, , , 3];

// Example 3: Promise methods -  Can't resolve array type
let d1 = Promise.race([]);
let e1 = Promise.all([]);
let f1 = Promise.allSettled([]);
let g1 = Promise.any([]);
let h1 = Promise.resolve([]);

// Example 4: new array object - Can't resolve array type
let bool1 = new Boolean([]);
let num1 = new Number([]);
let obj1 = new Object([]);

// Example 5: Weakset Usage - Can't resolve array type for has, add, delete
let set1 = new WeakSet<object>();
set1.has([]);
set1.add([]);
set1.delete([]);

// Example 6: WeakMap Usage - Can't resolve array type for has, set, delete, get
let map1 = new WeakMap<object, string>();
map1.has([]);
map1.set([], "");
map1.delete([]);
map1.get([]);

// Example 7: Set array Usage - Can't resolve array type for has, add, delete
let set2 = new Set<object>();
set2.has([]);
set2.add([]);
set2.delete([]);

// Example 8: Map Usage - Can't resolve array type for has, set, delete, get
let map2 = new Map<object, string>();
map2.has([]);
map2.set([], "");
map2.delete([]);
map2.get([]);