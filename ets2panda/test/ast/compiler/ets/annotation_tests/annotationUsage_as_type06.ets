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
    a:string = "1"
}

function foo1(a:/* @@ label */MyAnno){}
function foo2(): /* @@ label1 */MyAnno {}

foo1(new /* @@ label2 */MyAnno())

/* @@? 19:31 Error TypeError: Annotations cannot be used as a type. */
/* @@? 20:33 Error TypeError: Annotations cannot be used as a type. */
/* @@? 22:25 Error TypeError: Annotations cannot be used as a type. */
/* @@? 22:25 Error TypeError: Invalid type reference. */
