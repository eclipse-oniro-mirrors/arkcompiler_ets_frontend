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


let i : int = -5
i = /* @@ label_i1 */-5.0
i = /* @@ label_i2 */-5.f

i = 0
i = /* @@ label_i3 */0.0
i = /* @@ label_i4 */0.f

i = 5 
i = /* @@ label_i5 */5.0
i = /* @@ label_i6 */5.f

let d : double = -5
d = -5.0
d = -5.f

d = 0
d = 0.0
d = 0.f

d = 5
d = 5.0
d = 5.f

/* @@@ label_i1 Error Semantic error ESE0318: Type 'Double' cannot be assigned to type 'Int' */
/* @@@ label_i2 Error Semantic error ESE0318: Type 'Float' cannot be assigned to type 'Int' */
/* @@@ label_i3 Error Semantic error ESE0318: Type 'Double' cannot be assigned to type 'Int' */
/* @@@ label_i4 Error Semantic error ESE0318: Type 'Float' cannot be assigned to type 'Int' */
/* @@@ label_i5 Error Semantic error ESE0318: Type 'Double' cannot be assigned to type 'Int' */
/* @@@ label_i6 Error Semantic error ESE0318: Type 'Float' cannot be assigned to type 'Int' */