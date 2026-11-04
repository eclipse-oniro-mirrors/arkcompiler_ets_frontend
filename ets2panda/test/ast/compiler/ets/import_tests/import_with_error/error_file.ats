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

interface Foo {
    build(): void;
}

// Since UI plugin will not transform non-exported entity if they are not in the same cluster
// This code is just mimic UI scenario
class WrongImpl implements Foo {
}

export class ActualExport {
    public c: string = "";
    foo(): void {
        console.log(1);
    }
}

/* @@? 22:32 Error Semantic error ESE0190: WrongImpl is not abstract and does not override abstract method build(): undefined in Foo */
