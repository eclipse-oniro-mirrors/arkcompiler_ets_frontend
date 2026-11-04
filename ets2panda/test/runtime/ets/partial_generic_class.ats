/**
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http: //www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

class  C<T> {
    f: T;

    constructor(p: T) {
        this.f = p;
    }
}

function use1(p: Partial<C<string>>): string | undefined {
    let s: string | undefined = p.f;
    return s;
}

function use2(p: Partial<C<number>>): number | undefined {
    let n: number | undefined = p.f;
    return n;
}

function use3(p: Partial<C<String>>): String | undefined {
    let s: String | undefined = p.f;
    return s;
}

function main() {
    arktest.assertEQ(use1({f: "test"} as Partial<C<string>>), "test");
    arktest.assertEQ(use2({f: 123.456} as Partial<C<number>>), 123.456);
    arktest.assertEQ(use3({f: "test"} as Partial<C<String>>), "test");
}
