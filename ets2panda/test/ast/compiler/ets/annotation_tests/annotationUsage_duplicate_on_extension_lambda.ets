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

@Retention("SOURCE")
@interface Anno {}
class A { name = "Bob" }
let a = new A();

let show = @Anno()@/* @@ label */Anno(this: A): string => {
    return "Hello," + this.name;
}

/* @@@ label Error Semantic error ESE0041: Duplicate annotations are not allowed. The annotation 'Anno' has already been applied to this element. */
