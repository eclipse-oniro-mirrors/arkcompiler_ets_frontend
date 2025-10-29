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

interface I1 { foo () {} }
interface I2 { foo () {} }
class C implements I1, I2 /* @@ label */{
}

function main() {
    let c = new C;
    c.foo();
}


/* @@@ label Error Semantic error ESE0187: Method 'foo' is declared in I1 and I2 interfaces. */