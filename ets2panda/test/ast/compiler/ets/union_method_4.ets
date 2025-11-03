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

class A {
    private foo(v: string): void {}
    private bar(v: string): void {}
}

class B {
    foo(v: string): void {}
    private bar(v: string): void {}
}

function foo(x: A|B) {
    /* @@ label */x.foo("123")
}

function bar(x: A|B) {
    /* @@ label2 */x.bar("123")
}

/* @@? 27:19 Error TypeError: Member type must be the same for all union objects. */
/* @@? 27:19 Error TypeError: No matching call signature for foo("123") */
/* @@? 31:20 Error TypeError: Signature bar(v: String): void is not visible here. */
/* @@? 31:20 Error TypeError: No matching call signature for bar("123") */